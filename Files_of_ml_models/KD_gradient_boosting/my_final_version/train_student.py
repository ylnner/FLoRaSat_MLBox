import os
for _v in ("OMP_NUM_THREADS", "MKL_NUM_THREADS", "OPENBLAS_NUM_THREADS"):
    os.environ.setdefault(_v, "1")

import argparse
import concurrent.futures as cf
import copy
import glob
import json
import math
import multiprocessing as mp
import time

import numpy as np
import pandas as pd
import torch
import torch.nn as nn
from sklearn.metrics import (average_precision_score, precision_recall_curve, precision_recall_fscore_support,
                             roc_auc_score)
from sklearn.preprocessing import StandardScaler
from torch.utils.data import DataLoader, TensorDataset

from config import (ALPHAS, BASE_EPOCHS, BATCH_SIZE, FRACTIONS, LR, NUM_WORKERS, PATIENCE, SEEDS,
                        STUDENT_DROPOUT, STUDENT_HIDDEN, TEMPS, THRESHOLD_METRIC, EVAL_SUBSET)

assert EVAL_SUBSET in ("visible", "all"), f"EVAL_SUBSET debe ser 'visible' o 'all', no {EVAL_SUBSET!r}"
assert THRESHOLD_METRIC in ("f1_c1", "macro_f1"), f"THRESHOLD_METRIC debe ser 'f1_c1' o 'macro_f1', no {THRESHOLD_METRIC!r}"

SELECTION_COL = f"val_ap_{EVAL_SUBSET}"        # columna de las runs con la que se elige alpha/T
SUBSETS    = (("all", "todas"), ("visible", "visibles"))
THRESHOLDS = (("best_th", f"{THRESHOLD_METRIC}_val_{EVAL_SUBSET}"), ("th_05", "0.5"))

from common import results_dir, cache_path_with_N

from funcs_create_dataset import load_dataset, STUDENT_INPUTS
from global_funcs import set_seed, find_best_threshold

from StudentModel import StudentModel
from MyCustomClasses import MyDistillationLoss, MyCustomStudentDataset, MyCustonStudentDistillationDataset


def subset_indices(n, seed, frac):
    """
    Primeros n*frac de una permutacion, ordenados.
    """
    perm = np.random.default_rng(seed).permutation(n)
    return np.sort(perm[:max(1, int(round(frac * n)))])


def load_runs(str_N):
    return pd.DataFrame([json.load(open(f)) for f in glob.glob(os.path.join(results_dir(str_N, "student", "runs"), "*.json"))])

def run_name(frac, teacher, alpha, temp, seed):
    return f"f{frac:.2f}_{teacher}_a{alpha:g}_T{temp:g}_s{seed}"

def class_metrics(y, p, threshold, tag, undefined_as_nan=False):
    """
    Precision / recall / F1 de la clase 0 (falla: no transmitir) y 1 (llega: transmitir).
    undefined_as_nan=True: una metrica indefinida (clase sin ejemplos reales o sin predicciones) se guarda como NaN
    en vez de 0. Se usa en el subconjunto sin visibilidad, donde no hay ningun ejemplo de la clase 1.
    """

    pred = (p >= threshold).astype(int)

    zd = np.nan if undefined_as_nan else 0
    prec, rec, f1, support = precision_recall_fscore_support(y, pred, labels=[0, 1], zero_division=zd)
    if undefined_as_nan:
        f1 = np.where(np.isnan(prec) | np.isnan(rec), np.nan, f1)   # sin precision o recall, el F1 no existe

    out = {f"{tag}_{m}{c}": float(v[c]) for m, v in (("prec", prec), ("rec", rec), ("f1", f1)) for c in (0, 1)}
    out.update({f"{tag}_n{c}": int(support[c]) for c in (0, 1)})          # ejemplos reales de cada clase (lo usa metrics_by_visibility)

    present = support > 0                                                # macro solo sobre clases con ejemplos
    out[f"{tag}_macro_f1"]  = float(np.nanmean(f1[present])) if undefined_as_nan and present.any() else float(f1.mean())
    out[f"{tag}_acc"]       = float((pred == y).mean())
    out[f"{tag}_tx_frac"]   = float(pred.mean())                 # fraccion de transmisiones que se permiten

    return out


# Por defecto usamos el F1-Score. Estas funciones macro F1 tambien se usan para otra configuracion.
def find_best_macro_f1_threshold(y_true, y_prob):
    """
    Umbral que maximiza el macro-F1 (media del F1 de la clase 0 y de la clase 1). 
    Se transmite si p >= umbral.
    Recorre todos los umbrales posibles (cada valor distinto de p) con conteos acumulados.
    """

    y_true = np.asarray(y_true).astype(int)
    y_prob = np.asarray(y_prob)
    order  = np.argsort(-y_prob, kind="stable")
    
    p_sort, y_sort = y_prob[order], y_true[order]
    
    tp   = np.cumsum(y_sort)                      # positivos con p >= umbral
    fp   = np.cumsum(1 - y_sort)                  # negativos con p >= umbral
    last = np.r_[p_sort[1:] != p_sort[:-1], True]          # un punto por valor distinto de p
    
    tp, fp, thr = tp[last], fp[last], p_sort[last]
    n_pos, n_neg = y_true.sum(), len(y_true) - y_true.sum()
    fn, tn = n_pos - tp, n_neg - fp
    
    f1_1 = 2 * tp / np.maximum(2 * tp + fp + fn, 1)
    f1_0 = 2 * tn / np.maximum(2 * tn + fn + fp, 1)
    
    macro = (f1_0 + f1_1) / 2
    best  = int(np.argmax(macro))

    return float(thr[best]), float(macro[best])


def choose_threshold(y_true, y_prob):
    """
    Umbral segun config.THRESHOLD_METRIC: 'f1_c1' (mejor F1 de la clase 1, criterio anterior) o 'macro_f1'.
    """
    if THRESHOLD_METRIC == "macro_f1":
        return find_best_macro_f1_threshold(y_true, y_prob)
    
    thr, f1 = find_best_threshold(y_true, y_prob)
    return float(thr), float(f1)



def predict(model, loader):
    """    
    Run themodel y obtiene y_true y logits. No shuffle
    """
    model.eval()
    all_y_true = []
    all_logits = []
    with torch.no_grad():
        for batch in loader:
            student_logits = model(batch["x"])
            all_y_true.append(batch["y_true"])
            all_logits.append(student_logits)

    y_true = torch.cat(all_y_true)
    logits = torch.cat(all_logits)
    return y_true, logits

def train_student(train_loader, val_loader, pos_weight, is_kd, alpha, temperature, max_epochs, val_mask):
    """
    Usa early stopping sobre la AP de validacion, calculada sobre las filas de val_mask
    val_mask se setea de igual forma que EVAL_SUBSET. Si es 'visible' solo son las filas visibles, si es 'all' se aplica a todas.
    t_tr = logits del maestro (None = no_kd). 
    Return model, val_loss, p_val, best_epoch
    """

    # Model, losses and optimizer
    model     = StudentModel()
    loss_eval = nn.BCEWithLogitsLoss(pos_weight=pos_weight)        # loss de validacion, igual para no_kd y KD
    loss_kd   = MyDistillationLoss(alpha, temperature, pos_weight) if is_kd else None
    optimizer = torch.optim.Adam(model.parameters(), lr=LR)

    best_val_ap       = -1.0
    best_val_loss     = float("inf")
    best_state        = None
    best_p_val        = None
    best_epoch        = 0
    epochs_no_improve = 0

    for epoch in range(1, max_epochs +1):
        # Train
        model.train()

        for batch in train_loader:
            x_batch        = batch["x"]
            y_batch        = batch["y_true"]
            student_logits = model(x_batch)

            if is_kd:
                teacher_batch = batch["teacher_logits"]
                loss = loss_kd(student_logits, teacher_batch, y_batch)
            else:
                loss = loss_eval(student_logits, y_batch)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

        # Validation
        #y_val, logits_val   = predict(model, val_loader)
        #val_loss            = loss_eval(logits_val, y_val).item()

        # Validation
        y_val, logits_val = predict(model, val_loader)
        val_loss          = loss_eval(logits_val, y_val).item()          # se sigue guardando, solo como informacion
        p_val             = torch.sigmoid(logits_val).numpy()        
        val_ap            = average_precision_score(y_val[val_mask], p_val[val_mask])

        # Early stopping
        #if val_loss < best_val_loss:
        if val_ap > best_val_ap:
            best_val_loss     = val_loss
            best_val_ap       = val_ap
            best_state        = copy.deepcopy(model.state_dict())
            best_p_val        = torch.sigmoid(logits_val).numpy()
            best_epoch        = epoch
            epochs_no_improve = 0
        else:
            epochs_no_improve += 1
            if epochs_no_improve >= PATIENCE:
                break
    
    model.load_state_dict(best_state)
    model.eval()
    return model, best_val_loss, best_p_val, best_epoch    



def train_one(frac, teacher, alpha, temp, seed, str_N):    
    frac, alpha, temp, seed, = float(frac), float(alpha), float(temp), int(seed)
    # Obtiene nombre ed la run
    name = run_name(frac, teacher, alpha, temp, seed)

    out = os.path.join(results_dir(str_N, "student", "runs"), name + ".json")
    if os.path.exists(out):
        return
    torch.set_num_threads(1)
    t0 = time.time()

    is_kd = teacher != "no_kd"

    # Load data (cache .npz de la seed)
    train = load_dataset(cache_path_with_N(seed, "train", str_N))
    val   = load_dataset(cache_path_with_N(seed, "val", str_N))
    test  = load_dataset(cache_path_with_N(seed, "test", str_N))
    
    # Get subset of index according to the frac, it is only used to reduce training data
    idx = subset_indices(len(train["y_true"]), seed, frac)

    # El scaler se ajusta solo con l idx de train
    scaler       = StandardScaler().fit(train["X_student"][idx])
    train_scaled = scaler.transform(train["X_student"][idx]).astype(np.float32)
    val_scaled   = scaler.transform(val["X_student"]).astype(np.float32)
    test_scaled  = scaler.transform(test["X_student"]).astype(np.float32)

    # Se obtiene los idx solo del y_train
    y_train = train["y_true"][idx]
    y_val   = val["y_true"]
    y_test  = test["y_true"]

    # pos_weight compensa el desbalance de clases: cuantos fallos (0) hay por cada exito (1) en el subconjunto
    n_neg      = float((y_train == 0).sum())
    n_pos      = float((y_train == 1).sum())
    pos_weight = torch.tensor([n_neg / n_pos])

    # Create Dataset. Los logits del maestro corresponden SOLO a train (mismo orden que la cache):
    # se toman las mismas filas idx que el subconjunto.
    if is_kd:
        teacher_logits = torch.load(os.path.join(results_dir(str_N, f"teacher_gradient_boosting", "logits"), f"teacher_logits_seed{seed}.pt"))        

        assert len(teacher_logits) == len(train["y_true"]), "los logits no corresponden al train de esta seed"
        train_dataset = MyCustonStudentDistillationDataset(train_scaled, y_train, teacher_logits=teacher_logits[idx])        
    else:
        train_dataset = MyCustomStudentDataset(train_scaled, y_train)

    val_dataset  = MyCustomStudentDataset(val_scaled, y_val)
    test_dataset = MyCustomStudentDataset(test_scaled, y_test)


    # Create Loaders. set_seed con el mismo valor para no_kd y KD: mismos pesos iniciales y mismo orden de batches.
    # val y test llevan un generador propio: cada vez que se recorre un DataLoader, Pytorch toma un numero
    # del generador que tenga asignado. Si no especificamos en el test y val, evaluar la validacion en cada epoca consumiria
    # el generador global y cambiaria el dropout y el shuffle del train.
    set_seed(seed)
    train_loader = DataLoader(train_dataset, batch_size=BATCH_SIZE, shuffle=True)
    val_loader   = DataLoader(val_dataset, batch_size=BATCH_SIZE, shuffle=False, generator=torch.Generator())
    test_loader  = DataLoader(test_dataset, batch_size=BATCH_SIZE, shuffle=False, generator=torch.Generator())

    # Gettin val visible
    #vis_va, vis_te = val["below_mask"] == 0, test["below_mask"] == 0
    visible_val = val["below_mask"] == 0
    visible_test = test["below_mask"] == 0
    
    # Train
    #Colocamos max para cuando se usa todo el dataset y no solo entrene por 20 epocas. Sigue aplicando early stopping
    max_epochs = max(math.ceil(BASE_EPOCHS / frac), 60)
    sel_val = None
    if EVAL_SUBSET == "visible":
        sel_val = visible_val
    else:
        sel_val = np.ones(len(y_val), dtype = bool)

    model, val_loss, pred_val, best_epoch = train_student(train_loader, val_loader, pos_weight, is_kd, alpha, temp, max_epochs, val_mask = sel_val)    
    
    # Predict test
    _, logits_test = predict(model, test_loader)
    
    # Aplica sigmoind a los logits
    pred_test = torch.sigmoid(logits_test).numpy()
    #thr, _    = find_best_threshold(y_val, pred_val)
    #thr       = float(thr)

    if THRESHOLD_METRIC == "macro_f1":
        thr, _ = find_best_macro_f1_threshold(y_val, pred_val)
    elif THRESHOLD_METRIC == "f1_c1":
        thr, _    = find_best_threshold(y_val, pred_val)
        
    thr       = float(thr)
    

    y_test_int = y_test.astype(int)
    r = dict(frac    = frac, 
             teacher = teacher, 
             alpha   = alpha, 
             temp    = temp, 
             seed    = seed, 
             n_train = int(len(idx)),

             eval_subset        = EVAL_SUBSET,                 # filas de val usadas para early stopping, umbral y seleccion
             threshold_metric   = THRESHOLD_METRIC,          # criterio del umbral, F1 score, o Macro F1

             threshold      = thr,
             best_epoch     = best_epoch, 
             val_loss       = float(val_loss),              
             val_ap_visible = float(average_precision_score(y_val[visible_val], pred_val[visible_val])),          # criterio de seleccion si visible
             val_ap_all     = float(average_precision_score(y_val, pred_val)),                                   # criterio de seleccion si all

             test_ap_visible     = float(average_precision_score(y_test[visible_test], pred_test[visible_test])),             # metrica principal
             test_auc_visible    = float(roc_auc_score(y_test[visible_test], pred_test[visible_test])),
             test_auc_all        = float(roc_auc_score(y_test, pred_test)),     # inflada por las filas bajo la mascara (faciles)
             test_ap_all         = float(average_precision_score(y_test, pred_test)),
             test_belowmask_leak = float((pred_test[~visible_test] >= thr).mean()),
             test_tx_avoided     = float((pred_test < thr).mean()),
             test_success_lost   = float(((pred_test < thr) & (y_test_int == 1)).sum() / max((y_test_int == 1).sum(), 1))
             )

    
    for tname, th in (("best_th", thr), ("th_05", 0.5)):          # thr = umbral de threshold metric escogida en val 
        r.update(class_metrics(y_test_int[visible_test], pred_test[visible_test], th, f"{tname}_visible"))
        r.update(class_metrics(y_test_int[~visible_test], pred_test[~visible_test], th, f"{tname}_novisible",
                               undefined_as_nan=True))

        r.update(class_metrics(y_test_int, pred_test, th, f"{tname}_all"))
    r["secs"] = time.time() - t0


    torch.save(model.state_dict(), os.path.join(results_dir(str_N, "student", "models"), name + ".pth"))
    with open(os.path.join(results_dir(str_N, "student", "scalers"), name + ".json"), "w") as f:
        json.dump({"inputs": STUDENT_INPUTS, "mean": scaler.mean_.tolist(), "scale": scaler.scale_.tolist(),
                   "threshold": thr, "uso": "x_esc = (x - mean) / scale; p = sigmoid(MLP(x_esc)); "
                                            "transmitir si p >= threshold"}, f, indent=2)
    tmp = out + ".tmp"
    with open(tmp, "w") as f:
        json.dump(r, f)
    os.replace(tmp, out)


def select_best_configs(runs):
    """
    Mejor alpha/temperature por fraccion: mayor media de val_ap_visible o val_ap_all (segun se escoja) entre seeds.
    Solo se consideran combinaciones con todas las seeds terminadas.
    Se elige con validacion; el test solo se reporta.
    """
    kd_runs = runs[runs["teacher"] != "no_kd"]
    n_seeds = runs["seed"].nunique()

    # Media y desviacion por (fraccion, alpha, temperatura)
    grid = (kd_runs.groupby(["frac", "alpha", "temp"])
                   .agg(n_seeds             = ("seed", "nunique"),
                        val_ap_visible      = ("val_ap_visible", "mean"),
                        val_ap_visible_std  = ("val_ap_visible", "std"),
                        val_ap_all          = ("val_ap_all", "mean"),                    
                        val_ap_all_std      = ("val_ap_all", "std"),
                        test_ap_visible     = ("test_ap_visible", "mean"),     # metrica principal
                        test_ap_visible_std = ("test_ap_visible", "std"),
                        test_ap_all         = ("test_ap_all", "mean"),         # metrica principal
                        test_ap_all_std     = ("test_ap_all", "std"),                        
                        test_belowmask_leak = ("test_belowmask_leak", "mean"))
                   .reset_index())
    
    complete_grid = grid[grid["n_seeds"] == n_seeds]
        
    
    # Por cada fraccion, la fila con mayor SELECTION_COL
    best_rows = complete_grid.loc[complete_grid.groupby("frac")[SELECTION_COL].idxmax()].copy()
    best_rows["criterio"] = SELECTION_COL


    # Referencia sin KD por fraccion
    no_kd_runs = runs[runs["teacher"] == "no_kd"]
    no_kd = (no_kd_runs.groupby("frac")
                       .agg(no_kd_val_ap_visible      = ("val_ap_visible", "mean"),
                            no_kd_val_ap_all          = ("val_ap_all", "mean"),
                            no_kd_test_ap_visible     = ("test_ap_visible", "mean"),
                            no_kd_test_ap_all         = ("test_ap_all", "mean"),
                            no_kd_test_belowmask_leak = ("test_belowmask_leak", "mean"))                            
                       .reset_index())
    

    best = best_rows.merge(no_kd, on="frac", how="left")
    best["test_gain_visible"] = best["test_ap_visible"] - best["no_kd_test_ap_visible"]
    best["test_gain_all"]     = best["test_ap_all"]     - best["no_kd_test_ap_all"]

    best = best.sort_values("frac").reset_index(drop=True)
    return grid, best



def compare_with_no_kd(runs, best):
    """
    Por cada fraccion: media y desviacion entre seeds de cada metrica para no_kd y para la mejor alpha/T.
    Devuelve una fila por (fraccion, metrica) con la ganancia kd - no_kd.
    """

    """
    metrics_full = {
        "test_ap_all":              "auc_pr",
        "test_ap_visible":          "auc_pr_visible",
        "best_th_all_prec0":        "precision_0",
        "best_th_all_rec0":         "recall_0",
        "best_th_all_f10":          "f1_0",
        "best_th_all_prec1":        "precision_1",
        "best_th_all_rec1":         "recall_1",
        "best_th_all_f11":          "f1_1",
        "best_th_all_macro_f1":     "macro_f1",
        "test_belowmask_leak":      "belowmask_leak",
    }
    """

    metrics_full = [
        ("test_ap_visible",            "visibles",    "auc_pr"),
        ("test_auc_visible",           "visibles",    "auc_roc"),
        ("best_th_visible_prec0",      "visibles",    "precision_0"),
        ("best_th_visible_rec0",       "visibles",    "recall_0"),
        ("best_th_visible_f10",        "visibles",    "f1_0"),
        ("best_th_visible_prec1",      "visibles",    "precision_1"),
        ("best_th_visible_rec1",       "visibles",    "recall_1"),
        ("best_th_visible_f11",        "visibles",    "f1_1"),
        ("best_th_visible_macro_f1",   "visibles",    "macro_f1"),
        ("best_th_visible_tx_frac",    "visibles",    "tx_frac"),

        ("test_belowmask_leak",        "no_visibles", "belowmask_leak"),
        ("best_th_novisible_rec0",     "no_visibles", "recall_0"),
        ("best_th_novisible_f10",      "no_visibles", "f1_0"),

        ("test_ap_all",                "todas",       "auc_pr"),
        ("test_auc_all",               "todas",       "auc_roc"),
        ("best_th_all_prec0",          "todas",       "precision_0"),
        ("best_th_all_rec0",           "todas",       "recall_0"),
        ("best_th_all_f10",            "todas",       "f1_0"),
        ("best_th_all_prec1",          "todas",       "precision_1"),
        ("best_th_all_rec1",           "todas",       "recall_1"),
        ("best_th_all_f11",            "todas",       "f1_1"),
        ("best_th_all_macro_f1",       "todas",       "macro_f1"),
        ("best_th_all_tx_frac",        "todas",       "tx_frac"),
    ]    
    rows = []
    for _, best_row in best.iterrows():
        frac  = best_row["frac"]
        alpha = best_row["alpha"]
        temp  = best_row["temp"]

        same_frac  = runs["frac"] == frac
        no_kd_runs = runs[same_frac & (runs["teacher"] == "no_kd")]
        kd_runs    = runs[same_frac & (runs["teacher"] != "no_kd") & (runs["alpha"] == alpha) & (runs["temp"] == temp)]

        for column, subset, metric in metrics_full:
            no_kd_mean = no_kd_runs[column].mean()
            kd_mean    = kd_runs[column].mean()
            rows.append(dict(frac       = frac,
                             alpha      = alpha,
                             temp       = temp,
                             subconjunto = subset,
                             metric     = metric,
                             no_kd_mean = no_kd_mean,
                             no_kd_std  = no_kd_runs[column].std(),
                             kd_mean    = kd_mean,
                             kd_std     = kd_runs[column].std(),
                             gain       = kd_mean - no_kd_mean))
    return pd.DataFrame(rows)


def metrics_by_visibility(runs, best, str_N):
    """
    Una fila por (fraccion, modelo, umbral, subconjunto, clase) con precision, recall y F1 (media y std entre seeds).

    modelo: no_kd y el KD con el mejor alpha/T de esa fraccion.

    subconjunto: los de SUBSETS (todas las filas de test y solo visibles, below_mask == 0).

    clase: 0 (falla), 1 (llega) y macro (media de las clases con ejemplos reales).
    """
    rows = []
    for _, b in best.iterrows():
        frac = b["frac"]
        same = runs["frac"] == frac
        models = (("no_kd", 0.0, 0.0, runs[same & (runs["teacher"] == "no_kd")]),
                  ("kd", b["alpha"], b["temp"],
                   runs[same & (runs["teacher"] != "no_kd") & (runs["alpha"] == b["alpha"]) & (runs["temp"] == b["temp"])]))
        for model, alpha, temp, r in models:
            for th_key, th_name in THRESHOLDS:
                for sub_key, sub_name in SUBSETS:
                    pre = f"{th_key}_{sub_key}"
                    # Las runs ya hechas no se recalculan (train_one las salta): si vienen de otra version del codigo,
                    # les pueden faltar claves. Se avisa con un mensaje claro en vez de un KeyError de pandas.
                    needed  = [f"{pre}_{m}{c}" for m in ("prec", "rec", "f1", "n") for c in (0, 1)] + \
                              [f"{pre}_acc", f"{pre}_tx_frac"]
                    missing = [k for k in needed if k not in r.columns]
                    if missing:
                        raise KeyError(f"Las runs de {str_N} no tienen {missing} (son de una version anterior del codigo): "
                                       f"borra results/{str_N}/student y vuelve a entrenar")
                    common_cols = dict(N=str_N, frac=frac, modelo=model, alpha=alpha, temp=temp, n_seeds=len(r),
                                       umbral=th_name, umbral_mean=(r["threshold"].mean() if th_key == "best_th" else 0.5),
                                       subconjunto=sub_name, n_filas_mean=(r[f"{pre}_n0"] + r[f"{pre}_n1"]).mean(),
                                       accuracy_mean=r[f"{pre}_acc"].mean(), accuracy_std=r[f"{pre}_acc"].std(),
                                       tx_frac_mean=r[f"{pre}_tx_frac"].mean())
                    for clase in ("0", "1", "macro"):
                        if clase == "macro":
                            # media de las clases con ejemplos reales (en no_visibles solo la clase 0)
                            has = pd.concat([r[f"{pre}_n0"] > 0, r[f"{pre}_n1"] > 0], axis=1).to_numpy()
                            vals = {m: pd.Series(np.nanmean(np.where(has, r[[f"{pre}_{m}0", f"{pre}_{m}1"]].to_numpy(float),
                                                                     np.nan), axis=1), index=r.index)
                                    for m in ("prec", "rec", "f1")}
                            support = (r[f"{pre}_n0"] + r[f"{pre}_n1"]).mean()
                        else:
                            vals = {m: r[f"{pre}_{m}{clase}"] for m in ("prec", "rec", "f1")}
                            support = r[f"{pre}_n{clase}"].mean()
                        rows.append(dict(common_cols, clase=clase, soporte_mean=support,
                                         precision_mean=vals["prec"].mean(), precision_std=vals["prec"].std(),
                                         recall_mean=vals["rec"].mean(),     recall_std=vals["rec"].std(),
                                         f1_mean=vals["f1"].mean(),          f1_std=vals["f1"].std()))
    return pd.DataFrame(rows)



def summarize(str_N):
    runs        = load_runs(str_N)

    # Todas las runs deben venir del mismo esquema que config.EVAL_SUBSET (train_one se salta las runs ya hechas)
    for col, want in (("eval_subset", EVAL_SUBSET), ("threshold_metric", THRESHOLD_METRIC)):
        found = set(runs[col].unique()) if col in runs.columns else {"(runs anteriores, sin esta clave)"}
        if found != {want}:
            raise ValueError(f"Las runs de {str_N} tienen {col}={found} y config pide '{want}': "
                             f"borra results/{str_N}/student y vuelve a entrenar")    
        
    grid, best  = select_best_configs(runs)

    out_dir = results_dir(str_N, "student")
    grid.to_csv(os.path.join(out_dir, "grid_alpha_temp.csv"), index=False)   # todas las combinaciones
    best.to_csv(os.path.join(out_dir, "best_alpha_temp.csv"), index=False)   # la mejor por fraccion

    print(f"\nMejor alpha/T por fraccion (criterio: media de {SELECTION_COL})")
    for _, row in best.iterrows():
        print(f"  frac {row['frac']:.2f}: alpha={row['alpha']:g}  T={row['temp']:g}  "
              f"val AP={row[SELECTION_COL]:.4f}  "
              f"test AP visible={row['test_ap_visible']:.4f}  no_kd={row['no_kd_test_ap_visible']:.4f}  "
              f"ganancia={row['test_gain_visible']:+.4f}")
        
    print(f"guardado en {out_dir}/best_alpha_temp.csv y grid_alpha_temp.csv", flush=True)

    comparison = compare_with_no_kd(runs, best)
    comparison.to_csv(os.path.join(out_dir, "best_vs_no_kd_metrics.csv"), index=False)

    print(f"\nMetricas de test (umbral {THRESHOLD_METRIC} en val {EVAL_SUBSET}), media +- std entre seeds")
    for frac, frac_rows in comparison.groupby("frac"):
        first = frac_rows.iloc[0]
        print(f"  frac {frac:.2f}  (KD: alpha={first['alpha']:g}  T={first['temp']:g})")
        print(f"    {'subconjunto':12s} {'metrica':15s} {'no_kd':>17s} {'KD':>17s} {'ganancia':>9s}")
        for _, row in frac_rows.iterrows():
            print(f"    {row['subconjunto']:12s} {row['metric']:15s} "
                  f"{row['no_kd_mean']:.4f} +- {row['no_kd_std']:.4f} "
                  f"{row['kd_mean']:.4f} +- {row['kd_std']:.4f} "
                  f"{row['gain']:+9.4f}")
    print(f"guardado en {out_dir}/best_vs_no_kd_metrics.csv", flush=True)

    vis_table = metrics_by_visibility(runs, best, str_N)
    vis_table.to_csv(os.path.join(out_dir, "metrics_by_visibility.csv"), index=False)
    print(f"\nPrecision / recall / F1 por clase y visibilidad (umbral {THRESHOLD_METRIC} en val {EVAL_SUBSET}, media entre seeds)")
    view = vis_table[vis_table["umbral"] == THRESHOLDS[0][1]]
    for (frac, model), g in view.groupby(["frac", "modelo"], sort=False):
        print(f"  frac {frac:.2f}  {model}")
        for _, row in g.iterrows():
            print(f"    {row['subconjunto']:12s} clase {row['clase']:5s}  P={row['precision_mean']:.4f}  "
                  f"R={row['recall_mean']:.4f}  F1={row['f1_mean']:.4f}  soporte={row['soporte_mean']:.0f}")
    print(f"guardado en {out_dir}/metrics_by_visibility.csv", flush=True)    


def main():
    ap = argparse.ArgumentParser()
    #ap.add_argument("--phase", choices=["grid", "reps"], required=True)
    #ap.add_argument("--fractions", type=float, nargs="+", default=FRACTIONS)
    #ap.add_argument("--seeds", type=int, nargs="+", default=SEEDS)
    ap.add_argument("--N", type = int, default = 60)
    ap.add_argument("--workers", type=int, default=NUM_WORKERS)
    args = ap.parse_args()
    str_N = "N_" + str(args.N)
    print('str_N: ', str_N)
    #str_N_list = [str_N] * len(args.seeds)
    
    # Cada task es
    # f -> fracicon
    # modelo -> no_kd, student
    # a -> alpha
    # t -> temperature
    # s -> seed
    tasks  = [(f, "no_kd", 0, 0, s, str_N) for f in FRACTIONS for s in SEEDS]
    tasks += [(f, "student", a, t, s, str_N ) for f in FRACTIONS for a in ALPHAS for t in TEMPS for s in SEEDS]
    
    
    tasks.sort(key=lambda x: -x[0])          # fracciones grandes (las mas lentas) primero
    print(len(tasks), "tareas", flush=True)
    t0, done = time.time(), 0
    with cf.ProcessPoolExecutor(args.workers, mp_context=mp.get_context("spawn")) as pool:
        for fut in cf.as_completed([pool.submit(train_one, *t) for t in tasks]):
            fut.result()
            done += 1
            if done % 50 == 0:
                print(f"{done}/{len(tasks)} | {time.time() - t0:.0f} s", flush=True)
    print(f"fin : {time.time() - t0:.0f} s", flush=True)

    summarize(str_N)

if __name__ == "__main__":
    main()
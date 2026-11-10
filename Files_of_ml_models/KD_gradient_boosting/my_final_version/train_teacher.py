"""
Gradient boosting sobre las 8 entradas del estudiante + features geo.

Por cada seed:
  1. LOGITS OUT-OF-FOLD sobre el train (5 folds por simulacion): cada transmision recibe la prediccion de un
     modelo que no la vio. Son los que usa el estudiante.
     Se entrena con k folds y se testea con los restantes.
  2. Evaluacion del maestro: un modelo entrenado con el 100 % del train, predicho sobre validacion set.
     Se compara con la calidad OOF en el train y con la in-sample.

Uso:
    python train_teacher.py --workers 1 --N 60 # 10 seeds en paralelo
"""

import os
os.environ.setdefault("OMP_NUM_THREADS", "2")

import argparse
import concurrent.futures as cf
import json

import numpy as np
import torch
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.metrics import average_precision_score, roc_auc_score

from config import LOGIT_CLIP, SEEDS, TEACHER_FOLDS, TEACHER_ITERS
from common import cache_path_with_N, results_dir #results_dir_with_N # results_dir # cache_path
from funcs_create_dataset import load_dataset


def teacher_inputs(data):
    return np.hstack([data["X_student"], data["X_geo"]])        # 8 del estudiante


def new_model():
    return HistGradientBoostingClassifier(max_iter=TEACHER_ITERS, random_state=42)


def simulation_folds(id_simulation, seed):
    """
    Fold de cada fila; se reparten las simulaciones enteras
    # Llegan todas las simulaciones del seed
    # EL output representa a que fold corresponde cada simulacion. 
    En el ejemplo TEACHER_FOLDS = 3
    A, B, C, D, E, F -> id_simlacion.
    El value representa a que fold va cada simulacion
    {np.str_('A'): 0,
    np.str_('B'): 1,
    np.str_('C'): 2,
    np.str_('D'): 0,
    np.str_('E'): 1,
    np.str_('F'): 2}

    En el return se vuelve lista con el valor de cada fold de cada id_simulation
    [1, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2, 1, 1, 0, 1, 2, 2, 2, 1, 1, 0, 1, 0, 2, 2])
    """
    # Total simulations    
    sims = np.array(sorted(np.unique(id_simulation)))
    np.random.default_rng(seed).shuffle(sims)

    # Teacher_folds indica cuantos folds son
    # Se aplica validacion cruzada sobre los logits en todas las simulaciones por separado.
    # La prediccion de acda fila la hace un modelo que no vio esa fila al entrenar.
    # Todas las simulaciones se divide en TEACHER_FOLDS = 5 de simulaciones enteras.    
    # modelo 1: entrena con B C D E y predice A
    # modelo 2: entrena con A C D E y predice B    
    sim_to_fold = {s: i % TEACHER_FOLDS for i, s in enumerate(sims)}

    return np.array([sim_to_fold[s] for s in id_simulation])


def out_of_fold_proba(X, y, fold):
    # Aplica validacion cruzada sobre 
    p = np.zeros(len(y))
    for f in range(TEACHER_FOLDS):
        train   = fold != f
        held    = fold == f

        p[held] = new_model().fit(X[train], y[train]).predict_proba(X[held])[:, 1]
    return p

def out_of_fold_logits(X, y, fold):
    # Fold representa una lista con valores True, False para todas las id_simulaciones
    # True donde la simulacion es parte del train, es decir que el fold no lo saco del train
    # False donde la simulacion no es parte de train, el fon lo saco del train.
    z = np.zeros(len(y))
    # TEACHER_FOLDS = 5
    for f in range(TEACHER_FOLDS):
        # Obtenemos que simulaciones van a train y cuales no (held)
        train, held = fold != f, fold == f

        # decision_function obtiene el logit y lo guarda.
        # El modelo se entrena con las simulaciones del fold -> new_model().fit(X[train], y[train])
        # predice las simulaciones que estan fuera del fold. -> .decision_function(X[held])
        # Y usa eso para generar los logits de este fold
        # En z[held] se guarda el logit de las simulaciones fuera del fod. 
        z[held] = new_model().fit(X[train], y[train]).decision_function(X[held])   # log-odds, sin clip
    return z

def to_logits(p):
    # EL gradient bosting devuelve una probabilidad, no un logit. 
    # Para resolver esto se convierte en logit con logit(clip(p, 1e-4, 1−1e-4))
    p = np.clip(p, LOGIT_CLIP, 1 - LOGIT_CLIP)
    return torch.as_tensor(np.log(p / (1 - p)), dtype=torch.float32)


def visible_scores(y, p, below_mask):
    vis = below_mask == 0
    return {"auc_vis": float(roc_auc_score(y[vis], p[vis])), 
            "ap_vis": float(average_precision_score(y[vis], p[vis])),
            "acc_vis": float(np.mean((p[vis] >= 0.5) == y[vis]))}




def build_one_seed(seed, str_N):    
    # Lee el csv por N    
    train   = load_dataset(cache_path_with_N(seed, "train", str_N))
    val     = load_dataset(cache_path_with_N(seed, "val", str_N))

    # Obtiene X, y
    X       = teacher_inputs(train)
    y       = train["y_true"]

    # logits out-of-fold para destilar
    #p_oof = out_of_fold_proba(X, y, simulation_folds(train["id_simulation"], seed))
    #logits_file = os.path.join(results_dir("teacher_geo", "logits"), f"teacher_logits_seed{seed}.pt")
    #torch.save(to_logits(p_oof), logits_file)

    # simulation_folds(train["id_simulation"], seed)
    #       En el return se vuelve lista con el valor de cada fold de cada id_simulation
    #       [1, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2, 1, 1, 0, 1, 2, 2, 2, 1, 1, 0, 1, 0, 2, 2])

    # En z_oof se guardan todas las logits del oof. 
    z_oof = out_of_fold_logits(X, y, simulation_folds(train["id_simulation"], seed))
    logits_file = os.path.join(results_dir(str_N, "teacher_gradient_boosting", "logits"), f"teacher_logits_seed{seed}.pt")

    # Guardo el z_oof an logits file
    torch.save(torch.as_tensor(z_oof, dtype=torch.float32), logits_file)

    p_oof = 1 / (1 + np.exp(-z_oof)) # solo para las metricas OOF

    # Se reporta con el 100% del train (In-sample) solo para evaluacion. Esto no se destila, solo es para ver que tan bueno es el maestro con el 100%
    # Cuando se aplica out of fold (OOF) da una verdadera incertidumbe y el estudiante aprende mas de eso. 
    # evaluacion: modelo con el 100 % del train
    full = new_model().fit(X, y)
    metrics = {
        "seed": seed,
        "train_oof": visible_scores(y, p_oof, train["below_mask"]), # Reporta los que estan son visibles.
        "train_in_sample": visible_scores(y, full.predict_proba(X)[:, 1], train["below_mask"]), # Reporta los que son visibles
        "val_in_sample": visible_scores(val["y_true"], full.predict_proba(teacher_inputs(val))[:, 1], val["below_mask"]),
    }

    with open(os.path.join(results_dir(str_N, "teacher_gradient_boosting", "metrics"), f"teacher_metrics_seed{seed}.json"), "w") as f:
        json.dump(metrics, f, indent=2)
    return (f"[geo] seed {seed}: AUC visible  OOF train {metrics['train_oof']['auc_vis']:.3f} | "
            f"val {metrics['val_in_sample']['auc_vis']:.3f} | in-sample {metrics['train_in_sample']['auc_vis']:.3f} -> {logits_file}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, nargs="+", default=SEEDS)
    ap.add_argument("--workers", type=int, default=10)
    ap.add_argument("--N", type = int, default = 60)

    args = ap.parse_args()
    str_N = "N_" + str(args.N)
    print('str_N: ', str_N)
    str_N_list = [str_N] * len(args.seeds)
    if args.workers <= 1:
        for s in args.seeds:
            print(build_one_seed(s, str_N_list), flush=True)
    else:
        with cf.ProcessPoolExecutor(args.workers) as pool:
            for msg in pool.map(build_one_seed, args.seeds, str_N_list):
                print(msg, flush=True)


if __name__ == "__main__":
    main()
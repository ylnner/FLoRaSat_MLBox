"""
Gradient boosting sobre las 7 entradas del estudiante (sin raan) + features geo.

y' (y_net): representa si a copia/repeticion llegaria si se transmite con todos los dispositivos siendo FV_1 , es decir solo una transmision visible. 

El objetivo del maestro es la etiqueta de red y' (y_net): 

rcvOk
calculada con la replica del receptor en prepare_data.py. Cambios respecto a v2:
  - Objetivo y' en vez de rcvOk (rcvOk se midio con las 5 copias de todos radiadas, ~10 veces la carga real).

Por cada seed:
  1. LOGITS OUT-OF-FOLD sobre el train (5 folds por simulacion): cada transmision recibe la prediccion de un
     modelo que no la vio. Son los que usa el estudiante.
     Se entrena con k folds y se testea con los restantes.
  2. Evaluacion del maestro: un modelo entrenado con el 100 % del train, predicho sobre validacion set.
     Se compara con la calidad OOF en el train y con la in-sample.

    python train_teacher.py --workers 10 --N 300 # 10 seeds en paralelo
"""

import os
os.environ.setdefault("OMP_NUM_THREADS", "2")

import argparse
import concurrent.futures as cf
import multiprocessing as mp

import numpy as np
import torch
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.metrics import average_precision_score, roc_auc_score, brier_score_loss

from config import SEEDS, TEACHER_FOLDS, TEACHER_ITERS
from common import results_dir, load_split, write_json_atomic


def teacher_inputs(data):
    return np.hstack([data["X_student"], data["X_geo"]])        # 7 del estudiante (sin raan) + 3 geo


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
    # v3: con menos simulaciones que folds algun fold quedaria vacio (sin logits OOF propios)
    assert len(sims) >= TEACHER_FOLDS, f"{len(sims)} simulaciones en train y TEACHER_FOLDS = {TEACHER_FOLDS}"
    np.random.default_rng(seed).shuffle(sims)

    # Teacher_folds indica cuantos folds son
    # Se aplica validacion cruzada sobre los logits en todas las simulaciones por separado.
    # La prediccion de acda fila la hace un modelo que no vio esa fila al entrenar.
    # Todas las simulaciones se divide en TEACHER_FOLDS = 5 de simulaciones enteras.
    # modelo 1: entrena con B C D E y predice A
    # modelo 2: entrena con A C D E y predice B
    sim_to_fold = {s: i % TEACHER_FOLDS for i, s in enumerate(sims)}

    return np.array([sim_to_fold[s] for s in id_simulation])


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
        # v3: sin sample_weight (ver el docstring del modulo)
        z[held] = new_model().fit(X[train], y[train]).decision_function(X[held])   # log-odds, sin clip
    return z


def scores(y, p):
    """
    Calidad del maestro frente a y' (todas las filas ya son visibles).
    mean_p vs mean_y: calibracion global; si mean_p >> mean_y el maestro sobreestima la entrega (como v2 con pos_weight).
    brier: error cuadratico medio de la probabilidad (calibracion + discriminacion).
    """
    return {"auc": float(roc_auc_score(y, p)),
            "ap": float(average_precision_score(y, p)),
            "acc": float(np.mean((p >= 0.5) == y)),
            "mean_p": float(np.mean(p)),
            "mean_y": float(np.mean(y)),
            "brier": float(brier_score_loss(y, p))}


def build_one_seed(seed, str_N):
    # Lee el csv por N
    #la cache solo tiene filas visibles (elevSat >= 10)
    train   = load_split(seed, "train", str_N)
    val     = load_split(seed, "val", str_N)

    # Obtiene X, y_net
    X       = teacher_inputs(train)
    y       = train["y_net"]

    # simulation_folds(train["id_simulation"], seed)
    #       En el return se vuelve lista con el valor de cada fold de cada id_simulation
    #       [1, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2, 1, 1, 0, 1, 2, 2, 2, 1, 1, 0, 1, 0, 2, 2])

    # En z_oof se guardan todas las logits del oof.
    z_oof = out_of_fold_logits(X, y, simulation_folds(train["id_simulation"], seed))
    logits_file = os.path.join(results_dir(str_N, "teacher_gradient_boosting", "logits"), f"teacher_logits_seed{seed}.pt")

    # Guardo el z_oof an logits file. Mismo orden de filas que la cache de train (lo comprueba train_student.py)
    torch.save(torch.as_tensor(z_oof, dtype=torch.float32), logits_file)

    p_oof = 1 / (1 + np.exp(-z_oof)) # solo para las metricas OOF

    # Se reporta con el 100% del train (In-sample) solo para evaluacion. Esto no se destila, solo es para ver que tan bueno es el maestro con el 100%
    # Cuando se aplica out of fold (OOF) da una verdadera incertidumbe y el estudiante aprende mas de eso.
    # evaluacion: modelo con el 100 % del train
    full = new_model().fit(X, y)
    metrics = {
        "seed": seed,
        "target": "y_net",
        "n_train": int(len(y)),
        "train_oof": scores(y, p_oof),
        "train_in_sample": scores(y, full.predict_proba(X)[:, 1]),
        "val_in_sample": scores(val["y_net"], full.predict_proba(teacher_inputs(val))[:, 1]),
    }

    write_json_atomic(os.path.join(results_dir(str_N, "teacher_gradient_boosting", "metrics"),
                                   f"teacher_metrics_seed{seed}.json"), metrics)
    oof = metrics["train_oof"]
    return (f"[geo] seed {seed}: AUC OOF train {oof['auc']:.3f} | val {metrics['val_in_sample']['auc']:.3f} | "
            f"in-sample {metrics['train_in_sample']['auc']:.3f} | calibracion OOF p {oof['mean_p']:.3f} "
            f"vs y' {oof['mean_y']:.3f}, Brier {oof['brier']:.4f} -> {logits_file}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, nargs="+", default=SEEDS)
    ap.add_argument("--workers", type=int, default=10)
    ap.add_argument("--N", type = int, default = 300)

    args = ap.parse_args()
    str_N = "N_" + str(args.N)
    print('str_N: ', str_N)
    str_N_list = [str_N] * len(args.seeds)
    if args.workers <= 1:
        for s in args.seeds:
            print(build_one_seed(s, str_N), flush=True)
    else:
        # spawn: cada proceso arranca limpio (con Python 3.14 el metodo por defecto ya no es fork)
        with cf.ProcessPoolExecutor(args.workers, mp_context=mp.get_context("spawn")) as pool:
            for msg in pool.map(build_one_seed, args.seeds, str_N_list):
                print(msg, flush=True)


if __name__ == "__main__":
    main()

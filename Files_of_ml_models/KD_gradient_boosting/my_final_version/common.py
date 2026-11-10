"""
Utilidades compartidas: rutas de cache/resultados, carga de datos, expansion de grids y metricas.
Importar este modulo agrega SCRIPTS_DIR al sys.path para poder usar el codigo original (solo lectura).
"""
import os
import sys
import json
import itertools

import numpy as np
from sklearn.metrics import (accuracy_score, auc, precision_recall_curve, precision_score,
                             recall_score, f1_score, roc_auc_score)

from config import (SCRIPTS_DIR, CACHE_DIR, RESULTS_DIR, SELECTION_METRIC)

if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

from global_funcs import subset_metrics

# Claves del dataset que usa el modelo
DATA_KEYS = ["X_student", "X_geo", "y_true", "below_mask", "id_simulation", "src_id", "tx_start", "orig_index"]

SPLITS = ("train", "val", "test")


# ---------------------------------------------------------------------------
# Rutas
# ---------------------------------------------------------------------------
def cache_path(seed, split):
    return os.path.join(CACHE_DIR, f"seed_{seed}_{split}.npz")

def cache_path_with_N(seed, split, str_N):
    os.makedirs(os.path.join(CACHE_DIR, str_N), exist_ok=True)
    return os.path.join(CACHE_DIR, str_N, f"seed_{seed}_{split}.npz")

def phase_dir(phase, *sub):
    path = os.path.join(RESULTS_DIR, phase, *sub)
    os.makedirs(path, exist_ok=True)
    return path


def best_path(phase):
    return os.path.join(RESULTS_DIR, phase, "best.json")

def results_dir(*sub):
    path = os.path.join(RESULTS_DIR, *sub)
    os.makedirs(path, exist_ok=True)
    return path

"""
def results_dir_with_N(*sub, str_N):
    path = os.path.join(RESULTS_DIR, str_N, *sub)
    os.makedirs(path, exist_ok=True)
    return path
"""

# ---------------------------------------------------------------------------
# Lectura / escritura
# ---------------------------------------------------------------------------
def write_json_atomic(path, obj):
    """Escribe en un archivo temporal y lo renombra: un JSON nunca queda a medias si el proceso muere."""
    tmp = f"{path}.tmp{os.getpid()}"
    with open(tmp, "w") as f:
        json.dump(obj, f, indent=2, default=_json_default)
    os.replace(tmp, path)


def _json_default(o):
    if isinstance(o, (np.floating, np.integer)):
        return o.item()
    if isinstance(o, np.ndarray):
        return o.tolist()
    raise TypeError(f"No serializable: {type(o)}")


def read_json(path):
    with open(path) as f:
        return json.load(f)


def load_seed(seed):
    """Devuelve (train, val, test) como diccionarios de numpy, igual que load_and_prepare_dataset."""
    out = []
    for split in SPLITS:
        with np.load(cache_path(seed, split), allow_pickle=False) as z:
            d = {k: z[k] for k in z.files}
        d["link_ok"] = d["link_ok"].astype(bool)
        out.append(d)
    return tuple(out)


# ---------------------------------------------------------------------------
# Grids
# ---------------------------------------------------------------------------
def normalize_config(cfg):
    """JSON convierte las tuplas en listas; se restauran para que el config sea identico al original."""
    cfg = dict(cfg)
    cfg["static_hidden_dims"] = tuple(cfg["static_hidden_dims"])
    return cfg


def is_valid_config(cfg):
    """d_model debe ser par (encoding temporal) y divisible entre todas las cabezas de atencion."""
    d = cfg["d_model"]
    return d % 2 == 0 and all(d % cfg[h] == 0 for h in ("n_heads", "sab_nheads", "cross_attn_nheads"))


def expand_grid(grid, base):
    """Producto cartesiano del grid sobre la configuracion base. Devuelve la lista en orden estable."""
    keys = list(grid.keys())
    configs = []
    for values in itertools.product(*(grid[k] for k in keys)):
        cfg = dict(base)
        for k, v in zip(keys, values):
            if k == "heads":
                cfg["n_heads"] = cfg["sab_nheads"] = cfg["cross_attn_nheads"] = v
            else:
                cfg[k] = v
        if is_valid_config(cfg):
            configs.append(normalize_config(cfg))
    return configs


def teacher_stage_configs(phase):
    """Configuraciones de una etapa del maestro: parte de la mejor de la etapa anterior."""
    names = [name for name, _ in TEACHER_STAGES]
    idx = names.index(phase)
    if idx == 0:
        base = BASE_TEACHER_CONFIG
    else:
        prev = names[idx - 1]
        if not os.path.exists(best_path(prev)):
            raise FileNotFoundError(f"Falta {best_path(prev)}: ejecuta y agrega la etapa '{prev}' antes de '{phase}'.")
        base = read_json(best_path(prev))["config"]
    return expand_grid(TEACHER_STAGES[idx][1], base)


def last_teacher_stage():
    return TEACHER_STAGES[-1][0]


def selection_is_better(a, b):
    """True si el valor a es mejor que b segun SELECTION_METRIC."""
    if b is None:
        return True
    return a < b if SELECTION_METRIC == "val_loss" else a > b


# ---------------------------------------------------------------------------
# Metricas (mismas definiciones que funcs_teacher_train / func_student_train)
# ---------------------------------------------------------------------------
def auc_pr(y_true, y_prob):
    precision, recall, _ = precision_recall_curve(y_true, y_prob)
    return auc(recall, precision)


def test_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, threshold, below_mask_test = None):
    y_pred = (np.asarray(y_prob_test) >= threshold).astype(float)
    out = {
        "threshold":         float(threshold),
        "val_auc_roc":       roc_auc_score(y_true_val, y_prob_val),
        "val_auc_pr":        auc_pr(y_true_val, y_prob_val),
        "test_accuracy":     accuracy_score(y_true_test, y_pred),
        "test_auc_roc":      roc_auc_score(y_true_test, y_prob_test),
        "test_auc_pr":       auc_pr(y_true_test, y_prob_test),
        "test_prec_clase_0": precision_score(y_true_test, y_pred, pos_label=0, zero_division=0),
        "test_rec_clase_0":  recall_score(y_true_test, y_pred, pos_label=0, zero_division=0),
        "test_f1_clase_0":   f1_score(y_true_test, y_pred, pos_label=0, zero_division=0),
        "test_prec_clase_1": precision_score(y_true_test, y_pred, pos_label=1, zero_division=0),
        "test_rec_clase_1":  recall_score(y_true_test, y_pred, pos_label=1, zero_division=0),
        "test_f1_clase_1":   f1_score(y_true_test, y_pred, pos_label=1, zero_division=0),
    }
    if below_mask_test is not None:
        out.update(subset_metrics(y_true_test, y_prob_test, threshold, below_mask_test))

    return out

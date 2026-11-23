"""
Importar este modulo agrega SCRIPTS_DIR al sys.path para poder usar el codigo de scripts/.
El mensaje se identifica ahora con (simulacion, dispositivo, pid) en build_dataset, 
"""
import os
import sys
import json

import numpy as np

from config import SCRIPTS_DIR, CACHE_DIR, RESULTS_DIR, NTX

if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

from funcs_create_dataset import load_dataset
from network_receiver import message_layout

SPLITS = ("train", "val", "test")

# ---------------------------------------------------------------------------
# Rutas
# ---------------------------------------------------------------------------
def cache_path_with_N(seed, split, str_N):
    os.makedirs(os.path.join(CACHE_DIR, str_N), exist_ok=True)
    return os.path.join(CACHE_DIR, str_N, f"seed_{seed}_{split}.npz")


def results_dir(*sub):
    path = os.path.join(RESULTS_DIR, *sub)
    os.makedirs(path, exist_ok=True)
    return path


def load_split(seed, split, str_N):
    """
    Carga la cache .npz de (seed, split, N). La cache ya tiene solo las filas visibles (elevSat >= 10).

    Agrega la disposicion por mensaje que usan el estudiante y la evaluacion con la replica:
        slot_row    [mensajes x NTX]  fila de cada oportunidad visible del mensaje, en orden de tx_start (-1 = vacio)
        msg_index   mensaje de cada fila (0..M-1), en el mismo orden que slot_row
    """
    data = load_dataset(cache_path_with_N(seed, split, str_N))
    data["slot_row"], data["msg_index"] = message_layout(data["msg_id"], data["tx_start"], NTX)
    return data


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

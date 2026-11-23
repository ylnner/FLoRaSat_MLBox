"""
Paso 0: construye el dataset de cada (seed, split) UNA sola vez y lo guarda en cache/*.npz.
Asi las cientos de tareas de entrenamiento solo cargan un archivo, sin repetir build_dataset.

Uso:
    python prepare_data.py --workers 8            # todas las seeds de config.SEEDS
    python prepare_data.py --seeds 0 1 --workers 2
"""
import os
os.environ.setdefault("OMP_NUM_THREADS", "1")

import io
import sys
import time
import argparse
import contextlib
import concurrent.futures as cf
import multiprocessing as mp

import numpy as np

from config import DATA_DIR, CACHE_DIR, SEEDS
from common import cache_path_with_N, DATA_KEYS, SPLITS


def build_one(seed, split, N):
    str_N = "N_" + str(N)

    # inteta leerel .npz del dataset en caso de existir.
    out = cache_path_with_N(seed, split, str_N)

    if os.path.exists(out):
        return f"seed {seed} {split}: ya existe"

    import funcs_create_dataset

    
    csv  = os.path.join(DATA_DIR, str_N, f"seed_{seed}_final_{split}_data_transmission.csv")
    t0   = time.time()    
    data = funcs_create_dataset.build_dataset(csv)
    
    # Save in npz format the dataset
    tmp = f"{out}.tmp{os.getpid()}.npz"
    np.savez(tmp, **{k: data[k] for k in DATA_KEYS})
    os.replace(tmp, out)
    vis = data["below_mask"] == 0
    return (f"seed {seed} {split}: {len(data['y_true'])} filas | belowMask {100 * (~vis).mean():.1f} % | "
                f"rcvOk {100 * data['y_true'].mean():.1f} % (visibles {100 * data['y_true'][vis].mean():.1f} %) | "
                f"{time.time() - t0:.0f} s")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seeds", type = int, nargs = "+", default = SEEDS)
    parser.add_argument("--workers", type = int, default = 4)
    parser.add_argument("--N", type = int, default = 60)
    args = parser.parse_args()

    # read cache dir to save npz files. 
    os.makedirs(CACHE_DIR, exist_ok=True)
    
    jobs   = [(s, sp) for s in args.seeds for sp in SPLITS]
    ctx    = mp.get_context("spawn")
    failed = False
    with cf.ProcessPoolExecutor(max_workers=args.workers, mp_context=ctx) as pool:        
        # Ejecuta build_one with threads con la configuracion s (seeds) y sp (splits (train, test, val))
        futures = {pool.submit(build_one, s, sp, args.N): (s, sp) for s, sp in jobs}

        for fut in cf.as_completed(futures):
            try:
                # fut.result() devuelve el resultado de la funcion build_one. flush=True pone el output en la pantalla. 
                print(fut.result(), flush=True)
            except Exception as e:
                failed = True
                print(f"ERROR en {futures[fut]}: {e!r}", file=sys.stderr, flush=True)

    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    main()

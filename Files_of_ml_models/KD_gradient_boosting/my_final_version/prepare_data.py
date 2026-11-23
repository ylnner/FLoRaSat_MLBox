"""
Paso 0: construye el dataset de cada (seed, split) una sola vez y lo guarda en cache/*.npz.

build_dataset calcula ademas la etiqueta de red y' (y_net), receivable y el grafo de captura, y guarda solo las
filas visibles. 
Por cada (seed, split) se imprime la verificacion de la replica del receptor contra rcvOk (con todas
las filas transmitiendo, como en el dataset): si baja del 99,9 % hay que revisar la configuracion del simulador
(por ejemplo, si la regla R2 de Doppler esta activa) antes de entrenar. 

Las estadisticas se guardan en: cache/N_*/stats_seed_{seed}_{split}.json.

python prepare_data.py --workers 8 --N 300           # todas las seeds de config.SEEDS
"""
import os
os.environ.setdefault("OMP_NUM_THREADS", "1")

import sys
import time
import argparse
import concurrent.futures as cf
import multiprocessing as mp

import numpy as np

from config import DATA_DIR, CACHE_DIR, SEEDS
from common import cache_path_with_N, write_json_atomic, read_json, SPLITS

MIN_REPLICA_AGREEMENT = 0.999


def build_one(seed, split, N):
    str_N = "N_" + str(N)

    # inteta leerel .npz del dataset en caso de existir.
    out = cache_path_with_N(seed, split, str_N)
    #los CSV estan directamente en DATA_DIR (solo N = 300); str_N solo nombra las carpetas de cache y results
    csv = os.path.join(DATA_DIR, f"seed_{seed}_final_{split}_data_transmission.csv")
    stats_file = os.path.join(CACHE_DIR, str_N, f"stats_seed_{seed}_{split}.json")

    if os.path.exists(out):
        # la cache solo vale si se construyo con este mismo CSV (misma ruta, tamano y fecha). Si no, viene de otro
        # dataset (p. ej. el de ttxIdle 10-15 s) y hay que borrar cache/ y results/ para no entrenar con datos viejos.
        old = {}
        if os.path.exists(stats_file):
            old = read_json(stats_file)
        same_csv = (old.get("csv") == csv and old.get("csv_size") == os.path.getsize(csv)
                    and old.get("csv_mtime") == os.path.getmtime(csv))
        if not same_csv:
            raise RuntimeError(f"{out} se construyo con otro CSV: borra {CACHE_DIR} y {os.path.dirname(CACHE_DIR)}/results")
        return f"seed {seed} {split}: ya existe"

    import funcs_create_dataset

    t0   = time.time()
    data, stats = funcs_create_dataset.build_dataset(csv)

    # Save in npz format the dataset
    tmp = f"{out}.tmp{os.getpid()}.npz"
    np.savez(tmp, **{k: data[k] for k in funcs_create_dataset.DATA_KEYS})
    os.replace(tmp, out)

    stats["seed"] = seed
    stats["split"] = split
    stats["csv"] = csv
    stats["csv_size"] = os.path.getsize(csv)
    stats["csv_mtime"] = os.path.getmtime(csv)
    write_json_atomic(stats_file, stats)

    warning = ""
    if stats["replica_agree_visible"] < MIN_REPLICA_AGREEMENT:
        warning = "  <-- AVISO: la replica no reproduce rcvOk, revisar antes de entrenar"
    return (f"seed {seed} {split}: {stats['rows']} filas, {stats['visible_rows']} visibles | "
            f"replica = rcvOk en {100 * stats['replica_agree_visible']:.3f} % de las visibles | "
            f"rcvOk {100 * stats['rcvOk_visible']:.1f} % -> y' {100 * stats['y_net_visible']:.1f} % | "
            f"{len(data['edge_victim'])} aristas | {time.time() - t0:.0f} s{warning}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seeds", type = int, nargs = "+", default = SEEDS)
    parser.add_argument("--workers", type = int, default = 4)
    parser.add_argument("--N", type = int, default = 300)
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

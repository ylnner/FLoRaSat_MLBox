"""
La seleccion ya NO usa val_ap_visible ni best_alpha_temp.csv (ver el comentario sobre ARMS mas abajo).
Ahora usa student/runs_unique.csv, que escribe select_by_unique.py, y escribe G_off_por_seed.csv en vez de
val_ap_visible_por_seed.csv. El texto original de abajo describe la seleccion anterior.

Selecciona, por cada N y cada fraccion, los modelos del estudiante que se integraran en FLoRaSat: uno KD y uno noKD.
No reentrena nada.

KD:
  1. Toma el mejor alpha/T de cada fraccion de results/N_*/student/best_alpha_temp.csv. Su columna val_ap_visible
     es la media entre seeds de la val_ap_visible de ese alpha/T.
  2. Elige la seed cuya val_ap_visible (logs results/N_*/student/runs/*.json) es la mas cercana a esa media.
noKD:
  1. La columna no_kd_val_ap_visible del mismo CSV es la media entre seeds de la val_ap_visible de no_kd en esa
     fraccion.
  2. Elige la seed de no_kd cuya val_ap_visible es la mas cercana a esa media.

Copia el .pth (models/) y el scaler .json (scalers/) de cada seed elegida, con sus nombres originales, a
results/N_*/best_models/. Ahi escribe tambien:
    seleccion.csv                   el modelo elegido por fraccion y brazo (kd / no_kd); lo usa export_best_models.py
    val_ap_visible_por_seed.csv     todas las seeds, para revisar la eleccion
Antes de copiar borra los .pth, .pt, .json y .csv en la carpeta
"""
import os
import sys
import json
import glob
import shutil

import numpy as np
import pandas as pd

from common import results_dir


# Antes: (alpha, T) por la media de val_ap_visible y seed mas cercana a esa media.
# Ahora: para cada fraccion y brazo se elige el (alpha, T) con la mayor media entre seeds de G_off_pct, es decir,
# los mensajes unicos por encima de la mejor referencia (REP_k o Coin) con las mismas transmisiones, medidos offline
# reproduciendo el MAC del simulador (select_by_unique.py). noKD solo tiene alpha = 0, T = 0.
# Solo se aceptan configuraciones con todas las seeds. Despues se elige la seed cuya G_off_pct esta mas cerca de la
# media (igual que antes, pero con otra metrica). Correlacion G_off offline vs G del simulador: buena con N 60/100
# (+0,86/+0,84), debil con N 200/300 (+0,46/+0,29) -> con N >= 200 la eleccion es menos fiable.
ARMS = {"kd": "student", "no_kd": "no_kd"}
COLS = ["frac", "alpha", "temp", "seed", "name", "G_off", "G_off_pct", "thr_G_off",
        "val_ap_visible", "val_ap_all", "test_ap_visible", "threshold"]
NS   = [f"N_{n}" for n in (sys.argv[1:] or (60, 100, 200, 300))]

for str_N in NS:
    stud = results_dir(str_N, "student")
    runs = pd.DataFrame([dict(json.load(open(f)), name=os.path.basename(f)[:-5])
                         for f in glob.glob(os.path.join(stud, "runs", "*.json"))])
    uni  = pd.read_csv(os.path.join(stud, "runs_unique.csv"))[["name", "G_off", "G_off_pct", "thr_G_off"]]
    runs = runs.merge(uni, on="name", how="inner")
    n_seeds = runs.seed.nunique()
    out  = results_dir(str_N, "best_models")
    for f in os.listdir(out):
        if f.endswith((".pth", ".pt", ".json", ".csv")):
            os.remove(os.path.join(out, f))
    filas, sel = [], []

    for frac in sorted(runs.frac.unique()):
        for arm, teacher in ARMS.items():
            r = runs[(runs.teacher == teacher) & np.isclose(runs.frac, frac)]
            grid = r.groupby(["alpha", "temp"]).agg(n=("seed", "nunique"), mean=("G_off_pct", "mean")).reset_index()
            grid = grid[grid.n == n_seeds]
            b = grid.loc[grid["mean"].idxmax()]

            df = r[np.isclose(r.alpha, b.alpha) & np.isclose(r.temp, b.temp)][COLS].sort_values("seed").copy()
            df.insert(0, "arm", arm); df.insert(0, "N", str_N)
            df["mean_G_off_pct"]  = b["mean"]
            df["dist_a_la_media"] = (df.G_off_pct - b["mean"]).abs()
            g = df.loc[df.dist_a_la_media.idxmin()]
            filas.append(df); sel.append(g)

            for sub, ext in (("models", ".pth"), ("scalers", ".json")):
                shutil.copy2(os.path.join(stud, sub, g["name"] + ext), out)
            print(f"{str_N} {arm:5s} frac={frac:.2f} -> alpha {b.alpha} T {b.temp} seed {g.seed}: "
                  f"G_off {g.G_off_pct:.2f} % (media {b['mean']:.2f} %), umbral offline {g.thr_G_off}  {g['name']}")

    pd.concat(filas).to_csv(os.path.join(out, "G_off_por_seed.csv"), index=False)
    pd.DataFrame(sel).to_csv(os.path.join(out, "seleccion.csv"), index=False)

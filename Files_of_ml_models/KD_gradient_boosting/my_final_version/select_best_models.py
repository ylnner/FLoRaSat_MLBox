"""
Selecciona, por cada N, los modelos del estudiante que se integraran en FLoRaSat: uno KD y uno noKD (solo 100 %).
No reentrena nada.

La seleccion usa la misma metrica que el early stopping: mensajes unicos de validacion frente a FV_1 (en %),
medidos con la replica del receptor y la regla de despliegue (p >= 0.5, como mucho 1 copia). 

1. Toma la mejor configuracion (mu, alpha, T) de results/N_*/student/best_config.csv (la escribe train_student.py:
    mayor media entre seeds de val_gain_fv1_pct; empate -> menos transmisiones).
2. Elige la seed cuya val_gain_fv1_pct (logs results/N_*/student/runs/*.json) es la mas cercana a esa media.

Copia el .pth (models/) y el scaler .json (scalers/) de cada seed elegida, con sus nombres originales, a
results/N_*/best_models/. Ahi escribe tambien:
    seleccion.csv                   el modelo elegido por brazo (kd / no_kd); lo usa export_best_models.py
    val_gain_por_seed.csv           todas las seeds de la configuracion elegida, para revisar la eleccion
Antes de copiar borra los .pth, .pt, .json y .csv en la carpeta

python select_best_models.py 300
"""
import os
import sys
import json
import glob
import shutil

import numpy as np
import pandas as pd

from common import results_dir


ARMS = {"kd": "student", "no_kd": "no_kd"}
COLS = ["mu", "alpha", "temp", "seed", "name", "best_epoch", "val_unique", "val_tx", "val_gain_fv1_pct",
        "test_unique", "test_tx", "test_gain_fv1_pct", "threshold"]
NS   = [f"N_{n}" for n in (sys.argv[1:] or (300,))]

for str_N in NS:
    stud = results_dir(str_N, "student")
    runs = []
    for f in glob.glob(os.path.join(stud, "runs", "*.json")):
        run = json.load(open(f))
        run["name"] = os.path.basename(f)[:-5]
        runs.append(run)
    runs = pd.DataFrame(runs)
    best = pd.read_csv(os.path.join(stud, "best_config.csv"))

    out  = results_dir(str_N, "best_models")
    for f in os.listdir(out):
        if f.endswith((".pth", ".pt", ".json", ".csv")):
            os.remove(os.path.join(out, f))
    filas, sel = [], []

    for arm, teacher in ARMS.items():
        b = best[best["teacher"] == teacher]
        assert len(b) == 1, f"{str_N}: best_config.csv no tiene el brazo {teacher}"
        b = b.iloc[0]

        same = (runs["teacher"] == teacher) & np.isclose(runs["mu"], b["mu"]) & \
               np.isclose(runs["alpha"], b["alpha"]) & np.isclose(runs["temp"], b["temp"])
        df = runs[same][COLS].sort_values("seed").copy()
        df.insert(0, "arm", arm); df.insert(0, "N", str_N)
        df["mean_val_gain_fv1_pct"] = b["val_gain_fv1_pct"]
        df["dist_a_la_media"]       = (df["val_gain_fv1_pct"] - b["val_gain_fv1_pct"]).abs()
        g = df.loc[df["dist_a_la_media"].idxmin()]
        filas.append(df); sel.append(g)

        for sub, ext in (("models", ".pth"), ("scalers", ".json")):
            shutil.copy2(os.path.join(stud, sub, g["name"] + ext), out)
        print(f"{str_N} {arm:5s} -> mu {b['mu']:g} alpha {b['alpha']:g} T {b['temp']:g} seed {g['seed']}: "
              f"val {g['val_gain_fv1_pct']:+.2f} % vs FV_1 (media {b['val_gain_fv1_pct']:+.2f} %) | "
              f"test {g['test_gain_fv1_pct']:+.2f} %  {g['name']}")

    pd.concat(filas).to_csv(os.path.join(out, "val_gain_por_seed.csv"), index=False)
    pd.DataFrame(sel).to_csv(os.path.join(out, "seleccion.csv"), index=False)

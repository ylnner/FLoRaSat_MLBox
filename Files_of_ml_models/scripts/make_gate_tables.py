#!/usr/bin/env python3
"""Write the u -> threshold tables the load control gate reads.

For an admission factor u, a device keeps the top fraction u of its opportunities, so tau(u) is the
(1 - u) quantile of the score above the elevation mask, taken on the validation split of seed 2,
the seed of the deployed Transformer. The closed loop corrects any mismatch, because u follows the
measured load, so these tables only need the right order and rough scale.

Usage: make_gate_tables.py <repo> [--no-model]
Needs numpy and pandas, plus torch and joblib (with scikit-learn) unless --no-model is given.
"""
import sys
from pathlib import Path

import numpy as np
import pandas as pd

REPO = Path(sys.argv[1])
SCRIPTS = REPO / "Files_of_ml_models/scripts"
OUT = REPO / "simulations/Test_Individual/gate"
FEATS = ["latDev", "longDev", "elevSat", "loraTP", "loraSF", "doppler", "alt", "raan"]
SEQ, MASK = 8, 10.0
US = np.round(np.arange(0.02, 1.0001, 0.02), 2)


def write(name, scores):
    table = pd.DataFrame({"u": US, "tau": np.quantile(scores, 1 - US)})
    table.to_csv(OUT / name, index=False, float_format="%.6f")
    print(f"{name}: {len(scores)} frames, tau(0.5) = {np.quantile(scores, 0.5):.4f}")


OUT.mkdir(exist_ok=True)
val = pd.read_csv(SCRIPTS / "data/repetitions/seed_2_final_val_data_transmission.csv")
write("tau_elevation.csv", val.elevSat[val.elevSat >= MASK].values)

if "--no-model" not in sys.argv:
    import joblib
    import torch

    model = torch.jit.load(str(SCRIPTS / "best_models/transformer_scripted.pt")).eval()
    scaler = joblib.load(SCRIPTS / "best_models/transformer_scaler_seed2.pkl")
    scaler_dt = joblib.load(SCRIPTS / "best_models/transformer_scaler_delta_t_seed2.pkl")
    windows, elevation = [], []
    for _, g in val.groupby(["id_simulation", "srcId"]):
        g = g.sort_values("time")
        f, t = scaler.transform(g[FEATS]), g.time.values
        for i in range(len(g) - SEQ + 1):
            dt = (t[i + SEQ - 1] - t[i:i + SEQ]).reshape(-1, 1)
            windows.append(np.hstack((f[i:i + SEQ], scaler_dt.transform(dt), dt)))
            elevation.append(g.elevSat.values[i + SEQ - 1])
    with torch.no_grad():
        logits = model(torch.tensor(np.array(windows), dtype=torch.float32)).numpy().ravel()
    p = 1 / (1 + np.exp(-logits))
    write("tau_transformer.csv", p[np.array(elevation) >= MASK])

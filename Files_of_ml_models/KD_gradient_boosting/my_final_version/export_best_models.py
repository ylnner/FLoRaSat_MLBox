"""
Prepara los modelos de results/N_*/best_models/ (creados por select_best_models.py) para FLoRaSat.

Lee best_models/seleccion.csv y, por cada modelo elegido (KD y noKD, solo 100 %), escribe en la misma carpeta:
    KD:    mlp_kd_100_scripted.pt     + fr100.json
    noKD:  mlp_nokd_100_scripted.pt   + fr100_nokd.json
El .pt es el modelo en TorchScript (torch.jit.script). El .json es una copia del scaler de ese modelo con nombre
generico (inputs, mean, scale, threshold). 

El modelo TorchScript y el .pth original deben dar los mismos logits sobre una entrada aleatoria.

python export_best_models.py 300
"""
import os
import sys
import shutil

import pandas as pd
import torch

from common import results_dir            # agrega SCRIPTS_DIR al sys.path
from StudentModel import StudentModel
from funcs_create_dataset import STUDENT_INPUTS

NS = [f"N_{n}" for n in (sys.argv[1:] or (300,))]
NAMES = {"kd":    ("mlp_kd_100_scripted.pt",   "fr100.json"),
         "no_kd": ("mlp_nokd_100_scripted.pt", "fr100_nokd.json")}

for str_N in NS:
    path = os.path.join(results_dir(str_N), "best_models")
    sel_csv = os.path.join(path, "seleccion.csv")
    if not os.path.isfile(sel_csv):
        print(f"Aviso: no existe {sel_csv} (ejecuta antes select_best_models.py)")
        continue
    sel = pd.read_csv(sel_csv)
    assert not sel.duplicated(["arm"]).any(), f"{str_N}: mas de un modelo por brazo en {sel_csv}"

    for _, r in sel.iterrows():
        pt_name, sc_name = NAMES[r.arm]

        model = StudentModel()
        model.load_state_dict(torch.load(os.path.join(path, r["name"] + ".pth"), map_location="cpu"))
        model.eval()

        out_pt = os.path.join(path, pt_name)
        torch.jit.script(model).save(out_pt)

        x = torch.randn(256, len(STUDENT_INPUTS))
        with torch.no_grad():
            assert torch.allclose(model(x), torch.jit.load(out_pt)(x), atol=1e-6), f"{out_pt}: logits distintos"

        shutil.copy2(os.path.join(path, r["name"] + ".json"), os.path.join(path, sc_name))
        n_par = sum(p.numel() for p in model.parameters())
        print(f"{str_N} {r.arm:5s}: {r['name']}.pth -> {pt_name} ({n_par} parametros) + {sc_name}")

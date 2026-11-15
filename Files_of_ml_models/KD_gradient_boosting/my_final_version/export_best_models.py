"""
Prepara los modelos de results/N_*/best_models/ (creados por select_best_models.py) para FLoRaSat.

Lee best_models/seleccion.csv y, por cada modelo elegido (KD y noKD de cada fraccion), escribe en la misma carpeta:
    KD:    mlp_kd_{005,010,015,100}_scripted.pt     + fr{005,010,015,100}.json
    noKD:  mlp_nokd_{005,010,015,100}_scripted.pt   + fr{005,010,015,100}_nokd.json
El .pt es el modelo en TorchScript (torch.jit.script). El .json es una copia del scaler de ese modelo con nombre
generico (inputs, mean, scale, threshold).

El modelo TorchScript y el .pth original deben dar los mismos logits sobre una entrada aleatoria.

"""
import os
import sys
import shutil

import pandas as pd
import torch

from common import results_dir            # agrega SCRIPTS_DIR al sys.path
from StudentModel import StudentModel

NS = [f"N_{n}" for n in (sys.argv[1:] or (60, 100, 200, 300))]
CODE = {0.05: "005", 0.10: "010", 0.15: "015", 1.00: "100"}
NAMES = {"kd":    ("mlp_kd_{}_scripted.pt",   "fr{}.json"),
         "no_kd": ("mlp_nokd_{}_scripted.pt", "fr{}_nokd.json")}

for str_N in NS:
    path = os.path.join(results_dir(str_N), "best_models")
    sel_csv = os.path.join(path, "seleccion.csv")
    if not os.path.isfile(sel_csv):
        print(f"Aviso: no existe {sel_csv} (ejecuta antes select_best_models.py)")
        continue
    sel = pd.read_csv(sel_csv)
    assert not sel.duplicated(["arm", "frac"]).any(), f"{str_N}: mas de un modelo por brazo y fraccion en {sel_csv}"

    for _, r in sel.iterrows():
        code = CODE[round(r.frac, 2)]
        pt_name, sc_name = (n.format(code) for n in NAMES[r.arm])

        model = StudentModel()
        model.load_state_dict(torch.load(os.path.join(path, r["name"] + ".pth"), map_location="cpu"))
        model.eval()

        out_pt = os.path.join(path, pt_name)
        torch.jit.script(model).save(out_pt)

        x = torch.randn(256, 8)
        with torch.no_grad():
            assert torch.allclose(model(x), torch.jit.load(out_pt)(x), atol=1e-6), f"{out_pt}: logits distintos"

        shutil.copy2(os.path.join(path, r["name"] + ".json"), os.path.join(path, sc_name))
        n_par = sum(p.numel() for p in model.parameters())
        print(f"{str_N} {r.arm:5s}: {r['name']}.pth -> {pt_name} ({n_par} parametros) + {sc_name}")

"""
Reconstruye y guarda los scalers de los estudiantes YA entrenados (no reentrena nada).

Hasta el 2026-10-09 run_train_student.py no guardaba los scalers. No hace falta reentrenar: el scaler de un
estudiante se ajusta solo con su subconjunto de train, que es determinista (subset_indices(n, seed, fraccion)
sobre la misma cache), asi que se reconstruye exactamente igual que en el entrenamiento.

Para cada runs/*.json de results/student_low_data (MLP actual) y results/student_low_data_small (MLP pequeno)
escribe, con save_student_scalers de run_train_student.py (el mismo formato que usan ahora los entrenamientos):
    scalers/{nombre_del_run}.pkl    {'sequential', 'static'} (joblib), para Python
    scalers/{nombre_del_run}.json   media/escala de las 8 entradas en orden + umbral de validacion, para el simulador

Verificacion: para un modelo de cada fraccion, escala el test en bruto SOLO con el JSON (como lo haria el
simulador), predice con el .pth y compara el AUC-PR de test con el guardado en el run.

Uso:  python export_student_scalers.py
"""
import os
import glob
import json
from collections import defaultdict

import numpy as np
import torch

import run_train_student as base           # save_student_scalers, subset_indices, task_name, PHASE
from common import load_seed, auc_pr
from config import RESULTS_DIR
from global_funcs import create_scalers
from StudentModel import StudentModel

PHASES = {"student_low_data": (64, 32), "student_low_data_small": (16,)}   # carpeta -> capas ocultas del MLP


def raw_inputs(split):
    """Las 8 entradas del estudiante SIN escalar, en el orden de STUDENT_INPUTS."""
    return np.concatenate([split["seq_feats"][:, -1, :2], split["static_feats"]], axis=1).astype(np.float64)


def check(phase, hidden, run, test):
    name = base.task_name(run["fraction"], run["arm"], run["seed"])
    sc   = json.load(open(os.path.join(RESULTS_DIR, phase, "scalers", f"{name}.json")))
    x    = (raw_inputs(test) - np.array(sc["mean"])) / np.array(sc["scale"])
    model = StudentModel(n_features=8, hidden_dims=hidden, dropout=0.1)
    model.load_state_dict(torch.load(os.path.join(RESULTS_DIR, phase, "models", f"{name}.pth"), map_location="cpu"))
    model.eval()
    with torch.no_grad():
        p = torch.sigmoid(model.mlp(torch.as_tensor(x, dtype=torch.float32)).squeeze(-1)).numpy()
    return auc_pr(test["y_true"], p), run["best_threshold"]["test_auc_pr"]


def main():
    for phase, hidden in PHASES.items():
        base.PHASE = phase                                     # save_student_scalers escribe en esta carpeta
        runs = [json.load(open(f)) for f in sorted(glob.glob(os.path.join(RESULTS_DIR, phase, "runs", "*.json")))]
        by_seed = defaultdict(list)
        for r in runs:
            by_seed[r["seed"]].append(r)

        checked = {}
        for seed, seed_runs in sorted(by_seed.items()):
            train, _, test = load_seed(seed)
            scalers_by_frac = {}
            for r in seed_runs:
                f = r["fraction"]
                if f not in scalers_by_frac:
                    idx = base.subset_indices(len(train["y_true"]), seed, f)
                    assert len(idx) == r["n_train"], f"{phase} {f} seed {seed}: subconjunto distinto al del run"
                    scalers_by_frac[f] = create_scalers({k: v[idx] for k, v in train.items()})
                base.save_student_scalers(scalers_by_frac[f], base.task_name(f, r["arm"], seed),
                                          r["best_threshold"]["threshold"])
                if seed == 0 and f not in checked:
                    checked[f] = check(phase, hidden, r, test)
        print(f"{phase}: scalers de {len(runs)} modelos en results/{phase}/scalers/")
        for f, (got, ref) in sorted(checked.items()):
            print(f"    fraccion {f:.0%}: AUC-PR test con el JSON {got:.6f} | guardado en el run {ref:.6f} "
                  f"| {'OK' if abs(got - ref) < 1e-5 else 'DISTINTO'}")


if __name__ == "__main__":
    main()

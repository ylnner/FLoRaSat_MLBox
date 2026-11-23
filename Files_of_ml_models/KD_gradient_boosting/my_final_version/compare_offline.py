"""
Validacion offline (antes de simular): compara en el test de todas las seeds, con la replica del receptor,
    - los modelos v3 elegidos (results/N_*/best_models/mlp_{kd,nokd}_100_scripted.pt + fr100{,_nokd}.json)
    - los modelos v2 desplegados hoy en FLoRaSat (solo lectura, V2_MODELS_DIR), con dos reglas:
          (v3 sin raan: estos modelos tienen 8 entradas; se les pasa X_student + raan de la cache)
          budget1   p >= 0.5, como mucho 1 copia (la regla de v3)
          stop0.8   la regla de parada actual del .ini (stopTarget = 0.8, stopGamma = 1) con el umbral del grid de
                    los .ini que mas mensajes entrega en el test (es optimista para v2: el umbral se elige en test)
    - referencias: FV_1, FV_2 y REP (solo consideran todas las copias visibles)

Los modelos se usan como en el simulador: TorchScript + escalado SOLO con el JSON (x - mean) / scale.
Cada modelo v3 elegido se entreno con una sola seed; el test de las otras seeds son simulaciones distintas, asi que
todas cuentan como datos no vistos.

Output (results/N_*/compare_offline/):
    per_simulation.csv   mensajes unicos y transmisiones de cada politica en cada simulacion de test
    summary.csv          total por politica, diferencia frente a FV_1 en % y error estandar pareado por simulacion


python compare_offline.py 300
"""
import os
import sys
import json

import numpy as np
import pandas as pd
import torch

from config import SEEDS, NTX
from common import results_dir, load_split
from network_receiver import sequential_budget, received

# Modelos desplegados (FLoRaSat_MLBox, solo lectura)
V2_MODELS_DIR = "/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/simulations/Test_KD/mlmodels"
INI_THRESHOLDS = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95]     # grid de los .ini
STOP_TARGET    = 0.8
STOP_GAMMA     = 1.0
NS = [f"N_{n}" for n in (sys.argv[1:] or (300,))]


def load_deployed(pt_path, json_path):
    """Modelo TorchScript + scaler JSON, igual que Transmission_Predictor_MLP_KD.cc."""
    model  = torch.jit.load(pt_path)
    model.eval()
    scaler = json.load(open(json_path))
    return model, np.array(scaler["mean"]), np.array(scaler["scale"])


def deployed_proba(deployed, x_raw):
    model, mean, scale = deployed
    x = torch.as_tensor((x_raw - mean) / scale, dtype=torch.float32)
    with torch.no_grad():
        return torch.sigmoid(model(x)).numpy()


def stop_rule_tx(slot_row, p, threshold):
    """
    Regla de parada de FLoRasat (Ter_Dlk_KiWan.cc): en cada oportunidad visible transmite si p >= threshold y
    1 - prod(1 - gamma * p de las copias ya enviadas) < STOP_TARGET. Se recorre oportunidad a oportunidad
    (NTX = 5 pasos), vectorizado sobre todos los mensajes a la vez.
    """
    valid    = slot_row >= 0
    safe     = np.where(valid, slot_row, 0)
    p_slot   = np.where(valid, p[safe], 0.0)
    cum_miss = np.ones(len(slot_row))
    tx       = np.zeros(len(p), dtype=bool)
    for s in range(slot_row.shape[1]):
        send = valid[:, s] & (p_slot[:, s] >= threshold) & (1.0 - cum_miss < STOP_TARGET)
        tx[slot_row[send, s]] = True
        cum_miss[send] *= 1.0 - STOP_GAMMA * p_slot[send, s]
    return tx


def per_simulation(data, tx, seed, policy):
    ok   = received(data["edge_victim"], data["edge_killer"], data["receivable"], tx)
    rows = []
    for sim in np.unique(data["id_simulation"]):
        in_sim = data["id_simulation"] == sim
        rows.append(dict(seed=seed, id_simulation=str(sim), policy=policy,
                         unique=len(np.unique(data["msg_index"][ok & in_sim])),
                         tx=int((tx & in_sim).sum())))
    return rows


for str_N in NS:
    best_dir = os.path.join(results_dir(str_N), "best_models")
    v3 = {}
    for arm, pt, js in (("v3_kd", "mlp_kd_100_scripted.pt", "fr100.json"),
                        ("v3_nokd", "mlp_nokd_100_scripted.pt", "fr100_nokd.json")):
        if os.path.isfile(os.path.join(best_dir, pt)):
            v3[arm] = load_deployed(os.path.join(best_dir, pt), os.path.join(best_dir, js))
        else:
            print(f"Aviso: falta {best_dir}/{pt} (ejecuta export_best_models.py); se compara sin {arm}")
    v2 = {}
    for arm, pt, js in (("v2_kd", "mlp_kd_100_scripted.pt", "fr100.json"),
                        ("v2_nokd", "mlp_nokd_100_scripted.pt", "fr100_nokd.json")):
        v2[arm] = load_deployed(os.path.join(V2_MODELS_DIR, str_N, pt), os.path.join(V2_MODELS_DIR, str_N, js))

    rows = []
    for seed in SEEDS:
        test  = load_split(seed, "test", str_N)
        every = np.ones(len(test["y_net"]), dtype=bool)
        for ref, budget in (("FV_1", 1), ("FV_2", 2), ("REP", NTX)):
            rows += per_simulation(test, sequential_budget(test["slot_row"], every, budget), seed, ref)

        for arm, deployed in v3.items():
            p = deployed_proba(deployed, test["X_student"])
            rows += per_simulation(test, sequential_budget(test["slot_row"], p >= 0.5, 1), seed, arm)

        # v3 sin raan: los modelos desplegados hoy tienen 8 entradas (las 7 del estudiante + raan, en ese orden)
        x_with_raan = np.column_stack([test["X_student"], test["raan"]])
        for arm, deployed in v2.items():
            p = deployed_proba(deployed, x_with_raan)
            rows += per_simulation(test, sequential_budget(test["slot_row"], p >= 0.5, 1), seed, f"{arm}_budget1")
            for thr in INI_THRESHOLDS:
                rows += per_simulation(test, stop_rule_tx(test["slot_row"], p, thr), seed, f"{arm}_stop0.8_th{thr:g}")
        print(f"{str_N} seed {seed}: listo", flush=True)

    per_sim = pd.DataFrame(rows)

    # Con la regla de parada stopRule: solo el umbral que mas mensajes entrega (en total) por brazo
    totals = per_sim.groupby("policy")["unique"].sum()
    for arm in v2:
        candidates = [p for p in totals.index if p.startswith(f"{arm}_stop0.8_th")]
        best_thr   = totals[candidates].idxmax()
        drop       = [p for p in candidates if p != best_thr]
        per_sim    = per_sim[~per_sim["policy"].isin(drop)]

    out_dir = results_dir(str_N, "compare_offline")
    per_sim.to_csv(os.path.join(out_dir, "per_simulation.csv"), index=False)

    # Diferencia pareada frente a FV_1 en cada (seed, simulacion)
    wide = per_sim.pivot_table(index=["seed", "id_simulation"], columns="policy", values="unique")
    wide_tx = per_sim.pivot_table(index=["seed", "id_simulation"], columns="policy", values="tx")
    summary = []
    for policy in wide.columns:
        diff = wide[policy] - wide["FV_1"]
        summary.append(dict(policy        = policy,
                            unique        = wide[policy].sum(),
                            tx            = wide_tx[policy].sum(),
                            gain_fv1_pct  = 100.0 * diff.sum() / wide["FV_1"].sum(),
                            diff_per_sim  = diff.mean(),
                            se_per_sim    = diff.std(ddof=1) / np.sqrt(len(diff)),
                            n_sims        = len(diff)))
    summary = pd.DataFrame(summary).sort_values("unique", ascending=False)
    summary.to_csv(os.path.join(out_dir, "summary.csv"), index=False)

    print(f"\n{str_N}: test de {len(SEEDS)} seeds ({len(wide)} simulaciones), replica del receptor")
    for _, r in summary.iterrows():
        print(f"  {r['policy']:24s} unicos {r['unique']:8.0f}  tx {r['tx']:8.0f}  {r['gain_fv1_pct']:+6.2f} % vs FV_1  "
              f"(por simulacion {r['diff_per_sim']:+.2f} +- {r['se_per_sim']:.2f})")
    print(f"guardado en {out_dir}/summary.csv y per_simulation.csv", flush=True)

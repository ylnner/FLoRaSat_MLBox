"""
Extraccion del lazo cerrado de robustez EU_950 con el MODELO SIN RAAN (v3_sequential_noraan, 7 entradas).
Mismas metricas y chequeos que ../analysis_robustez_EU670/extract_eu670.py, para este escenario:
    - la corrida es posterior a NEW_RUNS_AFTER (modelos sin raan y C++ de 7 entradas desplegados el 2026-11-21 00:20)
    - la config del .sca es la del escenario (altitude 950.0, meanAnomaly 106.0, epochDay = baseDay = 1.5)
    - los dispositivos son los del escenario (Europa, set ML): longitud del terminal[0] en el rango esperado
    - el MLP tiene los 10 umbrales (0.10 ... 0.95), con y sin moneda (q = 0.5 / 0.7)

Carpeta: {ROOT}/Robustez_closed_loop/EU_950/N_<n>/results/   (n = 60, 100, 300, 500, 1000)
    KD_coin / NoKD_coin   ML_closed_loop_StopRule_Coin/FR_100p/mlp_{kd,no_kd}-coin-<q>-stop-0.01-raan-<r>-th-<θ>-General-<run>
    KD_v3 / NoKD_v3       ML_closed_loop_StopRule/FR_100p/mlp_{kd,no_kd}-stop-0.01-raan-<r>-th-<θ>-General-<run>   (q = 1)
    FV_1, FV_2            No_ML_FirstVisible/noML_FV-budget-<k>-raan-<r>-General-<run>
    RV_1, RV_2            No_ML_RandomVisible/noML_RV-budget-<k>-raan-<r>-General-<run>
    REP_1 ... REP_5       No_ML_Rep_1to5/REP_<k>/noML-raan-<r>-General-<run>

Uso:  python3 extract_eu950.py        Escribe data/per_run_eu950.csv. Lee el simulador en solo lectura.
"""
import csv
import glob
import os
import re
import time
from collections import Counter
from multiprocessing import Pool

import numpy as np

ROOT = "/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/simulations/Test_KD"
SCENARIOS = {"EU_950": 950.0}
DENSITIES = [60, 100, 300, 500, 1000]
# Modelos sin raan copiados a mlmodels/N_300 el 2026-11-21 00:09 y libflorasat.so (7 entradas) compilada a las 00:20
NEW_RUNS_AFTER = time.mktime((2026, 11, 21, 0, 20, 0, 0, 0, -1))
EXPECTED_ORBIT = {"*.satellite[0].NoradModule.meanAnomaly": 106.0,
                  "*.satellite[*].NoradModule.epochDay": 1.5, "*.satellite[*].NoradModule.baseDay": 1.5}
# Longitud del primer dispositivo: Europa (set ML) entre -30 y 45; Norteamerica entre -125 y -60
DEVICE_LONGITUDE_RANGE = (-30.0, 45.0)
OUT_CSV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "per_run_eu950.csv")
RUNS_PER_CONFIG = 50
NOT_APPLICABLE = -1.0
MODEL_FILE = {"KD": "mlp_kd_100_scripted.pt", "NoKD": "mlp_nokd_100_scripted.pt"}

TERMINAL = {"numTransmissionsApproved": "tx", "transmittedMessages": "msgs",
            "numSkippedByMLCoin": "skipped_coin", "numSkippedByStopRule": "skipped_stop"}
SAT_MAC = {"Number of unique messages received": "unique", "Number of successful receptions": "recv",
           "Number of duplicate transmissions": "duplicates"}
NODE_PREFIX = "Unique messages from Node "
VEC_SLOT = "Successful Repetition Number counted for MAC protocol"
VEC_DELAY = "All Nodes Message Generation Delay at Satellite"

COLUMNS = ["scenario", "N", "method", "threshold", "ml_coin_prob", "raan", "run",
           "tx", "msgs", "unique", "recv", "sat_rx_error", "der", "sat_below_sensitivity",
           "skipped_coin", "skipped_stop", "nodes_served", "jain", "slot1_frac", "slot_mean", "delay_mean"]

ML_COIN_NAME = re.compile(r"mlp_(kd|no_kd)-coin-([0-9.]+)-stop-0\.01-raan-(\d+)-th-([0-9.]+)-General-(\d+)\.sca$")
ML_NAME = re.compile(r"mlp_(kd|no_kd)-stop-0\.01-raan-(\d+)-th-([0-9.]+)-General-(\d+)\.sca$")
VIS_NAME = re.compile(r"noML_(FV|RV)-budget-(\d)-raan-(\d+)-General-(\d+)\.sca$")
REP_NAME = re.compile(r"noML-raan-(\d+)-General-(\d+)\.sca$")


def list_runs():
    """(path, campos del nombre) de cada corrida del docstring, para todos los N."""
    runs = []
    for scenario in SCENARIOS:
        runs += list_runs_scenario(scenario)
    return runs


def list_runs_scenario(scenario):
    runs = []
    for n in DENSITIES:
        folder = f"{ROOT}/Robustez_closed_loop/{scenario}/N_{n}/results"
        for f in sorted(glob.glob(f"{folder}/ML_closed_loop_StopRule_Coin/FR_100p/*.sca")):
            m = ML_COIN_NAME.search(f)
            if m.group(1) == "kd":
                arm = "KD"
            else:
                arm = "NoKD"
            runs.append((f, dict(scenario=scenario, N=n, method=f"{arm}_coin", arm=arm, threshold=float(m.group(4)),
                                 ml_coin_prob=float(m.group(2)), raan=int(m.group(3)), run=int(m.group(5)))))
        for f in sorted(glob.glob(f"{folder}/ML_closed_loop_StopRule/FR_100p/*.sca")):
            m = ML_NAME.search(f)
            if m.group(1) == "kd":
                arm = "KD"
            else:
                arm = "NoKD"
            runs.append((f, dict(scenario=scenario, N=n, method=f"{arm}_v3", arm=arm, threshold=float(m.group(3)), ml_coin_prob=1.0,
                                 raan=int(m.group(2)), run=int(m.group(4)))))
        for f in sorted(glob.glob(f"{folder}/No_ML_FirstVisible/*.sca") + glob.glob(f"{folder}/No_ML_RandomVisible/*.sca")):
            m = VIS_NAME.search(f)
            runs.append((f, dict(scenario=scenario, N=n, method=f"{m.group(1)}_{m.group(2)}", threshold=NOT_APPLICABLE,
                                 ml_coin_prob=NOT_APPLICABLE, raan=int(m.group(3)), run=int(m.group(4)))))
        for k in range(1, 6):
            for f in sorted(glob.glob(f"{folder}/No_ML_Rep_1to5/REP_{k}/*.sca")):
                m = REP_NAME.search(f)
                runs.append((f, dict(scenario=scenario, N=n, method=f"REP_{k}", threshold=NOT_APPLICABLE, ml_coin_prob=NOT_APPLICABLE,
                                     raan=int(m.group(1)), run=int(m.group(2)))))
    return runs


def read_sca(path, n):
    """Escalares del docstring, unicos por nodo y lineas config."""
    values = {name: 0.0 for name in TERMINAL.values()}
    has_coin_scalar = False
    per_node = []
    config = {}
    with open(path, errors="ignore") as fh:
        for line in fh:
            if line.startswith("config "):
                p = line.rstrip("\n").split(" ", 2)
                config[p[1]] = p[2].strip().strip('"').strip('\\"').strip('"')
                continue
            if not line.startswith("scalar "):
                continue
            p = line.rstrip("\n").split(" ")
            module, name, value = p[1], " ".join(p[2:-1]).strip('"'), float(p[-1])
            if ".terminal[" in module and module.endswith(".dlk.mac") and name in TERMINAL:
                values[TERMINAL[name]] += value
                if name == "numSkippedByMLCoin":
                    has_coin_scalar = True
            elif module.endswith("satellite[0].dlkDSL.mac"):
                if name in SAT_MAC:
                    values[SAT_MAC[name]] = value
                elif name.startswith(NODE_PREFIX):
                    per_node.append(value)
            elif module.endswith("satellite[0].phyDSL.radio.receiver") and name == "rcvBelowSensitivity":
                values["sat_below_sensitivity"] = value
            elif module.endswith("satellite[0].phyDSL.radio"):
                if name == "Number of receptions finished with error":
                    values["sat_rx_error"] = value
                elif name == "DER - Data Extraction Rate":
                    values["der"] = value
    u = np.zeros(n)
    u[:len(per_node)] = per_node
    values["nodes_served"] = float((u > 0).sum())
    values["jain"] = float(u.sum() ** 2 / (n * (u ** 2).sum()))
    values["per_node_sum"] = float(u.sum())
    return values, config, has_coin_scalar


def read_vec(path):
    """Slot y retardo de cada mensaje unico (vectores del satelite)."""
    ids = {}
    data = {VEC_SLOT: [], VEC_DELAY: []}
    with open(path, errors="ignore") as fh:
        for line in fh:
            if line.startswith("vector "):
                for name in data:
                    if f'satellite[0].dlkDSL.mac "{name}"' in line:
                        ids[line.split(" ")[1]] = name
            elif line[:1].isdigit():
                p = line.split()
                if p[0] in ids:
                    data[ids[p[0]]].append(float(p[-1]))
    return np.array(data[VEC_SLOT]), np.array(data[VEC_DELAY])


def check_config(fields, config):
    """La config escrita en el .sca debe coincidir con el nombre del archivo y con el escenario."""
    problems = []
    if abs(float(config.get("*.satellite[*].NoradModule.altitude", "nan")) - SCENARIOS[fields["scenario"]]) > 1e-9:
        problems.append("altitude != escenario")
    if config.get("**.numTerminals") != str(fields["N"]):
        problems.append("numTerminals != N")
    for key, expected in EXPECTED_ORBIT.items():
        if abs(float(config.get(key, "nan")) - expected) > 1e-9:
            problems.append(f"{key} != {expected}")
    if abs(float(config.get("*.satellite[0].NoradModule.raan", "nan")) - fields["raan"]) > 1e-9:
        problems.append("raan de la config != nombre")
    longitude = float(config.get("*.terminal[0].mob.centerLongitude", "nan"))
    if not (DEVICE_LONGITUDE_RANGE[0] <= longitude <= DEVICE_LONGITUDE_RANGE[1]):
        problems.append("dispositivos de otra region")
    method = fields["method"]
    if method.endswith("_coin") or method.endswith("_v3"):
        if abs(float(config.get("*.terminal[*].dlk.mac.mlBoxMLPKD.threshold", "nan")) - fields["threshold"]) > 1e-9:
            problems.append("threshold de la config != nombre")
        if abs(float(config.get("*.terminal[*].dlk.mac.stopTarget", "nan")) - 0.01) > 1e-9:
            problems.append("stopTarget != 0.01")
        if not config.get("*.terminal[*].dlk.mac.mlBoxMLPKD.modelPath", "").endswith(MODEL_FILE[fields["arm"]]):
            problems.append("modelo de la config != brazo del nombre")
    if method.endswith("_coin"):
        if config.get("*.terminal[*].dlk.mac.useMLCoin") != "true":
            problems.append("useMLCoin != true")
        if abs(float(config.get("*.terminal[*].dlk.mac.mlCoinProb", "nan")) - fields["ml_coin_prob"]) > 1e-9:
            problems.append("mlCoinProb de la config != nombre")
        if config.get("num-rngs") != "2":
            problems.append("num-rngs != 2")
    return problems


def one_run(item):
    path, fields = item
    values, config, has_coin_scalar = read_sca(path, fields["N"])
    slot, delay = read_vec(path[:-4] + ".vec")

    problems = check_config(fields, config)
    if os.path.getmtime(path) < NEW_RUNS_AFTER:
        problems.append("corrida anterior al modelo sin raan")
    if values["duplicates"] != 0:
        problems.append("duplicates != 0")
    if values["per_node_sum"] != values["unique"]:
        problems.append("suma por nodo != unique")
    if len(slot) != values["unique"] or len(delay) != values["unique"]:
        problems.append("largo de vectores != unique")
    if values["der"] > 0:
        not_started = values["tx"] - values["recv"] / values["der"]
        if not (-1e-6 <= not_started <= 1 + 1e-6):
            problems.append("tx - iniciadas fuera de [0, 1]")
    if fields["method"].endswith("_coin") and (not has_coin_scalar or values["skipped_coin"] <= 0):
        problems.append("corrida con moneda sin numSkippedByMLCoin > 0")

    row = dict(scenario=fields["scenario"], N=fields["N"], method=fields["method"], threshold=fields["threshold"],
               ml_coin_prob=fields["ml_coin_prob"], raan=fields["raan"], run=fields["run"])
    for col in COLUMNS:
        if col in values:
            row[col] = values[col]
    if len(slot) > 0:
        row["slot1_frac"] = float((slot == 1).mean())
        row["slot_mean"] = float(slot.mean())
        row["delay_mean"] = float(delay.mean())
    else:
        row["slot1_frac"] = row["slot_mean"] = row["delay_mean"] = float("nan")
    return row, problems, os.path.relpath(path, ROOT)


def main():
    todo = list_runs()
    print("corridas:", len(todo))
    with Pool(16) as pool:
        result = pool.map(one_run, todo, chunksize=8)
    rows = [r for r, _, _ in result]
    bad = [(f, p) for _, p, f in result if p]

    # Trafico identico: msgs de cada corrida ML / FV / RV == msgs de REP_5 con el mismo (N, raan, run)
    rep5 = {(r["scenario"], r["N"], r["raan"], r["run"]): r["msgs"] for r in rows if r["method"] == "REP_5"}
    n_paired = 0
    for r, _, f in result:
        if r["method"].startswith(("KD_", "NoKD_", "FV_", "RV_")):
            n_paired += 1
            if r["msgs"] != rep5[(r["scenario"], r["N"], r["raan"], r["run"])]:
                bad.append((f, ["msgs != REP_5 (otro trafico)"]))
    print("corridas ML / FV / RV comparadas con el trafico de REP_5:", n_paired)

    per_config = Counter((r["scenario"], r["N"], r["method"], r["threshold"], r["ml_coin_prob"]) for r in rows)
    wrong_size = {k: v for k, v in per_config.items() if v != RUNS_PER_CONFIG}
    print("configuraciones:", len(per_config), " con corridas != 50:", len(wrong_size), list(wrong_size.items())[:5])
    print("corridas por escenario y N:", dict(Counter((r["scenario"], r["N"]) for r in rows)))
    print("corridas con un chequeo fallido:", len(bad), bad[:5])
    if bad or wrong_size:
        raise SystemExit("chequeos fallidos: no se escribe nada")

    with open(OUT_CSV, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=COLUMNS)
        w.writeheader()
        w.writerows(rows)
    print("escrito", OUT_CSV)


if __name__ == "__main__":
    main()

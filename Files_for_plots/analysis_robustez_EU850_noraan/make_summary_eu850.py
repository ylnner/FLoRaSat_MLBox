"""
Tablas de EU_850 (modelo sin raan, 10 umbrales, con y sin moneda). Lee solo data/per_run_eu850.csv (extract_eu850.py).

Pareo: mismas (raan, run) -> mismo trafico (comprobado contra REP_5 en la extraccion). d_i = a_i - b_i, SE = s_d / sqrt(n).
Los % son respecto de la media de la referencia b.

Escribe en data/:
    summary_configs.csv   media y SE de tx / unique por configuracion, eficiencia (unique / tx) y % de tx bajo sensibilidad
    vs_baselines.csv      cada configuracion del MLP menos FV_1, FV_2, RV_1, RV_2, REP_1 y el mejor baseline del N
    kd_vs_nokd.csv        KD - NoKD con el mismo umbral y la misma moneda
    same_tx.csv           MLP sin moneda a igual tx que REP_1 / RV_1: por corrida se interpola la curva de umbrales
                          (unique frente a tx) en la tx de la corrida del baseline, y se compara pareado.
                          comparable = False si en mas de MAX_OUTSIDE corridas la tx del baseline cae fuera del rango de
                          la curva: ahi np.interp devuelve el extremo de la curva y ya no es una comparacion a igual tx
    new_vs_old.csv        modelo sin raan menos modelo con raan (OLD_RUNS_CSV, umbrales 0.1 / 0.2, sin moneda),
                          mismo brazo, umbral y N. Valido porque los baselines de las dos tandas son
                          identicos corrida a corrida (se comprueba aqui con FV_1, RV_1 y REP_1)
Uso:  python3 make_summary_eu850.py
"""
import os

import numpy as np
import pandas as pd

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "data")
RUNS_CSV = os.path.join(DATA, "per_run_eu850.csv")
# Corridas del modelo CON raan en este escenario (None si no existen). Umbrales 0.1 / 0.2, stopTarget 0.01.
OLD_RUNS_CSV = '/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/Files_for_plots/analysis_robustez_EU850/data/per_run.csv'

OLD_DENSITIES = [60, 100, 300, 500, 1000]
DENSITIES = [60, 100, 300, 500, 1000]
ARMS = ["KD", "NoKD"]
BASELINES = ["FV_1", "FV_2", "RV_1", "RV_2", "REP_1", "REP_2", "REP_3", "REP_4", "REP_5"]
COMPARED = ["FV_1", "FV_2", "RV_1", "RV_2", "REP_1"]
RUN_KEY = ["raan", "run"]
MAX_OUTSIDE = 5            # de 50 corridas (10 %)


def ml_method(arm, coin):
    if coin == 1.0:
        return f"{arm}_v3"
    return f"{arm}_coin"


def select(runs, n, method, threshold=None, coin=None):
    sel = (runs.N == n) & (runs.method == method)
    if threshold is not None:
        sel &= np.isclose(runs.threshold, threshold)
    if coin is not None:
        sel &= np.isclose(runs.ml_coin_prob, coin)
    sub = runs[sel].set_index(RUN_KEY).sort_index()
    assert len(sub) == 50, (n, method, threshold, coin, len(sub))
    return sub


def paired(a, b):
    """Diferencia pareada a - b en unique y tx, en % de la media de b."""
    assert (a.index == b.index).all()
    d_unique = a["unique"] - b["unique"]
    d_tx = a["tx"] - b["tx"]
    base_unique = b["unique"].mean()
    base_tx = b["tx"].mean()
    return dict(d_unique=d_unique.mean(), se_unique=d_unique.std(ddof=1) / np.sqrt(len(d_unique)),
                d_unique_pct=100 * d_unique.mean() / base_unique,
                se_unique_pct=100 * d_unique.std(ddof=1) / np.sqrt(len(d_unique)) / base_unique,
                d_tx_pct=100 * d_tx.mean() / base_tx)


def ml_configs(runs):
    """(threshold, coin) simulados del MLP."""
    sub = runs[runs.method.isin(["KD_v3", "KD_coin"])]
    return sorted(set(zip(sub.threshold.round(2), sub.ml_coin_prob.round(2))))


def best_baseline(runs, n):
    """Baseline con mas unicos medios en este N."""
    means = {b: select(runs, n, b)["unique"].mean() for b in BASELINES}
    return max(means, key=means.get)


def summary_configs(runs):
    rows = []
    for (n, method, threshold, coin), g in runs.groupby(["N", "method", "threshold", "ml_coin_prob"]):
        rows.append(dict(N=n, method=method, threshold=threshold, coin_prob=coin, n_runs=len(g),
                         tx=g.tx.mean(), tx_se=g.tx.std(ddof=1) / np.sqrt(len(g)),
                         unique=g.unique.mean(), unique_se=g.unique.std(ddof=1) / np.sqrt(len(g)),
                         recv=g.recv.mean(), below_sensitivity=g.sat_below_sensitivity.mean(),
                         efficiency=g.unique.mean() / g.tx.mean(),
                         below_sensitivity_pct_tx=100 * g.sat_below_sensitivity.mean() / g.tx.mean(),
                         delay_mean=g.delay_mean.mean(), jain=g.jain.mean()))
    return pd.DataFrame(rows)


def vs_baselines(runs):
    rows = []
    for n in DENSITIES:
        best = best_baseline(runs, n)
        for b in COMPARED + ["best:" + best]:
            b_name = b.replace("best:", "")
            base = select(runs, n, b_name)
            for arm in ARMS:
                for threshold, coin in ml_configs(runs):
                    a = select(runs, n, ml_method(arm, coin), threshold, coin)
                    rows.append(dict(N=n, arm=arm, threshold=threshold, coin_prob=coin, b=b, **paired(a, base)))
    return pd.DataFrame(rows)


def kd_vs_nokd(runs):
    rows = []
    for n in DENSITIES:
        for threshold, coin in ml_configs(runs):
            a = select(runs, n, ml_method("KD", coin), threshold, coin)
            b = select(runs, n, ml_method("NoKD", coin), threshold, coin)
            rows.append(dict(N=n, threshold=threshold, coin_prob=coin, **paired(a, b)))
    return pd.DataFrame(rows)


def same_tx(runs):
    """Por corrida: curva (tx, unique) de los 10 umbrales sin moneda, interpolada en la tx del baseline."""
    rows = []
    thresholds = sorted(runs.loc[runs.method == "KD_v3", "threshold"].unique())
    for n in DENSITIES:
        for b in ["REP_1", "RV_1"]:
            base = select(runs, n, b)
            for arm in ARMS:
                curves = [select(runs, n, f"{arm}_v3", th, 1.0) for th in thresholds]
                tx = np.column_stack([c["tx"].to_numpy() for c in curves])
                unique = np.column_stack([c["unique"].to_numpy() for c in curves])
                interp = []
                outside = 0
                for i in range(len(base)):
                    order = np.argsort(tx[i])
                    target = base["tx"].iloc[i]
                    if target < tx[i][order][0] or target > tx[i][order][-1]:
                        outside += 1
                    interp.append(np.interp(target, tx[i][order], unique[i][order]))
                d = np.array(interp) - base["unique"].to_numpy()
                rows.append(dict(N=n, arm=arm, b=b, d_unique_pct=100 * d.mean() / base["unique"].mean(),
                                 se_unique_pct=100 * d.std(ddof=1) / np.sqrt(len(d)) / base["unique"].mean(),
                                 runs_outside_curve=outside, comparable=outside <= MAX_OUTSIDE))
    return pd.DataFrame(rows)


def load_old():
    """Corridas del modelo con raan en este escenario, con los mismos nombres de metodo que aqui."""
    old = pd.read_csv(OLD_RUNS_CSV)
    old = old[old.N.isin(OLD_DENSITIES)].copy()
    old["threshold"] = old["threshold"].round(2)
    return old


def new_vs_old(runs, old):
    # los baselines de las dos tandas deben ser identicos corrida a corrida
    for n in OLD_DENSITIES:
        for b in ["FV_1", "RV_1", "REP_1"]:
            a = select(runs, n, b)
            c = select(old, n, b)
            assert (a["unique"] == c["unique"]).all() and (a["tx"] == c["tx"]).all(), (n, b)
    rows = []
    for n in OLD_DENSITIES:
        for arm in ARMS:
            for th in [0.1, 0.2]:
                a = select(runs, n, f"{arm}_v3", th, 1.0)
                c = select(old, n, f"{arm}_v3", th, 1.0)
                rows.append(dict(N=n, arm=arm, threshold=th, **paired(a, c)))
    return pd.DataFrame(rows)


def main():
    runs = pd.read_csv(RUNS_CSV)
    runs["threshold"] = runs["threshold"].round(2)
    outputs = {"summary_configs.csv": summary_configs(runs), "vs_baselines.csv": vs_baselines(runs),
               "kd_vs_nokd.csv": kd_vs_nokd(runs), "same_tx.csv": same_tx(runs)}
    if OLD_RUNS_CSV is not None:
        outputs["new_vs_old.csv"] = new_vs_old(runs, load_old())
    for name, table in outputs.items():
        table.to_csv(os.path.join(DATA, name), index=False)
        print(f"{name}: {len(table)} filas")
    for n in DENSITIES:
        print(f"N = {n}: mejor baseline {best_baseline(runs, n)}")


if __name__ == "__main__":
    main()

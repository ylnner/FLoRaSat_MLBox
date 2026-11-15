"""

Se ejecuta despues de entrenar los estudiantes y antes de select_best_models.py.

Para cada corrida del estudiante (KD y noKD) reproduce offline el MAC de FLoRaSat sobre la validacion de su
seed y calcula 

- G_off = mensajes unicos por encima de la mejor referencia offline (REP_k, Coin) con las mismas tx.

Mismo orden que Ter_Dlk_KiWan.cc: p >= umbral -> regla de parada -> mascara de 10 grados -> transmitir.

python select_by_unique.py 60 100 200 300      Escribe results*/N_*/student/runs_unique.csv

"""
import os, sys, json, glob
import numpy as np, pandas as pd, torch

from common import results_dir, cache_path_with_N, message_ids
from funcs_create_dataset import load_dataset
from StudentModel import StudentModel

THRESHOLDS  = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95]     # los de los .ini
STOP_TARGET = 0.8
STOP_GAMMA  = 1.0
NS = [f"N_{n}" for n in (sys.argv[1:] or (60, 100, 200, 300))]


# Agrupa las filas de validacion por mensaje (message_ids) y devuelve como colocarlas en una matriz
# mensajes x 5 oportunidades, en orden de tx_start.
def message_layout(data):
    m = message_ids(data)
    order = np.lexsort((data["tx_start"], m))
    m_sorted = m[order]
    first = np.flatnonzero(np.r_[True, m_sorted[1:] != m_sorted[:-1]])
    sizes = np.diff(np.r_[first, len(m_sorted)])
    row = np.repeat(np.arange(len(first)), sizes)
    col = np.arange(len(m_sorted)) - np.repeat(first, sizes)
    return order, row, col, len(first)


def to_matrix(layout, values):
    order, row, col, n = layout
    out = np.full((n, 5), np.nan)
    out[row, col] = values[order]
    return out


# Reproduce el MAC de Ter_Dlk_KiWan.cc para un umbral: en cada oportunidad se transmite si p >= umbral, la regla de
# parada aun no llego a 0,8 (1 - prod(1 - gamma*p) de las tx ya hechas) y la oportunidad es visible (mascara 10).
# Devuelve mensajes unicos entregados y numero de transmisiones.
def replay(P, Y, V, thr):
    cum_miss = np.ones(len(P))
    delivered = np.zeros(len(P), dtype=bool)
    n_tx = 0
    for j in range(5):
        p = np.nan_to_num(P[:, j])
        tx = (p >= thr) & ~np.isnan(P[:, j])
        tx &= (1 - cum_miss) < STOP_TARGET
        tx &= V[:, j] == 1
        n_tx += tx.sum()
        cum_miss = np.where(tx, cum_miss * (1 - STOP_GAMMA * p), cum_miss)
        delivered |= tx & (Y[:, j] == 1)
    return delivered.sum(), n_tx



# Referencias sin ML con la mascara: REP_k (transmitir en las k primeras oportunidades visibles) y Coin (transmitir
# cada oportunidad visible con probabilidad q, valor esperado). 
# B(tx) = mejor numero de unicos que alcanza alguna
# referencia con tx transmisiones. Fuera del rango de las referencias devuelve NaN.
def reference_envelope(Y, V):
    visible = V == 1
    received = Y == 1
    rep = pd.DataFrame([(visible[:, :k].sum(), received[:, :k].any(1).sum()) for k in range(1, 6)], columns=["tx", "u"])
    coin = pd.DataFrame([(q * visible.sum(), (1 - (1 - q) ** received.sum(1)).sum()) for q in np.arange(0.1, 0.95, 0.1)],
                        columns=["tx", "u"])
    lo, hi = min(rep.tx.min(), coin.tx.min()), max(rep.tx.max(), coin.tx.max())
    xs = np.linspace(lo, hi, 4001)
    ys = np.full_like(xs, -np.inf)
    for f in (rep.sort_values("tx"), coin.sort_values("tx")):
        inside = (xs >= f.tx.min()) & (xs <= f.tx.max())
        ys[inside] = np.maximum(ys[inside], np.interp(xs[inside], f.tx, f.u))
    ys = np.maximum.accumulate(ys)

    def B(tx):
        if lo <= tx <= hi:
            return np.interp(tx, xs, ys)
        return np.nan
    return B


for str_N in NS:
    stud = results_dir(str_N, "student")
    # Lee lasruns
    runs = [dict(json.load(open(f)), name=os.path.basename(f)[:-5]) for f in glob.glob(os.path.join(stud, "runs", "*.json"))]
    val_by_seed = {}
    rows = []
    for r in runs:
        # obtiene el seed
        seed = int(r["seed"])
        if seed not in val_by_seed:
            # Cargo dataset de val
            data = load_dataset(cache_path_with_N(seed, "val", str_N))
            layout = message_layout(data)
            Y = to_matrix(layout, data["y_true"].astype(float))
            V = to_matrix(layout, (data["below_mask"] == 0).astype(float))
            val_by_seed[seed] = (data, layout, Y, V, reference_envelope(Y, V))
        data, layout, Y, V, B = val_by_seed[seed]

        ## Scala data
        sc = json.load(open(os.path.join(stud, "scalers", r["name"] + ".json")))
        x = (data["X_student"] - np.array(sc["mean"])) / np.array(sc["scale"])

        # Carga el modelo y ejecuta para obtener el mejor seed
        model = StudentModel()
        model.load_state_dict(torch.load(os.path.join(stud, "models", r["name"] + ".pth"), map_location="cpu"))
        model.eval()
        with torch.no_grad():
            p = torch.sigmoid(model(torch.as_tensor(x, dtype=torch.float32))).reshape(-1).numpy()
        P = to_matrix(layout, p)

        # Calcula para cada modelo
        # G_off = unicos del modelo - B(tx del modelo); se queda el mejor umbral de la rejilla de los .ini.
        best_g, best_thr = np.nan, np.nan
        for thr in THRESHOLDS:
            u, t = replay(P, Y, V, thr)
            g = u - B(t)
            if not np.isnan(g) and (np.isnan(best_g) or g > best_g):
                best_g, best_thr = g, thr

        rows.append(dict(name=r["name"], frac=r["frac"], teacher=r["teacher"], alpha=r["alpha"], temp=r["temp"],
                         seed=seed, msgs=len(Y), G_off=best_g, G_off_pct=100 * best_g / len(Y), thr_G_off=best_thr))

    pd.DataFrame(rows).to_csv(os.path.join(stud, "runs_unique.csv"), index=False)
    print(f"{str_N}: {len(rows)} corridas -> {os.path.join(stud, 'runs_unique.csv')}")

"""
Validacion independiente de la cache .npz contra los CSV (no reutiliza funcs_create_dataset).
Uso (desde my_final_version/, despues de prepare_data.py):
    python validate_cache.py            # 10 seeds
    python validate_cache.py 0 3        # solo las seeds 0 y 3
Debe terminar con "FALLOS: 0". Escribe validate_cache_stats.csv con estadisticas por seed/split.
Supone CAUSAL = False (cuenta todos los solapados con elev >= 0) y N_MAX = 15.
"""
import os, sys, numpy as np, pandas as pd
from config import DATA_DIR, CACHE_DIR, N_MAX, CAUSAL
assert CAUSAL is False, "este validador asume CAUSAL = False en config.py"
CSV  = os.path.join(DATA_DIR, "seed_{s}_final_{sp}_data_transmission.csv")
NPZ  = os.path.join(CACHE_DIR, "seed_{s}_{sp}.npz")
SENS = {7: -123.0, 8: -126.0, 9: -129.0, 10: -133.0, 11: -135.0, 12: -137.0}
NMAX, SEQ = N_MAX, 8
seeds = range(10) if len(sys.argv) < 2 else [int(a) for a in sys.argv[1:]]

def reorder(df):
    parts = []
    for sim in sorted(df.id_simulation.unique()):
        d = df[df.id_simulation == sim].sort_values("tx_start").reset_index(drop=True)
        d["_gidx"] = np.arange(len(d))
        s = d.tx_start.values; e = s + d.tx_duration.values; ids = d.srcId.values; el = d.elevSat.values
        sf = d.loraSF.values; prx = d.rx_power_dbm_at_dev.values; bm = d.belowMask.values
        sens_sf = SENS_F(sf)
        n_all = np.zeros(len(d), int); n_sf = np.zeros(len(d), int); n_bm = np.zeros(len(d), int)
        n_vis_sf = np.zeros(len(d), int); n_caus = np.zeros(len(d), int); n_neg = np.zeros(len(d), int)
        for i in range(len(d)):
            m = (s < e[i]) & (s[i] < e) & (ids != ids[i])          # solapados, otro dispositivo
            n_neg[i] = (m & (el < 0)).sum()
            m &= el >= 0
            n_all[i] = m.sum(); n_sf[i] = (m & (sf == sf[i])).sum(); n_bm[i] = (m & (bm == 1)).sum()
            vis = m & (bm == 0) & (prx >= sens_sf)
            n_vis_sf[i] = (vis & (sf == sf[i])).sum(); n_caus[i] = (m & (s < s[i])).sum()
        d["n_all"], d["n_sf"], d["n_bm"], d["n_vis_sf"], d["n_caus"], d["n_neg"] = n_all, n_sf, n_bm, n_vis_sf, n_caus, n_neg
        # historial: elevSat de la transmision anterior del mismo dispositivo
        d["prev_elev"] = d.groupby("srcId").elevSat.shift(1)
        d["k_in_dev"] = d.groupby("srcId").cumcount()
        for dev in d.srcId.unique():
            parts.append(d[d.srcId == dev])
    return pd.concat(parts, ignore_index=True)

rows = []; fails = []
SENS_F = np.vectorize(SENS.get)
def chk(name, ok, seed, sp):
    if not ok: fails.append(f"seed {seed} {sp}: {name}")
for seed in seeds:
    combos = {}
    for sp in ("train", "val", "test"):
        df = pd.read_csv(CSV.format(s=seed, sp=sp)); r = reorder(df)
        z = dict(np.load(NPZ.format(s=seed, sp=sp)))
        N = len(r); c = lambda n, ok: chk(n, ok, seed, sp)
        c("N muestras == filas CSV", len(z["y_true"]) == N == len(df))
        c("claves", set(z) >= {"seq_feats","seq_mask","static_feats","own_tx","interf_set","interf_mask","context_feats","y_true","link_ok","meta_belowMask","meta_elevSat"})
        c("shapes", z["seq_feats"].shape == (N, SEQ, 4) and z["interf_set"].shape[:2] == (N, NMAX) and z["interf_set"].shape[2] in (9, 12) and z["context_feats"].shape == (N, 7) and z["static_feats"].shape == (N, 6))
        c("sin NaN/inf", all(np.isfinite(z[k]).all() for k in ("seq_feats","static_feats","own_tx","interf_set","context_feats")))
        # alineacion exacta con el CSV reordenado
        c("y_true == rcvOk", (z["y_true"] == r.rcvOk.values).all())
        c("meta_elevSat == elevSat", np.allclose(z["meta_elevSat"], r.elevSat.values, atol=1e-4))
        c("meta_belowMask == belowMask", (z["meta_belowMask"] == r.belowMask.values).all())
        c("belowMask <=> elev<10", (z["meta_belowMask"] == (z["meta_elevSat"] < 10)).all())
        c("belowMask => y=0", (z["y_true"][z["meta_belowMask"] == 1] == 0).all())
        c("seq ultima fila = target (doppler, elev, trel=0)", np.allclose(z["seq_feats"][:, -1, 1], r.elevSat.values, atol=1e-3)
          and np.allclose(z["seq_feats"][:, -1, 0], r.dopplerShift.values, rtol=1e-5, atol=1e-2) and (z["seq_feats"][:, -1, 3] == 0).all())
        hp = r.prev_elev.notna().values
        c("seq penultima fila = tx anterior del dispositivo", np.allclose(z["seq_feats"][hp, -2, 1], r.prev_elev.values[hp], atol=1e-3))
        c("seq_mask = min(k+1, 8)", (z["seq_mask"].sum(1) == np.minimum(r.k_in_dev.values + 1, SEQ)).all())
        c("trel <= 0 y creciente", (z["seq_feats"][..., 3] <= 1e-6).all())
        c("static = lat,lon,TP,SF,alt,raan", np.allclose(z["static_feats"], r[["latDev","longDev","loraTP","loraSF","alt","raan"]].values, rtol=1e-5, atol=1e-4))
        c("own_tx = SF, Prx_at_dev", np.allclose(z["own_tx"], r[["loraSF","rx_power_dbm_at_dev"]].values, atol=1e-3))
        # interferentes (no causal, elev>=0)
        im = z["interf_mask"]; I = z["interf_set"]; ctx = z["context_feats"]
        c("n_active == solapados elev>=0 (no causal)", (ctx[:, 0] == r.n_all.values).all())
        c("interf_mask.sum == min(n, 15)", (im.sum(1) == np.minimum(r.n_all.values, NMAX)).all())
        c("mask contiguo (slots validos primero)", (np.diff(im, axis=1) <= 0).all())
        c("same_sf_concurrent", (ctx[:, 1] == r.n_sf.values).all())
        c("n_same_sf_visible (excluye belowMask)", (ctx[:, 5] == r.n_vis_sf.values).all())
        c("has_visible coherente", ((ctx[:, 6] == 1) >= (ctx[:, 5] > 0)).all())
        v = im == 1
        c("ningun interferente con elev<0", (I[..., 6][v] >= 0).all())
        c("flag col8 == (elev_j<10)", (I[..., 8][v] == (I[..., 6][v] < 10)).all())
        nt = r.n_all.values <= NMAX
        c("sum flag == n interferentes belowMask (sin truncar)", (I[nt][..., 8].sum(1) == r.n_bm.values[nt]).all())
        c("same_sf col1 en {0,1}, coherente con delta_sf", ((I[..., 1][v] == 1) == (I[..., 2][v] == 0)).all())
        c("delta_t_overlap en (0,1]", ((I[..., 0][v] > 0) & (I[..., 0][v] <= 1 + 1e-5)).all())
        c("padding a cero", (I[~v] == 0).all())
        c("delta_p = own - p_j", np.allclose(I[..., 4][v], (z["own_tx"][:, 1:2] - I[..., 3])[v], atol=1e-3))
        c("link_ok todo True", z["link_ok"].all())
        if I.shape[2] == 12:   # cambio H: features de pares
            dd = np.abs(z["seq_feats"][:, -1:, 0] - I[..., 7]) / 1200.0
            c("H: col9 == |doppler_x - doppler_j| / 1200", np.allclose(I[..., 9][v], dd[v], atol=1e-3))
            c("H: margen de captura y temporal finitos", np.isfinite(I[..., 10][v]).all() and np.isfinite(I[..., 11][v]).all())
        tr = (r.n_all.values > NMAX)
        rows.append(dict(seed=seed, split=sp, N=N, pos=z["y_true"].mean(), belowMask=z["meta_belowMask"].mean(),
                         pos_visibles=z["y_true"][z["meta_belowMask"] == 0].mean(),
                         interf_media=r.n_all.mean(), interf_causal_media=r.n_caus.mean(),
                         frac_start_offset_ge0=(I[..., 5][v] >= 0).mean(), truncados=tr.mean(), max_interf=r.n_all.max(),
                         elevneg_excluidos=r.n_neg.mean(), frac_interf_bm=I[..., 8][v].mean()))
        combos[sp] = set(map(tuple, np.unique(z["static_feats"][:, 4:6], axis=0)))
    chk("(alt,raan) disjuntos entre splits", not (combos["train"] & combos["test"]) and not (combos["train"] & combos["val"]) and not (combos["val"] & combos["test"]), seed, "-")
    print(f"seed {seed} ok", flush=True)

R = pd.DataFrame(rows)
pd.set_option("display.width", 250); pd.set_option("display.max_columns", 30)
print(R.round(4).to_string(index=False))
print("\nTotales por split:"); print(R.groupby("split").N.sum())
R.to_csv("validate_cache_stats.csv", index=False)
print("\nFALLOS:", len(fails)); print("\n".join(fails[:60]))

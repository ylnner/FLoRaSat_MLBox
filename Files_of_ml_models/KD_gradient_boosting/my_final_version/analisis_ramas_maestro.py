"""
Qué aporta cada rama del maestro T1 causal (TeacherCollisionPredictor). Solo inferencia, sin entrenar.

Modelos: results/best_models_v1/models/transformer_best_seed{0..9}.pth (los genera run_custom_config.py).
Datos:   test de cada seed (load_seed + scale_splits, igual que en el entrenamiento).

Dos pruebas, ambas sobre el test y comparadas con el AUC-PR del maestro completo:

  A. Ablación por rama: la salida de una rama se sustituye por su media sobre el test (forward hook).
     La rama deja de distinguir unas filas de otras, pero el clasificador final sigue recibiendo un vector
     de su escala habitual. Caída de AUC-PR = lo que solo esa rama sabe (lo que no compensan las otras).
     También "solo esta rama": todas las demás a su media.
  B. Permutación por variable: una variable de entrada se baraja entre filas (en interf_set, entre los
     interferentes reales; en seq_feats, entre las posiciones reales). Caída de AUC-PR = importancia.

Ramas (salida que entra al final_classifier):
  seq      TransformerEncoderSequential   seq_feats [8, 4]          -> 32
  static   MLPStatic                      static_feats [6]          -> 32
  set      SetTransformerCollisionPred.   interf_set [15, 8] + own_tx [2] -> 32
  context  context_net (Linear + ReLU)    context_feats [7]         -> 16

Uso:  python analisis_ramas_maestro.py      (~1-2 min; escribe analisis_ramas_maestro_*.csv)
"""
import os
import itertools
import numpy as np
import pandas as pd
import torch

import common                                   # agrega scripts/ al sys.path
from common import load_seed, auc_pr
from global_funcs import scale_splits, create_sub_configs_dicts
from TeacherCollisionPredictor import TeacherCollisionPredictor

torch.set_num_threads(4)
HERE       = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(HERE, "results", "best_models_v1", "models")
SEEDS      = range(10)
N_PERM     = 3
CONFIG = {"d_model": 32, "dropout": 0.3, "lr": 0.0001, "batch_size": 32, "out_dim": 64,
          "static_hidden_dims": (64, 32), "n_heads": 4, "n_layers": 1, "inter_in_dim": 8,
          "num_sab_layers": 1, "sab_nheads": 4, "sab_ln": True, "own_in_dim": 2, "d_own": 8,
          "cross_attn_nheads": 4, "cross_attn_ln": True, "context_in_dim": 7, "context_out_dim": 16}
BRANCHES = {"seq": "seq_pred", "static": "static_pred", "set": "set_transformer_col_pred", "context": "context_net"}
KEYS     = ["seq_feats", "seq_mask", "static_feats", "interf_set", "interf_mask", "own_tx", "link_ok", "context_feats"]

FEATURES = (
    [("seq_feats", i, n) for i, n in enumerate(["doppler (hist.)", "elevSat (hist.)", "delta_t (hist.)", "t_rel (hist.)"])]
    + [("static_feats", i, n) for i, n in enumerate(["latDev", "longDev", "loraTP", "loraSF", "alt", "raan"])]
    + [("own_tx", i, n) for i, n in enumerate(["own SF", "own P_rx"])]
    + [("interf_set", i, n) for i, n in enumerate(["i: delta_t_overlap", "i: same_sf", "i: delta_sf", "i: p_rx_j",
                                                    "i: delta_p", "i: start_offset", "i: elevSat_j", "i: doppler_j"])]
    + [("context_feats", i, n) for i, n in enumerate(["c: n_interf", "c: n_same_sf", "c: strongest",
                                                       "c: delta_p_strongest", "c: own_margin",
                                                       "c: n_same_sf_visible", "c: has_visible_interf"])]
)
BRANCH_OF = {"seq_feats": "seq", "static_feats": "static", "own_tx": "set", "interf_set": "set", "context_feats": "context"}


def load_model(seed):
    m = TeacherCollisionPredictor(*create_sub_configs_dicts(CONFIG))
    m.load_state_dict(torch.load(os.path.join(MODELS_DIR, f"transformer_best_seed{seed}.pth"), map_location="cpu"))
    return m.eval()


@torch.no_grad()
def logits(model, data, replace=None):
    """replace = {rama: vector medio}: la salida de esa rama se sustituye por el vector (forward hook)."""
    hooks = [getattr(model, BRANCHES[b]).register_forward_hook(lambda mod, inp, out, v=v: v.expand_as(out))
             for b, v in (replace or {}).items()]
    out = torch.cat([model({k: v[i:i + 4096] for k, v in data.items()}) for i in range(0, len(data["y"]), 4096)])
    for h in hooks:
        h.remove()
    return out.numpy()


@torch.no_grad()
def branch_means(model, data):
    outs = {}
    hooks = [getattr(model, a).register_forward_hook(lambda mod, inp, out, b=b: outs.setdefault(b, []).append(out))
             for b, a in BRANCHES.items()]
    logits(model, data)
    for h in hooks:
        h.remove()
    return {b: torch.cat(v).mean(0, keepdim=True) for b, v in outs.items()}


def permute(data, key, col, rng):
    d = dict(data)
    x = d[key].clone()
    if key in ("interf_set", "seq_feats"):
        valid = (d["interf_mask"] if key == "interf_set" else d["seq_mask"]) == 1
        vals = x[..., col][valid]
        x[..., col][valid] = vals[torch.from_numpy(rng.permutation(len(vals)))]
    else:
        x[:, col] = x[torch.from_numpy(rng.permutation(len(x))), col]
    d[key] = x
    return d


def main():
    rows_b, rows_f, rows_g = [], [], []
    for seed in SEEDS:
        train, val, test = load_seed(seed)
        _, _, _, test_s = scale_splits(train, val, test)
        data = {k: torch.as_tensor(test_s[k]) for k in KEYS}
        data["y"] = torch.as_tensor(test_s["y_true"])
        y = test_s["y_true"]
        model = load_model(seed)
        base = auc_pr(y, logits(model, data))
        means = branch_means(model, data)
        rows_b.append({"seed": seed, "ablacion": "ninguna (maestro completo)", "auc_pr": base})

        # A. por rama
        for b in BRANCHES:
            rows_b.append({"seed": seed, "ablacion": f"sin {b}", "auc_pr": auc_pr(y, logits(model, data, {b: means[b]}))})
            rows_b.append({"seed": seed, "ablacion": f"solo {b}",
                           "auc_pr": auc_pr(y, logits(model, data, {o: means[o] for o in BRANCHES if o != b}))})
        for b1, b2 in itertools.combinations(BRANCHES, 2):
            rows_b.append({"seed": seed, "ablacion": f"sin {b1}+{b2}",
                           "auc_pr": auc_pr(y, logits(model, data, {b1: means[b1], b2: means[b2]}))})

        # A2. ¿dónde actúa la rama de interferentes? caída por subgrupo de filas
        n_int = test["interf_mask"].sum(1)
        groups = {"sin interferentes": n_int == 0, "con interferentes": n_int > 0}
        full = logits(model, data)
        for b in BRANCHES:
            abl = logits(model, data, {b: means[b]})
            for g, m in groups.items():
                rows_g.append({"seed": seed, "rama": b, "grupo": g, "n": int(m.sum()), "prev_y1": float(y[m].mean()),
                               "auc_pr_completo": auc_pr(y[m], full[m]), "auc_pr_sin_rama": auc_pr(y[m], abl[m])})

        # B. por variable
        rng = np.random.default_rng(seed)
        for key, col, name in FEATURES:
            v = np.mean([auc_pr(y, logits(model, permute(data, key, col, rng))) for _ in range(N_PERM)])
            rows_f.append({"seed": seed, "rama": BRANCH_OF[key], "variable": name, "caida_auc_pr": base - v})
        print(f"seed {seed}: AUC-PR maestro {base:.4f}", flush=True)

    b, f, g = pd.DataFrame(rows_b), pd.DataFrame(rows_f), pd.DataFrame(rows_g)
    b.to_csv(os.path.join(HERE, "analisis_ramas_maestro_por_seed.csv"), index=False)
    f.to_csv(os.path.join(HERE, "analisis_ramas_maestro_variables_por_seed.csv"), index=False)
    g.to_csv(os.path.join(HERE, "analisis_ramas_maestro_grupos_por_seed.csv"), index=False)

    pd.set_option("display.width", 200)
    base = b[b.ablacion.str.startswith("ninguna")].set_index("seed").auc_pr
    b["caida"] = base.loc[b.seed].values - b.auc_pr
    print("\nA. Ablación por rama (media de 10 seeds)")
    print(b.groupby("ablacion", sort=False).agg(auc_pr=("auc_pr", "mean"), caida=("caida", "mean"),
                                                caida_std=("caida", "std")).round(4).to_string())
    g["caida"] = g.auc_pr_completo - g.auc_pr_sin_rama
    print("\nA2. Caída por subgrupo")
    print(g.groupby(["rama", "grupo"]).agg(n=("n", "mean"), prev=("prev_y1", "mean"), completo=("auc_pr_completo", "mean"),
                                           caida=("caida", "mean")).round(4).to_string())
    print("\nB. Permutación por variable (caída de AUC-PR, media de 10 seeds)")
    print(f.groupby(["rama", "variable"], sort=False).caida_auc_pr.agg(["mean", "std"]).round(4)
          .sort_values("mean", ascending=False).to_string())


if __name__ == "__main__":
    main()

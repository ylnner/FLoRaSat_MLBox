"""
Genera mixes.csv: 
    una mezcla de probabilidades de SF/TP por corrida (split, alt, raan, seed). Misma tabla para N_60, N_100 y N_200.
"""
import csv
import os

import numpy as np

SEEDS    = range(10)
FLOOR    = 0.05                    # probabilidad minima de cada SF y de cada TP: mantiene todas las combinaciones observables
RNG_SEED = 20261105

# Distribucion actual de de simulaciones
# Formato: SIMS[split][altitud] = [RAANs]
SIMS = {
    "train": {600: [85, 115, 125, 135], 700: [85, 95, 105, 125], 740: [85, 95, 115, 125], 850: [277, 287, 317, 327]},
    "val":   {600: [105], 700: [75], 740: [75], 850: [297]},
    "test":  {600: [95], 700: [115], 740: [105], 850: [307]},
}

# Mezclas fijas de test: (p_sf10, p_sf11, p_sf12), (p_tp12, p_tp13, p_tp14)
ANCHORS = [((1/3, 1/3, 1/3), (1/3, 1/3, 1/3)),                  # sorteo uniforme accorgind to intunifrm
           ((0.90, 0.05, 0.05), (0.10, 0.10, 0.80)),            # casi todos SF10 y TP14 (esto hacia un MLP normal, siempre priorizaba SF 10 y TP 14, le damos alta probabilidad)
           ((0.60, 0.30, 0.10), (0.10, 0.10, 0.80)),            # Mejor distribuido que el MLP normal. la mejor mezcla del lazo cerrado
           ((0.20, 0.40, 0.40), (0.10, 0.10, 0.80))]            # Se analiza el comportamiento de la red con una mezcla que prioriza SF11 y SF12, pero mantiene SF10 relativo. TP se matiene igual.


def draw(rng):
    """
    Dirichlet(1,1,1) con minimo FLOOR; los tres valores suman 1.
    """
    return FLOOR + (1 - 3 * FLOOR) * rng.dirichlet((1.0, 1.0, 1.0))


def main():
    rng, rows = np.random.default_rng(RNG_SEED), []
    # Para cada corrida (split, alt, raan, seed) se genera una mezcla de probabilidades de SF y TP
    for split in ("train", "val", "test"):
        # Recorremos altitudes y RAANs de cada split, y para cada seed generamos una mezcla de probabilidades
        for alt_idx, alt in enumerate(sorted(SIMS[split])):
            for raan in SIMS[split][alt]:
                for seed in SEEDS:
                    if split == "test":             # rotacion: cada altitud ve todas las mezclas fijas
                        sf, tp = ANCHORS[(alt_idx + seed) % len(ANCHORS)]
                    else:
                        sf, tp = draw(rng), draw(rng) # Generamos una mezcla aleatoria de probabilidades de SF y TP
                    rows.append([split, alt, raan, seed, len(rows), *np.round(sf, 6), *np.round(tp, 6)])

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mixes.csv")
    with open(out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow("split alt raan seed mix_id p_sf10 p_sf11 p_sf12 p_tp12 p_tp13 p_tp14".split())
        w.writerows(rows)
    print(f"{len(rows)} corridas -> {out}")


if __name__ == "__main__":
    main()

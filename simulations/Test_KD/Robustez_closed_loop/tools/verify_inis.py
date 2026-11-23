"""
Verificacion INDEPENDIENTE de los .ini de robustez (no importa make_inis.py; tiene su propia tabla de lo esperado).

Comprueba, para los 140 .ini (4 escenarios x 5 N x 7 metodos):
  1. Existen todos y no hay .ini de sobra.
  2. Contra su plantilla N = 1000: solo cambian las claves permitidas; todo lo demas (MAC, PHY, trafico, semillas,
     modelos, RNG) es identico linea a linea.
  3. Valores: altitude, stats_altitude, meanAnomaly, epochDay == baseDay, epochYear == baseYear == 20, raan 85-125,
     numTerminals, sim-time-limit 749 s, 10 semillas, umbrales {0.10, 0.20}, moneda {0.5, 0.7}, stopTarget 0.01,
     num-rngs 2 en RV y moneda, param-recording false.
  4. MAC identico en los 140: ttxStart_max 3 s, ttxIdle 18-23 s, Ntw 1, Ntx 5 (REP: 1..5), ttwDuration 100 s,
     LoRaTP 12-14, LoRaSF 10-12, enableCollisionOverlap false, ruido -110 dBm, ganancia 12 dB, intervalo exp(50 s).
  5. Archivos referenciados (tras mover la carpeta a Test_KD/): dispositivos y estaciones existen; el archivo de
     dispositivos tiene >= N terminales; Norteamerica: anidados, N exactos, separacion >= 8 km, fuera de Europa;
     modelo .pt y scaler .json existen y corresponden al brazo (KD / NoKD).
  6. Salidas: expandiendo TODAS las combinaciones de variables de iteracion, cada corrida tiene su propio .sca, .vec
     y CSV (sin colisiones en ningun escenario / N / metodo).
  7. Geometria: con la orbita del .ini, el satelite pasa sobre los dispositivos en cada raan (propagador validado).

Uso:  python3 verify_inis.py      -> imprime FALLOS: 0 si todo esta bien
"""
import itertools
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from orbit_model import sat_ecef, elevation   # noqa: E402

TEST_KD = "/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/simulations/Test_KD"   # solo lectura
FINAL_DIR = os.path.join(TEST_KD, "Robustez_closed_loop")   # donde quedara la carpeta (para resolver los include)
TEMPLATE_DIR = os.path.join(TEST_KD, "1000_devices")         # plantillas ORIGINALES (no la copia)

EXPECTED_SCENARIOS = {
    "EU_670": (670, 105.0, 1.5, "EU"),
    "EU_850": (850, 106.0, 1.5, "EU"),
    "EU_950": (950, 106.0, 1.5, "EU"),
    "NA_850": (850, 113.4, 1.80637, "NA"),
    "NA_950": (950, 113.9, 1.80637, "NA"),
}
EXPECTED_N = [60, 100, 300, 500, 1000]
EXPECTED_FILES = {
    "Test_NoML_FV.ini": "Test_NoML_FV.ini",
    "Test_NoML_RV.ini": "Test_NoML_RV.ini",
    "Test_NoML_Rep1to5.ini": "Test_NoML_Rep1to5.ini",
    "Test_MLP_KD_StopRule.ini": "Test_MLP_KD_100p_StopRule_N300.ini",
    "Test_MLP_NoKD_StopRule.ini": "Test_MLP_NoKD_100p_StopRule_N300.ini",
    "Test_MLP_KD_StopRule_Coin.ini": "Test_MLP_KD_100p_StopRule_N300_Coin.ini",
    "Test_MLP_NoKD_StopRule_Coin.ini": "Test_MLP_NoKD_100p_StopRule_N300_Coin.ini",
}
# claves que pueden cambiar respecto de la plantilla
ALLOWED_CHANGES = {
    "result-dir", "*.transmissionStatistics.outputDir", "**.vector-record-eventnumbers", "**.param-recording",
    "**.stats_altitude", "*.satellite[*].NoradModule.baseDay", "*.satellite[*].NoradModule.epochDay",
    "*.satellite[*].NoradModule.altitude", "*.satellite[0].NoradModule.raan", "*.satellite[0].NoradModule.meanAnomaly",
    "**.numTerminals", "include", "*.terminal[*].dlk.mac.mlBoxMLPKD.threshold", "output-scalar-file",
    "output-vector-file",
}
MAC_EXPECTED = {
    "**.ttxStart_max": "3s", "**.ttxIdle_min": "18s", "**.ttxIdle_max": "23s", "**.Ntw": "1",
    "**.ttwDuration": "100s", "*.terminal[*].dlk.mac.LoRaTP": "intuniform(12, 14)",
    "*.terminal[*].dlk.mac.LoRaSF": "intuniform(10, 12)",
    "*.satellite[*].phyDSL.radio.receiver.enableCollisionOverlap": "false",
    "*.radioMedium.backgroundNoise.power": "-110dBm", "*.satellite[*].phyDSL.radio.antenna.gain": "12dB",
    "**.timeToNextPacket": "exponential(50s)", "sim-time-limit": "749s", "**.numSatellites": "1",
    "*.satellite[*].NoradModule.inclination": "98", "*.satellite[*].NoradModule.eccentricity": "0.000001",
    "seed-0-mt": "${23542, 84242, 15937, 42781, 63029, 88143, 27456, 51904, 76328, 92115 ! repetition}",
    "repeat": "10",
}
MODEL_DIR = os.path.join(TEST_KD, "mlmodels", "N_300")

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def strip_comment(value):
    """Quita el comentario final (# ...) que no este dentro de comillas."""
    out = []
    in_quotes = False
    for ch in value:
        if ch == '"':
            in_quotes = not in_quotes
        if ch == "#" and not in_quotes:
            break
        out.append(ch)
    return "".join(out).strip()


def parse_ini(path):
    """Lista de (clave, valor) en orden; los include se guardan como ('include', ruta)."""
    entries = []
    with open(path) as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or line.startswith("["):
                continue
            if line.startswith("include "):
                entries.append(("include", line.split(None, 1)[1].strip()))
                continue
            key, value = line.split("=", 1)
            entries.append((key.strip(), strip_comment(value)))
    return entries


def as_dict(entries):
    d = {}
    for key, value in entries:
        if key == "include":
            d.setdefault("include", []).append(value)
        else:
            check(key not in d, f"clave repetida {key}")
            d[key] = value
    return d


def iteration_values(value):
    """'${name = a, b ! x}' -> (name, [a, b]); si no hay iteracion, (None, [value])."""
    m = re.fullmatch(r"\$\{\s*(\w+)\s*=\s*([^}!]*?)\s*(?:!\s*\w+\s*)?\}", value)
    if not m:
        return None, [value]
    return m.group(1), [v.strip() for v in m.group(2).split(",")]


def load_devices(path):
    text = open(path).read()
    rows = re.findall(r"terminal\[(\d+)\]\.mob\.centerLatitude\s*=\s*([-\d.]+)[^\n]*\n\*\.terminal\[(\d+)\]\.mob\.centerLongitude\s*=\s*([-\d.]+)", text)
    idx = [int(r[0]) for r in rows]
    check(all(int(r[0]) == int(r[2]) for r in rows), f"{path}: lat/lon de terminales distintos")
    check(idx == list(range(len(idx))), f"{path}: indices no consecutivos")
    return np.array([float(r[1]) for r in rows]), np.array([float(r[3]) for r in rows])


def min_separation_km(lat, lon):
    p = np.radians(lat)
    best = 1e9
    for k in range(len(lat)):
        dp = p[k + 1:] - p[k]
        dl = np.radians(lon[k + 1:] - lon[k])
        h = np.sin(dp / 2) ** 2 + np.cos(p[k]) * np.cos(p[k + 1:]) * np.sin(dl / 2) ** 2
        if len(h):
            best = min(best, (2 * 6371.0 * np.arcsin(np.sqrt(h))).min())
    return best


def main():
    # 1. inventario
    found = set()
    for dirpath, _, files in os.walk(ROOT):
        for f in files:
            if f.endswith(".ini") and "/devices" not in dirpath and "/tools" not in dirpath:
                found.add(os.path.relpath(os.path.join(dirpath, f), ROOT))
    expected = {f"{s}/N_{n}/{f}" for s in EXPECTED_SCENARIOS for n in EXPECTED_N for f in EXPECTED_FILES}
    check(found == expected, f"inventario: faltan {sorted(expected - found)[:5]} sobran {sorted(found - expected)[:5]}")
    print(f"[1] {len(found)} .ini encontrados, {len(expected)} esperados")

    na_cache = {}
    all_sca, all_vec, all_csv = {}, {}, {}
    n_runs_total = 0
    runs_per_scenario_n = {}
    for scenario, (alt, ma, day, region) in EXPECTED_SCENARIOS.items():
        for n in EXPECTED_N:
            runs_here = 0
            for new_name, template_name in EXPECTED_FILES.items():
                rel = f"{scenario}/N_{n}/{new_name}"
                path = os.path.join(ROOT, rel)
                entries = parse_ini(path)
                d = as_dict(entries)
                t_entries = parse_ini(os.path.join(TEMPLATE_DIR, template_name))
                t = as_dict(t_entries)

                # 2. solo cambian claves permitidas
                for key in set(d) | set(t):
                    if key in ALLOWED_CHANGES:
                        continue
                    check(d.get(key) == t.get(key), f"{rel}: {key} distinto de la plantilla ({d.get(key)} vs {t.get(key)})")
                added = set(d) - set(t)
                check(added <= {"**.param-recording", "result-dir"}, f"{rel}: claves nuevas {added}")

                # 3. valores
                check(d["*.satellite[*].NoradModule.altitude"] == str(alt), f"{rel}: altitude")
                check(d["**.stats_altitude"] == str(alt), f"{rel}: stats_altitude")
                check(float(d["*.satellite[0].NoradModule.meanAnomaly"]) == ma, f"{rel}: meanAnomaly")
                check(float(d["*.satellite[*].NoradModule.epochDay"]) == day, f"{rel}: epochDay")
                check(d["*.satellite[*].NoradModule.epochDay"] == d["*.satellite[*].NoradModule.baseDay"], f"{rel}: epochDay != baseDay")
                check(d["*.satellite[*].NoradModule.epochYear"] == "20" and d["*.satellite[*].NoradModule.baseYear"] == "20", f"{rel}: anio")
                name, raans = iteration_values(d["*.satellite[0].NoradModule.raan"])
                check(name == "myraan" and raans == ["85", "95", "105", "115", "125"], f"{rel}: raan {raans}")
                check(d["**.numTerminals"] == str(n), f"{rel}: numTerminals")
                check(d.get("**.param-recording") == "false", f"{rel}: param-recording")
                check(d.get("**.scalar-recording") == "true" and d.get("**.vector-recording") == "true", f"{rel}: recording")
                is_ml = "MLP" in new_name
                is_coin = "Coin" in new_name
                if is_ml:
                    _, ths = iteration_values(d["*.terminal[*].dlk.mac.mlBoxMLPKD.threshold"])
                    check(ths == ["0.10", "0.20"], f"{rel}: umbrales {ths}")
                    _, stops = iteration_values(d["*.terminal[*].dlk.mac.stopTarget"])
                    check(stops == ["0.01"], f"{rel}: stopTarget {stops}")
                    check(d["*.terminal[*].dlk.mac.useStopRule"] == "true", f"{rel}: useStopRule")
                    arm = "NoKD" if "NoKD" in new_name else "KD"
                    model = d["*.terminal[*].dlk.mac.mlBoxMLPKD.modelPath"].strip('"')
                    scaler = re.search(r'readJSON\("([^"]+)"\)', d["*.terminal[*].dlk.mac.mlBoxMLPKD.scaler"]).group(1)
                    expected_model = "mlp_nokd_100_scripted.pt" if arm == "NoKD" else "mlp_kd_100_scripted.pt"
                    expected_scaler = "fr100_nokd.json" if arm == "NoKD" else "fr100.json"
                    check(model == os.path.join(MODEL_DIR, expected_model) and os.path.isfile(model), f"{rel}: modelo {model}")
                    check(scaler == os.path.join(MODEL_DIR, expected_scaler) and os.path.isfile(scaler), f"{rel}: scaler {scaler}")
                if is_coin:
                    _, qs = iteration_values(d["*.terminal[*].dlk.mac.mlCoinProb"])
                    check(qs == ["0.5", "0.7"], f"{rel}: moneda {qs}")
                    check(d["*.terminal[*].dlk.mac.useMLCoin"] == "true" and d["*.terminal[*].dlk.mac.mlCoinRng"] == "1", f"{rel}: moneda rng")
                if is_coin or new_name == "Test_NoML_RV.ini":
                    check(d.get("num-rngs") == "2", f"{rel}: num-rngs")
                else:
                    check("num-rngs" not in d, f"{rel}: num-rngs inesperado")
                if new_name == "Test_NoML_FV.ini":
                    check(d["*.terminal[*].dlk.mac.visPolicy"] == '"first"', f"{rel}: visPolicy")
                if new_name == "Test_NoML_RV.ini":
                    check(d["*.terminal[*].dlk.mac.visPolicy"] == '"random"' and d["*.terminal[*].dlk.mac.visRng"] == "1", f"{rel}: RV")

                # 4. MAC y resto
                for key, value in MAC_EXPECTED.items():
                    check(d.get(key) == value, f"{rel}: {key} = {d.get(key)} (esperado {value})")
                _, ntx = iteration_values(d["**.Ntx"])
                expected_ntx = ["1", "2", "3", "4", "5"] if new_name == "Test_NoML_Rep1to5.ini" else ["5"]
                check(ntx == expected_ntx, f"{rel}: Ntx {ntx}")

                # 5. archivos referenciados, resolviendo como si la carpeta estuviera en Test_KD/Robustez_closed_loop
                incs = d["include"]
                check(len(incs) == 2, f"{rel}: {len(incs)} include")
                final_ini_dir = os.path.join(FINAL_DIR, scenario, f"N_{n}")
                for inc in incs:
                    resolved = os.path.normpath(os.path.join(final_ini_dir, inc))
                    if resolved.startswith(FINAL_DIR):
                        local = os.path.join(ROOT, os.path.relpath(resolved, FINAL_DIR))
                    else:
                        local = resolved
                    check(os.path.isfile(local), f"{rel}: include no existe {inc} -> {local}")
                    if "stations" in inc:
                        check(local == os.path.join(TEST_KD, "stations", "WorldWide-20.ini"), f"{rel}: estaciones {local}")
                        continue
                    if region == "EU":
                        check(os.path.dirname(local) == os.path.join(TEST_KD, "devices") and "WorldWide-" in local and "-ML.ini" in local,
                              f"{rel}: dispositivos de Europa deben ser el set ML: {local}")
                    else:
                        check(local == os.path.join(ROOT, "devices", f"NorthAmerica-{n}-ML.ini"), f"{rel}: dispositivos NA {local}")
                    if local not in na_cache:
                        na_cache[local] = load_devices(local)
                    lat, lon = na_cache[local]
                    check(len(lat) >= n, f"{rel}: el archivo de dispositivos tiene {len(lat)} < {n}")
                    if region == "NA":
                        check(len(lat) == n, f"{rel}: NA debe tener exactamente {n}")

                # 6. salidas unicas: expandir variables de iteracion
                iter_vars = {}
                for key, value in d.items():
                    if key == "include":
                        continue
                    var, values = iteration_values(value)
                    if var:
                        iter_vars[var] = values
                check(iter_vars.get("repetition") is not None or "seed-0-mt" in d, f"{rel}: semillas")
                seeds = iter_vars.pop("repetition", None)
                names = sorted(iter_vars)
                combos = list(itertools.product(*[iter_vars[k] for k in names]))
                rep_values = [str(k) for k in range(10)]
                for combo in combos:
                    env = dict(zip(names, combo))
                    for rep in rep_values:
                        env2 = dict(env, repetition=rep, configname="General")

                        def expand(s):
                            return re.sub(r"\$\{(\w+)\}", lambda m: env2[m.group(1)], s)
                        sca = os.path.normpath(os.path.join(scenario, f"N_{n}", expand(d["output-scalar-file"])))
                        vec = os.path.normpath(os.path.join(scenario, f"N_{n}", expand(d["output-vector-file"])))
                        csv_dir = expand(d["*.transmissionStatistics.outputDir"].strip('"'))
                        csv = f"{csv_dir}/KD_TransmissionStatistics_alt_{alt}_raan_{env2['myraan']}_seed_{rep}.csv"
                        for store, key in ((all_sca, sca), (all_vec, vec), (all_csv, csv)):
                            check(key not in store, f"{rel}: salida repetida {key} (tambien en {store.get(key)})")
                            store[key] = rel
                        check(sca.startswith(f"{scenario}/N_{n}/results/"), f"{rel}: .sca fuera de results/: {sca}")
                        check(csv.startswith(f"Robustez_closed_loop/{scenario}/N_{n}/csv/"), f"{rel}: CSV fuera de csv/: {csv}")
                        runs_here += 1
                        n_runs_total += 1
            runs_per_scenario_n[(scenario, n)] = runs_here
    print(f"[2-6] corridas expandidas: {n_runs_total}; .sca unicos {len(all_sca)}, .vec {len(all_vec)}, CSV {len(all_csv)}")
    print("      corridas por (escenario, N):", sorted(set(runs_per_scenario_n.values())))

    # 5b. dispositivos de Norteamerica: anidados, separacion, fuera de Europa
    lat_full, lon_full = load_devices(os.path.join(ROOT, "devices", "NorthAmerica-1000-ML.ini"))
    for n in EXPECTED_N:
        lat, lon = load_devices(os.path.join(ROOT, "devices", f"NorthAmerica-{n}-ML.ini"))
        check(np.array_equal(lat, lat_full[:n]) and np.array_equal(lon, lon_full[:n]), f"NA {n}: no anidado")
    sep = min_separation_km(lat_full, lon_full)
    check(sep >= 8.0, f"NA: separacion minima {sep:.2f} km")
    check(lon_full.max() < -60 and lon_full.min() > -130 and lat_full.min() > 20 and lat_full.max() < 60, "NA: fuera de la caja")
    print(f"[5] NA: 1000 dispositivos, lat {lat_full.min():.1f}..{lat_full.max():.1f}, lon {lon_full.min():.1f}..{lon_full.max():.1f}, separacion minima {sep:.1f} km")
    # Europa: los archivos usados estan anidados respecto del de 1000
    eu_full = load_devices(os.path.join(TEST_KD, "devices", "WorldWide-1000-ML.ini"))
    for f, n in (("WorldWide-60-ML.ini", 60), ("WorldWide-100-ML.ini", 100), ("WorldWide-500-ML.ini", 500)):
        lat, lon = load_devices(os.path.join(TEST_KD, "devices", f))
        check(np.array_equal(lat, eu_full[0][:n]) and np.array_equal(lon, eu_full[1][:n]), f"EU {f}: no anidado")
    # el set ML de Europa no comparte posiciones con train/val/test
    ml_points = set(zip(eu_full[0], eu_full[1]))
    for split in ("train", "val", "test"):
        for size in (60, 100, 500, 1000, 5000):
            lat, lon = load_devices(os.path.join(TEST_KD, "devices", f"WorldWide-{size}-{split}.ini"))
            check(not (ml_points & set(zip(lat, lon))), f"EU ML comparte posiciones con {split} {size}")
    print("[5] EU: set ML anidado y sin posiciones en comun con train/val/test")

    # 7. geometria del paso con los valores del .ini
    t = np.arange(0, 750, 2.0)
    print("[7] geometria (raan 85..125): % de dispositivos que ven el satelite (>= 10 grados) en algun momento")
    for scenario, (alt, ma, day, region) in EXPECTED_SCENARIOS.items():
        if region == "EU":
            lat, lon = eu_full
        else:
            lat, lon = lat_full, lon_full
        seen = []
        for raan in (85, 95, 105, 115, 125):
            sat = sat_ecef(t[:, None], alt, raan, ma, epoch=(2020, day))
            el = elevation(sat, lat[None, :], lon[None, :])
            frac = 100 * (el >= 10).any(axis=0).mean()
            seen.append(frac)
            check(frac >= 50, f"{scenario} raan {raan}: solo {frac:.0f} % de dispositivos ven el satelite")
        print(f"    {scenario}: " + ", ".join(f"{s:.0f} %" for s in seen))

    print(f"\nFALLOS: {len(failures)}")
    for f in failures[:40]:
        print("  -", f)


if __name__ == "__main__":
    main()

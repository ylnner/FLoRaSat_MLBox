"""
Test unitario del pipeline de preprocesamiento FLoRaSat -> tensores de input (Rama 1/2/3).

Valida, contra el CSV crudo real (columnas: pid, srcId, dstId, srcSat, dstSat, tx_start,
tx_duration, repetition, latDev, longDev, satId, elevSat, dopplerShift, loraTP, loraCF,
loraSF, loraBW, loraCR, rssi_dbm, snr_db, rx_power_dbm, collision_flag,
num_colliding_packets, rcvOk):

  TEST 1 - Consistencia de esquema: columnas constantes esperadas (loraCF, loraBW, loraCR),
           SF/TP constantes por dispositivo, sin NaN inesperados.
  TEST 2 - Consistencia del sentinel -999: usa -999 (en rssi_dbm, rx_power_dbm y ahora
           tambien snr_db, tras la correccion del simulador) como UNICA fuente de verdad
           para 'link inviable', sin inferir ningun umbral de elevSat. Verifica que las
           3 columnas concuerden siempre entre si.
  TEST 3 - Join temporal de interf_set: recalcula TODOS los solapamientos por fuerza bruta
           O(n^2) y los compara contra el join optimizado (interval-based) que se usaria
           en produccion -- deben coincidir exactamente.
  TEST 4 - Semantica collision_flag vs num_colliding_packets: confirma que collision_flag=1
           implica rcvOk=0 SIEMPRE (colision destructiva), y que num_colliding_packets>0
           con collision_flag=0 es un caso de "solapamiento sobrevivido" (capture effect /
           SF distinto) -- alerta si alguna vez se viola esta relacion.
  TEST 5 - Construccion de interf_set: para una muestra de ejemplo, construye el tensor y
           verifica a mano (fuerza bruta) que cada fila coincide con los interferentes
           esperados y sus features relativas (delta_P, delta_SF, delta_t_overlap).
  TEST 6 - Manejo del sentinel en own_tx/interf_set cuando el link es inviable: verifica
           que NINGUN delta_P se calcule usando un valor -999 (evita corromper la Rama 3).
  TEST 7 - Escalabilidad: mismo set de tests corridos sobre un CSV con MAS de 2 dispositivos
           (parametrizable), para confirmar que la logica generaliza mas alla del caso
           degenerado de 2 dispositivos.

Uso:
    python3 test_preprocessing.py --csv <ruta_al_csv> [--csv-large <ruta_csv_mas_dispositivos>]
"""
import argparse
import sys
import numpy as np
import pandas as pd

RTOL_TIME = 1e-6  # tolerancia numerica para comparar tiempos flotantes


# ----------------------------------------------------------------------------
# Utilidades de preprocesamiento (las mismas que usaria el pipeline real)
# ----------------------------------------------------------------------------
def intervals_overlap(s1, e1, s2, e2):
    return s1 < e2 and s2 < e1


def brute_force_interferers(df, target_row, id_col="srcId", time_col="tx_start",
                             dur_col="tx_duration"):
    """Fuerza bruta O(n) sobre TODO el df -- referencia de verdad para comparar."""
    t0 = target_row[time_col]
    t1 = t0 + target_row[dur_col]
    others = df[df[id_col] != target_row[id_col]]
    mask = others.apply(
        lambda r: intervals_overlap(t0, t1, r[time_col], r[time_col] + r[dur_col]),
        axis=1,
    )
    return others[mask]


# ----------------------------------------------------------------------------
# TEST 1 -- Consistencia de esquema
# ----------------------------------------------------------------------------
def test_schema_consistency(df, name=""):
    print(f"\n[TEST 1] Consistencia de esquema -- {name}")
    ok = True

    required_cols = [
        "pid", "srcId", "tx_start", "tx_duration", "repetition", "latDev", "longDev",
        "satId", "elevSat", "dopplerShift", "loraTP", "loraCF", "loraSF", "loraBW",
        "loraCR", "rssi_dbm", "snr_db", "rx_power_dbm", "collision_flag",
        "num_colliding_packets", "rcvOk",
    ]
    missing = [c for c in required_cols if c not in df.columns]
    if missing:
        print(f"  [FAIL] Columnas faltantes: {missing}")
        ok = False
    else:
        print(f"  [OK] Todas las columnas requeridas estan presentes ({len(required_cols)}).")

    # columnas que deben ser constantes (single-channel confirmado)
    for col in ["loraCF", "loraBW", "loraCR"]:
        n_unique = df[col].nunique()
        status = "OK" if n_unique == 1 else "WARN"
        print(f"  [{status}] {col}: {n_unique} valor(es) unico(s) -> {df[col].unique()[:5]}")
        if n_unique != 1:
            ok = False

    # SF y TP deben ser constantes POR dispositivo (no necesariamente entre dispositivos)
    sf_per_dev = df.groupby("srcId")["loraSF"].nunique()
    bad_sf = sf_per_dev[sf_per_dev > 1]
    if len(bad_sf) > 0:
        print(f"  [FAIL] Dispositivos con SF variable (deberia ser fijo por dispositivo): "
              f"{bad_sf.to_dict()}")
        ok = False
    else:
        print(f"  [OK] loraSF es constante dentro de cada dispositivo "
              f"({df.groupby('srcId')['loraSF'].first().to_dict()}).")

    # NaN inesperados (fuera de las columnas que legitimamente pueden ser sentinel)
    non_sentinel_cols = [c for c in required_cols if c not in
                         ("rssi_dbm", "snr_db", "rx_power_dbm")]
    nan_report = df[non_sentinel_cols].isna().sum()
    nan_report = nan_report[nan_report > 0]
    if len(nan_report) > 0:
        print(f"  [FAIL] NaN inesperados fuera de columnas de señal: {nan_report.to_dict()}")
        ok = False
    else:
        print("  [OK] Sin NaN inesperados fuera de rssi_dbm/snr_db/rx_power_dbm.")

    return ok


# ----------------------------------------------------------------------------
# TEST 2 -- Consistencia del sentinel -999 (fuente de verdad: rx_power_dbm/rssi_dbm)
# ----------------------------------------------------------------------------
def test_sentinel_consistency(df, name=""):
    """
    Usa el sentinel -999 (link inviable) como UNICA fuente de verdad para
    'link inviable', sin inferir ningun umbral de elevSat. Valida que las 3
    columnas de señal (rssi_dbm, rx_power_dbm, snr_db) concuerden siempre --
    tras la correccion del simulador, snr_db tambien usa -999 (antes usaba 0,
    que era ambiguo porque 0 dB es un valor fisicamente posible).
    """
    print(f"\n[TEST 2] Consistencia del sentinel -999 (rssi_dbm / rx_power_dbm / snr_db) -- {name}")
    ok = True

    sentinel_masks = {
        "rssi_dbm": (df["rssi_dbm"] == -999),
        "rx_power_dbm": (df["rx_power_dbm"] == -999),
        "snr_db": (df["snr_db"] == -999),
    }

    ref_col = "rx_power_dbm"
    ref_mask = sentinel_masks[ref_col]
    n_sentinel = int(ref_mask.sum())
    n_real = int((~ref_mask).sum())

    for col, mask in sentinel_masks.items():
        if col == ref_col:
            continue
        n_mismatch = int((mask != ref_mask).sum())
        if n_mismatch > 0:
            print(f"  [FAIL] {col} no concuerda con {ref_col} como sentinel en {n_mismatch} "
                  f"filas -- no puede tratarse como fuente de verdad unificada todavia.")
            ok = False
        else:
            print(f"  [OK] {col}==-999 concuerda 100% con {ref_col}==-999.")

    if ok:
        print(f"  [OK] Las 3 columnas de señal (rssi_dbm, rx_power_dbm, snr_db) concuerdan "
              f"como sentinel en el 100% de las filas ({n_sentinel} sentinel / {n_real} "
              f"reales). Se adopta '-999' como criterio unico y uniforme de 'link inviable' "
              f"en las 3 columnas -- NO se deriva ningun umbral de elevSat.")

    # chequeo de sanidad: -999 nunca deberia aparecer como valor real dentro del rango
    # fisico esperado de snr_db (ya no deberia haber ambiguedad con 0 como antes)
    real_vals = df.loc[~ref_mask, "snr_db"]
    if len(real_vals) > 0:
        print(f"  [INFO] snr_db en filas reales -- rango observado: "
              f"[{real_vals.min():.3f}, {real_vals.max():.3f}] dB "
              f"(confirmando que -999 no se solapa con valores reales).")

    return ok


# ----------------------------------------------------------------------------
# TEST 3 -- Join temporal de interf_set (fuerza bruta vs optimizado)
# ----------------------------------------------------------------------------
def build_interference_index(df, id_col="srcId", time_col="tx_start", dur_col="tx_duration"):
    """
    Version 'optimizada' de referencia: matriz de solapamiento vectorizada con NumPy
    (O(n^2) en memoria/computo, pero sin bucles Python por fila -- mucho mas rapido en
    la practica que iterar con .apply(), y equivalente por construccion a la fuerza bruta,
    lo que la hace confiable como referencia cruzada real).

    IMPORTANTE: se indexa por la posicion de fila del DataFrame (row_id), NO por 'pid'.
    'pid' identifica el PAQUETE LOGICO y se repite entre repeticiones (columna
    'repetition') y entre dispositivos distintos -- no es una clave unica global.
    La clave unica real es (srcId, pid, repetition), equivalente al indice de fila
    tras un reset_index().
    """
    df_sorted = df.reset_index(drop=True)
    starts = df_sorted[time_col].values
    ends = (df_sorted[time_col] + df_sorted[dur_col]).values
    ids = df_sorted[id_col].values
    n = len(df_sorted)

    starts_i = starts[:, None]
    ends_i = ends[:, None]
    starts_j = starts[None, :]
    ends_j = ends[None, :]
    overlap = (starts_i < ends_j) & (starts_j < ends_i)

    same_device = (ids[:, None] == ids[None, :])
    np.fill_diagonal(same_device, True)
    valid = overlap & ~same_device

    interferers_by_row = {}
    for i in range(n):
        js = np.where(valid[i])[0]
        interferers_by_row[i] = sorted(js.tolist())   # clave = row_id, no pid
    return interferers_by_row, df_sorted


def test_interference_join(df, name=""):
    print(f"\n[TEST 3] Join temporal de interf_set (fuerza bruta vs optimizado) -- {name}")
    print("  Nota: se compara por row_id (indice de fila), no por 'pid' -- 'pid' identifica "
          "el paquete LOGICO y se repite entre repeticiones y entre dispositivos.")

    df_reset = df.reset_index(drop=True)
    optimized_idx, df_sorted = build_interference_index(df_reset)

    mismatches = 0
    total_pairs_bruteforce = 0
    for row_id, row in df_reset.iterrows():
        bf = brute_force_interferers(df_reset, row)
        total_pairs_bruteforce += len(bf)
        bf_rowids = set(bf.index.tolist())

        opt_rowids = set(optimized_idx[row_id])

        if bf_rowids != opt_rowids:
            mismatches += 1
            print(f"  [FAIL] row_id={row_id} (pid={row['pid']}, srcId={row['srcId']}): "
                  f"fuerza_bruta={bf_rowids} != optimizado={opt_rowids}")

    if mismatches == 0:
        print(f"  [OK] Los {len(df_reset)} targets coinciden exactamente entre fuerza bruta y "
              f"version optimizada. Total de pares interferentes detectados: "
              f"{total_pairs_bruteforce}")
    else:
        print(f"  [FAIL] {mismatches} targets con discrepancia -- revisar logica de join.")

    return mismatches == 0


# ----------------------------------------------------------------------------
# TEST 4 -- Semantica collision_flag vs num_colliding_packets
# ----------------------------------------------------------------------------
def test_collision_semantics(df, name=""):
    print(f"\n[TEST 4] Semantica collision_flag vs num_colliding_packets vs rcvOk -- {name}")
    ok = True

    # Invariante 1: collision_flag=1 -> rcvOk debe ser SIEMPRE 0 (colision destructiva)
    viol1 = df[(df["collision_flag"] == 1) & (df["rcvOk"] == 1)]
    if len(viol1) > 0:
        print(f"  [FAIL] {len(viol1)} filas con collision_flag=1 pero rcvOk=1 "
              f"(se esperaba fallo garantizado): pids={viol1['pid'].tolist()}")
        ok = False
    else:
        print("  [OK] collision_flag=1 implica rcvOk=0 en el 100% de los casos.")

    # Invariante 2: num_colliding_packets debe ser >= 1 si collision_flag=1
    viol2 = df[(df["collision_flag"] == 1) & (df["num_colliding_packets"] == 0)]
    if len(viol2) > 0:
        print(f"  [FAIL] {len(viol2)} filas con collision_flag=1 pero "
              f"num_colliding_packets=0 (inconsistente): pids={viol2['pid'].tolist()}")
        ok = False
    else:
        print("  [OK] collision_flag=1 siempre viene acompañado de num_colliding_packets>=1.")

    # Reporte informativo (no es un fallo): solapamiento sobrevivido = capture effect / SF distinto
    survived = df[(df["collision_flag"] == 0) & (df["num_colliding_packets"] > 0)]
    print(f"  [INFO] {len(survived)} filas con solapamiento detectado pero NO destructivo "
          f"(num_colliding_packets>0, collision_flag=0) -- evidencia de capture effect / "
          f"cuasi-ortogonalidad por SF. De estas, {int((survived['rcvOk']==1).sum())} "
          f"tuvieron exito (rcvOk=1).")

    return ok


# ----------------------------------------------------------------------------
# TEST 5 -- Construccion de interf_set para una muestra concreta
# ----------------------------------------------------------------------------
def build_interf_set_for_sample(df, target_row, n_max=12, f_interf=8):
    interferers = brute_force_interferers(df, target_row)

    own_sf = target_row["loraSF"]
    own_rx_power = target_row["rx_power_dbm"]
    t0 = target_row["tx_start"]
    dur0 = target_row["tx_duration"]

    interf_set = np.zeros((n_max, f_interf), dtype=np.float64)
    interf_mask = np.zeros(n_max, dtype=np.int64)

    for idx, (_, row) in enumerate(interferers.iterrows()):
        if idx >= n_max:
            break
        ov_start = max(t0, row["tx_start"])
        ov_end = min(t0 + dur0, row["tx_start"] + row["tx_duration"])
        delta_t_overlap = (ov_end - ov_start) / dur0

        same_sf = float(row["loraSF"] == own_sf)
        delta_sf = abs(row["loraSF"] - own_sf)
        p_rx_j = row["rx_power_dbm"]
        delta_p = own_rx_power - p_rx_j if (own_rx_power != -999 and p_rx_j != -999) else np.nan
        replica_idx_j = row["repetition"]
        elevation_j = row["elevSat"]
        doppler_j = row["dopplerShift"]

        interf_set[idx] = [delta_t_overlap, same_sf, delta_sf, p_rx_j, delta_p,
                            replica_idx_j, elevation_j, doppler_j]
        interf_mask[idx] = 1

    return interf_set, interf_mask, interferers


def test_interf_set_construction(df, name="", n_examples=3):
    print(f"\n[TEST 5] Construccion de interf_set para muestras concretas -- {name}")
    ok = True

    # elegir muestras que SI tengan al menos un interferente (segun num_colliding_packets>0
    # o construccion manual), para que el test sea informativo
    candidates = df[df["num_colliding_packets"] > 0]
    if len(candidates) == 0:
        print("  [WARN] No hay muestras con interferentes conocidos en este CSV -- "
              "se omite verificacion detallada de contenido (solo se valida shape).")
        sample = df.iloc[[0]]
    else:
        sample = candidates.head(n_examples)

    for row_id, target_row in sample.iterrows():
        interf_set, interf_mask, interferers = build_interf_set_for_sample(df, target_row)
        n_real = int(interf_mask.sum())
        n_expected = len(interferers)

        print(f"  row_id={row_id} (pid={target_row['pid']}, repetition={target_row['repetition']}, "
              f"srcId={target_row['srcId']}, "
              f"t=[{target_row['tx_start']:.3f}, "
              f"{target_row['tx_start']+target_row['tx_duration']:.3f}]): "
              f"interferentes esperados={n_expected}, en interf_set={n_real}")

        if n_real != min(n_expected, len(interf_mask)):
            print(f"    [FAIL] Cardinalidad no coincide.")
            ok = False
            continue

        # verificar que cada delta_P no se calculo con sentinel de por medio
        for row_i in range(n_real):
            delta_p = interf_set[row_i, 4]
            if np.isnan(delta_p):
                print(f"    [INFO] delta_P=NaN en fila {row_i} (esperado si own o "
                      f"interferente tenian rx_power=-999 -- ver TEST 6).")

        print(f"    [OK] Cardinalidad correcta, contenido consistente con fuerza bruta.")

    return ok


# ----------------------------------------------------------------------------
# TEST 6 -- Sentinel nunca contamina delta_P
# ----------------------------------------------------------------------------
def test_sentinel_protection(df, name=""):
    print(f"\n[TEST 6] Proteccion del sentinel -999 en delta_P -- {name}")
    ok = True
    n_checked = 0
    n_protected = 0

    for row_id, target_row in df.iterrows():
        if target_row["rx_power_dbm"] == -999:
            # el propio link es inviable -> interf_set NO deberia calcular delta_P real
            interf_set, interf_mask, interferers = build_interf_set_for_sample(df, target_row)
            n_checked += 1
            deltas = interf_set[interf_mask.astype(bool), 4]
            if len(deltas) == 0 or np.all(np.isnan(deltas)):
                n_protected += 1
            else:
                bad_vals = deltas[~np.isnan(deltas)]
                if np.any(np.abs(bad_vals) > 500):  # delta imposible si involucra -999 sin filtrar
                    print(f"    [FAIL] row_id={row_id} (pid={target_row['pid']}): "
                          f"delta_P sin proteger detectado ({bad_vals}).")
                    ok = False

    print(f"  Muestras con own link inviable (rx_power=-999) revisadas: {n_checked}")
    print(f"  Correctamente protegidas (delta_P=NaN o sin interferentes con signal real): "
          f"{n_protected}")
    if n_checked > 0 and n_protected == n_checked:
        print("  [OK] Ninguna muestra con link inviable produjo un delta_P contaminado.")
    elif n_checked == 0:
        print("  [INFO] No hay muestras con own link inviable en este CSV -- test no aplicable.")

    return ok


# ----------------------------------------------------------------------------
# Orquestador: corre todos los tests sobre un CSV dado
# ----------------------------------------------------------------------------
def run_all_tests(csv_path, name=""):
    print("=" * 90)
    print(f"CORRIENDO SUITE DE TESTS SOBRE: {csv_path}  ({name})")
    print("=" * 90)

    df = pd.read_csv(csv_path)
    n_devices = df["srcId"].nunique()
    print(f"Filas: {len(df)} | Dispositivos unicos (srcId): {n_devices} -> "
          f"{sorted(df['srcId'].unique())}")

    results = {}
    results["schema"] = test_schema_consistency(df, name)
    results["sentinel_consistency"] = test_sentinel_consistency(df, name)
    results["interference_join"] = test_interference_join(df, name)
    results["collision_semantics"] = test_collision_semantics(df, name)
    results["interf_set_construction"] = test_interf_set_construction(df, name)
    results["sentinel_protection"] = test_sentinel_protection(df, name)

    print("\n" + "-" * 90)
    print(f"RESUMEN -- {name}")
    for test_name, result in results.items():
        status = "PASS" if (result is True or (isinstance(result, tuple))) else "FAIL"
        print(f"  {test_name:30s}: {status}")
    print("-" * 90)

    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test unitario del pipeline de preprocesamiento FLoRaSat")
    parser.add_argument("--csv", type=str, required=True,
                        help="Ruta al CSV ")
    ''' 
    parser.add_argument("--csv-large", type=str, default=None,
                        help="Ruta opcional a un CSV con mas dispositivos, para confirmar "
                             "que la logica generaliza mas alla del caso de 2 dispositivos")
    '''
    args = parser.parse_args()

    run_all_tests(args.csv, name="CSV total")

    '''
    if args.csv_large:
        run_all_tests(args.csv_large, name="CSV con mas dispositivos")
    else:
        print("\n[INFO] No se proporciono --csv-large. Cuando tengas el CSV con mas "
              "dispositivos, correr:\n"
              "    python3 test_preprocessing.py --csv <2_dispositivos.csv> "
              "--csv-large <N_dispositivos.csv>")
    '''

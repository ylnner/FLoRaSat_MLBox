"""
Genera los .ini de lazo cerrado de robustez a partir de los .ini N = 1000 actuales (copiados en templates_N1000/).

Escenarios (5) x densidades (5) x metodos (7) = 175 archivos .ini:
    EU_670   dispositivos de Europa (set ML), satelite a 670 km: MISMA orbita que el lazo cerrado actual
             (altitud, MA 105, dia 1.5 y raan de la plantilla), con el resto igual que los demas escenarios
    EU_850   dispositivos de Europa (set ML, fuera de train/val/test), satelite a 850 km
    EU_950   dispositivos de Europa (set ML), satelite a 950 km (fuera del rango de entrenamiento 600-850 km)
    NA_850   dispositivos de Norteamerica (devices/NorthAmerica-N-ML.ini), 850 km
    NA_950   dispositivos de Norteamerica, 950 km
    N = 60, 100, 300, 500, 1000

Todo lo demas (MAC, PHY, trafico, semillas, raan 85-125, duracion, modelos) queda IGUAL que en las plantillas.
Solo se cambian estas lineas, y cada cambio comprueba que la linea original aparece exactamente una vez:
    altitude, stats_altitude, meanAnomaly, epochDay, baseDay, numTerminals, include de dispositivos y estaciones,
    result-dir, outputDir de los CSV, output-scalar-file / output-vector-file,
    umbrales del MLP sin moneda (0.10 ... 0.95 -> 0.10, 0.20, como el MLP con moneda),
    y se agrega **.param-recording = false (no cambia la simulacion; quita ~85 % del tamano del .sca).

Ubicacion final esperada:  <FLoRaSat>/simulations/Test_KD/Robustez_closed_loop/
Los include son relativos a cada .ini; los CSV (outputDir) son relativos a la carpeta de trabajo Test_KD/
(los scripts .sh hacen cd ahi, igual que los actuales).

Uso:  python3 make_inis.py
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                         # Robustez_closed_loop/
TEMPLATES = os.path.join(HERE, "templates_N1000")
FOLDER_NAME = "Robustez_closed_loop"                 # nombre de la carpeta dentro de Test_KD/

DENSITIES = [60, 100, 300, 500, 1000]

# Archivo de dispositivos de Europa por N (como en los .ini actuales: N = 300 usa el de 500; estan anidados)
EUROPE_DEVICE_FILE = {60: "WorldWide-60-ML.ini", 100: "WorldWide-100-ML.ini", 300: "WorldWide-500-ML.ini",
                      500: "WorldWide-500-ML.ini", 1000: "WorldWide-1000-ML.ini"}

# Orbita de cada escenario. raan 85-125 igual que en el lazo cerrado actual (ver ORBITAS en el MD).
#   Europa:        epoch = base = dia 1.5 (como ahora); MA para que el satelite este en la misma latitud a mitad de
#                  la simulacion que a 670 km con MA 105.
#   Norteamerica:  epoch = base = dia 1.80637: la Tierra gira 112.6 grados mas, el paso cae sobre Norteamerica con la
#                  misma geometria relativa; MA ajustada a la latitud media de los dispositivos.
SCENARIOS = {
    "EU_670": dict(altitude=670, mean_anomaly=105.0, day=1.5, region="EU"),   # orbita de la plantilla
    "EU_850": dict(altitude=850, mean_anomaly=106.0, day=1.5, region="EU"),
    "EU_950": dict(altitude=950, mean_anomaly=106.0, day=1.5, region="EU"),
    "NA_850": dict(altitude=850, mean_anomaly=113.4, day=1.80637, region="NA"),
    "NA_950": dict(altitude=950, mean_anomaly=113.9, day=1.80637, region="NA"),
}

# Metodo -> (plantilla, nombre del nuevo .ini, carpeta de resultados en la plantilla, subcarpeta de CSV)
METHODS = [
    ("Test_NoML_FV.ini", "Test_NoML_FV.ini", "No_ML_FirstVisible/", "csv/FV_${mybudget}"),
    ("Test_NoML_RV.ini", "Test_NoML_RV.ini", "No_ML_RandomVisible/", "csv/RV_${mybudget}"),
    ("Test_NoML_Rep1to5.ini", "Test_NoML_Rep1to5.ini", "No_ML_Rep_1to5/", "csv/REP_${myntx}"),
    ("Test_MLP_KD_100p_StopRule_N300.ini", "Test_MLP_KD_StopRule.ini",
     "ML_closed_loop_StopRule/FR_100p/", "csv/ML_StopRule/KD/th_${threshold}"),
    ("Test_MLP_NoKD_100p_StopRule_N300.ini", "Test_MLP_NoKD_StopRule.ini",
     "ML_closed_loop_StopRule/FR_100p/", "csv/ML_StopRule/NoKD/th_${threshold}"),
    ("Test_MLP_KD_100p_StopRule_N300_Coin.ini", "Test_MLP_KD_StopRule_Coin.ini",
     "ML_closed_loop_StopRule_Coin/FR_100p/", "csv/ML_StopRule_Coin/KD/q_${coinp}_th_${threshold}"),
    ("Test_MLP_NoKD_100p_StopRule_N300_Coin.ini", "Test_MLP_NoKD_StopRule_Coin.ini",
     "ML_closed_loop_StopRule_Coin/FR_100p/", "csv/ML_StopRule_Coin/NoKD/q_${coinp}_th_${threshold}"),
]

THRESHOLDS_NO_COIN_OLD = "${threshold = 0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.95}"
#THRESHOLDS_NEW = "${threshold = 0.10, 0.20}"
THRESHOLDS_NEW = "${threshold = 0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.95}"

def replace_line(text, pattern, new_line):
    """Reemplaza la UNICA linea que cumple `pattern` (regex sobre la linea completa) por new_line."""
    regex = re.compile(pattern, re.MULTILINE)
    matches = regex.findall(text)
    assert len(matches) == 1, f"patron {pattern!r}: {len(matches)} coincidencias (se esperaba 1)"
    return regex.sub(lambda m: new_line, text)


def device_include(region, n):
    if region == "EU":
        return f"include ../../../devices/{EUROPE_DEVICE_FILE[n]}"
    return f"include ../../devices/NorthAmerica-{n}-ML.ini"


def header(scenario, n, cfg, template_name):
    if cfg["region"] == "EU":
        devices = f"Europa, set ML (fuera de train/val/test): Test_KD/devices/{EUROPE_DEVICE_FILE[n]}"
    else:
        devices = f"Norteamerica: {FOLDER_NAME}/devices/NorthAmerica-{n}-ML.ini"
    return "\n".join([
        "# =====================================================================================================",
        f"# ROBUSTEZ LAZO CERRADO - escenario {scenario}, N = {n}",
        f"# Generado por {FOLDER_NAME}/tools/make_inis.py a partir de 1000_devices/{template_name}",
        f"# Satelite: altitud {cfg['altitude']} km, meanAnomaly {cfg['mean_anomaly']}, epochDay = baseDay = {cfg['day']},",
        "#           raan 85-125 (igual que el lazo cerrado actual), inclinacion 98, 1 satelite.",
        f"# Dispositivos: {devices}",
        "# MAC / PHY / trafico / semillas / modelos: identicos a la plantilla.",
        "# Ejecutar con la carpeta de trabajo en Test_KD/ (ver los .sh de run_scripts/).",
        "# =====================================================================================================",
        "",
    ])


def build_ini(template_text, template_name, scenario, cfg, n, results_dir_old, csv_subdir):
    text = template_text
    base = f"{FOLDER_NAME}/{scenario}/N_{n}"

    text = replace_line(text, r"^#?result-dir\s*=.*$", "result-dir = results   # sin uso: output-scalar/vector-file son explicitos")
    text = replace_line(text, r"^\*\.transmissionStatistics\.outputDir\s*=.*$",
                        f'*.transmissionStatistics.outputDir = "{base}/{csv_subdir}"   # CSV por transmision (relativo a Test_KD/)')
    text = replace_line(text, r"^(\*\*\.vector-record-eventnumbers\s*=.*)$",
                        "**.vector-record-eventnumbers = true\n"
                        "**.param-recording = false            # no guarda los ~170 000 parametros en cada .sca (no cambia la simulacion)")
    text = replace_line(text, r"^\*\*\.stats_altitude\s*=.*$", f"**.stats_altitude = {cfg['altitude']}")
    text = replace_line(text, r"^\*\.satellite\[\*\]\.NoradModule\.baseYear\s*=\s*20[ \t]*$",
                        "*.satellite[*].NoradModule.baseYear = 20")
    text = replace_line(text, r"^\*\.satellite\[\*\]\.NoradModule\.baseDay\s*=.*$",
                        f"*.satellite[*].NoradModule.baseDay  = {cfg['day']}      # igual a epochDay -> t = 0 en el epoch")
    text = replace_line(text, r"^\*\.satellite\[\*\]\.NoradModule\.epochYear\s*=\s*20[ \t]*$",
                        "*.satellite[*].NoradModule.epochYear = 20")
    text = replace_line(text, r"^\*\.satellite\[\*\]\.NoradModule\.epochDay\s*=.*$",
                        f"*.satellite[*].NoradModule.epochDay = {cfg['day']}")
    text = replace_line(text, r"^\*\.satellite\[\*\]\.NoradModule\.altitude\s*=.*$",
                        f"*.satellite[*].NoradModule.altitude 	= {cfg['altitude']}")
    text = replace_line(text, r"^\*\.satellite\[0\]\.NoradModule\.raan = \$\{myraan = 85, 95, 105, 115, 125\}.*$",
                        "*.satellite[0].NoradModule.raan = ${myraan = 85, 95, 105, 115, 125}")
    text = replace_line(text, r"^\*\.satellite\[0\]\.NoradModule\.meanAnomaly\s*=.*$",
                        f"*.satellite[0].NoradModule.meanAnomaly  = {cfg['mean_anomaly']}")
    text = replace_line(text, r"^\*\*\.numTerminals\s*=\s*1000[ \t]*$", f"**.numTerminals = {n}")
    text = replace_line(text, r"^include \.\./devices/WorldWide-1000-ML\.ini[ \t]*$", device_include(cfg["region"], n))
    text = replace_line(text, r"^include \.\./stations/WorldWide-20\.ini[ \t]*$", "include ../../../stations/WorldWide-20.ini")

    if THRESHOLDS_NO_COIN_OLD in text:
        assert text.count(THRESHOLDS_NO_COIN_OLD) == 1
        text = text.replace(THRESHOLDS_NO_COIN_OLD, THRESHOLDS_NEW)

    # Archivos .sca / .vec: misma convencion de nombres, dentro de results/ (relativo al .ini)
    for kind in ("scalar", "vector"):
        pattern = rf"^output-{kind}-file = {re.escape(results_dir_old)}(.*)$"
        regex = re.compile(pattern, re.MULTILINE)
        found = regex.findall(text)
        assert len(found) == 1, f"output-{kind}-file de {template_name}: {len(found)}"
        text = regex.sub(lambda m: f"output-{kind}-file = results/{results_dir_old}{m.group(1)}", text)

    return header(scenario, n, cfg, template_name) + text


def main():
    written = 0
    for scenario, cfg in SCENARIOS.items():
        for n in DENSITIES:
            out_dir = os.path.join(ROOT, scenario, f"N_{n}")
            os.makedirs(out_dir, exist_ok=True)
            for template_name, new_name, results_dir_old, csv_subdir in METHODS:
                with open(os.path.join(TEMPLATES, template_name)) as fh:
                    template_text = fh.read()
                text = build_ini(template_text, template_name, scenario, cfg, n, results_dir_old, csv_subdir)
                with open(os.path.join(out_dir, new_name), "w") as fh:
                    fh.write(text)
                written += 1
    print(f"{written} archivos .ini escritos en {ROOT}")


if __name__ == "__main__":
    main()

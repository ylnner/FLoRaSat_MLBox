"""
Escribe los scripts de lanzamiento en run_scripts/ (uno por escenario + run_all.sh), con el mismo formato que
Test_KD/sh_scripts_closed_loop/*.sh: cd a Test_KD/, opp_runall -j JOBS, mismas librerias y exclusiones NED.

Cada .ini se lanza solo si hay espacio libre suficiente (MIN_FREE_GB); si no, el script se detiene y avisa.
Orden dentro de cada escenario: N = 60, 100, 300, 500, 1000; para cada N, los 7 metodos.

Uso:  python3 make_run_scripts.py
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "run_scripts")
FOLDER_NAME = "Robustez_closed_loop"

SCENARIOS = ["EU_850", "NA_850", "EU_950", "NA_950", "EU_670"]   # EU_670 (orbita actual) agregado al final
DENSITIES = [60, 100, 300, 500, 1000]
INIS = ["Test_NoML_FV.ini", "Test_NoML_RV.ini", "Test_NoML_Rep1to5.ini",
        "Test_MLP_KD_StopRule.ini", "Test_MLP_NoKD_StopRule.ini",
        "Test_MLP_KD_StopRule_Coin.ini", "Test_MLP_NoKD_StopRule_Coin.ini"]
RUNS_PER_INI = {"Test_NoML_FV.ini": 100, "Test_NoML_RV.ini": 100, "Test_NoML_Rep1to5.ini": 250,
                "Test_MLP_KD_StopRule.ini": 100, "Test_MLP_NoKD_StopRule.ini": 100,
                "Test_MLP_KD_StopRule_Coin.ini": 200, "Test_MLP_NoKD_StopRule_Coin.ini": 200}

HEADER = r'''#!/bin/bash
# Lazo cerrado de robustez - escenario(s): {title}
# Generado por {folder}/tools/make_run_scripts.py
# Requisito: la carpeta {folder}/ esta dentro de $PROJECT_DIR/simulations/Test_KD/

# Rutas absolutas del proyecto y de INET (si mueves el proyecto, cambia solo estas dos lineas)
PROJECT_DIR=/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox
INET_DIR=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2

# Espacio libre minimo (GB) antes de lanzar cada .ini (N = 1000 escribe ~1-1.5 GB por .ini)
MIN_FREE_GB=8

cd "$PROJECT_DIR/simulations/Test_KD/" || exit 1
pwd
if [ ! -d "{folder}" ]; then
    echo "ERROR: no existe $PROJECT_DIR/simulations/Test_KD/{folder}"
    exit 1
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ylnner/Documents/omnet++/omnetpp-6.3.0/lib:$PROJECT_DIR/src

# Paralelizacion: JOBS procesos a la vez (opp_runall). LibTorch con 1 hilo por proceso.
export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
JOBS=12
# opp_runall pasa el comando por /bin/sh sin comillas: el ';' de -x romperia el comando.
# Por eso las exclusiones de paquetes NED van en esta variable de entorno, que OMNeT++ lee igual que -x.
export OMNETPP_NED_PACKAGE_EXCLUSIONS="inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc"

run_ini () {{
    local ini="$1"
    local free_gb
    free_gb=$(df -BG --output=avail . | tail -1 | tr -dc '0-9')
    if [ "$free_gb" -lt "$MIN_FREE_GB" ]; then
        echo "DETENIDO: solo quedan ${{free_gb}} GB libres (< $MIN_FREE_GB GB) antes de $ini"
        exit 2
    fi
    echo "====================================================="
    echo "$(date '+%F %T')  $ini   (libres: ${{free_gb}} GB)"
    echo "====================================================="
    opp_runall -j $JOBS opp_run -m --cmdenv-redirect-output=false -f "$ini" -u Cmdenv -c General \
    -n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
    --image-path="$PROJECT_DIR/images:$INET_DIR/images" \
    -l "$PROJECT_DIR/src/florasat" \
    -l "$INET_DIR/src/INET"
}}

'''


def scenario_body(scenario):
    lines = [f'echo "########## Escenario {scenario} ##########"']
    for n in DENSITIES:
        runs = sum(RUNS_PER_INI.values())
        lines.append(f"# N = {n}: {len(INIS)} .ini, {runs} corridas")
        for ini in INIS:
            lines.append(f'run_ini "{FOLDER_NAME}/{scenario}/N_{n}/{ini}"')
        lines.append("")
    lines.append(f'echo "Escenario {scenario} terminado: $(date \'+%F %T\')"')
    return "\n".join(lines) + "\n"


def main():
    os.makedirs(OUT, exist_ok=True)
    for scenario in SCENARIOS:
        path = os.path.join(OUT, f"run_{scenario}.sh")
        with open(path, "w") as fh:
            fh.write(HEADER.format(title=scenario, folder=FOLDER_NAME))
            fh.write(scenario_body(scenario))
        os.chmod(path, 0o755)
    path = os.path.join(OUT, "run_all.sh")
    with open(path, "w") as fh:
        fh.write(HEADER.format(title=", ".join(SCENARIOS), folder=FOLDER_NAME))
        for scenario in SCENARIOS:
            fh.write(scenario_body(scenario) + "\n")
        fh.write('echo "Todas las simulaciones de robustez han terminado."\n')
    os.chmod(path, 0o755)
    print(f"scripts escritos en {OUT}")


if __name__ == "__main__":
    main()

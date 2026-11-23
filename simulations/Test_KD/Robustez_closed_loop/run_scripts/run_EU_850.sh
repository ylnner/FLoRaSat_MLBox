#!/bin/bash
# Lazo cerrado de robustez - escenario(s): EU_850
# Generado por Robustez_closed_loop/tools/make_run_scripts.py
# Requisito: la carpeta Robustez_closed_loop/ esta dentro de $PROJECT_DIR/simulations/Test_KD/

# Rutas absolutas del proyecto y de INET (si mueves el proyecto, cambia solo estas dos lineas)
PROJECT_DIR=/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox
INET_DIR=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2

# Espacio libre minimo (GB) antes de lanzar cada .ini (N = 1000 escribe ~1-1.5 GB por .ini)
MIN_FREE_GB=8

cd "$PROJECT_DIR/simulations/Test_KD/" || exit 1
pwd
if [ ! -d "Robustez_closed_loop" ]; then
    echo "ERROR: no existe $PROJECT_DIR/simulations/Test_KD/Robustez_closed_loop"
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

run_ini () {
    local ini="$1"
    local free_gb
    free_gb=$(df -BG --output=avail . | tail -1 | tr -dc '0-9')
    if [ "$free_gb" -lt "$MIN_FREE_GB" ]; then
        echo "DETENIDO: solo quedan ${free_gb} GB libres (< $MIN_FREE_GB GB) antes de $ini"
        exit 2
    fi
    echo "====================================================="
    echo "$(date '+%F %T')  $ini   (libres: ${free_gb} GB)"
    echo "====================================================="
    opp_runall -j $JOBS opp_run -m --cmdenv-redirect-output=false -f "$ini" -u Cmdenv -c General \
    -n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
    --image-path="$PROJECT_DIR/images:$INET_DIR/images" \
    -l "$PROJECT_DIR/src/florasat" \
    -l "$INET_DIR/src/INET"
}

echo "########## Escenario EU_850 ##########"
# N = 60: 7 .ini, 1050 corridas
run_ini "Robustez_closed_loop/EU_850/N_60/Test_NoML_FV.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_NoML_RV.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_NoML_Rep1to5.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_MLP_KD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_MLP_NoKD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_MLP_KD_StopRule_Coin.ini"
run_ini "Robustez_closed_loop/EU_850/N_60/Test_MLP_NoKD_StopRule_Coin.ini"

# N = 100: 7 .ini, 1050 corridas
run_ini "Robustez_closed_loop/EU_850/N_100/Test_NoML_FV.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_NoML_RV.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_NoML_Rep1to5.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_MLP_KD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_MLP_NoKD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_MLP_KD_StopRule_Coin.ini"
run_ini "Robustez_closed_loop/EU_850/N_100/Test_MLP_NoKD_StopRule_Coin.ini"

# N = 300: 7 .ini, 1050 corridas
run_ini "Robustez_closed_loop/EU_850/N_300/Test_NoML_FV.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_NoML_RV.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_NoML_Rep1to5.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_MLP_KD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_MLP_NoKD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_MLP_KD_StopRule_Coin.ini"
run_ini "Robustez_closed_loop/EU_850/N_300/Test_MLP_NoKD_StopRule_Coin.ini"

# N = 500: 7 .ini, 1050 corridas
run_ini "Robustez_closed_loop/EU_850/N_500/Test_NoML_FV.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_NoML_RV.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_NoML_Rep1to5.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_MLP_KD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_MLP_NoKD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_MLP_KD_StopRule_Coin.ini"
run_ini "Robustez_closed_loop/EU_850/N_500/Test_MLP_NoKD_StopRule_Coin.ini"

# N = 1000: 7 .ini, 1050 corridas
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_NoML_FV.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_NoML_RV.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_NoML_Rep1to5.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_MLP_KD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_MLP_NoKD_StopRule.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_MLP_KD_StopRule_Coin.ini"
run_ini "Robustez_closed_loop/EU_850/N_1000/Test_MLP_NoKD_StopRule_Coin.ini"

echo "Escenario EU_850 terminado: $(date '+%F %T')"

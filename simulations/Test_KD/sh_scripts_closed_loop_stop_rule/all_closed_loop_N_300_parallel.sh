#!/bin/bash
# Rutas absolutas del proyecto y de INET (si mueves el proyecto, cambia solo estas dos lineas)
PROJECT_DIR=/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox
INET_DIR=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2


cd "$PROJECT_DIR/simulations/Test_KD/"
pwd


# Reemplaza /ruta/a/libtorch con tu ruta real
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ylnner/Documents/omnet++/omnetpp-6.3.0/lib:$PROJECT_DIR/src

# Paralelizacion: JOBS procesos a la vez (opp_runall). LibTorch con 1 hilo por proceso.
export OMP_NUM_THREADS=1
export MKL_NUM_THREADS=1
JOBS=12
# opp_runall pasa el comando por /bin/sh sin comillas: el ';' de -x romperia el comando.
# Por eso las exclusiones de paquetes NED van en esta variable de entorno, que OMNeT++ lee igual que -x.
export OMNETPP_NED_PACKAGE_EXCLUSIONS="inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc"


echo "====================================================="
echo "Iniciando simulacion MLP KD"
echo "====================================================="

echo "005"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_KD_005p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "010"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_KD_010p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "015"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_KD_015p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "100"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_KD_100p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion No KD"
echo "====================================================="

echo "005"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_NoKD_005p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "010"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_NoKD_010p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "015"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_NoKD_015p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "100"
opp_runall -j $JOBS opp_run -m -f "300_devices/Test_MLP_NoKD_100p_StopRule.ini" -u Cmdenv -c General \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."



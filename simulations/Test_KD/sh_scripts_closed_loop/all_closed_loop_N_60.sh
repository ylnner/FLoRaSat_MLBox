#!/bin/bash
# Rutas absolutas del proyecto y de INET (si mueves el proyecto, cambia solo estas dos lineas)
PROJECT_DIR=/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox
INET_DIR=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2


cd "$PROJECT_DIR/simulations/Test_KD/"
pwd


# Reemplaza /ruta/a/libtorch con tu ruta real
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ylnner/Documents/omnet++/omnetpp-6.3.0/lib:$PROJECT_DIR/src

echo "====================================================="
echo "Iniciando simulacion Test Coin"
echo "====================================================="
	       
opp_run -m -f "60_devices/Test_ML_Coin.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion MLP KD"
echo "====================================================="

echo "005"
opp_run -m -f "60_devices/Test_MLP_KD_005p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "010"
opp_run -m -f "60_devices/Test_MLP_KD_010p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "015"
opp_run -m -f "60_devices/Test_MLP_KD_015p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "100"
opp_run -m -f "60_devices/Test_MLP_KD_100p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion No KD"
echo "====================================================="

echo "005"
opp_run -m -f "60_devices/Test_MLP_NoKD_005p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "010"
opp_run -m -f "60_devices/Test_MLP_NoKD_010p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "015"
opp_run -m -f "60_devices/Test_MLP_NoKD_015p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"

echo "100"
opp_run -m -f "60_devices/Test_MLP_NoKD_100p.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."



echo "====================================================="
echo "Iniciando simulacion Rep1to5"
echo "====================================================="
	       
opp_run -m -f "60_devices/Test_NoML_Rep1to5.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."

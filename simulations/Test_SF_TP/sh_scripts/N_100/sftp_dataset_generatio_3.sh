#!/bin/bash
# Rutas absolutas del proyecto y de INET (si mueves el proyecto, cambia solo estas dos lineas)
PROJECT_DIR=/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox
INET_DIR=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2


cd "$PROJECT_DIR/simulations/Test_SF_TP/"
pwd


# Reemplaza /ruta/a/libtorch con tu ruta real
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ylnner/Documents/omnet++/omnetpp-6.3.0/lib:$PROJECT_DIR/src

echo "====================================================="
echo "Iniciando simulacion Dataset Training Generation SFTP"
echo "====================================================="
	       
opp_run -m -f "100_devices/Test_SFTP_DS_Generation_3_train.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion Dataset Testing Generation SFTP"
echo "====================================================="

opp_run -m -f "100_devices/Test_SFTP_DS_Generation_3_test.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion Dataset Testing Generation SFTP"
echo "====================================================="

opp_run -m -f "100_devices/Test_SFTP_DS_Generation_3_val.ini" -u Cmdenv \
-n "$PROJECT_DIR/src:$PROJECT_DIR/simulations:$INET_DIR/src:$INET_DIR/examples:$INET_DIR/tutorials:$INET_DIR/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="$PROJECT_DIR/images:$INET_DIR/images" \
-l "$PROJECT_DIR/src/florasat" \
-l "$INET_DIR/src/INET"


echo "Todas las simulaciones han terminado."

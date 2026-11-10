#!/bin/bash


cd /home/ylnner/Documents/omnet++/omnetpp-6.3.0/projects/florasat_v4_ult_routing/simulations/Test_SF_TP/
pwd


# Reemplaza /ruta/a/libtorch con tu ruta real
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ylnner/Documents/omnet++/omnetpp-6.3.0/lib:../../src/florasat

echo "====================================================="
echo "Iniciando simulacion Dataset Training Generation SFTP"
echo "====================================================="
	       
opp_run -m -f "60_devices/Test_SFTP_DS_Generation_3_train.ini" -u Cmdenv \
-n "../../src:..:../../../../samples/inet4.5_2/src:../../../../samples/inet4.5_2/examples:../../../../samples/inet4.5_2/tutorials:../../../../samples/inet4.5_2/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="../../images:../../../../samples/inet4.5_2/images" \
-l "../../src/florasat" \
-l "../../../../samples/inet4.5_2/src/INET"


echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion Dataset Testing Generation SFTP"
echo "====================================================="

opp_run -m -f "60_devices/Test_SFTP_DS_Generation_3_test.ini" -u Cmdenv \
-n "../../src:..:../../../../samples/inet4.5_2/src:../../../../samples/inet4.5_2/examples:../../../../samples/inet4.5_2/tutorials:../../../../samples/inet4.5_2/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="../../images:../../../../samples/inet4.5_2/images" \
-l "../../src/florasat" \
-l "../../../../samples/inet4.5_2/src/INET"


echo "Todas las simulaciones han terminado."


echo "====================================================="
echo "Iniciando simulacion Dataset Testing Generation SFTP"
echo "====================================================="

opp_run -m -f "60_devices/Test_SFTP_DS_Generation_3_val.ini" -u Cmdenv \
-n "../../src:..:../../../../samples/inet4.5_2/src:../../../../samples/inet4.5_2/examples:../../../../samples/inet4.5_2/tutorials:../../../../samples/inet4.5_2/showcases" \
-x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
--image-path="../../images:../../../../samples/inet4.5_2/images" \
-l "../../src/florasat" \
-l "../../../../samples/inet4.5_2/src/INET"


echo "Todas las simulaciones han terminado."

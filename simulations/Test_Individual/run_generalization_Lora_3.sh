#!/bin/bash

#RAAN_VALUES=(210 200 190 180 170)
cd /home/ylnner/Documents/omnet++/omnetpp-6.3.0/projects/florasat_v4_ult_routing/simulations/Test_Individual/
pwd

RAAN_VALUES=(140 150 160 170 180 190)


# 1. Calcular el total de elementos y el 80%
TOTAL_ELEMENTS=${#RAAN_VALUES[@]}
NUM_SELECTED=$(( (TOTAL_ELEMENTS * 80) / 100 ))

# 2. Mezclar aleatoriamente los elementos del arreglo
SHUFFLED_RAAN=($(shuf -e "${RAAN_VALUES[@]}"))

# 3. Dividir el arreglo en dos grupos (80% y el resto)
GROUP_80=("${SHUFFLED_RAAN[@]:0:$NUM_SELECTED}")
GROUP_20=("${SHUFFLED_RAAN[@]:$NUM_SELECTED}")

echo "====================================================="
echo "Total de elementos RAAN: $TOTAL_ELEMENTS"
echo "Cantidad seleccionada (80%): $NUM_SELECTED"
echo "Grupo 80% (Aleatorio): ${GROUP_80[@]}"
echo "Grupo Restante: ${GROUP_20[@]}"
echo "====================================================="


FORMULA_SF_TRAIN="9 + 2 * intuniform(0, 1)"
FORMULA_TP_TRAIN="11 + 2 * intuniform(0, 1)"


for raan in "${GROUP_80[@]}"
do
    echo "====================================================="
    echo "Iniciando simulacion (Grupo 80%) con raan = $raan"
    echo "====================================================="
    opp_run -m -f "Test_Generalization_LoRa_pass_3.ini" -u Cmdenv \
    -n "../../src:..:../../../../samples/inet4.5/src:../../../../samples/inet4.5/examples:../../../../samples/inet4.5/tutorials:../../../../samples/inet4.5/showcases" \
    -x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
    --image-path="../../images:../../../../samples/inet4.5/images" \
    -l "../../src/florasat" \
    -l "../../../../samples/inet4.5/src/INET" \
    "--*.satellite[0].NoradModule.raan=$raan" \
    "--*.stats_raan=$raan" \
    "--*.stats_altitude=740" \
    "--*.train_or_test=\"train\"" \
    "--*.terminal[*].dlk.mac.LoRaSF=$FORMULA_SF_TRAIN" \
    "--*.terminal[*].dlk.mac.LoRaTP=$FORMULA_TP_TRAIN"    
done


FORMULA_SF_TEST="10 + 2 * intuniform(0, 1)"
FORMULA_TP_TEST="12 + 2 * intuniform(0, 1)"

for raan in "${GROUP_20[@]}"
do
    echo "====================================================="
    echo "Iniciando simulacion (Grupo Restante) con raan = $raan"
    echo "====================================================="

    opp_run -m -f "Test_Generalization_LoRa_pass_3.ini" -u Cmdenv \
    -n "../../src:..:../../../../samples/inet4.5/src:../../../../samples/inet4.5/examples:../../../../samples/inet4.5/tutorials:../../../../samples/inet4.5/showcases" \
    -x "inet.emulation;inet.showcases.visualizer.osg;inet.showcases.emulation;inet.clock.common;inet.clock.model;inet.visualizer.osg;inet.examples.voipstream;inet.clock.oscillator;inet.examples.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.clock.base;inet.examples.clock;inet.transportlayer.tcp_nsc" \
    --image-path="../../images:../../../../samples/inet4.5/images" \
    -l "../../src/florasat" \
    -l "../../../../samples/inet4.5/src/INET" \
    "--*.satellite[0].NoradModule.raan=$raan" \
    "--*.stats_raan=$raan" \
    "--*.stats_altitude=740" \
    "--*.train_or_test=\"test\"" \
    "--*.terminal[*].dlk.mac.LoRaSF=$FORMULA_SF_TEST" \
    "--*.terminal[*].dlk.mac.LoRaTP=$FORMULA_TP_TEST"    
    
done


echo "Todas las simulaciones han terminado."




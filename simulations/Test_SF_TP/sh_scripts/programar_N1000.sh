#!/bin/bash
# Espera a que termine la fase A de Claude (PID 711369) y lanza all_1000_dataset.sh (JOBS=12, sin .out).
# Solo guarda las lineas de error en errores_N1000.log; inicio, fin y codigo de salida en estado_N1000.log.
cd "$(dirname "$0")"
tail --pid=711369 -f /dev/null
sleep 60
source /home/ylnner/Documents/omnet++/omnetpp-6.3.0/setenv -q
export MAKEFLAGS=-k   # si una corrida falla, make sigue con las demas del mismo .ini
if ! command -v opp_runall >/dev/null; then
    echo "ERROR: opp_runall no encontrado tras source setenv ($(date))" > errores_N1000.log
    exit 1
fi
echo "inicio $(date)" > estado_N1000.log
./all_1000_dataset.sh 2>&1 | grep -E --line-buffered "<!>|[Ee]rror|Abort|Segmentation|Killed|not found|No such file|No se pudo|Sin mezcla" > errores_N1000.log
echo "fin $(date), codigo de salida ${PIPESTATUS[0]}" >> estado_N1000.log

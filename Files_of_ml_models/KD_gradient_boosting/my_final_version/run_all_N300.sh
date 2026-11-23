#!/bin/bash
# Pipeline completo v3 para N = 300 (solo 100 %). Ejecutar desde my_final_version/.
# v3 sin raan: el estudiante y el maestro usan 7 entradas (STUDENT_INPUTS de scripts/funcs_create_dataset.py).
# Antes: poner en config.py DATA_DIR la carpeta del dataset nuevo (ttxIdle 18-23 s) y borrar cache/ y results/
# si vienen de otro dataset (los pasos se saltan lo que ya existe).
set -e

echo "Dataset (y', grafo de captura, solo filas visibles)..."
python prepare_data.py --workers 10 --N 300

echo "Maestro N = 300..."
python train_teacher.py --workers 10 --N 300

echo "Estudiante N = 300 (noKD y KD, perdida secuencial)..."
python train_student.py --N 300 --workers 20

python select_best_models.py 300
python export_best_models.py 300

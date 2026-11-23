#!/bin/bash
set -e
export KD_ONLY_VISIBLE=0
bash all_train_master.sh             # maestros 

#python train_student.py --N 100 200 300       # y 100, 200, 300
for N in 300 200 100 60; do
    echo "Estudiante N = $N..."
    python train_student.py --N $N
done

python select_by_unique.py 60 100 200 300
python select_best_models.py 60 100 200 300 
python export_best_models.py 60 100 200 300	

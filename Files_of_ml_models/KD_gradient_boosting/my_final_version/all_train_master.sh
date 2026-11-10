#!/bin/bash

echo "Training Teacher N = 60..."
python train_teacher.py --workers 20 --N 60

echo "Training Teacher N = 100..."
python train_teacher.py --workers 20 --N 100

echo "Training Teacher N = 200..."
python train_teacher.py --workers 20 --N 200

echo "Training Teacher N = 300..."
python train_teacher.py --workers 20 --N 300

echo "Training Teacher N = 400..."
python train_teacher.py --workers 20 --N 400

echo "Training Teacher N = 500..."
python train_teacher.py --workers 20 --N 500

echo "All training teacher are finished."
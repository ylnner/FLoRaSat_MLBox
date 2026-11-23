#!/bin/bash

echo "Starting all_closed_loop_baselines..."
#./all_closed_loop_baselines.sh

echo "Starting all_closed_loop_KD..."
./all_closed_loop_KD.sh

echo "Starting all_closed_loop_NoKD ..."
./all_closed_loop_NoKD.sh

echo "All experiments finished."

#!/bin/bash

echo "Starting all_closed_loop_baselines..."
./all_closed_loop_baselines_N500.sh

echo "Starting all_closed_loop_KD..."
./all_closed_loop_KD_N500.sh

echo "Starting all_closed_loop_NoKD ..."
./all_closed_loop_NoKD_N500.sh

echo "All experiments finished."

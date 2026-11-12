#!/bin/bash

echo "Starting Main N=60..."
./all_closed_loop_N_60_parallel.sh

echo "Starting Main N=100 ..."
./all_closed_loop_N_100_parallel.sh

echo "Starting Main N=200 ..."
./all_closed_loop_N_200_parallel.sh

#echo "Starting Main N=300 ..."
#./all_closed_loop_N_300_parallel.sh

echo "All experiments finished."

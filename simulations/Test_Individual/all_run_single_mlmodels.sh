#!/bin/bash

echo "Starting ML Transformer..."
./run_mlmodels_single.sh

echo "Starting ML BiLSTM ..."
./run_mlmodels_single_bilstm.sh

echo "Starting Analytical ..."
./run_mlmodels_single_analytical.sh

echo "Starting noML..."
./run_mlmodels_single_noml.sh

echo "All experiments finished."

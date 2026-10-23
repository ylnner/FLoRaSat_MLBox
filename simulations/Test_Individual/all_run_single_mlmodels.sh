#!/bin/bash

#echo "Starting ML Transformer..."
#./run_mlmodels_single.sh

echo "Starting ML BiLSTM ..."
./run_mlmodels_single_bilstm.sh

#echo "Starting Analytical ..."
#./run_mlmodels_single_analytical.sh

#echo "Starting noML..."
#./run_mlmodels_single_noml.sh

#echo "Starting noMLN1to5..."
#./run_mlmodels_single_noml_N1to5.sh

echo "All experiments finished."

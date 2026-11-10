#!/bin/bash

echo "Starting Main Scenario 1..."
./N_200/sftp_dataset_generatio_1.sh

echo "Starting Main Scenario 2 ..."
./N_200/sftp_dataset_generatio_2.sh

echo "Starting Main Scenario 3 ..."
./N_200/sftp_dataset_generatio_3.sh

echo "Starting Main Scenario 4 ..."
./N_200/sftp_dataset_generatio_4.sh

echo "All experiments finished."

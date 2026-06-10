import pandas as pd
import numpy as np
import os
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, roc_auc_score

import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split
import math
import torch.optim as optim
import itertools
import time
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score

import LoraCollisionLSTM

def load_and_prepare_data(seq_length=16, global_path_train='', global_path_test='', global_path_val=''):    
    print("Loading dataset...")

    df_train = pd.read_csv(global_path_train)
    df_test  = pd.read_csv(global_path_test)
    df_val   = pd.read_csv(global_path_val)

    # Selecting continuous features
    continuous_features = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']
    
    X_train_features = df_train[continuous_features].values
    #y_train          = df_train['rcvOk'].values

    X_test_features = df_test[continuous_features].values
    #y_test          = df_test['rcvOk'].values

    X_val_features = df_val[continuous_features].values
    


    scaler = StandardScaler()
    scaler.fit(np.concatenate((X_train_features, X_test_features, X_val_features), axis=0))

    #X_train = scaler.transform(X_train_features)
    #X_test  = scaler.transform(X_test_features)

    def create_sequences_data(df, scaler):
        cols_to_drop = ['dstId', 'srcSat', 'dstSat', 'loraCF', 'loraBW', 'loraCR', 'satId', 'srcId']
        df           = df.drop(columns=cols_to_drop)

        # Sort by id_simulaiton and time
        df = df.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)
        
        # Apply StandardScaler         
        df[continuous_features] = scaler.transform(df[continuous_features])

        # Process data
        seq_X   = []
        label_y = []
        for sim_id, group_df in df.groupby('id_simulation'):
            num_features_array  = group_df[continuous_features].values
            ###cat_features_array  = group_df['srcId'].values
            time_array          = group_df['time'].values
            target_array        = group_df['rcvOk'].values
            num_packets         = len(group_df)
            
            # Discard if the simulation has less than SEQ_LENGHTS
            if num_packets < seq_length:
                continue

            # Create sliding windows - Ventanas deslizantes
            for i in range(num_packets - seq_length + 1):
                # Select the window        
                window_num      = num_features_array[i : i + seq_length]
                ###window_cat      = cat_features_array[i : i + SEQ_LENGTH].reshape(-1, 1)
                window_times    = time_array[i : i + seq_length]
                window_targets  = target_array[i : i + seq_length]
                
                # El paquete central a predecir es el último de la ventana
                # The prediction should be the last elemento of the window
                target_idx      = seq_length - 1
                label_target    = window_targets[target_idx]
                time_target     = window_times[target_idx]
                            
                # Si un paquete ocurrió en el mismo segundo que el objetivo, delta_t = 0        
                delta_t = (window_times - time_target).reshape(-1, 1)
                #delta_t =  time_target - window_times
                #print('delta_t: ', delta_t)
                
                #window_X = np.hstack((window_num, window_cat, delta_t))
                window_X = np.hstack((window_num, delta_t))

                seq_X.append(window_X)
                label_y.append(label_target)
                    
        X = torch.tensor(np.array(seq_X), dtype=torch.float32)
        y = torch.tensor(np.array(label_y), dtype=torch.float32)

        return X, y
    

    X_train, y_train = create_sequences_data(df_train, scaler)
    X_test , y_test  = create_sequences_data(df_test, scaler)
    X_val  , y_val   = create_sequences_data(df_val, scaler)


    return X_train, y_train, X_test, y_test, X_val  , y_val, scaler



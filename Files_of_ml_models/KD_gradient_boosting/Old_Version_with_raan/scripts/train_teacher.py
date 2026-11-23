#from funcs_teacher_train import train_single_config
import numpy as np
import pandas as pd
import os
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split

import funcs_teacher_train
import funcs_create_dataset
import func_student_train
import itertools
import time
import gc
import joblib

from sklearn.preprocessing import StandardScaler
from TeacherCollisionPredictor import TeacherCollisionPredictor
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score

from StudentModel import StudentModel
from global_funcs import parse_data

NUM_SEEDS = 1 #10

def compute_link_ok(own_tx_raw):
    ''' 
    Se revisa  link_ok = P_rx_own_est !=999, este valor -999 se usa como sentinel para especificar que 
    la transmision no es posible porque el satelite esta muy lejos o no es posible. Se tiene que hacer antes de
    Scaler porque se vuelve indistinguible y esta mala informacion se propagara.
    
    Es true cuando el  P_rx_own_est  es diferente a -999 (SENTINEL)
    Es false todo lo contrario. 
    '''
    return (own_tx_raw[:, 1] != -999)

def load_and_prepare_dataset(base_path):
    dict_datasets_train = {}
    dict_datasets_test  = {}
    dict_datasets_val   = {}
    #num_seeds           = 10

    for seed in range(NUM_SEEDS):
        print(f"-------> Loading Dataset for Seed {seed+1}/{NUM_SEEDS} ---")

        # Create names of files
        path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
        path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
        path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

        CAUSAL = False
        ds_train = funcs_create_dataset.build_dataset(path_train, mean_interarrival_s = 50, n_max = 15, causal= CAUSAL)
        ds_test  = funcs_create_dataset.build_dataset(path_test, mean_interarrival_s = 50, n_max = 15, causal= CAUSAL)
        ds_val   = funcs_create_dataset.build_dataset(path_val, mean_interarrival_s = 50, n_max = 15, causal= CAUSAL)

        #print('ds_test: ', type(ds_test.keys))
        #print(ds_test['seq_feats'])

        train_raw   = parse_data(ds_train) # parse data returns the same dict only with specific types
        test_raw    = parse_data(ds_test)
        val_raw     = parse_data(ds_val)
        
        #print('val_raw: ', val_raw.keys(), )
        #print('own_tx: ', val_raw['own_tx'].shape, val_raw['own_tx'])
        #print(val_raw)
        #input("Presiona Enter para continuar...")
        #'''
        for split_name, split_raw in [("train", train_raw), ("val", val_raw), ("test", test_raw)]:
            ###print('own_tx:', split_raw["own_tx"].shape, split_raw["own_tx"])
            split_raw["link_ok"] = compute_link_ok(split_raw["own_tx"])
            ###print('link_ok:', split_raw["link_ok"].shape, split_raw["link_ok"])
            print(f"  {split_name}/link_ok=True: {split_raw['link_ok'].mean()*100:.1f}%")
            #input('Press to continue...')
        #'''

        dict_datasets_train[seed] = train_raw
        dict_datasets_test[seed]  = test_raw
        dict_datasets_val[seed]   = val_raw


    return dict_datasets_train, dict_datasets_test, dict_datasets_val








if __name__ == "__main__":

    base_path = '/home/ylnner/Documents/KD_new_model/data/repetitions/'
    dict_datasets_train, dict_datasets_test, dict_datasets_val = load_and_prepare_dataset(base_path)

    ######################################################
    ### TRAINING TEACHER MODEL
    ######################################################
    epochs = 20 # 20
    device = 'cpu'
    print("-------> TRAINING THE TEACHER MODEL WITH FINE-TUNING  ---")
    best_config = funcs_teacher_train.run_full_train(epochs, dict_datasets_train, dict_datasets_test, dict_datasets_val, device)

    print("-------> BEST HYPERPARAMETERS FOUNDED IN VALIDATION, TRAINING AGAIN AND TESTING IN TEST SET ---")    
    out_dir = '/home/ylnner/Documents/KD_new_model/best_models'
    os.makedirs(out_dir, exist_ok=True)
    # Devuelve tambien los dataloaders de train SIN shuffle construidos con los mismos datos escalados
    # con los que se entreno cada maestro: el logit i corresponde a la muestra i de dict_datasets_train[seed].
    models_by_seed, dict_train_dataloader_distillation = funcs_teacher_train.train_and_test_with_best_config(epochs, out_dir, best_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device)


    ######################################################
    ### DISTILLING LOGITS FOR THE BEST CONFIG BY SEED
    ######################################################
    print("-------> DISTILLING KNOWLEDGE OF TEACHERS MODELS (ONE MODEL BY SEED)  ---")
    # calculate logits of master for destillation. No shuffle in dataloader
    dict_master_logits = {}
    for seed in range(NUM_SEEDS):
        teacher_model    = models_by_seed[seed]
        train_dataloader = dict_train_dataloader_distillation[seed]
        with torch.no_grad():
            teacher_model.eval()
            model_logits = []
            for batch in train_dataloader:
                batch = funcs_teacher_train.move_batch_to_device(batch, device)
                logits = teacher_model(batch)
                model_logits.append(logits.cpu())
            dict_master_logits[seed] = torch.cat(model_logits, dim = 0)
            print('Master logits for seed: ', seed, dict_master_logits[seed].shape)

    ######################################################
    ### TRAINING STUDENT MODEL with KD
    ######################################################
    small_config = {
        'batch_size': best_config['batch_size'], 
        'lr': best_config['lr'], 
        # pos_weight se calcula dentro de func_student_train a partir del train real
    }
    print("-------> TRAINING STUDENT MODEL WITH KD, SEARCH FOR BEST ALPHA AND TEMPERATURE AND VALIDATE ON VALIDATION SET ---")
    best_alpha, best_temperature, best_gs_loss = func_student_train.run_full_train_student_kd(epochs, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device, dict_master_logits)

    print("-------> TRAINING THE STUDENT MODEL WITH KD WITH THE BEST ALPHA AND TEMPERATURE, USE TEST SET TO TESTING  ---")
    func_student_train.train_and_test_with_best_config(epochs, out_dir, best_alpha, best_temperature, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device, dict_master_logits)


    ######################################################
    ### TRAINING STUDENT MODEL without KD
    ######################################################
    print("-------> TRAINING THE STUDENT MODEL WITHOUT KD. NO SEARCH OF HYPERPARAMETERS, IMMEDIATELY USE TEST SET TO TESTING  ---")
    func_student_train.run_full_train_student_no_kd(epochs, out_dir, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device)

    print("-------> FINISHED, LOOK CSV FILES!!! ---")



    
        

        
        


import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
import numpy as np
from sklearn.utils.class_weight import compute_class_weight
from sklearn.metrics import log_loss, roc_auc_score, f1_score
from itertools import product
import os

from sklearn.metrics import (roc_auc_score, precision_recall_curve, 
                             accuracy_score, precision_score, recall_score, f1_score)



def grid_search(df_val, grid_params):    
    print('grid_params: ', grid_params)
    #### Iterar sobre time e id_simulation
    ###df_train = df_train.sort_values('time').reset_index(drop=True)
    
    # Sort by id_simulaiton and time    
    df_val_aux = df_val.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)


    BW = 125000               
    base_p_link = df_val_aux['minPowerdBm'] - df_val_aux['sensitivitydBm']
        
    BW_SF = BW / (2 ** df_val_aux['loraSF'])
    base_p_doppler = np.abs(df_val_aux['doppler']) / BW_SF
        
    ToA = df_val_aux['duration']
    y_true = df_val_aux['rcvOk'].values
    
    best_loss = float('inf')
    best_params = None

    for T_v in grid_params['T_window']:
        lambda_trx_list = []
                
        for sim_id, group in df_val_aux.groupby('id_simulation', sort=True):
            times = group['time'].values
            # Get limits (left, right) of each row accordding to the T_v/2
            lim_left = times - (T_v / 2)
            lim_right = times + (T_v / 2)

            # Get idxs of the previous calculated limits 
            idx_left = np.searchsorted(times, lim_left, side='left')
            idx_right = np.searchsorted(times, lim_right, side='right')

            # Calculate the number of trxs/registers between the idxs
            num_trx_window = np.maximum((idx_right - idx_left) - 1, 0)        
            lambda_trx = num_trx_window / T_v
            lambda_trx_list.append(lambda_trx)




        # Unimos las densidades calculadas (mantienen el mismo orden que el dataframe)
        # Concatenate traffic density , they kep the original order after the sort by id_simulation, time
        lambda_traffic = np.concatenate(lambda_trx_list)

        P_collision = np.exp(-2 * lambda_traffic * ToA)        
        #print('before second for')
        for alpha, k in product(grid_params['alpha'], grid_params['k']):            
            # Calculate p_doppler
            P_doppler = np.exp(-alpha * base_p_doppler)
            
            # Calculate p_link
            P_link = 1 / (1 + np.exp(-k * base_p_link))
            
            # Final probability
            y_pred = P_link * P_doppler * P_collision
            y_pred_clipped = np.clip(y_pred, 1e-15, 1 - 1e-15)
            
            loss = log_loss(y_true, y_pred_clipped)
            if loss < best_loss:
                best_loss = loss
                best_params = {'T_window': T_v, 'alpha': alpha, 'k': k}

    return best_params, best_loss

def evaluate_test(df_test, params):
    #### Iterar sobre time e id_simulation
    #####df_test = df_test.sort_values('time').reset_index(drop=True)
    # Sort by id_simulaiton and time    
    df_test_aux = df_test.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)

    BW = 125000                           
    BW_SF = BW / (2 ** df_test_aux['loraSF'])
    
    # P_link    
    base_p_link = df_test_aux['minPowerdBm'] - df_test_aux['sensitivitydBm']
    P_link = 1 / (1 + np.exp(-params['k'] * base_p_link))
    
    # P_doppler
    base_p_doppler = np.abs(df_test_aux['doppler']) / BW_SF
    P_doppler = np.exp(-params['alpha'] * base_p_doppler)
    
    # P_collision    
    T_v = params['T_window']
    ToA = df_test_aux['duration']
    lambda_trx_list = []
    
    # Iteramos sobre cada simulación para que no haya "colisiones fantasma" entre ellas
    for sim_id, group in df_test_aux.groupby('id_simulation', sort=True):
        times = group['time'].values
        # Get limits (left, right) of each row accordding to the T_v/2
        lim_left = times - (T_v / 2)
        lim_right = times + (T_v / 2)

        # Get idxs of the previous calculated limits 
        idx_left = np.searchsorted(times, lim_left, side='left')
        idx_right = np.searchsorted(times, lim_right, side='right')

        # Calculate the number of trxs/registers between the idxs
        num_trx_window = np.maximum((idx_right - idx_left) - 1, 0)        
        lambda_trx = num_trx_window / T_v
        lambda_trx_list.append(lambda_trx)
    
    # Concatenate traffic density , they kep the original order after the sort by id_simulation, time
    lambda_traffic = np.concatenate(lambda_trx_list)
    P_collision = np.exp(-2 * lambda_traffic * ToA)    
    
    # Predicción final
    df_test_aux['P_RcvOk_Pred'] = P_link * P_doppler * P_collision
    
    y_true = df_test_aux['rcvOk'].values
    y_pred = df_test_aux['P_RcvOk_Pred'].values
    
    loss = log_loss(y_true, np.clip(y_pred, 1e-15, 1 - 1e-15))
    auc = roc_auc_score(y_true, y_pred)
    f1 = f1_score(y_true, (y_pred > 0.5).astype(int))
    
    return loss, auc, f1, y_true, y_pred

# =====================================================
# GRID SEARCH
# ======================================================
grid_params = {
    'T_window': [1, 3, 5, 10, 20],
    'k': [0.5, 1, 2, 3, 4, 5],    
    'alpha': [0.0001, 0.001, 0.01, 0.1, 1] 
}

base_path = 'data/repetitions/'
num_seeds = 10
features = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

results_list = []
metrics_history_best_threshold = []
metrics_history_default_threshold = []
for seed in range(num_seeds):
    print(f"--- Procesando Seed {seed} ---")
    
    # Construcción dinámica de nombres de archivos
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    # Lectura de datos
    df_train = pd.read_csv(path_train)
    df_test  = pd.read_csv(path_test)
    df_val   = pd.read_csv(path_val)

    if ' minPowerdBm' in df_train.columns:
        df_train.rename(columns={' minPowerdBm': 'minPowerdBm'}, inplace=True)
        df_test.rename(columns={' minPowerdBm': 'minPowerdBm'}, inplace=True)
        df_val.rename(columns={' minPowerdBm': 'minPowerdBm'}, inplace=True)

    print("GridSearch...")
    best_params, min_loss_train = grid_search(df_val, grid_params)
    print(f"-> Best params: {best_params}")

    print("Calculating best threshold on validation set...")
    _, _, _, y_true_val, y_pred_val = evaluate_test(df_val, best_params)
     # Find the threshold
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_pred_val)

    print("*"*50)
    print("Validation set stats:")
    print(df_val['rcvOk'].mean())   # tasa real de éxito
    print(y_pred_val.mean())        # probabilidad media predicha
    print(y_pred_val.min(), y_pred_val.max())  # rango real de predicciones
    print("*"*50)

    # Calculate F1Score by threshold in validation set
    f1_scores_by_threshold = 2 * (precisions[:-1] * recalls[:-1]) / (precisions[:-1] + recalls[:-1] + 1e-10)
    
    # Get best threshold    
    best_idx = np.argmax(f1_scores_by_threshold)
    best_threshold = thresholds[best_idx]
    print(f"-> Best threshold in validaiton set: {best_threshold:.6f}")

    print("Evaluating test set with best params...")
    loss_test, auc_test, _, y_true_test, y_pred_test = evaluate_test(df_test, best_params)
    
    # Apply best threshorld calculated in validation sset
    y_pred_test_bin = (y_pred_test > best_threshold).astype(int)

    acc_cal = accuracy_score(y_true_test, y_pred_test_bin)
    prec_cal = precision_score(y_true_test, y_pred_test_bin, zero_division=0)
    rec_cal = recall_score(y_true_test, y_pred_test_bin, zero_division=0)
    f1_cal = f1_score(y_true_test, y_pred_test_bin, zero_division=0)

    metrics_history_best_threshold.append({
        'seed': seed,
        'best_threshold': best_threshold,
        'log_loss': loss_test,
        'auc_roc': auc_test,
        'accuracy': acc_cal,
        'precision': prec_cal,
        'recall': rec_cal,
        'f1_score': f1_cal
    })

    # Apply default threshorld (0.5) calculated in validation sset
    y_pred_test_bin = (y_pred_test > 0.5).astype(int)

    acc_cal = accuracy_score(y_true_test, y_pred_test_bin)
    prec_cal = precision_score(y_true_test, y_pred_test_bin, zero_division=0)
    rec_cal = recall_score(y_true_test, y_pred_test_bin, zero_division=0)
    f1_cal = f1_score(y_true_test, y_pred_test_bin, zero_division=0)

    metrics_history_default_threshold.append({
        'seed': seed,
        'default_threshold': 0.5,
        'log_loss': loss_test,
        'auc_roc': auc_test,
        'accuracy': acc_cal,
        'precision': prec_cal,
        'recall': rec_cal,
        'f1_score': f1_cal
    })


# Crear DataFrame final y guardar
df_results = pd.DataFrame(metrics_history_best_threshold)
df_results.to_csv('baselines_math_best_threshold_results_v3.csv', index=False)

df_results = pd.DataFrame(metrics_history_default_threshold)
df_results.to_csv('baselines_math_default_threshold_results_v3.csv', index=False)

print("Finish running, results saved in 'baselines_math_best_threshold_results_v3.csv'. and 'baselines_math_default_threshold_results_v3.csv'.")
#print(df_results.head())
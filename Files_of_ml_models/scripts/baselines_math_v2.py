import pandas as pd
import numpy as np
import os
from itertools import product
from sklearn.metrics import (log_loss, roc_auc_score, precision_recall_curve, auc,
                             accuracy_score, precision_score, recall_score, f1_score)

def grid_search(df_val, grid_params):    
    print('grid_params: ', grid_params)
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
            lim_left = times - (T_v / 2)
            lim_right = times + (T_v / 2)
            #lim_left = times - T_v

            idx_left = np.searchsorted(times, lim_left, side='left')
            idx_right = np.searchsorted(times, lim_right, side='right')
            #idx_right = np.searchsorted(times, times, side='left')

            num_trx_window = np.maximum((idx_right - idx_left) - 1, 0)
            #num_trx_window = np.maximum((idx_right - idx_left), 0)

            lambda_trx = num_trx_window / T_v
            lambda_trx_list.append(lambda_trx)

        lambda_traffic = np.concatenate(lambda_trx_list)
        P_collision = np.exp(-2 * lambda_traffic * ToA)        
        
        for alpha, k in product(grid_params['alpha'], grid_params['k']):            
            P_doppler = np.exp(-alpha * base_p_doppler)
            P_link = 1 / (1 + np.exp(-k * base_p_link))
            
            y_pred = P_link * P_doppler * P_collision
            y_pred_clipped = np.clip(y_pred, 1e-15, 1 - 1e-15)
            
            loss = log_loss(y_true, y_pred_clipped)
            if loss < best_loss:
                best_loss = loss
                best_params = {'T_window': T_v, 'alpha': alpha, 'k': k}

    return best_params, best_loss

def evaluate_test(df_test, params):
    df_test_aux = df_test.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)

    BW = 125000                           
    BW_SF = BW / (2 ** df_test_aux['loraSF'])
    
    base_p_link = df_test_aux['minPowerdBm'] - df_test_aux['sensitivitydBm']
    P_link = 1 / (1 + np.exp(-params['k'] * base_p_link))
    
    base_p_doppler = np.abs(df_test_aux['doppler']) / BW_SF
    P_doppler = np.exp(-params['alpha'] * base_p_doppler)
    
    T_v = params['T_window']
    ToA = df_test_aux['duration']
    lambda_trx_list = []
    
    for sim_id, group in df_test_aux.groupby('id_simulation', sort=True):
        times = group['time'].values
        lim_left = times - (T_v / 2)
        lim_right = times + (T_v / 2)
        #lim_left = times - T_v

        idx_left = np.searchsorted(times, lim_left, side='left')
        idx_right = np.searchsorted(times, lim_right, side='right')
        #idx_right = np.searchsorted(times, times, side='left')

        num_trx_window = np.maximum((idx_right - idx_left) - 1, 0)
        #num_trx_window = np.maximum((idx_right - idx_left), 0)

        lambda_trx = num_trx_window / T_v
        lambda_trx_list.append(lambda_trx)
    
    lambda_traffic = np.concatenate(lambda_trx_list)
    P_collision = np.exp(-2 * lambda_traffic * ToA)    
    
    df_test_aux['P_RcvOk_Pred'] = P_link * P_doppler * P_collision
    
    y_true = df_test_aux['rcvOk'].values
    y_pred = df_test_aux['P_RcvOk_Pred'].values
    
    loss = log_loss(y_true, np.clip(y_pred, 1e-15, 1 - 1e-15))
    auc = roc_auc_score(y_true, y_pred)
    
    # Nota: Removí el cálculo del F1 interno aquí porque ahora se calcula afuera por clases de forma exhaustiva
    return loss, auc, y_true, y_pred

# =====================================================
# GRID SEARCH & EVALUATION
# ======================================================
#grid_params = {
#    'T_window': [1, 3, 5, 10, 20, 30, 40, 50, 60],
#    'k': [0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],    
#    'alpha': [0.000001, 0.0001, 0.0002, 0.0003, 0.0004, 0.001, 0.01, 0.1, 1] 
#}

grid_params = {
    # Ventanas más cortas y dinámicas (un minuto es una eternidad para colisiones de milisegundos)
    'T_window': [0.5, 1, 2, 5, 10, 15, 20, 30],
    
    # Permitir una pendiente de enlace más suave o agresiva si es necesario
    'k': [0.1, 0.5, 1, 2, 5, 10, 15, 20],    
    
    # Permitir que alpha sea aún más pequeño (o cero) para ver si realmente aporta
    'alpha': [0.0, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 0.1, 1.0] 
}

base_path = 'data/repetitions/'
num_seeds = 10
features = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

metrics_history_best_threshold = []
metrics_history_default_threshold = []

for seed in range(num_seeds):
    print(f"--- Procesando Seed {seed} ---")
    
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

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

    with open("best_params_math_v2.txt", "w") as f:
        f.write(str(best_params))

    print("Calculating best threshold on validation set...")
    _, _, y_true_val, y_pred_val = evaluate_test(df_val, best_params)
    
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_pred_val)

    # Calcular F1Score por umbral en validación
    f1_scores_by_threshold = 2 * (precisions[:-1] * recalls[:-1]) / (precisions[:-1] + recalls[:-1] + 1e-10)
    best_idx = np.argmax(f1_scores_by_threshold)
    best_threshold = thresholds[best_idx]
    print(f"-> Best threshold in validation set: {best_threshold:.6f}")

    print("Evaluating test set with best params...")
    loss_test, auc_roc_test, y_true_test, y_pred_test = evaluate_test(df_test, best_params)
    
    # Calcular AUC-PR para el test set
    p_curve, r_curve, _ = precision_recall_curve(y_true_test, y_pred_test)
    auc_pr_test = auc(r_curve, p_curve)

    # ─────────────────────────────────────────────────────────────────
    # EV 1: EVALUACIÓN CON EL MEJOR UMBRAL (BEST THRESHOLD)
    # ─────────────────────────────────────────────────────────────────
    y_pred_best_bin = (y_pred_test > best_threshold).astype(int)

    prec_best_per_class = precision_score(y_true_test, y_pred_best_bin, average=None, zero_division=0)
    rec_best_per_class  = recall_score(y_true_test, y_pred_best_bin, average=None, zero_division=0)
    f1_best_per_class   = f1_score(y_true_test, y_pred_best_bin, average=None, zero_division=0)

    metrics_history_best_threshold.append({
        'seed': seed,
        'best_threshold': best_threshold,
        'log_loss': loss_test,
        'auc_roc': auc_roc_test,
        'auc_pr': auc_pr_test,
        'accuracy': accuracy_score(y_true_test, y_pred_best_bin),
        # Clase 0
        'precision_c0': prec_best_per_class[0],
        'recall_c0':    rec_best_per_class[0],
        'f1_score_c0':  f1_best_per_class[0],
        # Clase 1
        'precision_c1': prec_best_per_class[1],
        'recall_c1':    rec_best_per_class[1],
        'f1_score_c1':  f1_best_per_class[1]
    })

    # ─────────────────────────────────────────────────────────────────
    # EV 2: EVALUACIÓN CON UMBRAL POR DEFECTO (0.5)
    # ─────────────────────────────────────────────────────────────────
    y_pred_def_bin = (y_pred_test > 0.5).astype(int)

    prec_def_per_class = precision_score(y_true_test, y_pred_def_bin, average=None, zero_division=0)
    rec_def_per_class  = recall_score(y_true_test, y_pred_def_bin, average=None, zero_division=0)
    f1_def_per_class   = f1_score(y_true_test, y_pred_def_bin, average=None, zero_division=0)

    metrics_history_default_threshold.append({
        'seed': seed,
        'default_threshold': 0.5,
        'log_loss': loss_test,
        'auc_roc': auc_roc_test,
        'auc_pr': auc_pr_test,
        'accuracy': accuracy_score(y_true_test, y_pred_def_bin),
        # Clase 0
        'precision_c0': prec_def_per_class[0],
        'recall_c0':    rec_def_per_class[0],
        'f1_score_c0':  f1_def_per_class[0],
        # Clase 1
        'precision_c1': prec_def_per_class[1],
        'recall_c1':    rec_def_per_class[1],
        'f1_score_c1':  f1_def_per_class[1]
    })

# Guardar DataFrames finales
df_results_best = pd.DataFrame(metrics_history_best_threshold)
df_results_best.to_csv('baselines_math_best_threshold_results_v3_final.csv', index=False)

df_results_def = pd.DataFrame(metrics_history_default_threshold)
df_results_def.to_csv('baselines_math_default_threshold_results_v3_final.csv', index=False)

print("Finish running. Results saved correctly with per-class granular metrics and AUC-PR.")
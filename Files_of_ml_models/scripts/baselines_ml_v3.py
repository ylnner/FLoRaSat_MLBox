import joblib
import pandas as pd
import numpy as np
import os
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import GradientBoostingClassifier, RandomForestClassifier
from sklearn.neural_network import MLPClassifier
# NUEVAS IMPORTACIONES: precision_recall_curve y auc para calcular el AUC-PR
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, roc_auc_score, precision_recall_curve, auc
from sklearn.utils.class_weight import compute_sample_weight 


cols_to_drop = ['dstId', 'srcSat', 'dstSat', 'loraCF', 'loraBW', 'loraCR', 'satId']
base_path   = 'data/repetitions/'
models_path = 'models_window/'          
num_seeds   = 10
features    = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

def compute_delta_t_for_scaling(df, seq_length):
    # Calculate raw entire delta_t to scale them
    
    df = df.drop(columns=cols_to_drop)
    df = df.sort_values(by=['id_simulation', 'srcId', 'time']).reset_index(drop=True)

    all_deltas = []
    for sim_id, group_df in df.groupby(['id_simulation', 'srcId']):
        time_array  = group_df['time'].values
        num_packets = len(group_df)

        if num_packets < seq_length:
            continue

        for i in range(num_packets - seq_length + 1):
            window_times = time_array[i : i + seq_length]
            target_idx   = seq_length - 1
            time_target  = window_times[target_idx]

            delta_t_raw  = (time_target - window_times).reshape(-1, 1)
            all_deltas.extend(delta_t_raw)

    return np.array(all_deltas)#.reshape(-1, 1)

def create_sequences_data(df, scaler, seq_length, scaler_delta_t):    
    df = df.drop(columns=cols_to_drop)

    # Sort by id_simulaiton and time
    #df = df.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)
    df = df.sort_values(by=['id_simulation', 'srcId','time']).reset_index(drop=True)
    
    # Apply StandardScaler         
    df[features] = scaler.transform(df[features])

    # Process data
    seq_X   = []
    label_y = []
    for sim_id, group_df in df.groupby(['id_simulation', 'srcId']):
        num_features_array  = group_df[features].values
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
            
            # The prediction should be the last elemento of the window
            target_idx      = seq_length - 1
            label_target    = window_targets[target_idx]
            time_target     = window_times[target_idx]
            

            # Si un paquete ocurrió en el mismo segundo que el objetivo, delta_t = 0
            # Se hace cambio para adicionar el delta_t como feature, en lugar de solo usar como PE.
            delta_t_raw  = (time_target - window_times).reshape(-1, 1)
            delta_t_norm = scaler_delta_t.transform(delta_t_raw) 

            #window_X = np.hstack((window_num, window_cat, delta_t))
            window_X = np.hstack((window_num, delta_t_norm))
            window_X_flat = window_X.flatten()

            seq_X.append(window_X_flat)
            label_y.append(label_target)


    X = np.array(seq_X, dtype=np.float32)
    y = np.array(label_y, dtype=int)

    return X, y

def load_and_prepare_data(seq_length,global_path_train='', global_path_test='', global_path_val=''):    
    print("Loading dataset...")

    df_train = pd.read_csv(global_path_train)
    df_test  = pd.read_csv(global_path_test)
    df_val   = pd.read_csv(global_path_val)

    X_train_features = df_train[features].values
    X_test_features = df_test[features].values
    X_val_features = df_val[features].values


    scaler = StandardScaler()
    scaler.fit(X_train_features)

    delta_t_train_raw = compute_delta_t_for_scaling(df_train, seq_length)
    scaler_delta_t = StandardScaler()
    scaler_delta_t.fit(delta_t_train_raw)
    
    X_train, y_train = create_sequences_data(df_train, scaler, seq_length, scaler_delta_t)
    X_test , y_test  = create_sequences_data(df_test, scaler, seq_length, scaler_delta_t)
    X_val  , y_val   = create_sequences_data(df_val, scaler, seq_length, scaler_delta_t)


    return X_train, y_train, X_test, y_test, X_val, y_val, scaler, scaler_delta_t

def find_best_threshold(y_true_val, y_prob_val):
    # Find the best threshold that maximize the F1-Score for the possitive class
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_prob_val)
    
    # Calculate F1-Score for each possible threshold
    f1_scores = (2 * precisions * recalls) / (precisions + recalls + 1e-10)
        
    best_idx = np.argmax(f1_scores)        
    best_th = thresholds[best_idx] if best_idx < len(thresholds) else thresholds[-1]
    best_f1 = f1_scores[best_idx]
    
    return best_th, best_f1



results_list = []
results_list_best_th = []

for seed in range(num_seeds):
    print(f"--- Procesando Seed {seed} ---")
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')


    X_train, y_train, X_test, y_test, X_val, y_val, scaler, scaler_delta_t = load_and_prepare_data(seq_length = 8, global_path_train=path_train, global_path_test=path_test, global_path_val=path_val)    
    scaler_path = os.path.join(models_path, f'scaler_seed{seed}.pkl')
    joblib.dump(scaler, scaler_path)

    # Calculamos los pesos de las muestras para pasárselos al Gradient Boosting
    sample_weights_train = compute_sample_weight(class_weight='balanced', y=y_train)

    #balanced_subsample
    models = {
        "Logistic Regression": LogisticRegression(max_iter=1000, class_weight='balanced', random_state=42),
        "Random Forest": RandomForestClassifier(n_estimators=100, class_weight='balanced', random_state=42),
        "MLP": MLPClassifier(hidden_layer_sizes=(64, 32), max_iter=1000, random_state=42),
        "Gradient Boosting": GradientBoostingClassifier(n_estimators=100, random_state=42)
    }

    for name, model in models.items():
        if name == "Gradient Boosting":
            model.fit(X_train, y_train, sample_weight=sample_weights_train)
        else:
            model.fit(X_train, y_train)
            
        y_pred_test = model.predict(X_test)
        y_pred_val  = model.predict(X_val)

        y_prob_test = model.predict_proba(X_test)[:, 1]
        y_prob_val  = model.predict_proba(X_val)[:, 1]

        # Search for the best threshold based on validation set
        best_threshold, best_f1_val = find_best_threshold(y_val, y_prob_val)
        print(f"[{name}] Best threshold: {best_threshold:.4f} (F1-Val estimado: {best_f1_val:.4f})")


        # ── AUC-PR ──
        # Test
        p_test, r_test, _ = precision_recall_curve(y_test, y_prob_test)
        auc_pr_test = auc(r_test, p_test)
        # Validation
        p_val, r_val, _ = precision_recall_curve(y_val, y_prob_val)
        auc_pr_val = auc(r_val, p_val)
        
        # Metrics for class 0 and class 1 separately
        prec_test_per_class = precision_score(y_test, y_pred_test, average=None, zero_division=0)
        rec_test_per_class  = recall_score(y_test, y_pred_test, average=None, zero_division=0)
        f1_test_per_class   = f1_score(y_test, y_pred_test, average=None, zero_division=0)

        prec_val_per_class  = precision_score(y_val, y_pred_val, average=None, zero_division=0)
        rec_val_per_class   = recall_score(y_val, y_pred_val, average=None, zero_division=0)
        f1_val_per_class    = f1_score(y_val, y_pred_val, average=None, zero_division=0)

        metrics = {
            "seed":  seed,
            "model": name,
            "threshold" : 0.5, 
            # Test Metrics
            #"accuracy_test":   accuracy_score(y_test, y_pred_test),
            #"auc_roc_test":    roc_auc_score(y_test, y_prob_test),
            "auc_pr_test":     auc_pr_test,
            
            # Class 0 (Negative)
            "precision_test_c0": prec_test_per_class[0],
            "recall_test_c0":    rec_test_per_class[0],
            "f1_score_test_c0":  f1_test_per_class[0],
            
            # Class 1 (Positive)
            "precision_test_c1": prec_test_per_class[1],
            "recall_test_c1":    rec_test_per_class[1],
            "f1_score_test_c1":  f1_test_per_class[1],

            # Val Metrics
            #"accuracy_val":    accuracy_score(y_val, y_pred_val),
            #"auc_roc_val":     roc_auc_score(y_val, y_prob_val),
            "auc_pr_val":      auc_pr_val,
            
            # Class 0 (Negative)
            "precision_val_c0":  prec_val_per_class[0],
            "recall_val_c0":     rec_val_per_class[0],
            "f1_score_val_c0":   f1_val_per_class[0],
            
            # Class 1 (Positive)
            "precision_val_c1":  prec_val_per_class[1],
            "recall_val_c1":     rec_val_per_class[1],
            "f1_score_val_c1":   f1_val_per_class[1],
        }
        results_list.append(metrics)
        
        # Preds with the best threshold
        y_pred_val_opt  = (y_prob_val >= best_threshold).astype(int)
        y_pred_test_opt = (y_prob_test >= best_threshold).astype(int)

        p_test, r_test, _ = precision_recall_curve(y_test, y_prob_test)
        auc_pr_test = auc(r_test, p_test)
        
        # Metrics for class with the best threshold
        prec_test_per_class = precision_score(y_test, y_pred_test_opt, average=None, zero_division=0)
        rec_test_per_class  = recall_score(y_test, y_pred_test_opt, average=None, zero_division=0)
        f1_test_per_class   = f1_score(y_test, y_pred_test_opt, average=None, zero_division=0)

        prec_val_per_class  = precision_score(y_val, y_pred_val_opt, average=None, zero_division=0)
        rec_val_per_class   = recall_score(y_val, y_pred_val_opt, average=None, zero_division=0)
        f1_val_per_class    = f1_score(y_val, y_pred_val_opt, average=None, zero_division=0)

        metrics = {
            "seed":  seed,
            "model": name,
            "threshold": best_threshold,
            
            # Test Metrics with optimal threshold                     
            "auc_pr_test":     auc_pr_test,
            
            "precision_test_c0": prec_test_per_class[0],
            "recall_test_c0":    rec_test_per_class[0],
            "f1_score_test_c0":  f1_test_per_class[0],
            
            "precision_test_c1": prec_test_per_class[1],
            "recall_test_c1":    rec_test_per_class[1],
            "f1_score_test_c1":  f1_test_per_class[1],
            
            # Val metrics with optimal threshold
            "auc_pr_val":      auc_pr_val,
            
            "precision_val_c0":  prec_val_per_class[0],
            "recall_val_c0":     rec_val_per_class[0],
            "f1_score_val_c0":   f1_val_per_class[0],
            
            "precision_val_c1":  prec_val_per_class[1],
            "recall_val_c1":     rec_val_per_class[1],
            "f1_score_val_c1":   f1_val_per_class[1],
        }
        results_list_best_th.append(metrics)

        model_filename = f"{name.replace(' ', '_')}_seed{seed}.pkl"
        joblib.dump(model, os.path.join(models_path, model_filename))

df_results = pd.DataFrame(results_list)
df_results.to_csv('baselines_ml_results_v4_imbalanced_flatten.csv', index=False)

df_results_best_th = pd.DataFrame(results_list_best_th)
df_results_best_th.to_csv('baselines_ml_results_v4_imbalanced_flatten_best_th.csv', index=False)
 
print("Training complete. Results saved to csv files.")
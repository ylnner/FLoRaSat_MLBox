import joblib
import pandas as pd
import numpy as np
import os
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score


base_path   = 'data/repetitions/'
models_path = 'models/'          # trained models & scalers will be saved here
num_seeds   = 10
features    = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

#os.makedirs(models_path, exist_ok=True)
 
results_list = []

for seed in range(num_seeds):
    print(f"--- Procesando Seed {seed} ---")
    ### Construcción dinámica de nombres de archivos
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    ### Lectura de datos
    df_train = pd.read_csv(path_train)
    df_test  = pd.read_csv(path_test)
    df_val   = pd.read_csv(path_val)

    X_train_raw = df_train[features].values
    y_train     = df_train['rcvOk'].values
    X_test_raw  = df_test[features].values
    y_test      = df_test['rcvOk'].values
    X_val_raw   = df_val[features].values
    y_val       = df_val['rcvOk'].values

    

    # ── Scaler: fit ONLY on train ─────────────────────────────────────────────
    # FIX: previously fit on train+test+val, leaking test/val statistics.
    scaler = StandardScaler()
    scaler.fit(X_train_raw)
 
    X_train = scaler.transform(X_train_raw)
    X_test  = scaler.transform(X_test_raw)
    X_val   = scaler.transform(X_val_raw)


    # Save scaler so evaluation can reuse the exact same transformation
    scaler_path = os.path.join(models_path, f'scaler_seed{seed}.pkl')
    joblib.dump(scaler, scaler_path)
    

    ''' 
    scaler = StandardScaler()
    scaler.fit(np.concatenate((X_train_raw, X_test_raw, X_val_raw), axis=0))
    
    
    X_train = scaler.transform(X_train_raw)
    X_test  = scaler.transform(X_test_raw)
    X_val   = scaler.transform(X_val_raw)
    '''


    models = {
        "Logistic Regression": LogisticRegression(max_iter=1000, random_state=42),
        "Random Forest": RandomForestClassifier(n_estimators=100, random_state=42),
        "MLP": MLPClassifier(hidden_layer_sizes=(64, 32), max_iter=1000, random_state=42)
    }

    for name, model in models.items():
        model.fit(X_train, y_train)
        y_pred_test = model.predict(X_test)
        y_pred_val  = model.predict(X_val)

        # Use predict_proba for AUC-ROC (more informative than hard labels)
        # FIX: previously passed hard predictions instead of probabilities.
        y_prob_test = model.predict_proba(X_test)[:, 1]
        y_prob_val  = model.predict_proba(X_val)[:, 1]


        # ── Cálculo de AUC-PR ──
        # Test
        p_test, r_test, _ = precision_recall_curve(y_test, y_prob_test)
        auc_pr_test = auc(r_test, p_test)
        # Validation
        p_val, r_val, _ = precision_recall_curve(y_val, y_prob_val)
        auc_pr_val = auc(r_val, p_val)

        # ── Métricas por clase (average=None nos da un array: [clase_0, clase_1]) ──
        prec_test_per_class = precision_score(y_test, y_pred_test, average=None, zero_division=0)
        rec_test_per_class  = recall_score(y_test, y_pred_test, average=None, zero_division=0)
        f1_test_per_class   = f1_score(y_test, y_pred_test, average=None, zero_division=0)

        prec_val_per_class  = precision_score(y_val, y_pred_val, average=None, zero_division=0)
        rec_val_per_class   = recall_score(y_val, y_pred_val, average=None, zero_division=0)
        f1_val_per_class    = f1_score(y_val, y_pred_val, average=None, zero_division=0)

        ''' 
        metrics = {
            "seed":  seed,
            "model": name,
            # Test metrics
            "accuracy_test":  accuracy_score(y_test, y_pred_test),
            "precision_test": precision_score(y_test, y_pred_test, zero_division=0),
            "recall_test":    recall_score(y_test, y_pred_test, zero_division=0),
            "f1_score_test":  f1_score(y_test, y_pred_test, zero_division=0),
            "auc_roc_test":   roc_auc_score(y_test, y_prob_test),

            # Validation metrics — used for seed selection, NOT for final reporting
            "accuracy_val":   accuracy_score(y_val, y_pred_val),
            "precision_val":  precision_score(y_val, y_pred_val, zero_division=0),
            "recall_val":     recall_score(y_val, y_pred_val, zero_division=0),
            "f1_score_val":   f1_score(y_val, y_pred_val, zero_division=0),
            "auc_roc_val":    roc_auc_score(y_val, y_prob_val),
        }
        '''
        metrics = {
            "seed":  seed,
            "model": name,
            
            # --- METRICAS TEST ---
            "accuracy_test":   accuracy_score(y_test, y_pred_test),
            "auc_roc_test":    roc_auc_score(y_test, y_prob_test),
            "auc_pr_test":     auc_pr_test,
            
            # Clase 0 (Negativa)
            "precision_test_c0": prec_test_per_class[0],
            "recall_test_c0":    rec_test_per_class[0],
            "f1_score_test_c0":  f1_test_per_class[0],
            
            # Clase 1 (Positiva)
            "precision_test_c1": prec_test_per_class[1],
            "recall_test_c1":    rec_test_per_class[1],
            "f1_score_test_c1":  f1_test_per_class[1],

            # --- METRICAS VALIDACIÓN ---
            "accuracy_val":    accuracy_score(y_val, y_pred_val),
            "auc_roc_val":     roc_auc_score(y_val, y_prob_val),
            "auc_pr_val":      auc_pr_val,
            
            # Clase 0 (Negativa)
            "precision_val_c0":  prec_val_per_class[0],
            "recall_val_c0":     rec_val_per_class[0],
            "f1_score_val_c0":   f1_val_per_class[0],
            
            # Clase 1 (Positiva)
            "precision_val_c1":  prec_val_per_class[1],
            "recall_val_c1":     rec_val_per_class[1],
            "f1_score_val_c1":   f1_val_per_class[1],
        }
        results_list.append(metrics)

        # Save the trained model for this (model, seed) pair
        model_filename = f"{name.replace(' ', '_')}_seed{seed}.pkl"
        joblib.dump(model, os.path.join(models_path, model_filename))


        ''' 
        y_pred = model.predict(X_test)
        
        # Cálculo de métricas
        metrics = {
            "seed": seed,
            "model": name,
            "accuracy": accuracy_score(y_test, y_pred),
            "precision": precision_score(y_test, y_pred, zero_division=0),
            "recall": recall_score(y_test, y_pred, zero_division=0),
            "f1_score": f1_score(y_test, y_pred, zero_division=0),
            "auc_roc": roc_auc_score(y_test, y_pred)
        }
        results_list.append(metrics)
        '''

df_results = pd.DataFrame(results_list)
df_results.to_csv('baselines_ml_results_v3.csv', index=False)
 
print("Training complete. Results saved to 'baselines_ml_results_v3.csv'.")
print("Trained models and scalers saved to 'models/'.")


# Crear DataFrame final y guardar
#df_results = pd.DataFrame(results_list)
#df_results.to_csv('baselines_ml_results.csv', index=False)

print("Finish running, results saved in 'baselines_ml_results_v3.csv'.")
#print(df_results.head())
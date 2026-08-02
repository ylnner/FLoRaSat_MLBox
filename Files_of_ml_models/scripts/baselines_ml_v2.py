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

def find_best_threshold(y_true_val, y_prob_val):
    # Find the best threshold that maximize the F1-Score for the possitive class
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_prob_val)
    
    # Calculate F1-Score for each possible threshold
    f1_scores = (2 * precisions * recalls) / (precisions + recalls + 1e-10)
        
    best_idx = np.argmax(f1_scores)        
    best_th = thresholds[best_idx] if best_idx < len(thresholds) else thresholds[-1]
    best_f1 = f1_scores[best_idx]
    
    return best_th, best_f1

base_path   = 'data/repetitions/'
models_path = 'models/'          
num_seeds   = 10
features    = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

results_list = []
results_list_best_th = []

for seed in range(num_seeds):
    print(f"--- Procesando Seed {seed} ---")
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    df_train = pd.read_csv(path_train)
    df_test  = pd.read_csv(path_test)
    df_val   = pd.read_csv(path_val)


    X_train_raw = df_train[features].values    
    X_test_raw  = df_test[features].values
    X_val_raw   = df_val[features].values


    y_train     = df_train['rcvOk'].values
    y_test      = df_test['rcvOk'].values
    y_val       = df_val['rcvOk'].values

    scaler = StandardScaler()
    scaler.fit(X_train_raw)
 
    X_train = scaler.transform(X_train_raw)
    X_test  = scaler.transform(X_test_raw)
    X_val   = scaler.transform(X_val_raw)

    scaler_path = os.path.join(models_path, f'scaler_seed{seed}.pkl')
    joblib.dump(scaler, scaler_path)

    # Calculamos los pesos de las muestras para pasárselos al Gradient Boosting
    sample_weights_train = compute_sample_weight(class_weight='balanced', y=y_train)

    ''' 
    models = {
        "Logistic Regression": LogisticRegression(max_iter=1000, class_weight='balanced', random_state=42),
        "Random Forest": RandomForestClassifier(n_estimators=100, class_weight='balanced', random_state=42),
        "MLP": MLPClassifier(hidden_layer_sizes=(64, 32), max_iter=1000, random_state=42)
    }
    '''

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
            "threshold:" : 0.5, 
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
df_results.to_csv('baselines_ml_results_v4_imbalanced.csv', index=False)

df_results_best_th = pd.DataFrame(results_list_best_th)
df_results_best_th.to_csv('baselines_ml_results_v4_imbalanced_best_th.csv', index=False)
 
print("Training complete. Results saved to 'baselines_ml_results_v4_imbalanced.csv'.")
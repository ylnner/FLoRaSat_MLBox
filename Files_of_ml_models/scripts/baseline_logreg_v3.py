"""
Non-sequential baseline: logistic regression on the target transmission only.

This baseline answers "how much does the sequence actually contribute?". It uses exactly
the same data, splits, feature scaling, class weighting and threshold protocol as
baseline_transformer_v3.py / baseslines_bilstm_v3.py, and differs in one respect only:
the classifier sees the features of the target transmission alone instead of the window
of seq_length transmissions.

To keep the comparison exact, the evaluated rows are the same ones the sequence models
score: for each (id_simulation, srcId) group, the target of a window is its last element,
so the first (seq_length - 1) transmissions of every device are not evaluated.

Outputs (same column names as the sequence baselines, so the CSVs can be concatenated):
    baselines_logreg_results_best_threshold.csv
    baselines_logreg_results_default_threshold.csv
"""

import os
import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import (log_loss, roc_auc_score, precision_recall_curve, auc,
                             accuracy_score, precision_score, recall_score, f1_score)

base_path  = 'data/repetitions/'
num_seeds  = 10
seq_length = 8
features   = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

# Same inverse class ratio used as pos_weight in the BCE loss of the sequence models
pos_weight = 77.62 / 22.38


def get_window_targets(df, seq_length):
    """Keep the rows that are the last element of a full per-device window.

    Windows are built per (id_simulation, srcId), as in create_sequences_data of the
    sequence baselines, so this returns exactly the transmissions those models predict.
    """
    df = df.sort_values(by=['id_simulation', 'srcId', 'time']).reset_index(drop=True)

    keep_idx = []
    for _, group_df in df.groupby(['id_simulation', 'srcId'], sort=False):
        idx = group_df.index.values
        if len(idx) >= seq_length:
            keep_idx.append(idx[seq_length - 1:])

    return df.loc[np.concatenate(keep_idx)]


def find_best_threshold(y_true_val, y_prob_val):
    # Find the best threshold that maximize the F1-Score for the possitive class
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_prob_val)

    f1_scores = (2 * precisions * recalls) / (precisions + recalls + 1e-10)

    best_idx = np.argmax(f1_scores)
    best_th = thresholds[best_idx] if best_idx < len(thresholds) else thresholds[-1]
    best_f1 = f1_scores[best_idx]

    return best_th, best_f1


def collect_metrics(seed, threshold, y_true_test, y_prob_test, val_loss, y_true_val, y_prob_val):
    y_pred_test = (y_prob_test >= threshold).astype(int)

    p_curve_test, r_curve_test, _ = precision_recall_curve(y_true_test, y_prob_test)
    test_auc_pr = auc(r_curve_test, p_curve_test)

    p_curve_val, r_curve_val, _ = precision_recall_curve(y_true_val, y_prob_val)
    val_auc_pr = auc(r_curve_val, p_curve_val)

    return {
        "seed": seed,
        "best_threshold_used": threshold,
        "val_loss": val_loss,
        "val_auc_roc": roc_auc_score(y_true_val, y_prob_val),
        "val_auc_pr": val_auc_pr,

        "test_accuracy": accuracy_score(y_true_test, y_pred_test),
        "test_auc_roc": roc_auc_score(y_true_test, y_prob_test),
        "test_auc_pr": test_auc_pr,

        # Class 0 (Negative)
        "test_prec_clase_0": precision_score(y_true_test, y_pred_test, pos_label=0, zero_division=0),
        "test_rec_clase_0": recall_score(y_true_test, y_pred_test, pos_label=0, zero_division=0),
        "test_f1_clase_0": f1_score(y_true_test, y_pred_test, pos_label=0, zero_division=0),

        # Class 1 (Positive)
        "test_prec_clase_1": precision_score(y_true_test, y_pred_test, pos_label=1, zero_division=0),
        "test_rec_clase_1": recall_score(y_true_test, y_pred_test, pos_label=1, zero_division=0),
        "test_f1_clase_1": f1_score(y_true_test, y_pred_test, pos_label=1, zero_division=0),
    }


metrics_best_threshold    = []
metrics_default_threshold = []

for seed in range(num_seeds):
    print(f"\n===== Seed {seed} =====")

    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    df_train = get_window_targets(pd.read_csv(path_train), seq_length)
    df_test  = get_window_targets(pd.read_csv(path_test),  seq_length)
    df_val   = get_window_targets(pd.read_csv(path_val),   seq_length)

    # Scaler fitted on the training split only, as in the sequence baselines
    scaler = StandardScaler()
    scaler.fit(df_train[features].values)

    X_train = scaler.transform(df_train[features].values)
    X_val   = scaler.transform(df_val[features].values)
    X_test  = scaler.transform(df_test[features].values)

    y_train = df_train['rcvOk'].values
    y_val   = df_val['rcvOk'].values
    y_test  = df_test['rcvOk'].values

    model = LogisticRegression(max_iter=5000, class_weight={0: 1.0, 1: pos_weight})
    model.fit(X_train, y_train)

    y_prob_val  = model.predict_proba(X_val)[:, 1]
    y_prob_test = model.predict_proba(X_test)[:, 1]

    val_loss = log_loss(y_val, np.clip(y_prob_val, 1e-15, 1 - 1e-15))

    # Threshold frozen on validation, then applied to test
    best_th, best_f1 = find_best_threshold(y_val, y_prob_val)
    print(f"  best threshold on validation: {best_th:.4f} (val F1 class 1: {best_f1:.4f})")

    metrics_best_threshold.append(
        collect_metrics(seed, best_th, y_test, y_prob_test, val_loss, y_val, y_prob_val))
    metrics_default_threshold.append(
        collect_metrics(seed, 0.5, y_test, y_prob_test, val_loss, y_val, y_prob_val))

    print(f"  test AUC-PR: {metrics_best_threshold[-1]['test_auc_pr']:.4f}")


def save_with_average(rows, filename, threshold_col='best_threshold_used'):
    df = pd.DataFrame(rows)
    avg = df.mean(numeric_only=True).to_dict()
    avg['seed'] = np.nan
    avg[threshold_col] = 'AVERAGE'
    df = pd.concat([df, pd.DataFrame([avg])], ignore_index=True)
    df.to_csv(filename, index=False)
    return df


df_best = save_with_average(metrics_best_threshold, 'baselines_logreg_results_best_threshold.csv')
save_with_average(metrics_default_threshold, 'baselines_logreg_results_default_threshold.csv')

avg = df_best.iloc[-1]
print("\n===== Average over seeds (threshold frozen on validation) =====")
print(f"  AUC-PR : {avg['test_auc_pr']:.4f}")
print(f"  Class 0: P {100*avg['test_prec_clase_0']:.2f}  R {100*avg['test_rec_clase_0']:.2f}  F1 {100*avg['test_f1_clase_0']:.2f}")
print(f"  Class 1: P {100*avg['test_prec_clase_1']:.2f}  R {100*avg['test_rec_clase_1']:.2f}  F1 {100*avg['test_f1_clase_1']:.2f}")
print("\nFinish running. Results saved correctly with per-class granular metrics and AUC-PR.")

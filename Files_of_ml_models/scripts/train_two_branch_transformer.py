import pandas as pd
import numpy as np
import os
import random
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader
import torch.optim as optim
import itertools
import time
import joblib
import gc

from TimePositionalEncoding import TimePositionalEncoding
from TwoBranchLoraCollisionTransformer import TwoBranchLoraCollisionTransformer

# =====================================================================================
# FEATURES
# =====================================================================================
continuous_features = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']
cols_to_drop        = ['dstId', 'srcSat', 'dstSat', 'loraCF', 'loraBW', 'loraCR', 'satId']
STATIC_HIDDEN_DIMS = (64, 32) # Dims for MLP
# =====================================================================================
# FUNCTIONS
# =====================================================================================
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
    df[continuous_features] = scaler.transform(df[continuous_features])

    # Process data
    seq_X   = []
    label_y = []
    for sim_id, group_df in df.groupby(['id_simulation', 'srcId']):
        num_features_array  = group_df[continuous_features].values
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
            window_X = np.hstack((window_num, delta_t_norm, delta_t_raw))

            seq_X.append(window_X)
            label_y.append(label_target)
                
    X = torch.tensor(np.array(seq_X), dtype=torch.float32)
    y = torch.tensor(np.array(label_y), dtype=torch.float32)

    return X, y

def load_and_prepare_data(seq_length,global_path_train='', global_path_test='', global_path_val=''):    
    print("Loading dataset...")

    df_train = pd.read_csv(global_path_train)
    df_test  = pd.read_csv(global_path_test)
    df_val   = pd.read_csv(global_path_val)

    X_train_features = df_train[continuous_features].values
    X_test_features = df_test[continuous_features].values
    X_val_features = df_val[continuous_features].values


    scaler = StandardScaler()
    scaler.fit(X_train_features)

    delta_t_train_raw = compute_delta_t_for_scaling(df_train, seq_length)
    scaler_delta_t = StandardScaler()
    scaler_delta_t.fit(delta_t_train_raw)
    
    X_train, y_train = create_sequences_data(df_train, scaler, seq_length, scaler_delta_t)
    X_test , y_test  = create_sequences_data(df_test, scaler, seq_length, scaler_delta_t)
    X_val  , y_val   = create_sequences_data(df_val, scaler, seq_length, scaler_delta_t)


    return X_train, y_train, X_test, y_test, X_val, y_val, scaler, scaler_delta_t


# =====================================================================================
# TRAIN / EVAL LOOP (misma logica que el baseline: BCEWithLogitsLoss + pos_weight,
# metricas por epoca, etc.) La unica diferencia real es la construccion del modelo.
# =====================================================================================
def train_single_config(config, train_loader, val_loader, device, epochs=20):
    logs = {'config': config, 'train_loss_log': [], 'train_acc_log': [], 'val_loss_log': [], 'val_acc_log': []}

    model = TwoBranchLoraCollisionTransformer(        
        d_model=config['d_model'],
        n_heads=config['n_heads'],
        n_layers=config['n_layers'],
        dropout=config['dropout'],
        static_hidden_dims=STATIC_HIDDEN_DIMS,
        fusion_hidden=config['fusion_hidden'],
        add_sigmoid=False,
    ).to(device)

    # pos_weight para manejar el desbalance de clases (igual que en el baseline)
    pos_weight = torch.tensor([77.62 / 22.38], device=device)
    criterion = nn.BCEWithLogitsLoss(pos_weight=pos_weight)
    optimizer = optim.Adam(model.parameters(), lr=config['lr'])

    avg_val_loss = 0.0
    y_true_epoch, y_pred_epoch, y_prob_epoch = None, None, None

    for epoch in range(epochs):
        model.train()
        train_loss      = 0.0
        correct_train   = 0
        total_train     = 0

        for batch_X, batch_y in train_loader:
            batch_X, batch_y = batch_X.to(device), batch_y.to(device)

            optimizer.zero_grad()

            # Forward pass
            # In this case the LoraCollisionTransformer model returns logits (raw scores) instead of probabilities. 
            # The BCEWithLogitsLoss function expects logits as input, so we don't apply a sigmoid activation here. 
            # The loss function will handle the conversion to probabilities internally.
            logits = model(batch_X) # Usa internamente el squeeze(-1)

            # Calculate error loss
            loss = criterion(logits, batch_y)

            # Backpropagation
            loss.backward()

            # Update weights
            optimizer.step()

            # loss.item - The average of the batch losses will give you an estimate of the “epoch loss” during training. Returns the value of this tensor as a standard Python number         
            train_loss += loss.item() * batch_X.size(0)

            predictions_prob = torch.sigmoid(logits)  # Convert logits to probabilities   
            preds_class = (predictions_prob >= 0.5).float()

            correct_train += (preds_class == batch_y).sum().item()
            total_train += batch_y.size(0)

        avg_train_loss = train_loss / total_train
        train_acc = correct_train / total_train
        logs['train_loss_log'].append(avg_train_loss)
        logs['train_acc_log'].append(train_acc)

        # Validation
        model.eval()
        val_loss    = 0.0  
        correct_val = 0
        total_val   = 0

        y_true_epoch = []
        y_prob_epoch = []
        y_pred_epoch = []
        with torch.no_grad():
            for batch_X, batch_y in val_loader:
                batch_X, batch_y = batch_X.to(device), batch_y.to(device)

                # Predict
                logits_val  = model(batch_X)
                loss        = criterion(logits_val, batch_y)
                val_loss   += loss.item() * batch_X.size(0)

                probs_val = torch.sigmoid(logits_val)
                preds_class_val = (probs_val >= 0.5).float()
                correct_val += (preds_class_val == batch_y).sum().item()
                total_val += batch_y.size(0)

                y_pred_epoch.extend(preds_class_val.cpu().numpy())
                y_true_epoch.extend(batch_y.cpu().numpy())
                y_prob_epoch.extend(probs_val.cpu().numpy())

        avg_val_loss = val_loss / total_val
        val_acc = correct_val / total_val
        logs['val_loss_log'].append(avg_val_loss)
        logs['val_acc_log'].append(val_acc)

        y_true_epoch = np.array(y_true_epoch)
        y_pred_epoch = np.array(y_pred_epoch)
        y_prob_epoch = np.array(y_prob_epoch)

        # Print metrics
        acc = accuracy_score(y_true_epoch, y_pred_epoch)
        auc_roc = roc_auc_score(y_true_epoch, y_prob_epoch)

        prec_c0 = precision_score(y_true_epoch, y_pred_epoch, pos_label=0, zero_division=0)
        rec_c0  = recall_score(y_true_epoch, y_pred_epoch, pos_label=0, zero_division=0)
        f1_c0   = f1_score(y_true_epoch, y_pred_epoch, pos_label=0, zero_division=0)

        prec_c1 = precision_score(y_true_epoch, y_pred_epoch, pos_label=1, zero_division=0)
        rec_c1  = recall_score(y_true_epoch, y_pred_epoch, pos_label=1, zero_division=0)
        f1_c1   = f1_score(y_true_epoch, y_pred_epoch, pos_label=1, zero_division=0)

        precision_curve, recall_curve, _ = precision_recall_curve(y_true_epoch, y_prob_epoch)
        auc_pr = auc(recall_curve, precision_curve)

        print(f"Epoch {epoch+1:02d}/{epochs} | Train Loss: {avg_train_loss:.4f} | Val Loss: {avg_val_loss:.4f} | Val Acc: {val_acc*100:.1f}%")
        print(f"  [Clase 0 - Neg] Precision: {prec_c0*100:.1f}% | Recall: {rec_c0*100:.1f}% | F1: {f1_c0*100:.1f}%")
        print(f"  [Clase 1 - Pos] Precision: {prec_c1*100:.1f}% | Recall: {rec_c1*100:.1f}% | F1: {f1_c1*100:.1f}%")
        print(f"  [Global] Accuracy: {acc:.4f} | AUC-ROC: {auc_roc:.4f} | AUC-PR: {auc_pr:.4f}\n")

    return avg_val_loss, model, logs, y_true_epoch, y_pred_epoch, y_prob_epoch


def find_best_threshold(y_true_val, y_prob_val):
    # Find the best threshold that maximize the F1-Score for the possitive class
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_prob_val)
    
    # Calculate F1-Score for each possible threshold
    f1_scores = (2 * precisions * recalls) / (precisions + recalls + 1e-10)
        
    best_idx = np.argmax(f1_scores)        
    best_th = thresholds[best_idx] if best_idx < len(thresholds) else thresholds[-1]
    best_f1 = f1_scores[best_idx]
    
    return best_th, best_f1


# =====================================================================================
# GRID SEARCH 
# =====================================================================================
base_path = 'data/repetitions/'
num_seeds  = 10
features   = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan'] 
device = "cpu"
print(f"Using: {device}")
 
param_grid = {
    'seq_length'    : [8],
    # --- Transformer Encoder ---
    'd_model'       : [32],
    'n_heads'       : [2, 4],
    'n_layers'      : [1, 2],
    'dropout'       : [0.1, 0.2, 0.3],
    'lr'            : [0.0005, 0.001],
    'batch_size'    : [32],
    # --- Fusion ---
    'fusion_hidden' : [16, 32],
}
 
keys, values = zip(*param_grid.items())
combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]
print(f"{len(combinations)} hyperparameter combinations to evaluate.")

print(f"Starting  {len(combinations)} combinations ...")
print("="*60)

start_total = time.time()  
gs_results   = []
best_gs_loss = float('inf')
best_config  = None


for i, config in enumerate(combinations):
    print(f"---> Processing Combination {i+1}/{len(combinations)} ---")
    print(f"Configuration: {config} - i: {i}")    

    seed_losses = []

    for seed in range(num_seeds):
        print(f"---> Processing Combination {i+1}/{len(combinations)} ---")
        print(f"-------> Processing Seed {seed+1}/{num_seeds} ---")
        
        path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
        path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
        path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')
    
        X_train, y_train, X_test, y_test, X_val, y_val, scaler, scaler_delta_t = load_and_prepare_data(
            seq_length=config['seq_length'], global_path_train=path_train, global_path_test=path_test, global_path_val=path_val
        )

        # Create datasets
        train_dataset = TensorDataset(X_train, y_train)
        test_dataset  = TensorDataset(X_test, y_test)
        val_dataset   = TensorDataset(X_val, y_val)

        # Create DataLoaders
        train_loader = DataLoader(train_dataset, batch_size=config['batch_size'], shuffle=True)
        test_loader  = DataLoader(test_dataset, batch_size=config['batch_size'], shuffle=False)
        val_loader   = DataLoader(val_dataset, batch_size=config['batch_size'], shuffle=False)        

        # Training
        final_loss, model, logs, y_true, y_pred, _ = train_single_config(config, train_loader, val_loader, device, epochs=20)
        print(f"-> Val Loss: {final_loss:.4f}")

        seed_losses.append(final_loss)
         
        del model 
        del train_loader 
        del test_loader 
        del val_loader
        gc.collect()

     # Calculate mean and std of losses for the current combination considering all seeds
    mean_loss = float(np.mean(seed_losses))
    std_loss  = float(np.std(seed_losses))
    print(f"  => Mean val_loss for this combination: {mean_loss:.4f} ± {std_loss:.4f}")

    row = {
        **config,
        'mean_val_loss': mean_loss,
        'std_val_loss':  std_loss,
        **{f'val_loss_seed_{s}': seed_losses[s] for s in range(num_seeds)},
    }
    gs_results.append(row)


    if mean_loss < best_gs_loss:
        best_gs_loss = mean_loss
        best_config  = config
        print(f"  ** New best config (mean_val_loss={mean_loss:.4f})")


# Create DataFrame with results and save to CSV
pd.DataFrame(gs_results).to_csv('two_branch_transformer_gridsearch_results.csv', index=False)
print(f"Best config: {best_config}  (mean val_loss={best_gs_loss:.4f})")
print("Grid search results saved to 'two_branch_transformer_gridsearch_results.csv'.")



print("="*60)
print("Training with best config founded ...")
print("="*60)
 
os.makedirs('models/', exist_ok=True)
 
results_all_seeds = []
results_all_seeds_best_threshold = []

for seed in range(num_seeds):
    print(f"-------> Processing Seed {seed+1}/{num_seeds} ---")

    # Create nombres of files
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    X_train, y_train, X_test, y_test, X_val, y_val, scaler, scaler_delta_t = load_and_prepare_data(
        seq_length=best_config['seq_length'], global_path_train=path_train, global_path_test=path_test, global_path_val=path_val
    )

    # Create datasets
    train_dataset = TensorDataset(X_train, y_train)
    test_dataset  = TensorDataset(X_test, y_test)
    val_dataset   = TensorDataset(X_val, y_val)

    # Create DataLoaders
    train_loader = DataLoader(train_dataset, batch_size=best_config['batch_size'], shuffle=True)
    test_loader  = DataLoader(test_dataset, batch_size=best_config['batch_size'], shuffle=False)
    val_loader   = DataLoader(val_dataset, batch_size=best_config['batch_size'], shuffle=False)

    # Training
    val_loss, model, logs, y_true_val, y_pred_val, y_prob_val = train_single_config(best_config, train_loader, val_loader, device, epochs=20)

    # Find the best thereshold
    best_val_threshold, max_val_f1 = find_best_threshold(y_true_val, y_prob_val)
    print(f"  [Seed {seed}] FInd best threshold in validation set: {best_val_threshold:.4f} (Val F1-C1: {max_val_f1*100:.1f}%)")

    model_path = os.path.join('models/', f'two_branch_transformer_best_seed{seed}.pth')
    scaler_path = os.path.join('models/', f'two_branch_transformer_scaler_seed{seed}.pkl')
    scaler_delta_t_path = os.path.join('models/', f'two_branch_transformer_scaler_delta_t_seed{seed}.pkl')
    torch.save(model.state_dict(), model_path)
    joblib.dump(scaler, scaler_path)
    joblib.dump(scaler_delta_t, scaler_delta_t_path)

    # Evaluate on test set
    model.eval()
    y_true_test = []
    y_prob_test = []
    y_pred_test = []
    with torch.no_grad():
        for batch_X, batch_y in test_loader:
            batch_X, batch_y = batch_X.to(device), batch_y.to(device)
            
            logits      = model(batch_X)
            probs       = torch.sigmoid(logits)  # Convert logits to probabilities
            preds_class = (probs >= 0.5).float()

            y_prob_test.extend(probs.cpu().numpy())
            y_pred_test.extend(preds_class.cpu().numpy())
            y_true_test.extend(batch_y.cpu().numpy())

 
    y_true_test = np.array(y_true_test)
    y_pred_test = np.array(y_pred_test)
    y_prob_test = np.array(y_prob_test)
 
    p_curve_test, r_curve_test, _ = precision_recall_curve(y_true_test, y_prob_test)
    test_auc_pr = auc(r_curve_test, p_curve_test)
 
    p_curve_val, r_curve_val, _ = precision_recall_curve(y_true_val, y_prob_val)
    val_auc_pr = auc(r_curve_val, p_curve_val)
 
    # Calculate metrics for the current seed and save
    metrics = {
        "seed": seed,
        "threshold" : 0.5,
        "val_loss": val_loss,
        "val_auc_roc": roc_auc_score(y_true_val, y_pred_val),
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

    results_all_seeds.append(metrics)


    # Use the new thereshold
    new_y_pred_test = (y_prob_test >= best_val_threshold).astype(float)
    
    # Calculate new metrics using the new y_pred_test
    p_curve_test, r_curve_test, _ = precision_recall_curve(y_true_test, y_prob_test)
    test_auc_pr = auc(r_curve_test, p_curve_test)

    p_curve_val, r_curve_val, _ = precision_recall_curve(y_true_val, y_prob_val)
    val_auc_pr = auc(r_curve_val, p_curve_val)

    # Store metrics
    new_metrics = {
        "seed": seed,
        "best_threshold_used": best_val_threshold, 
        "val_loss": val_loss,
        "val_auc_roc": roc_auc_score(y_true_val, y_prob_val),
        "val_auc_pr": val_auc_pr,
        
        "test_accuracy": accuracy_score(y_true_test, new_y_pred_test),
        "test_auc_roc": roc_auc_score(y_true_test, y_prob_test),
        "test_auc_pr": test_auc_pr,
        
        # Clase 0 (Negative) - Evaluado con el nuevo umbral
        "test_prec_clase_0": precision_score(y_true_test, new_y_pred_test, pos_label=0, zero_division=0),
        "test_rec_clase_0": recall_score(y_true_test, new_y_pred_test, pos_label=0, zero_division=0),
        "test_f1_clase_0": f1_score(y_true_test, new_y_pred_test, pos_label=0, zero_division=0),
        
        # Clase 1 (Positive) - Evaluado con el nuevo umbral
        "test_prec_clase_1": precision_score(y_true_test, new_y_pred_test, pos_label=1, zero_division=0),
        "test_rec_clase_1": recall_score(y_true_test, new_y_pred_test, pos_label=1, zero_division=0),
        "test_f1_clase_1": f1_score(y_true_test, new_y_pred_test, pos_label=1, zero_division=0),
    }
    results_all_seeds_best_threshold.append(new_metrics)
 
    del model
    del train_loader
    del test_loader
    del val_loader
    del train_dataset
    del test_dataset
    del val_dataset
    gc.collect()
 
pd.DataFrame(results_all_seeds).to_csv('two_branch_transformer_results.csv', index=False)
pd.DataFrame(results_all_seeds_best_threshold).to_csv('two_branch_transformer_results_best_threshold.csv', index=False)
print("Resultados guardados en 'two_branch_transformer_results.csv' y '..._best_threshold.csv'.")

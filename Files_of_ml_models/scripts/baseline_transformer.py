import pandas as pd
import numpy as np
import os
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, roc_auc_score
from sklearn.preprocessing import StandardScaler
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split
import math
import torch.optim as optim
import itertools
import time
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
from TimePositionalEncoding import TimePositionalEncoding
from LoraCollisionTransformer import LoraCollisionTransformer
import joblib
import gc

# Selecting continuous features and cols to drop
continuous_features = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']
cols_to_drop        = ['dstId', 'srcSat', 'dstSat', 'loraCF', 'loraBW', 'loraCR', 'satId', 'srcId']


def create_sequences_data(df, scaler, seq_length):    
    df = df.drop(columns=cols_to_drop)

    # Sort by id_simulaiton and time
    df = df.sort_values(by=['id_simulation', 'time']).reset_index(drop=True)
    
    # Apply StandardScaler         
    df[continuous_features] = scaler.transform(df[continuous_features])

    # Process data
    seq_X   = []
    label_y = []
    for sim_id, group_df in df.groupby('id_simulation'):
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

def load_and_prepare_data(seq_length=16,global_path_train='', global_path_test='', global_path_val=''):    
    print("Loading dataset...")

    df_train = pd.read_csv(global_path_train)
    df_test  = pd.read_csv(global_path_test)
    df_val   = pd.read_csv(global_path_val)

    X_train_features = df_train[continuous_features].values
    X_test_features = df_test[continuous_features].values
    X_val_features = df_val[continuous_features].values


    scaler = StandardScaler()
    scaler.fit(X_train_features)
    
    X_train, y_train = create_sequences_data(df_train, scaler, seq_length)
    X_test , y_test  = create_sequences_data(df_test, scaler, seq_length)
    X_val  , y_val   = create_sequences_data(df_val, scaler, seq_length)


    return X_train, y_train, X_test, y_test, X_val, y_val, scaler


def train_single_config(config, train_loader, val_loader, device, epochs=20, early_stopping = False):
    logs = {}
    logs['config']          = config
    logs['train_loss_log']  = []    
    logs['train_acc_log']   = []
    logs['val_loss_log']    = []
    logs['val_acc_log']     = []


    # Instanciar modelo con la configuración actual
    model = LoraCollisionTransformer(
        num_numerical_features  = 8, # 8 reales + 1 t_delta (el delta se adiciona despues)
        d_model                 = config['d_model'],
        n_heads                 = config['n_heads'],
        n_layers                = config['n_layers'],
        dropout                 = config['dropout']
    ).to(device)
    
    criterion = nn.BCELoss()
    optimizer = optim.Adam(model.parameters(), lr=config['lr'])
        
    best_val_acc = 0.0 
    avg_val_loss = 0.0
    for epoch in range(epochs):
        # Train
        model.train()
        train_loss      = 0.0
        correct_train   = 0
        total_train     = 0

        #for X_b, y_b in train_loader:
        for batch_X, batch_y in train_loader:
            batch_X, batch_y = batch_X.to(device), batch_y.to(device)

            # Clean gradient 
            optimizer.zero_grad()

            # Forward pass
            predictions = model(batch_X) # Usa internamente el squeeze(-1)

            # Calculate error loss
            loss = criterion(predictions, batch_y)

            # Backpropagation
            loss.backward()

            # Update weights
            optimizer.step()

            # loss.item - The average of the batch losses will give you an estimate of the “epoch loss” during training. Returns the value of this tensor as a standard Python number 
            train_loss += loss.item() * batch_X.size(0)
            
            # Convert to 0 - 1 the vector batch
            predictions_class = (predictions >= 0.5).float()
            
            correct_train += (predictions_class == batch_y).sum().item()
            total_train   += batch_y.size(0)


        avg_train_loss  = train_loss / total_train
        train_acc       = correct_train / total_train
        
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
            #for X_b, y_b in val_loader:
            for batch_X, batch_y in val_loader:
                batch_X, batch_y = batch_X.to(device), batch_y.to(device)
                
                # Predict
                predictions = model(batch_X)
                loss        = criterion(predictions, batch_y)
                val_loss   += loss.item() * batch_X.size(0)

                # Calculate correct or not 
                predictions_class = (predictions >= 0.5).float()
                correct_val      += (predictions_class == batch_y).sum().item()
                total_val        += batch_y.size(0)

                # Other method to calculate metrics
                y_pred_epoch.extend(predictions_class.cpu().numpy())  
                y_true_epoch.extend(batch_y.cpu().numpy())
                y_prob_epoch.extend(predictions.cpu().numpy())
        
        avg_val_loss = val_loss / total_val
        val_acc      = correct_val / total_val

        logs['val_loss_log'].append(avg_val_loss)
        logs['val_acc_log'].append(val_acc)

        if early_stopping:            
            print(f"Early stopping at epoch {epoch+1} with val_loss: {avg_val_loss:.4f}")
            # Save model for 
            model_path  = os.path.join('models/', f'transformer_best_seed{seed}_epoch{epoch+1}.pth')
            #scaler_path = os.path.join('models/', f'transformer_scaler_seed{seed}_epoch{epoch+1}.pkl')
            torch.save(model.state_dict(), model_path)
            #joblib.dump(scaler, scaler_path)

        
        # Print metrics
        acc = accuracy_score(y_true_epoch, y_pred_epoch)
        prec = precision_score(y_true_epoch, y_pred_epoch, zero_division=0)
        rec = recall_score(y_true_epoch, y_pred_epoch, zero_division=0)
        f1 = f1_score(y_true_epoch, y_pred_epoch, zero_division=0)
        auc = roc_auc_score(y_true_epoch, y_prob_epoch)
        
        print(f"Epoch {epoch+1:02d}/{epochs} | "
        f"Train Loss: {avg_train_loss:.4f} - Acc: {train_acc*100:.1f}% | "
        f"Val Loss: {avg_val_loss:.4f} - Acc: {val_acc*100:.1f}% | " 
        f"Val Acc: {acc:.4f} - Val Prec: {prec*100:.1f}% | "
        f"Val Rec: {rec:.4f} - Val F1: {f1*100:.1f}% | "
        f"Val AUC-ROC: {auc:.4f} "
        )
    
    return avg_val_loss, model, logs, y_true_epoch, y_pred_epoch




base_path  = 'data/repetitions/'
num_seeds  = 10
seq_length = 16
features   = ['latDev', 'longDev', 'elevSat', 'loraTP', 'loraSF', 'doppler', 'alt', 'raan']

device = "cpu"
print(f"Using : {device}")    

# Setting params    
''' 
param_grid = {
    'd_model'   : [32, 64], #128                # Dimension of the model
    'n_heads'   : [2, 4],                       # Number of projections
    'n_layers'  : [1, 2, 3],                    # Number of encoder layers
    'dropout'   : [0.1, 0.2, 0.3, 0.4, 0.5], # , 0.2                # doprout
    'lr'        : [0.001], #, 0.0005            # Learning rate
    'batch_size': [32]                          # Batch Size
}
'''
param_grid = {
    'd_model'   : [32], #128                # Dimension of the model
    'n_heads'   : [4, 8],                       # Number of projections
    'n_layers'  : [1],                    # Number of encoder layers
    'dropout'   : [0.4, 0.5, 0.6, 0.7], # , 0.2                # doprout
    'lr'        : [0.001, 0.005], #, 0.0005            # Learning rate
    'batch_size': [32, 64]                          # Batch Size
}

# Generate the combination of parameters
keys, values = zip(*param_grid.items())
combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]
print(f"{len(combinations)} hyperparameter combinations to evaluate.")



# Read all csv files and create datasets for each seed
dict_train_dataset = {} 
dict_test_dataset  = {}
dict_val_dataset   = {}
for seed in range(num_seeds):
    print(f"---> Reading files for Seed {seed+1}/{num_seeds} ---")

    # Construcción dinámica de nombres de archivos
    path_train = os.path.join(base_path, f'seed_{seed}_final_train_data_transmission.csv')
    path_test  = os.path.join(base_path, f'seed_{seed}_final_test_data_transmission.csv')
    path_val   = os.path.join(base_path, f'seed_{seed}_final_val_data_transmission.csv')

    X_train, y_train, X_test, y_test, X_val  , y_val, scaler = load_and_prepare_data(seq_length = 16, global_path_train=path_train, global_path_test=path_test, global_path_val=path_val)
            
    # Crteata Datasets
    train_dataset = TensorDataset(X_train, y_train)
    test_dataset  = TensorDataset(X_test, y_test)
    val_dataset   = TensorDataset(X_val, y_val)

    # Store Datasets in dictionaries
    dict_train_dataset[seed] = train_dataset
    dict_test_dataset[seed]  = test_dataset
    dict_val_dataset[seed]   = val_dataset




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
        print(f"-------> Processing Seed {seed+1}/{num_seeds} ---")
        # Get DataLoaders from dictionaries
        train_dataset = dict_train_dataset[seed]
        test_dataset  = dict_test_dataset[seed]
        val_dataset   = dict_val_dataset[seed]

        # Create DataLoaders
        train_loader = DataLoader(train_dataset, batch_size=config['batch_size'], shuffle=True)
        test_loader  = DataLoader(test_dataset, batch_size=config['batch_size'], shuffle=False)
        val_loader   = DataLoader(val_dataset, batch_size=config['batch_size'], shuffle=False)     
                
        # Training
        final_loss, model, logs, y_true, y_pred = train_single_config(config, train_loader, val_loader, device, epochs=20, early_stopping=False)

        print(f"-> Best Loss: {final_loss:.4f}")                    
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
pd.DataFrame(gs_results).to_csv('transformer_gridsearch_results.csv', index=False)
print(f"Best config: {best_config}  (mean val_loss={best_gs_loss:.4f})")
print("Grid search results saved to 'transformer_gridsearch_results.csv'.")



print("="*60)
print("Training with best config ...")
print("="*60)


results_all_seeds = []
for seed in range(num_seeds):
    print(f"-------> Processing Seed {seed+1}/{num_seeds} ---")

    # Get DataLoaders from dictionaries
    train_dataset = dict_train_dataset[seed]
    test_dataset  = dict_test_dataset[seed]
    val_dataset   = dict_val_dataset[seed]

    # Create DataLoaders
    train_loader = DataLoader(train_dataset, batch_size=best_config['batch_size'], shuffle=True)
    test_loader  = DataLoader(test_dataset, batch_size=best_config['batch_size'], shuffle=False)
    val_loader   = DataLoader(val_dataset, batch_size=best_config['batch_size'], shuffle=False)


    # Training
    val_loss, model, logs, y_true_val, y_prob_val = train_single_config(best_config, train_loader, val_loader, device, epochs=20, early_stopping=True)

    # Save model and scaler for the current seed
    model_path  = os.path.join('models/', f'transformer_best_seed{seed}.pth')
    scaler_path = os.path.join('models/', f'transformer_scaler_seed{seed}.pkl')
    torch.save(model.state_dict(), model_path)
    joblib.dump(scaler, scaler_path)


    # Evaluate on test set
    model.eval()
    y_true_test = []
    y_prob_test = []
    y_pred_test = []
    with torch.no_grad():
        for batch_X, batch_y in test_loader:
            batch_X, batch_y = batch_X.to(device), batch_y.to(device)
            
            probs       = model(batch_X)
            preds_class = (probs >= 0.5).float()

            y_prob_test.extend(probs.cpu().numpy())
            y_pred_test.extend(preds_class.cpu().numpy())
            y_true_test.extend(batch_y.cpu().numpy())

        # Calculate metrics for the current seed
    metrics = {
        "seed": seed,
        "val_loss": val_loss,
        "val_auc_roc": roc_auc_score(y_true_val, y_prob_val),
        "val_f1": f1_score(y_true_val, (np.array(y_prob_val) >= 0.5).astype(int), zero_division=0),
        "test_accuracy": accuracy_score(y_true_test, y_pred_test),
        "test_precision": precision_score(y_true_test, y_pred_test, zero_division=0),
        "test_recall": recall_score(y_true_test, y_pred_test, zero_division=0),
        "test_f1_score": f1_score(y_true_test, y_pred_test, zero_division=0),
        "test_auc_roc": roc_auc_score(y_true_test, y_prob_test),
    }

    results_all_seeds.append(metrics)

    del model
    del train_loader
    del test_loader
    del val_loader
    del train_dataset
    del test_dataset
    del val_dataset
    gc.collect()

df_results = pd.DataFrame(results_all_seeds)
df_results.to_csv('baselines_transformer_results.csv', index=False)
print("Results saved to 'baselines_transformer_results.csv'.")
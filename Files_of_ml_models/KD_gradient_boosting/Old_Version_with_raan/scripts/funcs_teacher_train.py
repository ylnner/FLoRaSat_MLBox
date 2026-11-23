import pandas as pd
import os
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset
import time
import joblib
import gc
import copy
from global_funcs import move_batch_to_device, create_sub_configs_dicts, find_best_threshold, scale_splits, save_scalers, set_seed, compute_pos_weight, subset_metrics
import global_funcs
import itertools

from TeacherCollisionPredictor import TeacherCollisionPredictor
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score
from MyCustomClasses import MyCustomDataset


NUM_SEEDS = 1

def train_single_config(config, epochs, lr, train_loader, val_loader, device):

    #############################################
    ### Setting configs for Teacher Model
    #############################################
    config_teacher, config_seq, config_static, config_set_trf, config_context = create_sub_configs_dicts(config)
    
    teacher_model = TeacherCollisionPredictor(config_teacher, config_seq, config_static, config_set_trf, config_context).to(device)
    # pos_weight = #neg/#pos calculado sobre el train real - 81.38/18.62
    pos_weight    = compute_pos_weight(train_loader.dataset.data['y_true'].numpy(), device)

    # Early stopping
    """
    best_val_loss = float('inf')
    best_state    = None
    best_outputs  = None
    """

    best_val_acc = 0.0
    avg_val_acc  = 0.0

    criterion   = nn.BCEWithLogitsLoss(pos_weight=pos_weight)            
    optimizer   = optim.Adam(teacher_model.parameters(), lr=lr)
        
    for epoch in range(epochs):
        # Train
        teacher_model.train()
        train_loss      = 0.0
        correct_train   = 0
        total_train     = 0
                
        #for batch_X, batch_y in train_loader:
        for batch in train_loader:
            #print('batch[y_true]: ', batch['y_true'].size(0))
            #print('batch[seq_feats]: ', batch['seq_feats'].size(0))

            #batch_X, batch_y = batch_X.to(device), batch_y.to(device)
            batch = move_batch_to_device(batch, device)

            # Clean gradient 
            optimizer.zero_grad()

            # Forward pass
            # In this case the LoraCollisionTransformer model returns logits (raw scores) instead of probabilities. 
            # The BCEWithLogitsLoss function expects logits as input, so we don't apply a sigmoid activation here. 
            # The loss function will handle the conversion to probabilities internally.
            logits = teacher_model(batch) 

            # Calculate error loss
            #print('logits: ', logits.shape)
            #print('batch[y_true]', batch['y_true'].shape)
            loss = criterion(logits, batch['y_true'])

            # Backpropagation
            loss.backward()

            # Update weights
            optimizer.step()

            # loss.item - The average of the batch losses will give you an estimate of the “epoch loss” during training. Returns the value of this tensor as a standard Python number 
            train_loss += loss.item() * batch['y_true'].size(0)
            
            predictions_prob = torch.sigmoid(logits)  # Convert logits to probabilities            
            predictions_class = (predictions_prob >= 0.5).float() # Convert to 0 - 1 the vector batch
            
            correct_train += (predictions_class == batch['y_true']).sum().item()
            total_train   += batch['y_true'].size(0)

        ##################### End for

        avg_train_loss  = train_loss / total_train
        train_acc       = correct_train / total_train

        # Validation
        teacher_model.eval()        
        val_loss    = 0.0  
        correct_val = 0
        total_val   = 0
        
        y_true_epoch = []
        y_prob_epoch = []
        y_pred_epoch = []


        with torch.no_grad():            
            for batch in val_loader:
                batch = move_batch_to_device(batch, device)

                # Predict
                logits_val  = teacher_model(batch)
                loss        = criterion(logits_val, batch['y_true'])
                val_loss   += loss.item() * batch['y_true'].size(0)

                # Calculate correct or not 
                preds_prob_val  = torch.sigmoid(logits_val)  # Convert logits to probabilities                
                preds_class_val = (preds_prob_val >= 0.5).float()
                correct_val      += (preds_class_val == batch['y_true']).sum().item()
                total_val        += batch['y_true'].size(0)

                # Required to calculate the metrics
                y_pred_epoch.extend(preds_class_val.cpu().numpy())  
                y_true_epoch.extend(batch['y_true'].cpu().numpy())
                y_prob_epoch.extend(preds_prob_val.cpu().numpy())


        avg_val_loss = val_loss / total_val
        val_acc      = correct_val / total_val

        # Calculate new metrics for unbalanced data (precision, recall, f1-score, AUC-ROC) for the validation set
        y_true_epoch = np.array(y_true_epoch)
        y_pred_epoch = np.array(y_pred_epoch)
        y_prob_epoch = np.array(y_prob_epoch)

        # Print metrics
        acc     = accuracy_score(y_true_epoch, y_pred_epoch)
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


        """
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            best_state    = copy.deepcopy(teacher_model.state_dict())
            best_outputs  = (y_true_epoch, y_pred_epoch, y_prob_epoch)
            print(f"  ** Best epoch so far: {epoch+1}")
        """
    ##################### End for

    """
    # Early stopping
    teacher_model.load_state_dict(best_state)
    teacher_model.eval()
    y_true_epoch, y_pred_epoch, y_prob_epoch = best_outputs
    """

    return avg_val_loss, teacher_model, y_true_epoch, y_pred_epoch, y_prob_epoch
            


def run_full_train(epochs, dict_datasets_train, dict_datasets_test, dict_datasets_val, device = 'cpu'):    
    print(f"Using : {device}")  

    gs_results   = []    
    best_gs_loss = float('inf')
    best_config  = None

    # Setting params
    param_grid = {
        # config teacher
        'd_model': [32],
        'dropout' : [0.1, 0.2, 0.3],
        'lr' : [0.001, 0.0005],
        'batch_size': [32], 
        'out_dim' : [32], # intermediate layer previous to fianl pred

        # static
        'static_hidden_dims': [(64, 32)],        

        # sequential
        'n_heads': [2, 4],
        'n_layers': [1, 2],        

        # Set Transformer
        'inter_in_dim': [9], # Features used in input
        'num_sab_layers': [1, 2, 4], # Number of stack sab layers
        'sab_nheads': [2, 4],       # Number of projections
        'sab_ln': [True],           # If true apply Layer normalizatoin
        'own_in_dim': [2],          # Features of own_tx (current transmission which is comparted to the itnerf_set)
        'd_own': [8, 16],           # Dimension out of apply encoder to own_tx
        'cross_attn_nheads': [2, 4], # Number of nheads to apply cross attn in comparison
        'cross_attn_ln': [True],    # If true apply Layer normalizatoin

        # contextual
        'context_in_dim': [7],      # Number of features in the input (6 + flag has_real_power)
        'context_out_dim': [16, 32],#Dimension projections        
    }

    param_grid = {
        # config teacher
        'd_model': [32],
        'dropout' : [0.1],
        'lr' : [0.001],
        'batch_size': [32], 
        'out_dim' : [32], # intermediate layer previous to fianl pred

        # static
        'static_hidden_dims': [(32, 4)],        

        # sequential
        'n_heads': [4],
        'n_layers': [1],        

        # Set Transformer
        'inter_in_dim': [9],        # Features used in input
        'num_sab_layers': [2],      # Number of stack sab layers
        'sab_nheads': [4],          # Number of projections
        'sab_ln': [True],           # If true apply Layer normalizatoin
        'own_in_dim': [2],          # Features of own_tx (current transmission which is comparted to the itnerf_set)
        'd_own': [16],              # Dimension out of apply encoder to own_tx
        'cross_attn_nheads': [4],   # Number of nheads to apply cross attn in comparison
        'cross_attn_ln': [True],    # If true apply Layer normalizatoin

        # contextual
        'context_in_dim': [7],      # Number of features in the input (6 + flag has_real_power)
        'context_out_dim': [32],    #Dimension projections        
    }
        
    # Generate the combination of parameters
    keys, values = zip(*param_grid.items())
    combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]
    print(f"{len(combinations)} hyperparameter combinations to evaluate.")

    for i, config in enumerate(combinations):
        print(f"---> Processing Combination {i+1}/{len(combinations)} ---")
        print(f"Configuration: {config} - i: {i}")    

        seed_losses = []
        for seed in range(NUM_SEEDS):
            print(f"-------> Processing Seed {seed+1}/{NUM_SEEDS} ---")

            print(f"  train: {len(dict_datasets_train[seed]['y_true'])} records")
            print(f"  val:   {len(dict_datasets_val[seed]['y_true'])} records")
            print(f"  test:  {len(dict_datasets_test[seed]['y_true'])} records")

            print("\n" + "=" * 80)
            print("Applying StandardScaler only in train data (fit only on valid / non-padded values)...")
            scalers, train_scaled, val_scaled, test_scaled = scale_splits(dict_datasets_train[seed], dict_datasets_val[seed], dict_datasets_test[seed])

            # Create Dataset
            train_dataset   = MyCustomDataset(train_scaled)
            val_dataset     = MyCustomDataset(val_scaled)

            # Create Loaders. El test NO se usa en la busqueda de hiperparametros.
            set_seed(seed)
            train_loader = DataLoader(train_dataset, batch_size = config['batch_size'], shuffle=True)
            val_loader   = DataLoader(val_dataset, batch_size = config['batch_size'], shuffle=False)

            #------------------------------------------------------------------
            print("=" * 80)
            print("Training Master")
            print("=" * 80)
            print('config: ', config)
            
            
            #############################################
            ### Training the model with the configuration
            #############################################
            #  we pass val data, so we get val loss                                                                                                        
            final_val_loss, teacher_model, y_true, y_pred, _  = train_single_config(config, epochs, config['lr'], train_loader, val_loader, device)
            

            print(f"-> Best Val Loss: {final_val_loss:.4f}")
            seed_losses.append(final_val_loss)

            del teacher_model
            del train_loader
            del val_loader
            gc.collect()

        ###### End seeds

        # Calculate mean and std of losses for the current combination considering all seeds
        mean_loss = float(np.mean(seed_losses))
        std_loss  = float(np.std(seed_losses))
        print(f"  => Mean val_loss for this combination: {mean_loss:.4f} ± {std_loss:.4f}")


        row = {
            **config,
            'mean_val_loss': mean_loss,
            'std_val_loss':  std_loss,
            **{f'val_loss_seed_{s}': seed_losses[s] for s in range(NUM_SEEDS)},
        }
        gs_results.append(row)


        if mean_loss < best_gs_loss:
            best_gs_loss = mean_loss
            best_config  = config
            print(f"  ** New best config (mean_val_loss={mean_loss:.4f})")

    # Create DataFrame with results and save to CSV
    pd.DataFrame(gs_results).to_csv('teacher_gridsearch_results.csv', index=False)
    print(f"Best config: {best_config}  (mean val_loss={best_gs_loss:.4f})")
    print("Grid search results saved to 'teacher_gridsearch_results.csv'.")

    return best_config



#def train_and_test_with_best_config(epochs, out_dir, best_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device):
# below_mask_test se usa como flag para hacer un analisis por subconjunto below or not below
def train_and_test_with_best_config(epochs, out_dir, best_config, dataloader_train, dataloader_test, dataloader_val, seed, device, below_mask_test = None):
    #num_seeds = 10
    #device = "cpu"

    print("="*60)
    print("Training with best config founded ...")
    print("="*60)

    results_all_seeds = []
    results_all_seeds_best_threshold = []

    models_by_seed = {}
    # Dataloaders de train SIN shuffle (mismo orden que dict_datasets_train[seed]) para obtener los
    # logits del maestro. Se crean aqui, con los mismos datos escalados con los que se entreno este maestro.
    dict_train_dataloader_distillation = {}
    local_total_seeds = 10
    #for seed in range(NUM_SEEDS):
    print(f"-------> Processing Seed {seed} ---")

    """
    print(f"  train: {len(dict_datasets_train[seed]['y_true'])} records")
    print(f"  val:   {len(dict_datasets_val[seed]['y_true'])} records")
    print(f"  test:  {len(dict_datasets_test[seed]['y_true'])} records")

    print("\n" + "=" * 80)
    print("Applying StandardScaler only in train data (fit only on valid / non-padded values)...")
    scalers, train_scaled, val_scaled, test_scaled = scale_splits(dict_datasets_train[seed], dict_datasets_val[seed], dict_datasets_test[seed])

    # Create Dataset
    train_dataset   = MyCustomDataset(train_scaled)
    test_dataset    = MyCustomDataset(test_scaled)
    val_dataset     = MyCustomDataset(val_scaled)

    # Create Loaders
    
    
    set_seed(seed)
    train_loader = DataLoader(train_dataset, batch_size = best_config['batch_size'], shuffle=True)
    val_loader   = DataLoader(val_dataset, batch_size = best_config['batch_size'], shuffle=False)
    test_loader  = DataLoader(test_dataset, batch_size = best_config['batch_size'], shuffle=False)
    dict_train_dataloader_distillation[seed] = DataLoader(train_dataset, batch_size = best_config['batch_size'], shuffle=False)
    """

    train_loader = dataloader_train
    val_loader   = dataloader_val
    test_loader  = dataloader_test

    #############################################
    ### Training the model with the configuration
    #############################################
    #  we pass val data, so we get val loss        
    final_val_loss, teacher_model, y_true_val, y_pred_val, y_prob_val  = train_single_config(best_config, epochs, best_config['lr'], train_loader, val_loader, device)


    # Find the best thereshold
    best_val_threshold, max_val_f1 = find_best_threshold(y_true_val, y_prob_val)
    print(f"  [Seed {seed}] FInd best threshold in validation set: {best_val_threshold:.4f} (Val F1-C1: {max_val_f1*100:.1f}%)")

    # Save model and scaler for the current seed
    model_path  = os.path.join(out_dir, f'transformer_best_seed{seed}.pth')        

    torch.save(teacher_model.state_dict(), model_path)
    save_scalers(scalers, out_dir, '', seed)

    
    #models_by_seed.append(teacher_model)
    models_by_seed[seed] = teacher_model

    # Evaluate on test set
    teacher_model.eval()
    y_true_test = []
    y_prob_test = []
    y_pred_test = []

    with torch.no_grad():
        for batch in test_loader:                
            batch = move_batch_to_device(batch, device)

            logits      = teacher_model(batch)
            probs       = torch.sigmoid(logits)  # Convert logits to probabilities
            preds_class = (probs >= 0.5).float()

            y_prob_test.extend(probs.cpu().numpy())
            y_pred_test.extend(preds_class.cpu().numpy())
            y_true_test.extend(batch['y_true'].cpu().numpy())


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
        "val_loss": final_val_loss,
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
        "best_threshold_used": best_val_threshold, # Guardamos qué umbral se usó
        "val_loss": final_val_loss,
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

    if below_mask_test is not None:
        new_metrics.update(subset_metrics(y_true_test, y_prob_test, best_val_threshold, below_mask_test))

    results_all_seeds_best_threshold.append(new_metrics)


    del teacher_model
    del train_loader
    del test_loader
    del val_loader
    del train_dataset
    del test_dataset
    del val_dataset
    gc.collect()


    df_results = pd.DataFrame(results_all_seeds)
    df_results.to_csv('teacher_model_results.csv', index=False)

    df_results = pd.DataFrame(results_all_seeds_best_threshold)
    df_results.to_csv('teacher_model_results_best_threshold.csv', index=False)
    print("Results saved to 'teacher_model_results.csv'.")

    return models_by_seed, dict_train_dataloader_distillation
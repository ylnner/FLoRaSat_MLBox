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
import itertools
import gc
import copy

from MyCustomClasses import MyCustomDataset, MyCustomDistillationDataset, MyDistillationLoss

#from MyCustomDataset import 
#from MyCustomDataset import 
#from MyDistillationLoss import 


from StudentModel import StudentModel
from global_funcs import move_batch_to_device, find_best_threshold, scale_splits, save_scalers, set_seed, compute_pos_weight
import global_funcs

NUM_SEEDS =1 # 10
        

def train_single_config(epochs, small_config, train_loader, val_loader, alpha, temperature, device, is_kd = True,
                        patience = None):
    # patience (early stopping):
    #   None -> comportamiento de siempre: se entrenan las 'epochs' completas y se devuelve el modelo de la ULTIMA epoca.
    #   N    -> se detiene si val_loss no mejora durante N epocas seguidas y se devuelve el modelo de la MEJOR epoca
    #           (menor val_loss). 'epochs' pasa a ser el maximo de epocas.
    best_val_acc = 0.0
    avg_val_acc  = 0.0

    lr = small_config['lr']
    # pos_weight = #neg/#pos calculado sobre el train real (antes fijo a 81.38/18.62)
    pos_weight = compute_pos_weight(train_loader.dataset.data['y_true'].numpy(), device)
    # Create model 
    # The model does not include sigmoid at the end, so we can get the logits.
    # hidden_dims NO incluye la salida: StudentModel agrega un Linear(.,1) final sin activacion.
    model = StudentModel(n_features=8, hidden_dims=(64, 32), dropout=0.1).to(device)

    
    if is_kd:
        criterion_kd    = MyDistillationLoss(alpha = alpha, temperature = temperature, pos_weight = pos_weight)

    #pos_weight      = torch.tensor([81.38/18.62], device=device) # 18.62% vs 81.38%
    criterion_eval  = nn.BCEWithLogitsLoss(pos_weight = pos_weight)
    optimizer       = optim.Adam(model.parameters(), lr=lr)

    # [patience] estado del early stopping (solo se usa si patience no es None)
    best_val_loss     = float('inf')
    best_state        = None    # pesos de la mejor epoca
    best_outputs      = None    # (y_true, y_pred, y_prob) de validacion en la mejor epoca
    best_epoch        = 0
    epochs_no_improve = 0       # epocas seguidas sin mejorar val_loss

    for epoch in range(epochs):
        # Train
        model.train()
        train_loss      = 0.0
        correct_train   = 0
        total_train     = 0

        for batch in train_loader:
            batch = move_batch_to_device(batch, device)
            # Por defecto el modelo student no tiene sigmoid al final, 
            # asi que se el forward devuelve logits (soft labels)
            student_logits = model(batch)
            if is_kd:
                teacher_logits = batch.pop('teacher_logits')
                loss = criterion_kd(student_logits, teacher_logits, batch['y_true'])
            else:
                loss = criterion_eval(student_logits, batch['y_true'])


            # Clean gradient 
            optimizer.zero_grad()
            
            # Backpropagation
            loss.backward()

            # Update weights
            optimizer.step()

            # loss.item - The average of the batch losses will give you an estimate of the “epoch loss” during training. Returns the value of this tensor as a standard Python number 
            train_loss += loss.item() * batch['y_true'].size(0)

            # Aplica sigmoid para obtener probabilities
            predictions_prob = torch.sigmoid(student_logits)  # Convert logits to probabilities            
            predictions_class = (predictions_prob >= 0.5).float() # Convert to 0 - 1 the vector batch
            
            correct_train += (predictions_class == batch['y_true']).sum().item()
            total_train   += batch['y_true'].size(0)

        ##################### End for

        avg_train_loss  = train_loss / total_train
        train_acc       = correct_train / total_train

        # Validation
        model.eval()        
        val_loss    = 0.0  
        correct_val = 0
        total_val   = 0
        
        y_true_epoch = []
        y_prob_epoch = []
        y_pred_epoch = []

        with torch.no_grad():            
            for batch in val_loader:
                batch.pop('teacher_logits', None)
                batch = move_batch_to_device(batch, device)
                # Predict
                logits_val  = model(batch)
                loss        = criterion_eval(logits_val, batch['y_true'])
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

        # [patience] early stopping: guardar la mejor epoca y parar si no mejora en 'patience' epocas
        if patience is not None:
            if avg_val_loss < best_val_loss:
                best_val_loss     = avg_val_loss
                best_state        = copy.deepcopy(model.state_dict())   # deepcopy: si no, se sobrescribe al seguir entrenando
                best_outputs      = (y_true_epoch, y_pred_epoch, y_prob_epoch)
                best_epoch        = epoch + 1
                epochs_no_improve = 0
            else:
                epochs_no_improve += 1
                if epochs_no_improve >= patience:
                    print(f"  ** Early stopping en la epoca {epoch+1}: val_loss sin mejorar en {patience} epocas "
                          f"(mejor epoca {best_epoch}, val_loss {best_val_loss:.4f})")
                    break



    ##################### End for

    # [patience] devolver el modelo y las predicciones de validacion de la MEJOR epoca, no de la ultima
    if patience is not None and best_state is not None:
        model.load_state_dict(best_state)
        model.eval()
        avg_val_loss = best_val_loss
        y_true_epoch, y_pred_epoch, y_prob_epoch = best_outputs
        print(f"  ** Modelo restaurado a la mejor epoca: {best_epoch} (val_loss {best_val_loss:.4f})")

    return model, avg_val_loss, y_true_epoch, y_pred_epoch, y_prob_epoch



def run_full_train_student_no_kd(epochs, out_dir, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device):
    print(f"Using : {device}")  
    
    results_all_seeds = []
    results_all_seeds_best_threshold = []
    
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
        train_dataset   = MyCustomDataset(data = train_scaled)
        test_dataset    = MyCustomDataset(data = test_scaled)
        val_dataset     = MyCustomDataset(data = val_scaled)

        # Create Loaders
        set_seed(seed)
        train_loader = DataLoader(train_dataset, batch_size = small_config['batch_size'], shuffle=True)
        val_loader   = DataLoader(val_dataset, batch_size = small_config['batch_size'], shuffle=False)
        test_loader  = DataLoader(test_dataset, batch_size = small_config['batch_size'], shuffle=False)

        is_kd = False
        model, final_val_loss, y_true_val, y_pred_val, y_prob_val = train_single_config(epochs, small_config, train_loader, val_loader, None, None, device, is_kd)

        #del model

        ###### End seeds

        best_val_threshold, max_val_f1 = find_best_threshold(y_true_val, y_prob_val)
        print(f"  [Seed {seed}] FInd best threshold in validation set: {best_val_threshold:.4f} (Val F1-C1: {max_val_f1*100:.1f}%)")

        # Save model and scaler for the current seed
        model_path  = os.path.join(out_dir, f'student_no_kd_best_seed{seed}.pth')        

        torch.save(model.state_dict(), model_path)
        save_scalers(scalers, out_dir, 'student_no_kd_', seed)

        

        # Evaluate on test set
        model.eval()
        y_true_test = []
        y_prob_test = []
        y_pred_test = []

        with torch.no_grad():
            for batch in test_loader:                
                batch = move_batch_to_device(batch, device)

                logits      = model(batch)
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
        results_all_seeds_best_threshold.append(new_metrics)


        del model
        del train_loader
        del test_loader
        del val_loader
        del train_dataset
        del test_dataset
        del val_dataset
        gc.collect()

    df_results = pd.DataFrame(results_all_seeds)
    df_results.to_csv('student_no_kd_model_results.csv', index=False)

    df_results = pd.DataFrame(results_all_seeds_best_threshold)
    df_results.to_csv('student_no_kd_model_results_best_threshold.csv', index=False)
    print("Results saved to 'student_no_kd_model_results.csv'.")





def run_full_train_student_kd(epochs, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device, teacher_logits):
    print(f"Using : {device}")  

    gs_results       = []    
    best_gs_loss     = float('inf')
    best_temperature = None
    best_alpha       = None    


    param_grid = {        
        'alpha': [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9],
        'temperature': [2, 3, 4, 5]
    }

    # Generate the combination of parameters
    keys, values = zip(*param_grid.items())
    combinations = [dict(zip(keys, v)) for v in itertools.product(*values)]
    print(f"{len(combinations)} hyperparameter combinations to evaluate.")
    
    for i, config_dist in enumerate(combinations):
        print(f"---> Processing Combination {i+1}/{len(combinations)} ---")
        print(f"Configuration: {config_dist} - i: {i}")    

        alpha       = config_dist['alpha']
        temperature = config_dist['temperature']

        seed_losses = []
        for seed in range(NUM_SEEDS):
            print(f"-------> Processing Seed {seed+1}/{NUM_SEEDS} ---")

            print(f"  train: {len(dict_datasets_train[seed]['y_true'])} records")
            print(f"  val:   {len(dict_datasets_val[seed]['y_true'])} records")
            print(f"  test:  {len(dict_datasets_test[seed]['y_true'])} records")

            print("\n" + "=" * 80)
            print("Applying StandardScaler only in train data (fit only on valid / non-padded values)...")
            scalers, train_scaled, val_scaled, test_scaled = scale_splits(dict_datasets_train[seed], dict_datasets_val[seed], dict_datasets_test[seed])

            # Create Dataset. Los logits del maestro corresponden SOLO a train (mismo orden, sin shuffle).
            # Val no los necesita: la validacion usa BCE contra y_true.
            train_dataset   = MyCustomDistillationDataset(data = train_scaled, teacher_logits=teacher_logits[seed])
            val_dataset     = MyCustomDataset(data = val_scaled)

            # Create Loaders. El test NO se usa en la busqueda de alpha/temperature.
            set_seed(seed)
            train_loader = DataLoader(train_dataset, batch_size = small_config['batch_size'], shuffle=True)
            val_loader   = DataLoader(val_dataset, batch_size = small_config['batch_size'], shuffle=False)

            is_kd = True
            model, avg_val_loss, y_true_epoch, y_pred_epoch, y_prob_epoch = train_single_config(epochs, small_config, train_loader, val_loader, alpha, temperature, device, is_kd)

            del model


            print(f"-> Best Val Loss: {avg_val_loss:.4f}")
            seed_losses.append(avg_val_loss)

        ###### End seeds

        # Calculate mean and std of losses for the current combination considering all seeds
        mean_loss = float(np.mean(seed_losses))
        std_loss  = float(np.std(seed_losses))
        print(f"  => Mean val_loss for this combination: {mean_loss:.4f} ± {std_loss:.4f}")


        row = {
            **config_dist,
            'alpha': alpha, 
            'temperature': temperature,
            'mean_val_loss': mean_loss,
            'std_val_loss':  std_loss,
            **{f'val_loss_seed_{s}': seed_losses[s] for s in range(NUM_SEEDS)},
        }
        gs_results.append(row)


        if mean_loss < best_gs_loss:
            best_gs_loss = mean_loss
            #best_config  = config
            best_alpha = alpha
            best_temperature = temperature
            print(f"  ** New best combination of alpha/temperature (mean_val_loss={mean_loss:.4f})")


    # Create DataFrame with results and save to CSV
    pd.DataFrame(gs_results).to_csv('student_kd_gridsearch_results.csv', index=False)
    print(f"Best config: alpha={best_alpha}, temperature={best_temperature}  (mean val_loss={best_gs_loss:.4f})")
    print("Grid search results saved to 'student_kd_gridsearch_results.csv'.")

    return best_alpha, best_temperature, best_gs_loss


def train_and_test_with_best_config(epochs, out_dir, best_alpha, best_temperature, small_config, dict_datasets_train, dict_datasets_test, dict_datasets_val, device, teacher_logits):
    print("="*60)
    print("Training with best config founded ...")
    print("="*60)

    results_all_seeds = []
    results_all_seeds_best_threshold = []

    print(f"Using : {device}")  

    gs_results   = []    
    best_gs_loss = float('inf')
    seed_losses  = []
    for seed in range(NUM_SEEDS):
        print(f"-------> Processing Seed {seed+1}/{NUM_SEEDS} ---")

        print(f"  train: {len(dict_datasets_train[seed]['y_true'])} records")
        print(f"  val:   {len(dict_datasets_val[seed]['y_true'])} records")
        print(f"  test:  {len(dict_datasets_test[seed]['y_true'])} records")

        print("\n" + "=" * 80)
        print("Applying StandardScaler only in train data (fit only on valid / non-padded values)...")
        scalers, train_scaled, val_scaled, test_scaled = scale_splits(dict_datasets_train[seed], dict_datasets_val[seed], dict_datasets_test[seed])

        # Create Dataset. Los logits del maestro corresponden SOLO a train (mismo orden, sin shuffle).
        train_dataset   = MyCustomDistillationDataset(data = train_scaled, teacher_logits=teacher_logits[seed])
        test_dataset    = MyCustomDataset(data = test_scaled)
        val_dataset     = MyCustomDataset(data = val_scaled)

        # Create Loaders
        set_seed(seed)
        train_loader = DataLoader(train_dataset, batch_size = small_config['batch_size'], shuffle=True)
        val_loader   = DataLoader(val_dataset, batch_size = small_config['batch_size'], shuffle=False)
        test_loader  = DataLoader(test_dataset, batch_size = small_config['batch_size'], shuffle=False)

        is_kd = True
        model, final_val_loss, y_true_val, y_pred_val, y_prob_val  = train_single_config(epochs, small_config, train_loader, val_loader, best_alpha, best_temperature, device, is_kd)
        
        
        # Find the best thereshold
        best_val_threshold, max_val_f1 = find_best_threshold(y_true_val, y_prob_val)
        print(f"  [Seed {seed}] FInd best threshold in validation set: {best_val_threshold:.4f} (Val F1-C1: {max_val_f1*100:.1f}%)")

        # Save model and scaler for the current seed
        model_path  = os.path.join(out_dir, f'student_best_seed{seed}.pth')        

        torch.save(model.state_dict(), model_path)
        save_scalers(scalers, out_dir, 'student_kd_', seed)

        

        # Evaluate on test set
        model.eval()
        y_true_test = []
        y_prob_test = []
        y_pred_test = []

        with torch.no_grad():
            for batch in test_loader:                
                batch = move_batch_to_device(batch, device)

                logits      = model(batch)
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
        results_all_seeds_best_threshold.append(new_metrics)


        del model
        del train_loader
        del test_loader
        del val_loader
        del train_dataset
        del test_dataset
        del val_dataset
        gc.collect()

    df_results = pd.DataFrame(results_all_seeds)
    df_results.to_csv('student_model_results.csv', index=False)

    df_results = pd.DataFrame(results_all_seeds_best_threshold)
    df_results.to_csv('student_model_results_best_threshold.csv', index=False)
    print("Results saved to 'student_model_results.csv'.")


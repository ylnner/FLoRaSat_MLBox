
import pandas as pd
import os

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset
import time
import joblib
import gc
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, auc, precision_recall_curve, precision_score, recall_score, f1_score, roc_auc_score


#NUM_SEEDS = 10

def subset_metrics(y_true, y_prob, threshold, below_mask, prefix="test"):
    """
    Metricas por subconjunto. below_mask: array 0/1 alineado con y_true/y_prob.
    """
    y_true      = np.asarray(y_true).astype(int)
    y_prob      = np.asarray(y_prob)
    below_mask  = np.asarray(below_mask).astype(bool)
    y_pred      = (y_prob >= threshold).astype(int)
    out         = {}

    vis = ~below_mask
    if vis.any() and len(np.unique(y_true[vis])) == 2:
        p, r, _                          = precision_recall_curve(y_true[vis], y_prob[vis])
        out[f"{prefix}_visible_auc_roc"] = roc_auc_score(y_true[vis], y_prob[vis])
        out[f"{prefix}_visible_auc_pr"]  = auc(r, p)
        out[f"{prefix}_visible_f1_c1"]   = f1_score(y_true[vis], y_pred[vis], pos_label=1, zero_division=0)
        out[f"{prefix}_visible_rec_c1"]  = recall_score(y_true[vis], y_pred[vis], pos_label=1, zero_division=0)

    # Fuga: intentos bajo la mascara que el modelo dejaria transmitir (todos fallan seguro)
    out[f"{prefix}_belowmask_leak"] = float(y_pred[below_mask].mean()) if below_mask.any() else 0.0

    # Porcentaje de intentos evitados y % de exitos perdidos
    out[f"{prefix}_tx_avoided"]     = float((y_pred == 0).mean())
    out[f"{prefix}_success_lost"]   = float(((y_pred == 0) & (y_true == 1)).sum() / max((y_true == 1).sum(), 1))
    return out


def find_best_threshold(y_true_val, y_prob_val):
    # Find the best threshold that maximize the F1-Score for the possitive class
    precisions, recalls, thresholds = precision_recall_curve(y_true_val, y_prob_val)
    
    # Calculate F1-Score for each possible threshold
    f1_scores = (2 * precisions * recalls) / (precisions + recalls + 1e-10)
        
    best_idx = np.argmax(f1_scores)        
    best_th = thresholds[best_idx] if best_idx < len(thresholds) else thresholds[-1]
    best_f1 = f1_scores[best_idx]
    
    return best_th, best_f1


def parse_data(data):
    out = dict(data)
    for k in ["seq_feats", "static_feats", "own_tx", "interf_set", "context_feats"]:
        out[k] = out[k].astype(np.float32)

    for k in ["seq_mask", "interf_mask"]:
        out[k] = out[k].astype(np.int64)

    out["y_true"] = out["y_true"].astype(np.float32)

    if "link_ok" in out:
        out["link_ok"] = out["link_ok"].astype(bool)

    return out

def move_batch_to_device(batch, device):
    return {k: v.to(device) for k, v in batch.items()}


def transform_scaler_3d_data(data, scaler):
    original_shape = data.shape
    new_data = scaler.transform(data.reshape(-1, data.shape[-1]))
    return new_data.reshape(original_shape)


def create_sub_configs_dicts(global_config):
    #############################################
    ### Setting configs for Teacher Model
    #############################################

    # Global
    config_teacher  = {}
    config_teacher['out_dim']   = global_config['out_dim']
    config_teacher['dropout']   = global_config['dropout']
    config_teacher['lr']        = global_config['lr']
    config_teacher['d_model']   = global_config['d_model']

    #- Sequential
    config_seq                   = {}
    config_seq['d_model']        = global_config['d_model']
    config_seq['n_heads']        = global_config['n_heads']
    config_seq['n_layers']       = global_config['n_layers']      
    config_seq['dropout']        = global_config['dropout']
    config_seq['lr']        	 = global_config['lr']

    #- Staitc
    config_static   = {}
    config_static['static_hidden_dims'] = global_config['static_hidden_dims']
    config_static['dropout']            = global_config['dropout']
    config_static['lr']                 = global_config['lr']

    #- Set Transformer
    config_set_trf  = {}
    config_set_trf['d_model']           = global_config['d_model']
    config_set_trf['inter_in_dim']      = global_config['inter_in_dim']
    config_set_trf['num_sab_layers']    = global_config['num_sab_layers']
    config_set_trf['sab_nheads']        = global_config['sab_nheads']
    config_set_trf['sab_ln']            = global_config['sab_ln']
    config_set_trf['own_in_dim']        = global_config['own_in_dim']
    config_set_trf['cross_attn_nheads'] = global_config['cross_attn_nheads']    
    config_set_trf['cross_attn_ln']     = global_config['cross_attn_ln']
    config_set_trf['d_own']             = global_config['d_own']
    config_set_trf['dropout']           = global_config['dropout']
    config_set_trf['lr']                = global_config['lr']

    #- Contextual data
    config_context  = {}
    config_context['d_model']         = global_config['d_model']
    config_context['dropout']         = global_config['dropout']
    config_context['lr']              = global_config['lr']
    config_context['context_in_dim']  = global_config['context_in_dim']
    config_context['context_out_dim'] = global_config['context_out_dim']             

    return config_teacher, config_seq, config_static, config_set_trf, config_context


# ---------------------------------------------------------------------------
# Escalado con mascaras
# ---------------------------------------------------------------------------
# Los StandardScaler se ajustan solo con valores reales, quitando los valores de padding y sentinel. 
# Si se ajustan con todas las filas, el padding (ceros) y el sentinel -999 dominan la media/std: la P_rx real de own_tx terminaba
# con std ~0.008 tras escalar (practicamente constante). Despues de transformar, las posiciones
# invalidas (padding, interferentes enmascarados, link no viable) se ponen a 0 para que ningun
# valor anomalo (-999 escalado) fluya por la red, aunque el modelo igualmente las enmascare.

SEQ_SCALED_COLS      = [0, 1, 2]    # doppler, elevSat, delta_t. Col 3 (tiempo relativo) NO se escala: va en bruto al TimePositionalEncoding
CTX_ALWAYS_COLS      = [0, 1, 4, 5] # n_active, same_sf_concurrent, n_unique_devices, lambda_own_hz
CTX_POWER_COLS       = [2, 3]       # strongest, delta_p_strongest: solo existen si has_real_power == 1
CTX_POWER_FLAG_COL   = 6            # has_real_power (0/1), no se escala


def create_scalers(data):
    """Ajusta los scalers con los datos de TRAIN, usando solo las filas/posiciones validas."""
    # Crea una matriz indicando que valores son los que tienen == 1 en seq_masl, interf_mask. EN caso de link_ok se genera un vector. 
    # Esto se hace para evitar que el scaler considere los valores que han pasado por el padding.
    seq_valid    = data['seq_mask'] == 1                       # [N, S]
    interf_valid = data['interf_mask'] == 1                    # [N, n_max]
    link_ok      = data['link_ok'].astype(bool)                # [N]
    ctx_power_ok = data['context_feats'][:, CTX_POWER_FLAG_COL] == 1

    scalers = {
        'sequential':    StandardScaler().fit(data['seq_feats'][seq_valid][:, SEQ_SCALED_COLS]),
        'static':        StandardScaler().fit(data['static_feats']),
        'own_tx':        StandardScaler().fit(data['own_tx'][link_ok]),
        'interf_set':    StandardScaler().fit(data['interf_set'][interf_valid]),
        'context_feats': StandardScaler().fit(data['context_feats'][:, CTX_ALWAYS_COLS]),
        'context_power': StandardScaler().fit(data['context_feats'][ctx_power_ok][:, CTX_POWER_COLS]),
    }
    return scalers


def apply_scalers(data, scalers):
    """
    Devuele una copia escalada de lso datos. 
    """
    out = dict(data)

    # Secuencia: escala doppler/elevSat/delta_t, deja el tiempo relativo en bruto, anula el padding
    seq                         = data['seq_feats'].copy()
    seq[..., SEQ_SCALED_COLS]   = transform_scaler_3d_data(seq[..., SEQ_SCALED_COLS], scalers['sequential'])
    seq[data['seq_mask'] == 0]  = 0.0 
    out['seq_feats']            = seq.astype(np.float32)

    # Static
    out['static_feats'] = scalers['static'].transform(data['static_feats']).astype(np.float32)

    # own_tx: filas sin link (-999) las convierte a 0. 
    # Su salida se sustituye por emb_no_link_valid en el modelo
    own                                 = scalers['own_tx'].transform(data['own_tx'])
    own[~data['link_ok'].astype(bool)]  = 0.0   # [~ es la negacion: `~link_ok` vale True donde no hay link.
    out['own_tx']                       = own.astype(np.float32)

    # interf_set: slots de padding y de interferentes con P_rx=-999 (mask 0) -> 0
    interf                              = transform_scaler_3d_data(data['interf_set'], scalers['interf_set'])
    interf[data['interf_mask'] == 0]    = 0.0
    out['interf_set']                   = interf.astype(np.float32)

    # context: columnas de potencia solo escaladas donde existen, 0 donde el flag lo indica
    ctx                                     = data['context_feats'].copy()
    power_ok                                = ctx[:, CTX_POWER_FLAG_COL] == 1
    ctx[:, CTX_ALWAYS_COLS]                 = scalers['context_feats'].transform(ctx[:, CTX_ALWAYS_COLS])
    ctx[:, CTX_POWER_COLS]                  = scalers['context_power'].transform(ctx[:, CTX_POWER_COLS])
    ctx[np.ix_(~power_ok, CTX_POWER_COLS)]  = 0.0
    out['context_feats']                    = ctx.astype(np.float32)

    return out


def scale_splits(train_raw, val_raw, test_raw):
    """    
    Crea los escalares solo con la train data y aplica para train, val y test.
    """
    scalers = create_scalers(train_raw)
    return scalers, apply_scalers(train_raw, scalers), apply_scalers(val_raw, scalers), apply_scalers(test_raw, scalers)


def save_scalers(scalers, out_dir, prefix, seed):
    for name, scaler in scalers.items():
        joblib.dump(scaler, os.path.join(out_dir, f'{prefix}scaler_{name}_seed{seed}.pkl'))


def set_seed(seed):
    np.random.seed(seed)
    torch.manual_seed(seed)


def compute_pos_weight(y_true, device):
    """
    pos_weight = #negativos / #positivos calculado sobre el train real # 81.38/18.62
    """
    y = np.asarray(y_true)
    n_pos = float((y == 1).sum())
    n_neg = float((y == 0).sum())

    print('n_neg: ', n_neg)
    print('n_pos: ', n_pos)
    print('n_neg / n_pos: ', (n_neg / n_pos))
    return torch.tensor([n_neg / n_pos], dtype=torch.float32, device=device)

"""
Tareas individuales. Cada funcion entrena UNA combinacion (configuracion, seed) y guarda su
resultado en un JSON. Si el JSON ya existe, la tarea no hace nada (permite reanudar).

Se usan las funciones originales sin cambios:
    funcs_teacher_train.train_single_config   (maestro)
    func_student_train.train_single_config    (estudiante, con y sin KD)
y se reproduce la misma secuencia que el codigo original: scale_splits -> datasets ->
set_seed(seed) -> DataLoaders -> entrenamiento -> umbral en val -> metricas en test.
"""
import os
import re
import time
import contextlib

import numpy as np

import common  # agrega SCRIPTS_DIR al sys.path (debe ir antes de importar el codigo original)
from common import load_seed, phase_dir, write_json_atomic, auc_pr, test_metrics

import torch
from torch.utils.data import DataLoader
from sklearn.metrics import roc_auc_score

import funcs_teacher_train
import func_student_train
from global_funcs import scale_splits, save_scalers, set_seed, find_best_threshold, move_batch_to_device
from MyCustomClasses import MyCustomDataset, MyCustomDistillationDataset

_EPOCH_RE  = re.compile(r"Epoch (\d+)/\d+ \| Train Loss: ([-\d.naif]+) \| Val Loss: ([-\d.naif]+) \| Val Acc: ([-\d.naif]+)%")
_GLOBAL_RE = re.compile(r"AUC-ROC: ([-\d.naif]+) \| AUC-PR: ([-\d.naif]+)")


@contextlib.contextmanager
def log_to(path):
    """Todo lo que imprime el codigo original (metricas por epoca) va al log de la tarea."""
    with open(path, "w") as f, contextlib.redirect_stdout(f), contextlib.redirect_stderr(f):
        yield


def parse_epochs(log_path):
    """Extrae del log las metricas de validacion de cada epoca."""
    with open(log_path) as f:
        text = f.read()
    epochs = []
    for m, g in zip(_EPOCH_RE.finditer(text), _GLOBAL_RE.finditer(text)):
        epochs.append({"epoch": int(m.group(1)), "train_loss": float(m.group(2)),
                       "val_loss": float(m.group(3)), "val_acc": float(m.group(4)) / 100,
                       "val_auc_roc": float(g.group(1)), "val_auc_pr": float(g.group(2))})
    return epochs


def predict(model, loader, device):
    model.eval()
    y_true, y_prob = [], []
    with torch.no_grad():
        for batch in loader:
            batch.pop("teacher_logits", None)
            batch = move_batch_to_device(batch, device)
            y_prob.append(torch.sigmoid(model(batch)).cpu().numpy())
            y_true.append(batch["y_true"].cpu().numpy())
    return np.concatenate(y_true), np.concatenate(y_prob)


def final_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, below_mask_test=None):
    best_th, _ = find_best_threshold(y_true_val, y_prob_val)
    return {"threshold_0.5":  test_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, 0.5, below_mask_test),
            "best_threshold": test_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, best_th, below_mask_test)}


# ---------------------------------------------------------------------------
# Maestro: una configuracion del grid con una seed
# ---------------------------------------------------------------------------
def teacher_grid_task(phase, config_id, config, seed, epochs, device):
    out = os.path.join(phase_dir(phase, "runs"), f"cfg{config_id:04d}_seed{seed}.json")
    if os.path.exists(out):
        return out
    log = os.path.join(phase_dir(phase, "logs"), f"cfg{config_id:04d}_seed{seed}.log")
    t0 = time.time()

    train, val, test = load_seed(seed)
    _, train_s, val_s, _ = scale_splits(train, val, test)
    train_ds, val_ds = MyCustomDataset(train_s), MyCustomDataset(val_s)
    set_seed(seed)
    train_loader = DataLoader(train_ds, batch_size=config["batch_size"], shuffle=True)
    val_loader   = DataLoader(val_ds, batch_size=config["batch_size"], shuffle=False)

    with log_to(log):
        val_loss, model, y_true, _, y_prob = funcs_teacher_train.train_single_config(
            config, epochs, config["lr"], train_loader, val_loader, device)

    write_json_atomic(out, {
        "phase": phase, "config_id": config_id, "seed": seed, "config": config,
        "val_loss": val_loss, "val_auc_roc": roc_auc_score(y_true, y_prob), "val_auc_pr": auc_pr(y_true, y_prob),
        "epochs": parse_epochs(log), "seconds": time.time() - t0,
    })
    return out


# ---------------------------------------------------------------------------
# Maestro final: mejor configuracion, una seed. Guarda modelo, scalers y logits de train.
# ---------------------------------------------------------------------------
def teacher_final_task(config, seed, epochs, device):
    base = phase_dir("teacher_final")
    out = os.path.join(phase_dir("teacher_final", "runs"), f"seed{seed}.json")
    if os.path.exists(out):
        return out
    log = os.path.join(phase_dir("teacher_final", "logs"), f"seed{seed}.log")
    t0 = time.time()

    train, val, test = load_seed(seed)
    scalers, train_s, val_s, test_s = scale_splits(train, val, test)
    train_ds, val_ds, test_ds = MyCustomDataset(train_s), MyCustomDataset(val_s), MyCustomDataset(test_s)
    set_seed(seed)
    bs = config["batch_size"]
    train_loader   = DataLoader(train_ds, batch_size=bs, shuffle=True)
    val_loader     = DataLoader(val_ds, batch_size=bs, shuffle=False)
    test_loader    = DataLoader(test_ds, batch_size=bs, shuffle=False)
    distill_loader = DataLoader(train_ds, batch_size=bs, shuffle=False)   # mismo orden que el train

    with log_to(log):
        val_loss, model, y_true_val, _, y_prob_val = funcs_teacher_train.train_single_config(
            config, epochs, config["lr"], train_loader, val_loader, device)

    models_dir = phase_dir("teacher_final", "models")
    torch.save(model.state_dict(), os.path.join(models_dir, f"transformer_best_seed{seed}.pth"))
    save_scalers(scalers, models_dir, "", seed)

    # Logits del maestro sobre el train (sin shuffle): el logit i corresponde a la muestra i
    model.eval()
    with torch.no_grad():
        logits = torch.cat([model(move_batch_to_device(b, device)).cpu() for b in distill_loader])
    assert len(logits) == len(train["y_true"])
    torch.save(logits, os.path.join(phase_dir("teacher_final", "logits"), f"teacher_logits_seed{seed}.pt"))

    y_true_test, y_prob_test = predict(model, test_loader, device)
    write_json_atomic(out, {
        "seed": seed, "config": config, "val_loss": val_loss,
        **final_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, below_mask_test=test["meta_belowMask"]),
        "epochs": parse_epochs(log), "seconds": time.time() - t0,
    })
    return out


def _load_teacher_logits(seed):
    return torch.load(os.path.join(phase_dir("teacher_final", "logits"), f"teacher_logits_seed{seed}.pt"))


# ---------------------------------------------------------------------------
# Estudiante: una combinacion (alpha, T) con una seed
# ---------------------------------------------------------------------------
def student_grid_task(alpha, temperature, small_config, seed, epochs, device):
    name = f"a{alpha}_T{temperature}_seed{seed}"
    out = os.path.join(phase_dir("student_grid", "runs"), f"{name}.json")
    if os.path.exists(out):
        return out
    log = os.path.join(phase_dir("student_grid", "logs"), f"{name}.log")
    t0 = time.time()

    train, val, test = load_seed(seed)
    _, train_s, val_s, _ = scale_splits(train, val, test)
    train_ds = MyCustomDistillationDataset(data=train_s, teacher_logits=_load_teacher_logits(seed))
    val_ds   = MyCustomDataset(data=val_s)
    set_seed(seed)
    train_loader = DataLoader(train_ds, batch_size=small_config["batch_size"], shuffle=True)
    val_loader   = DataLoader(val_ds, batch_size=small_config["batch_size"], shuffle=False)

    with log_to(log):
        _, val_loss, y_true, _, y_prob = func_student_train.train_single_config(
            epochs, small_config, train_loader, val_loader, alpha, temperature, device, True)

    write_json_atomic(out, {
        "alpha": alpha, "temperature": temperature, "seed": seed,
        "val_loss": val_loss, "val_auc_roc": roc_auc_score(y_true, y_prob), "val_auc_pr": auc_pr(y_true, y_prob),
        "epochs": parse_epochs(log), "seconds": time.time() - t0,
    })
    return out


# ---------------------------------------------------------------------------
# Estudiante final: con KD (mejor alpha/T) o sin KD, una seed, evaluado en test
# ---------------------------------------------------------------------------
def student_final_task(kind, alpha, temperature, small_config, seed, epochs, device):
    assert kind in ("kd", "no_kd")
    out = os.path.join(phase_dir("student_final", "runs"), f"{kind}_seed{seed}.json")
    if os.path.exists(out):
        return out
    log = os.path.join(phase_dir("student_final", "logs"), f"{kind}_seed{seed}.log")
    t0 = time.time()

    train, val, test = load_seed(seed)
    scalers, train_s, val_s, test_s = scale_splits(train, val, test)
    if kind == "kd":
        train_ds = MyCustomDistillationDataset(data=train_s, teacher_logits=_load_teacher_logits(seed))
    else:
        train_ds = MyCustomDataset(data=train_s)
    val_ds, test_ds = MyCustomDataset(data=val_s), MyCustomDataset(data=test_s)
    set_seed(seed)
    bs = small_config["batch_size"]
    train_loader = DataLoader(train_ds, batch_size=bs, shuffle=True)
    val_loader   = DataLoader(val_ds, batch_size=bs, shuffle=False)
    test_loader  = DataLoader(test_ds, batch_size=bs, shuffle=False)

    with log_to(log):
        model, val_loss, y_true_val, _, y_prob_val = func_student_train.train_single_config(
            epochs, small_config, train_loader, val_loader,
            alpha if kind == "kd" else None, temperature if kind == "kd" else None, device, kind == "kd")

    models_dir = phase_dir("student_final", "models")
    prefix = "student_kd_" if kind == "kd" else "student_no_kd_"
    model_name = f"student_best_seed{seed}.pth" if kind == "kd" else f"student_no_kd_best_seed{seed}.pth"
    torch.save(model.state_dict(), os.path.join(models_dir, model_name))
    save_scalers(scalers, models_dir, prefix, seed)

    y_true_test, y_prob_test = predict(model, test_loader, device)
    write_json_atomic(out, {
        "kind": kind, "alpha": alpha, "temperature": temperature, "seed": seed, "val_loss": val_loss,
        **final_metrics(y_true_val, y_prob_val, y_true_test, y_prob_test, below_mask_test=test["meta_belowMask"]),
        "epochs": parse_epochs(log), "seconds": time.time() - t0,
    })
    return out

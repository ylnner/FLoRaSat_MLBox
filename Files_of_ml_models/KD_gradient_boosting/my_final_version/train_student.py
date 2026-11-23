"""
(MLP 8-64-32-1, el mismo que se despliega en FLoRaSat) con la perdida secuencial por mensaje.

  - Perdida = SequentialMessageLoss (scripts/MyCustomClasses.py): maximiza la probabilidad de entregar el mensaje con
    la 1.a transmision, menos mu por transmision esperada. Sin pos_weight ni pesos 1/k.
  - KD: la etiqueta suave del maestro entra como probabilidad de entrega: target = alpha * y' + (1 - alpha) * sigmoid(z_T / T).
  - Umbral fijo 0.5 y como mucho 1 transmision por mensaje (DEPLOY_THRESHOLD, DEPLOY_BUDGET): es la regla del C++ con
    threshold = 0.5 y stopTarget = 0.01. Ya no se busca un umbral por F1.
  - Early stopping y seleccion por mensajes unicos de validacion, medidos con la replica del receptor
    (scripts/network_receiver.py) sobre la red de validacion con la regla de despliegue.


python train_student.py --N 300 --workers 20
"""
import os
for _v in ("OMP_NUM_THREADS", "MKL_NUM_THREADS", "OPENBLAS_NUM_THREADS"):
    os.environ.setdefault(_v, "1")

import argparse
import concurrent.futures as cf
import copy
import glob
import json
import multiprocessing as mp
import time

import numpy as np
import pandas as pd
import torch
from sklearn.metrics import average_precision_score, roc_auc_score
from sklearn.preprocessing import StandardScaler
from torch.utils.data import DataLoader, BatchSampler, RandomSampler

from config import (ALPHAS, TEMPS, MUS, BATCH_MESSAGES, LR, MAX_EPOCHS, NUM_WORKERS, PATIENCE, SEEDS, NTX,
                    DEPLOY_THRESHOLD, DEPLOY_BUDGET)
from common import results_dir, load_split, write_json_atomic

from funcs_create_dataset import STUDENT_INPUTS
from network_receiver import evaluate_policy, sequential_budget, received, unique_delivered

from StudentModel import StudentModel
from MyCustomClasses import SequentialMessageLoss, MyCustomMessageDataset, MyCustomMessageDistillationDataset

# Referencias offline que se calculan en cada split (mismas filas y misma replica): candidata = toda fila visible,
# como mucho k copias por mensaje. FV_1 es la referencia principal (la mejor en closed-loop con N = 300).
REFERENCES = {"fv1": 1, "fv2": 2, "rep": NTX}


def set_seed(seed):
    np.random.seed(seed)
    torch.manual_seed(seed)


def load_runs(str_N):
    return pd.DataFrame([json.load(open(f)) for f in glob.glob(os.path.join(results_dir(str_N, "student", "runs"), "*.json"))])

def run_name(teacher, mu, alpha, temp, seed):
    return f"{teacher}_mu{mu:g}_a{alpha:g}_T{temp:g}_s{seed}"


def message_tensor(slot_row, values, fill=0.0):
    """
    Pasa un array por fila (n_filas, ...) a uno por mensaje (mensajes, NTX, ...) con slot_row.
    Las oportunidades que no existen (slot_row = -1) quedan con `fill`.

    En caso de y_net, se tiene asi:
        Un array por filas, por transmision:
        y_net = [0, 1, 0, 1, 1], q corresponde msgs (B, A, A, B, A)
        
        Un array por mensaje
        Una fila de la matriz por mensaje. Dentro de ella, sus oportunidades van en columnas
                    1era  2da   3era   oportunidad
        y_msgs = [[ 0,    1,    1 ],   ← mensaje A
                  [ 1,    0,    0 ]]   ← mensaje B (la 3era no existe, es relleno)

        Entonces, es super rapido acceder a primera oportunidad visible de cada mesaje: y_msgs[:, 0]
    En caso de x es similar. 

    Es necesario por que El loss secuencial razona por mensaje: 
        transmite en la 1era con probabilidad p_1; si no, en la 2da con p_2(1-p_1); 
        
        Para calcularlo de forma vectorizada necesita tener juntas, y en orden, las oportunidades de cada mensaje. 
        Eso es el formato [mensajes, NTX].
    
    """
    # slot_row: matriz [mensajes × NTX], con NTX = 5
    # Guarda el indice de la priemra transmision visible de ese mensaje

    # Hay transmisiones visibles, por default slot_row -1
    valid = slot_row >= 0
    # Reemplaza donde no hay atrnasisiones visibles por 0
    safe  = np.where(valid, slot_row, 0)
    # Crea un copia de values donde haya transmisiones visibles
    out   = values[safe].copy()
    # El retaste de valid lo llena de fill
    out[~valid] = fill
    return out


def predict(model, x_rows):
    """
    Run the model sobre todas las filas (ya escaladas) de un split y devuelve p = sigmoid(logit) por fila.
    sin DataLoader: el MLP es pequeno y un solo forward sobre todas las filas es mas rapido que recorrer batches.
    """
    model.eval()
    with torch.no_grad():
        logits = model(torch.as_tensor(x_rows, dtype=torch.float32))
    return torch.sigmoid(logits).numpy()


def deploy_unique(data, p):
    """
    Mensajes unicos y transmisiones con la regla de despliegue (p >= 0.5, como mucho 1 copia por mensaje).
    Version rapida de evaluate_policy  para el early stopping de cada epoca.

    Ejecuciones mas rapidas.

    Cuenta cuantos mensajes unicos llegan y cuantas transmisiones gasto? 
    Lo hace sobre validacion usando la replica del receptor, sin simular.
    """
    tx = sequential_budget(data["slot_row"], p >= DEPLOY_THRESHOLD, DEPLOY_BUDGET)
    ok = received(data["edge_victim"], data["edge_killer"], data["receivable"], tx)
    return unique_delivered(ok, data["msg_index"]), int(tx.sum())


def references(data, split):
    """Mensajes unicos y transmisiones de FV_1, FV_2 y REP (las 5 copias visibles) en un split."""
    out = {}
    every_row = np.ones(len(data["y_net"]), dtype=bool)
    for ref, budget in REFERENCES.items():
        unique, tx, by_sim = evaluate_policy(data, every_row, budget)
        out[f"{split}_{ref}_unique"] = unique
        out[f"{split}_{ref}_tx"] = tx
        out[f"{split}_{ref}_unique_by_sim"] = by_sim
    return out


def train_student(train_loader, val, val_scaled, is_kd, alpha, temperature, mu):
    """
    Usa early stopping sobre los mensajes unicos de validacion con la regla de despliegue (replica del receptor).
    Empate en unicos -> se prefiere la epoca con menos transmisiones.
    Return model, best_val (unique, tx), best_epoch
    """

    # Model, losses and optimizer
    model     = StudentModel()
    loss_fn   = SequentialMessageLoss(alpha, temperature, mu)
    optimizer = torch.optim.Adam(model.parameters(), lr=LR)

    best_val          = (-1, 0)
    best_state        = None
    best_epoch        = 0
    epochs_no_improve = 0

    for epoch in range(1, MAX_EPOCHS + 1):
        # Train
        model.train()

        for batch in train_loader:
            x_batch        = batch["x"]                     # [mensajes, NTX, 7]
            y_batch        = batch["y_net"]                 # [mensajes, NTX]
            mask_batch     = batch["mask"]                  # [mensajes, NTX]
            student_logits = model(x_batch)                 # [mensajes, NTX]: el MLP se aplica a cada oportunidad

            if is_kd:
                teacher_batch = batch["teacher_logits"]
                loss = loss_fn(student_logits, y_batch, mask_batch, teacher_batch)
            else:
                loss = loss_fn(student_logits, y_batch, mask_batch)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

        # Validation
        # ahora el early stopping se hace por unique messages delivered
        val_unique, val_tx = deploy_unique(val, predict(model, val_scaled))

        # Early stopping
        better = val_unique > best_val[0] or (val_unique == best_val[0] and val_tx < best_val[1])
        if better:
            best_val          = (val_unique, val_tx)
            best_state        = copy.deepcopy(model.state_dict())
            best_epoch        = epoch
            epochs_no_improve = 0
        else:
            epochs_no_improve += 1
            if epochs_no_improve >= PATIENCE:
                break

    model.load_state_dict(best_state)
    model.eval()
    return model, best_val, best_epoch


def policy_diagnostics(data, p, prefix):
    """
    Diagnostico de la politica desplegada en un split:
        silent_frac   mensajes sin ninguna transmision (ninguna oportunidad con p >= 0.5)
        waited_frac   de los mensajes transmitidos, los que NO usan su primera oportunidad visible (lo que FV_1 no hace)
        ap / auc      discriminacion de p frente a y'
        mean_p        media de p frente a mean_y_net (calibracion global)
    """
    tx = sequential_budget(data["slot_row"], p >= DEPLOY_THRESHOLD, DEPLOY_BUDGET)
    sent_by_slot = message_tensor(data["slot_row"], tx, fill=False)          # [mensajes, NTX]
    sent_msgs    = sent_by_slot.any(axis=1)
    waited       = sent_msgs & ~sent_by_slot[:, 0]
    return {f"{prefix}_silent_frac": float(1.0 - sent_msgs.mean()),
            f"{prefix}_waited_frac": float(waited.sum() / max(sent_msgs.sum(), 1)),
            f"{prefix}_ap":          float(average_precision_score(data["y_net"], p)),
            f"{prefix}_auc":         float(roc_auc_score(data["y_net"], p)),
            f"{prefix}_mean_p":      float(p.mean()),
            f"{prefix}_mean_y_net":  float(data["y_net"].mean())}


def train_one(teacher, mu, alpha, temp, seed, str_N):
    #   MUS     coste por transmision esperada (mu = 0: solo maximiza mensajes entregados) (0.0, 0/1, 0.2)

    mu, alpha, temp, seed, = float(mu), float(alpha), float(temp), int(seed)
    # Obtiene nombre ed la run
    name = run_name(teacher, mu, alpha, temp, seed)

    out = os.path.join(results_dir(str_N, "student", "runs"), name + ".json")
    if os.path.exists(out):
        return
    torch.set_num_threads(1)
    t0 = time.time()

    is_kd = teacher != "no_kd"

    # v3: la cache solo tiene filas visibles (elevSat >= 10)
    train = load_split(seed, "train", str_N)
    val   = load_split(seed, "val", str_N)
    test  = load_split(seed, "test", str_N)

    # El scaler se ajusta con todas las filas (visibles) de train
    scaler       = StandardScaler().fit(train["X_student"])
    train_scaled = scaler.transform(train["X_student"]).astype(np.float32)
    val_scaled   = scaler.transform(val["X_student"]).astype(np.float32)
    test_scaled  = scaler.transform(test["X_student"]).astype(np.float32)

    # Tensores por mensaje del train: [mensajes, NTX, ...] con slot_row (oportunidades en orden de tiempo)
    x_msgs    = message_tensor(train["slot_row"], train_scaled)
    y_msgs    = message_tensor(train["slot_row"], train["y_net"])
    mask_msgs = (train["slot_row"] >= 0).astype(np.float32)

    # Create Dataset. Los logits del maestro corresponden SOLO a train (mismo orden que la cache)
    if is_kd:
        teacher_logits = torch.load(os.path.join(results_dir(str_N, f"teacher_gradient_boosting", "logits"), f"teacher_logits_seed{seed}.pt"))

        assert len(teacher_logits) == len(train["y_net"]), "los logits no corresponden al train de esta seed"
        z_msgs        = message_tensor(train["slot_row"], teacher_logits.numpy())
        train_dataset = MyCustomMessageDistillationDataset(x_msgs, y_msgs, mask_msgs, z_msgs)
    else:
        train_dataset = MyCustomMessageDataset(x_msgs, y_msgs, mask_msgs)


    # Create Loaders. set_seed con el mismo valor para no_kd y KD: mismos pesos iniciales y mismo orden de batches.
    # BatchSampler(RandomSampler) entrega al dataset una LISTA de indices por batch: __getitem__ indexa los tensores
    # de una vez (x[idx] con idx = lista) en lugar de llamar 256 veces por batch y apilar. 
    # 
    # batch_size=None le dice al DataLoader que el item ya es el batch completo. Mismo resultado que shuffle=True, bastante mas rapido.
    # El sampler lleva su propio generador (semilla fija) para que el orden de batches no dependa del generador global.

    set_seed(seed)
    sampler      = BatchSampler(RandomSampler(train_dataset, generator=torch.Generator().manual_seed(seed)),
                                batch_size=BATCH_MESSAGES, drop_last=False)
    train_loader = DataLoader(train_dataset, sampler=sampler, batch_size=None)

    # aqui ya ha iterado sobre epochs
    model, (val_unique, val_tx), best_epoch = train_student(train_loader, val, val_scaled, is_kd, alpha, temp, mu)

    # Predict val y test con el mejor modelo
    p_val  = predict(model, val_scaled)
    p_test = predict(model, test_scaled)

    # Regla de despliegue (la misma que el C++ con threshold = 0.5 y stopTarget = 0.01):
    # transmitir en la primera oportunidad visible con p >= DEPLOY_THRESHOLD, como mucho DEPLOY_BUDGET copias.
    #DEPLOY_THRESHOLD = 0.5
    #DEPLOY_BUDGET    = 1
    test_unique, test_tx, test_by_sim = evaluate_policy(test, p_test >= DEPLOY_THRESHOLD, DEPLOY_BUDGET)

    r = dict(teacher = teacher,
             mu      = mu,
             alpha   = alpha,
             temp    = temp,
             seed    = seed,
             n_train_msgs = int(len(train["slot_row"])),
             n_train_rows = int(len(train["y_net"])),
             threshold    = DEPLOY_THRESHOLD,
             budget       = DEPLOY_BUDGET,
             best_epoch   = best_epoch,

             val_unique   = int(val_unique),           # criterio de early stopping y de seleccion
             val_tx       = int(val_tx),
             test_unique  = int(test_unique),          # metrica principal
             test_tx      = int(test_tx),
             test_unique_by_sim = test_by_sim,         # para comparar pareado por simulacion
             )
    r.update(references(val, "val"))
    r.update(references(test, "test"))
    # Ganancia en % frente a FV_1 (misma red, mismas filas)
    r["val_gain_fv1_pct"]  = 100.0 * (r["val_unique"] - r["val_fv1_unique"]) / r["val_fv1_unique"]
    r["test_gain_fv1_pct"] = 100.0 * (r["test_unique"] - r["test_fv1_unique"]) / r["test_fv1_unique"]
    r.update(policy_diagnostics(val, p_val, "val"))
    r.update(policy_diagnostics(test, p_test, "test"))
    r["secs"] = time.time() - t0


    torch.save(model.state_dict(), os.path.join(results_dir(str_N, "student", "models"), name + ".pth"))
    write_json_atomic(os.path.join(results_dir(str_N, "student", "scalers"), name + ".json"),
                      {"inputs": STUDENT_INPUTS, "mean": scaler.mean_.tolist(), "scale": scaler.scale_.tolist(),
                       "threshold": DEPLOY_THRESHOLD,
                       "uso": "x_esc = (x - mean) / scale; p = sigmoid(MLP(x_esc)); transmitir si p >= threshold y "
                              "visible; regla de parada con stopTarget = 0.01 (como mucho 1 copia por mensaje)"})
    write_json_atomic(out, r)


def select_best_configs(runs):
    """
    Mejor configuracion por brazo (no_kd / KD): mayor media entre seeds de val_gain_fv1_pct (mensajes unicos de
    validacion frente a FV_1, en %); empate -> menos transmisiones de validacion.
    Solo se consideran combinaciones con todas las seeds terminadas.
    Se elige con validacion; el test solo se reporta.
    """
    n_seeds = runs["seed"].nunique()

    # Media y desviacion por (brazo, mu, alpha, temperatura)
    grid = (runs.groupby(["teacher", "mu", "alpha", "temp"])
                .agg(n_seeds            = ("seed", "nunique"),
                     val_gain_fv1_pct   = ("val_gain_fv1_pct", "mean"),
                     val_gain_std       = ("val_gain_fv1_pct", "std"),
                     val_tx             = ("val_tx", "mean"),
                     val_fv1_tx         = ("val_fv1_tx", "mean"),
                     test_gain_fv1_pct  = ("test_gain_fv1_pct", "mean"),     # metrica principal
                     test_gain_std      = ("test_gain_fv1_pct", "std"),
                     test_unique        = ("test_unique", "mean"),
                     test_tx            = ("test_tx", "mean"),
                     test_waited_frac   = ("test_waited_frac", "mean"),
                     test_silent_frac   = ("test_silent_frac", "mean"),
                     best_epoch         = ("best_epoch", "mean"))
                .reset_index())

    complete_grid = grid[grid["n_seeds"] == n_seeds]

    # Por brazo, la fila con mayor val_gain_fv1_pct (empate: menos val_tx). sort_values con dos claves y despues
    # drop_duplicates deja la primera fila de cada brazo, que es la mejor.
    ordered = complete_grid.sort_values(["val_gain_fv1_pct", "val_tx"], ascending=[False, True])
    best    = ordered.drop_duplicates("teacher").sort_values("teacher").reset_index(drop=True)
    return grid, best


def compare_with_no_kd(runs, best):
    """
    KD (su mejor configuracion) frente a noKD (la suya) en test, pareado: misma seed y misma simulacion.
        por seed:        diferencia de mensajes unicos de test en cada una de las 10 seeds
        por simulacion:  diferencia en cada simulacion de test de cada seed (n = seeds x simulaciones)
    Devuelve una fila por nivel con la media de la diferencia, su error estandar y cuantas diferencias son > 0.
    """
    picks = {}
    for _, b in best.iterrows():
        same = (runs["teacher"] == b["teacher"]) & np.isclose(runs["mu"], b["mu"]) & \
               np.isclose(runs["alpha"], b["alpha"]) & np.isclose(runs["temp"], b["temp"])
        picks[b["teacher"]] = runs[same].set_index("seed").sort_index()

    kd    = picks["student"]
    no_kd = picks["no_kd"]

    seed_diff = (kd["test_unique"] - no_kd["test_unique"]).to_numpy(float)

    sim_diff = []
    for seed in kd.index:
        kd_sims    = kd.loc[seed, "test_unique_by_sim"]
        no_kd_sims = no_kd.loc[seed, "test_unique_by_sim"]
        for sim in kd_sims:
            sim_diff.append(kd_sims[sim] - no_kd_sims[sim])
    sim_diff = np.array(sim_diff, dtype=float)

    rows = []
    for level, diff in (("seed", seed_diff), ("simulacion", sim_diff)):
        rows.append(dict(nivel       = level,
                         n           = len(diff),
                         kd_minus_no_kd_mean = diff.mean(),
                         se          = diff.std(ddof=1) / np.sqrt(len(diff)),
                         n_positive  = int((diff > 0).sum()),
                         n_negative  = int((diff < 0).sum())))
    return pd.DataFrame(rows)


def summarize(str_N):
    runs        = load_runs(str_N)
    grid, best  = select_best_configs(runs)

    out_dir = results_dir(str_N, "student")
    grid.to_csv(os.path.join(out_dir, "grid_mu_alpha_temp.csv"), index=False)   # todas las combinaciones
    best.to_csv(os.path.join(out_dir, "best_config.csv"), index=False)          # la mejor por brazo

    print(f"\nMejor configuracion por brazo (criterio: media de val_gain_fv1_pct, empate -> menos val_tx)")
    for _, row in best.iterrows():
        print(f"  {row['teacher']:7s}: mu={row['mu']:g}  alpha={row['alpha']:g}  T={row['temp']:g}  "
              f"val {row['val_gain_fv1_pct']:+.2f} % vs FV_1 | test {row['test_gain_fv1_pct']:+.2f} % "
              f"(std {row['test_gain_std']:.2f}) | tx test {row['test_tx']:.0f} | "
              f"esperan {100 * row['test_waited_frac']:.1f} % | callan {100 * row['test_silent_frac']:.1f} %")
    print(f"guardado en {out_dir}/best_config.csv y grid_mu_alpha_temp.csv", flush=True)

    if set(best["teacher"]) == {"student", "no_kd"}:
        comparison = compare_with_no_kd(runs, best)
        comparison.to_csv(os.path.join(out_dir, "best_kd_vs_no_kd.csv"), index=False)
        print(f"\nKD - noKD en test (mensajes unicos, pareado)")
        for _, row in comparison.iterrows():
            print(f"  por {row['nivel']:10s} (n = {row['n']:4d}): {row['kd_minus_no_kd_mean']:+.2f} "
                  f"+- {row['se']:.2f} (SE) | > 0 en {row['n_positive']}, < 0 en {row['n_negative']}")
        print(f"guardado en {out_dir}/best_kd_vs_no_kd.csv", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--N", type = int, default = 300)
    ap.add_argument("--workers", type=int, default=NUM_WORKERS)
    args = ap.parse_args()
    str_N = "N_" + str(args.N)
    print('str_N: ', str_N)

    # Cada task es
    # modelo -> no_kd, student
    # mu -> coste por transmision esperada
    # a -> alpha (no_kd: 1, solo y')
    # t -> temperature (no_kd: 0, no se usa)
    # s -> seed
    tasks  = [("no_kd", m, 1, 0, s, str_N) for m in MUS for s in SEEDS]
    tasks += [("student", m, a, t, s, str_N) for m in MUS for a in ALPHAS for t in TEMPS for s in SEEDS]

    print(len(tasks), "tareas", flush=True)
    t0, done = time.time(), 0
    with cf.ProcessPoolExecutor(args.workers, mp_context=mp.get_context("spawn")) as pool:
        for fut in cf.as_completed([pool.submit(train_one, *t) for t in tasks]):
            fut.result()
            done += 1
            if done % 10 == 0:
                print(f"{done}/{len(tasks)} | {time.time() - t0:.0f} s", flush=True)
    print(f"fin : {time.time() - t0:.0f} s", flush=True)

    summarize(str_N)

if __name__ == "__main__":
    main()

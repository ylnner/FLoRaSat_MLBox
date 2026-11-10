"""
Configuracion de la ejecucion paralela: rutas, seeds, epocas y grids de hiperparametros.

Las rutas se pueden cambiar con variables de entorno:
    KD_SCRIPTS_DIR   carpeta con el codigo original (scripts/)
    KD_DATA_DIR      carpeta con los CSV seed_k_final_{train,val,test}_data_transmission.csv
    KD_CACHE_DIR     donde se guardan los datasets ya construidos (.npz)
    KD_RESULTS_DIR   donde se guardan resultados, logs, modelos y logits
"""
import os

ROOT        = os.path.dirname(os.path.abspath(__file__))
PROJECT     = os.path.dirname(ROOT)                                     # KD_new_model/
SCRIPTS_DIR = os.environ.get('KD_SCRIPTS_DIR', os.path.join(PROJECT, 'scripts'))
DATA_DIR    = os.environ.get('KD_DATA_DIR', os.path.join(PROJECT, 'data', 'repetitions', 'new_data'))
CACHE_DIR   = os.environ.get('KD_CACHE_DIR', os.path.join(ROOT, 'cache'))
RESULTS_DIR = os.environ.get('KD_RESULTS_DIR', os.path.join(ROOT, 'results'))

SEEDS  = list(range(10))
EPOCHS = 20
CAUSAL = False


# Criterio para elegir la mejor configuracion (media entre seeds).
# 'val_loss' es el criterio del codigo original (menor es mejor).
# Alternativas: 'val_auc_pr' o 'val_auc_roc' (mayor es mejor).
SELECTION_METRIC = 'auc_pr'

# ---------------------------------------------------------------- lista N
NSs = ["N_60", "N_100", "N_200", "N_300", "N_400", "N_500"]

# ---------------------------------------------------------------- maestro
TEACHER_FOLDS = 5          # validacion cruzada por simulacion (logits out-of-fold)
TEACHER_ITERS = 300        # maximos arboles del HistGradientBoostingClassifier
LOGIT_CLIP    = 1e-4       # p se recorta a [1e-4, 1 - 1e-4] antes de pasar a logit

EVAL_SUBSET  = os.environ.get('KD_EVAL_SUBSET', 'all')
THRESHOLD_METRIC = os.environ.get('KD_THRESHOLD_METRIC', 'f1_c1') # otro valor es macro_f1


# ---------------------------------------------------------------- estudiante (igual que run_train_student.py)
STUDENT_HIDDEN  = (64, 32)
STUDENT_DROPOUT = 0.1
LR              = 1e-3
BATCH_SIZE      = 32
PATIENCE        = 30       # early stopping sobre AP de val 
BASE_EPOCHS     = 20       # maximo de epocas = ceil(BASE_EPOCHS / fraccion)

FRACTIONS = [0.05, 0.10, 0.15, 1.0]
ALPHAS    = (0, 0.1, 0.2, 0.3, 0.5) # alpha for KD
TEMPS     = (0.5, 1, 2, 4, 8)          # temperature for JD

#FRACTIONS = [0.02]
#ALPHAS    = (0.1, 0.3, 0.5) # alpha for KD
#TEMPS     = (1, 2)          # temperature for JD


NUM_WORKERS = 20           # procesos en paralelo (la maquina tiene 22 hilos)


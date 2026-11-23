"""
Configuracion de la ejecucion paralela: rutas, seeds, epocas y grids de hiperparametros.

todas las rutas se fijan aqui (ya no hay variables de entorno). Solo se entrena con el 100 % del train.
Para usar otro dataset nuevo basta con cambiar DATA_DIR.
"""
import os

# ROOT = esta carpeta (my_final_version) y PROJECT = v3_sequential, calculadas desde la ubicacion de config.py:
ROOT        = os.path.dirname(os.path.abspath(__file__))
PROJECT     = os.path.dirname(ROOT)
SCRIPTS_DIR = os.path.join(PROJECT, "scripts")

# Carpeta con los CSV seed_k_final_{train,val,test}_data_transmission.csv de N = 300
DATA_DIR    = "/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/Files_of_ml_models/KD_gradient_boosting/new_data"

CACHE_DIR   = os.path.join(ROOT, "cache")       # datasets ya construidos (.npz), solo filas visibles
RESULTS_DIR = os.path.join(ROOT, "results")     # resultados, logs, modelos y logits

SEEDS  = list(range(10))

# ---------------------------------------------------------------- dataset / receptor
# Copias por mensaje en el dataset (Ntx del .ini). Es el numero maximo de oportunidades de un mensaje.
NTX = 5

# ---------------------------------------------------------------- maestro
TEACHER_FOLDS = 5          # validacion cruzada por simulacion (logits out-of-fold)
TEACHER_ITERS = 300        # maximos arboles del HistGradientBoostingClassifier


# ---------------------------------------------------------------- estudiante
STUDENT_HIDDEN  = (64, 32)
STUDENT_DROPOUT = 0.1
LR              = 1e-3
BATCH_MESSAGES  = 256      # v3: el batch se cuenta en mensajes (cada uno con hasta NTX oportunidades)
MAX_EPOCHS      = 200
PATIENCE        = 30       # early stopping sobre los mensajes unicos de validacion (replica del receptor)

# Regla de despliegue (la misma que el C++ con threshold = 0.5 y stopTarget = 0.01):
# transmitir en la primera oportunidad visible con p >= DEPLOY_THRESHOLD, como mucho DEPLOY_BUDGET copias.
DEPLOY_THRESHOLD = 0.5
DEPLOY_BUDGET    = 1

# Grid de la perdida secuencial:
#   MUS     coste por transmision esperada (mu = 0: solo maximiza mensajes entregados)
#   ALPHAS  peso de y' frente a la etiqueta suave del maestro (solo KD; noKD usa alpha = 1)
#   TEMPS   temperatura de la etiqueta suave del maestro (solo KD)
MUS    = (0.0, 0.1, 0.2)
ALPHAS = (0.3, 0.6)
TEMPS  = (1, 2)

NUM_WORKERS = 20           # procesos en paralelo (la maquina tiene 22 hilos)

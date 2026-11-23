import numpy as np
import pandas as pd



# Entradas del estudiante, en el orden en que entran al MLP (= run_train_student.STUDENT_INPUTS,
# = orden del JSON de scalers que lee el simulador)
STUDENT_INPUTS  = ["dopplerShift", "elevSat", "latDev", "longDev", "loraTP", "loraSF", "alt", "raan"]
#GEO_COLS        = ["g_near_dopp", "g_near_dopp_load", "g_kill", "g_kill_load", "g_up"]
GEO_COLS        = ["g_kill", "g_kill_load", "g_up"]

# Columnas que dependen del resultado de la recepcion: NUNCA pueden ser entradas
FORBIDDEN_AS_INPUT  = ["rssi_dbm", "snr_db", "rx_power_dbm", "collision_flag", "num_colliding_packets", "belowMask", "rcvOk"]
REQUIRED            = STUDENT_INPUTS + ["id_simulation", "srcId", "tx_start", "tx_duration", "rx_power_dbm_at_dev",
                             "loraCF", "belowMask", "rcvOk", "collision_flag", "rssi_dbm"]


# Umbral de captura de FLoRaSat (LoRaReceiver.h): NON_ORTH_DELTA[SF_x - 7][SF_j - 7], en dB
NON_ORTH_DELTA = np.array([[  1,  -8,  -9,  -9,  -9,  -9],
                           [-11,   1, -11, -12, -13, -13],
                           [-15, -13,   1, -13, -14, -15],
                           [-19, -18, -17,   1, -17, -18],
                           [-22, -22, -21, -20,   1, -20],
                           [-25, -25, -25, -24, -23,   1]])


# Escala de las features. Viene de timeToNextPacket = exponential(50s) del .ini.
LOAD_SCALE = 50.0

SENTINEL = -999
""" 
R1: efecto captura (isPacketCollided)
R2: solapamiento en tiempo y frecuencia (detectCollisionOverlap)
"""


################################################################
################################################################
################################################################

DATA_KEYS = ["X_student", "X_geo", "y_true", "below_mask", "id_simulation", "src_id", "tx_start", "orig_index"]

def load_dataset(path):
    with np.load(path, allow_pickle=False) as z:
        return {k: z[k] for k in DATA_KEYS}


def check_csv(df, name=""):
    """    
    Revisa consistencia del CSV con condiciones de las features.
    """
    missing = [c for c in REQUIRED if c not in df.columns]
    assert not missing, f"{name}: faltan columnas {missing}"

    inputs = STUDENT_INPUTS + ["tx_start", "tx_duration", "rx_power_dbm_at_dev"]
    assert not df[inputs].isna().any().any(), f"{name}: hay NaN en las entradas"
    assert (df.rx_power_dbm_at_dev != SENTINEL).all(), f"{name}: rx_power_dbm_at_dev con -999"
    assert df.loraSF.between(7, 12).all(), f"{name}: SF fuera de 7-12"
    
     # R1 (isPacketCollided) y g_kill suponen una sola portadora: todos los paquetes comparten canal
    assert df.loraCF.nunique() == 1, f"{name}: hay varias loraCF"
    assert (df.groupby("srcId").loraSF.nunique() == 1).all(), f"{name}: un dispositivo cambia de SF"
    
    # Condiciones necesarias de un paquete recibido (modelo de colision solo R1).
    # rssi_dbm se registra para todo paquete que supera la sensibilidad, tambien si R1 lo mata,
    # asi que solo se exige en un sentido: si rcvOk = 1, se cumplen las tres condiciones.
    received_ok = (df.belowMask == 0) & (df.collision_flag == 0) & (df.rssi_dbm != SENTINEL)
    assert ((df.rcvOk == 0) | received_ok).all(), f"{name}: hay paquetes recibidos que no cumplen las condiciones"


def add_geo_features(df):
    """ 
    x - Dispositivo actual
    j - Otros dispositvos 

    Para cada dispositivo j de la misma simulacion se interpola linealmente, en el instante t de x, su
    elevacion, su doppler y su potencia rx_power_dbm_at_dev, a partir de las propias transmisiones de j en el CSV.

        g_up              n de j con elevacion >= 0                                  (carga del footprint)
        g_kill            de esos, n con P_x - P_j < NON_ORTH_DELTA[SF_x][SF_j]       (candidatos a R1)
        g_kill_load       lo mismo, ponderado por (dur_j + dur_x) / LOAD_SCALE       (prob. de solaparse)

    Limitaciones: la interpolacion cruza los huecos entre pasadas; "sobre el horizonte" es
    elev >= 0 (no la mascara de 10 grados); LOAD_SCALE es una constante en vez del intervalo real de j.


    """

    out = []

    # Se agrupa por cada simulacion
    for _, sim in df.groupby("id_simulation"):
        # Ordena por tx_start
        sim      = sim.sort_values("tx_start")
        
        # Get data
        t        = sim['tx_start'].values
        own_dev  = sim['srcId'].values
        own_pw   = sim['rx_power_dbm_at_dev'].values
        own_sf   = sim['loraSF'].values.astype(int)
        own_dur  = sim['tx_duration'].values

        # Create dict
        feats = {c: np.zeros(len(sim)) for c in GEO_COLS}
        
        # Se agrupa por dispositivo de cada simulacion
        for dev, other in sim.groupby("srcId"):
            # Itera sobre cada dispositivo
            other = other.sort_values("tx_start")

            # En este contexto, 
            # x representa todas las transmisiones de la simulacion
            # j representa todas las otras transmisiones a parte de x dentro de la simulacion
            def at_t(col):
                # np.interp: One-dimensional linear interpolation for monotonically increasing sample points.
                # x = 3.6
                # xp = [2, 4, 6]
                # fp = [1, 3, 5]
                # gfg = np.interp(x, xp, fp) -> 2.6

                # Se interporal para saber si otro dispositivo j podia chocar con la transmision x, 
                # para esto es necsario saber los posibles valores para j en el instante x
                # Interpola el valor de 'col' para la transmision j en el tiempo de x                
                #
                # 
                # t -> tiempo de todas las transmisiones x de la simulacion
                # other.x_start.values -> tiempo de las transmisiones j de la simulacion
                # other[col] -> puede ser elevSat, Doppler, rx_power_dbm_at_dev
                v = np.interp(t, other["tx_start"].values, other[col].values, left=np.nan, right=np.nan)

                # Descarta el mismo dispositivo
                v[own_dev == dev] = np.nan
                return v

            # Obtener interpolaciones
            elev_j = at_t("elevSat")             
            pw_j   = at_t("rx_power_dbm_at_dev")

            # Obtiene el SF, dur de j
            sf_j         = int(other["loraSF"].iloc[0])
            dur_j        = other["tx_duration"].iloc[0]

            # Verifica si se sobrepasan, revisa la duracion. Duracion de la transmision j + duracion transmision x
            # g_kill solo mira la geometria: si j esta en posicion de danhar a x (potencia suficiente para R1).
            # Pero R1 necesita tambien un solapamiento en el tiempo. Si j no transmite mientras x esta en el aire, no pasa nada.
            # Contar cada candidato como 1 supone que todos se solapan seguro, y no siempre es asi.

            #El paquete de j choca con x si j empieza dentro de esta ventana
            #
            #start_x − dur_j        start_x              start_x + dur_x
            #      |◄──── dur_j ────►|◄────── dur_x ─────►  |
            #      └───── ventana de longitud dur_x + dur_j ┘

            #Si j empieza antes de start_x − dur_j, ya termino cuando x empieza. Si empieza despues de start_x + dur_x, x ya termino
            #Si j transmite una vez cada T segundos, en un momento que no depende de x, la probabilidad de que caiga en esa ventana es:
            # P(solape) = (dur_x + dur_j) / LOAD_SCALE. LOAD_SCALE es un valor arbitrario. 

            # Lo que conviene ensenhar al estudiante es la probabilidad de exito antes de saber quien transmitira. 
            # El estudiante decide antes de transmitir y no puede saber si j transmitira en ese mismo instante.

            overlap_load = (dur_j + own_dur) / LOAD_SCALE

            # Verifica que el elev_j > 0
            up   = elev_j > 0                        

            # En los dispositivos up, que las
            # R1: efecto captura (isPacketCollided) 
            # TOdas las columnas usadas del csv estan en dBm
            # Aplica mismo control que en LoraReceiver.cc, funcion isPacketCollided.
            kill = up & (own_pw - pw_j < NON_ORTH_DELTA[own_sf - 7, sf_j - 7])      # R1 posible

            feats["g_up"]             += up
            feats["g_kill"]           += kill
            feats["g_kill_load"]      += kill * overlap_load
        
        out.append(pd.DataFrame(feats, index=sim.index))

    return pd.concat(out).loc[df.index]

def reorder(df):
    """    
    Reordenar todo el csv al enfoque inicial.
    _orig = indice de la fila en el CSV original.
    """
    df          = df.copy()
    df["_orig"] = np.arange(len(df))
    parts       = []

    for sim in sorted(df["id_simulation"].unique()):
        g = df[df["id_simulation"] == sim].sort_values("tx_start").reset_index(drop=True)

        for dev in g["srcId"].unique():
            parts.append(g[g["srcId"] == dev])

    return pd.concat(parts, ignore_index=True)



    
def build_dataset(csv_path):

    df = pd.read_csv(csv_path)
    
    check_csv(df, csv_path)
    

    geo      = add_geo_features(df)
    #print('AQUI 1')
    df_final = reorder(df)
    #print('AQUI 2')

    # Asigna las columnas nuevas calculadas en add_geo_features a los indices adecuados de df_final.
    df_final[GEO_COLS] = geo.loc[df_final["_orig"].values, GEO_COLS].values
    #print('AQUI 3')

    assert not df_final[GEO_COLS].isna().any().any(), "NaN en las features geo"
    assert not set(FORBIDDEN_AS_INPUT) & set(STUDENT_INPUTS + GEO_COLS), "columna prohibida como entrada"

    return {
        "X_student":     df_final[STUDENT_INPUTS].to_numpy(np.float64),
        "X_geo":         df_final[GEO_COLS].to_numpy(np.float64),
        "y_true":        df_final["rcvOk"].to_numpy(np.float32),
        "below_mask":    df_final["belowMask"].to_numpy(np.int8),
        "id_simulation": df_final["id_simulation"].astype(str).to_numpy(dtype=np.str_),   # texto unicode (sin pickle)
        "src_id":        df_final["srcId"].to_numpy(np.int64),
        "tx_start":      df_final["tx_start"].to_numpy(np.float64),
        "orig_index":    df_final["_orig"].to_numpy(np.int64),
    }


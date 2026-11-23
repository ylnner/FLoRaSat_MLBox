import numpy as np
import pandas as pd

# v3: la etiqueta de red y' y el grafo de captura se calculan con la replica del receptor
from network_receiver import ELEV_MASK, build_kill_graph, network_label, received


# Entradas del estudiante, en el orden en que entran al MLP (= run_train_student.STUDENT_INPUTS,
# = orden del JSON de scalers que lee el simulador)
# sin raan: raan esta confundido con alt en el dataset (850 km solo con raan 277-327) y hace que el modelo dse confuda
STUDENT_INPUTS  = ["dopplerShift", "elevSat", "latDev", "longDev", "loraTP", "loraSF", "alt"]
#GEO_COLS        = ["g_near_dopp", "g_near_dopp_load", "g_kill", "g_kill_load", "g_up"]
GEO_COLS        = ["g_kill", "g_kill_load", "g_up"]

# Columnas que dependen del resultado de la recepcion: NUNCA pueden ser entradas
FORBIDDEN_AS_INPUT  = ["rssi_dbm", "snr_db", "rx_power_dbm", "collision_flag", "num_colliding_packets", "belowMask", "rcvOk"]

# se comprueba porque la replica del receptor supone BW = 125 kHz
REQUIRED            = STUDENT_INPUTS + ["raan", "id_simulation", "srcId", "pid", "tx_start", "tx_duration", "rx_power_dbm_at_dev",
                             "loraCF", "loraBW", "belowMask", "rcvOk", "collision_flag", "rssi_dbm"]


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

# Nueva version: la cache solo guarda las filas VISIBLES (elevSat >= 10): el C++ nunca transmite por debajo de la mascara, asi que
# esas filas no son decisiones del modelo. Claves por fila visible:
#   X_student, X_geo     entradas del estudiante y features geo del maestro (las geo se calculan con TODAS las filas)
#   raan                 Sin raan: NO es entrada. Solo para evaluar en compare_offline.py los modelos desplegados
#                        que todavia tienen 8 entradas (X_student + raan)
#   y_true               rcvOk de FLoRaSat (red del dataset: todas las copias radiadas). Solo como referencia
#   y_net                y': la copia llegaria con la red FV_1 (etiqueta de entrenamiento)
#   receivable           pasa mascara y sensibilidad (sin colisiones)
#   msg_id               id del mensaje (simulacion, dispositivo, pid), unico dentro del split
#   id_simulation, src_id, pid, tx_start, orig_index
#   edge_victim, edge_killer   grafo de captura entre filas visibles (indices de este mismo array)
DATA_KEYS = ["X_student", "X_geo", "raan", "y_true", "y_net", "receivable", "msg_id", "id_simulation", "src_id", "pid",
             "tx_start", "orig_index", "edge_victim", "edge_killer"]

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
    # La replica del receptor (network_receiver.py) tiene la sensibilidad de SF10-12 con BW 125 kHz
    assert df.loraSF.between(10, 12).all(), f"{name}: la replica del receptor solo tiene SF10-12"
    assert (df.loraBW == 125000).all(), f"{name}: la replica del receptor supone BW = 125 kHz"
    # La mascara de belowMask debe ser la misma que la de la replica (elevSat >= 10)
    assert ((df.belowMask == 0) == (df.elevSat >= ELEV_MASK)).all(), f"{name}: belowMask no coincide con elevSat >= 10"
    # Cada mensaje (simulacion, dispositivo, pid) tiene sus copias en filas distintas
    assert not df.duplicated(["id_simulation", "srcId", "pid", "tx_start"]).any(), f"{name}: filas repetidas"
    
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



    
def network_columns(df_final):
    """
    etiqueta de red y' y grafo de captura de un split (todas las simulaciones del CSV).

    1 Grafo "j mata a r" con TODAS las filas (tambien las de elevSat 0-10, que en el dataset existan y pueden
       interferir). Con todas las filas transmitiendo se reproduce rcvOk: es la verificacion de la replica.
    2 Red de referencia FV_1: cada mensaje envia solo su primera copia visible (elevSat >= 10), como la red
       desplegada. y' de cada fila = llegaria si se transmite con esa red en el aire.
    3 Se quedan solo las filas visibles y las aristas entre filas visibles , renumeradas a los indices de las filas visibles.
    """

    sim      = df_final["id_simulation"].to_numpy()
    tx_start = df_final["tx_start"].to_numpy(np.float64)
    elev     = df_final["elevSat"].to_numpy(np.float64)
    lora_sf  = df_final["loraSF"].to_numpy().astype(np.int64)

    #receivable = [True, True, True, False, True, False] Marca si la fila llegaría sin interferencia
    # edge_victim = array len k 
    # edge_killer = aray len k
    # la arista k es el indice entre ambos
    # si edge_killer[k] transmite, entonces edge_victim[k] no llega   
    receivable, edge_victim, edge_killer = build_kill_graph(sim, tx_start,
                                                            df_final["tx_duration"].to_numpy(np.float64), lora_sf,
                                                            df_final["rx_power_dbm_at_dev"].to_numpy(np.float64), elev)

    # Verificacion: con todas las filas transmitiendo (como en el dataset) la replica debe dar rcvOk
    all_tx      = np.ones(len(df_final), dtype=bool)
    replica_ok  = received(edge_victim, edge_killer, receivable, all_tx)
    visible     = elev >= ELEV_MASK
    rcv_ok      = df_final["rcvOk"].to_numpy().astype(bool)
    agree_visible = float(np.mean(replica_ok[visible] == rcv_ok[visible]))

    # Red FV_1: primera copia visible de cada mensaje.     
    # (df_final ya esta ordenado por simulacion, dispositivo y tiempo; se ordena de nuevo por seguridad).
    visible_rows = df_final[visible].sort_values(["id_simulation", "srcId", "pid", "tx_start"])
    # # cumcount numera las filas visibles dentro de cada grupo 'id_simulation, srcId, pid' en orden de tx_start    
    visible_rank = visible_rows.groupby(["id_simulation", "srcId", "pid"]).cumcount()

    tx_reference = np.zeros(len(df_final), dtype=bool)
    # Un vector booleano con True en todos que tiene visible_rank , que corresponde a FV 1
    tx_reference[visible_rows.index[visible_rank.to_numpy() == 0]] = True

    # COn esto filtramos todas las visibles y las que transmiten primero en su ventana.

    # Genreamos nuevos labels
    y_net = network_label(edge_victim, edge_killer, receivable, tx_reference)

    # Renumera las aristas a las filas visibles; las aristas con una fila no visible se descartan
    new_index   = np.full(len(df_final), -1, dtype=np.int64)
    new_index[visible] = np.arange(visible.sum())
    keep        = visible[edge_victim] & visible[edge_killer]

    stats = {"rows": int(len(df_final)), "visible_rows": int(visible.sum()),
             "replica_agree_visible": agree_visible,
             "rcvOk_visible": float(rcv_ok[visible].mean()), "y_net_visible": float(y_net[visible].mean()),
             "receivable_visible": float(receivable[visible].mean()),
             "reference_tx": int(tx_reference.sum())}
    return {"visible": visible, "y_net": y_net, "receivable": receivable,
            "edge_victim": new_index[edge_victim[keep]], "edge_killer": new_index[edge_killer[keep]], "stats": stats}


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

    # y', receivable y grafo de captura (con todas las filas); despues solo se guardan las filas visibles
    net      = network_columns(df_final)
    visible  = net["visible"]
    df_vis   = df_final[visible]

    # v3: id de mensaje = (simulacion, dispositivo, pid). ngroup numera cada grupo con un entero (0..M-1).
    msg_id = df_vis.groupby(["id_simulation", "srcId", "pid"], sort=True).ngroup().to_numpy(np.int64)

    data = {
        "X_student":     df_vis[STUDENT_INPUTS].to_numpy(np.float64),
        "X_geo":         df_vis[GEO_COLS].to_numpy(np.float64),
        "raan":          df_vis["raan"].to_numpy(np.float64),          # solo referencia, no es entrada
        "y_true":        df_vis["rcvOk"].to_numpy(np.float32),
        "y_net":         net["y_net"][visible].astype(np.float32),
        "receivable":    net["receivable"][visible],
        "msg_id":        msg_id,
        "id_simulation": df_vis["id_simulation"].astype(str).to_numpy(dtype=np.str_),   # texto unicode (sin pickle)
        "src_id":        df_vis["srcId"].to_numpy(np.int64),
        "pid":           df_vis["pid"].to_numpy(np.int64),
        "tx_start":      df_vis["tx_start"].to_numpy(np.float64),
        "orig_index":    df_vis["_orig"].to_numpy(np.int64),
        "edge_victim":   net["edge_victim"],
        "edge_killer":   net["edge_killer"],
    }
    return data, net["stats"]

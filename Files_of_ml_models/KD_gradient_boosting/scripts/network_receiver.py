"""
Replica en Python del receptor del satelite de FLoRaSat (LoRaReceiver.cc, solo la regla R1 de captura; . 

Se usa para dos cosas:
    1. Calcular la etiqueta de red y' (y_net) de cada copia visible: la copia llegaria si se transmite en su instante,
       con el resto de la red transmitiendo como FV_1 (cada mensaje de los demas nodos envia solo su primera copia
       visible). Es la carga de la red desplegada, no la del dataset (donde se transmiten las 5 copias de todos).
    2. Evaluar politicas sobre la validacion y el test (mensajes unicos y transmisiones), sin simular.

Una copia/repeticion 'x' llega si cumple las tres reglas:
    1. Mascara:       elevSat >= 10 grados.
    2. Sensibilidad:  rx_power_dbm_at_dev >= SENS_DBM[SF_x].
    3. Captura (R1):  para cada interferente j que se solapa en el tiempo con x (j transmite y tiene elevSat > 0),
                      x muere si P_x - P_j < NON_ORTH_DELTA[SF_x][SF_j] y j termina despues de los 2 primeros
                      simbolos del preambulo de x.

El SF, la TP, el instante y la potencia de cada fila son los del CSV, asi que "j mataria a x si los dos transmiten" es
una relacion FIJA de la simulacion. 

Se calcula una sola vez como una lista de aristas (victima x, asesino j); despues
cualquier politica es un vector booleano tx sobre las filas y se evalua con un bincount (muy rapido).

Se valido contra rcvOk de FLoRaSat (todas las filas transmiten, como en el dataset): 99,99-100 % de las filas visibles
coinciden en N = 300. prepare_data.py repite esta verificacion en cada (seed, split).
"""
import numpy as np

ELEV_MASK        = 10.0                                         # con elevSat < 10 grados el paquete nunca llega
SENS_DBM         = {10: -133.0, 11: -135.0, 12: -137.0}         # sensibilidad por SF con BW 125 kHz
BW_HZ            = 125000.0                                     # ancho de banda de todas las transmisiones
PREAMBLE_SYMBOLS = 2                                            # un interferente que acaba en los 2 primeros simbolos no mata
MAX_TOA_S        = 1.646592                                     # tiempo en el aire de SF12 (el mas largo), en s

# Umbral de captura de FLoRaSat (LoRaReceiver.h): NON_ORTH_DELTA[SF_x - 7][SF_j - 7], en dB
NON_ORTH_DELTA = np.array([[  1,  -8,  -9,  -9,  -9,  -9],
                           [-11,   1, -11, -12, -13, -13],
                           [-15, -13,   1, -13, -14, -15],
                           [-19, -18, -17,   1, -17, -18],
                           [-22, -22, -21, -20,   1, -20],
                           [-25, -25, -25, -24, -23,   1]])


# ---------------------------------------------------------------------------------------------------------------------
# Grafo de captura
# ---------------------------------------------------------------------------------------------------------------------
def build_kill_graph(id_simulation, tx_start, tx_duration, lora_sf, rx_power, elev):
    """
    Aristas (victima r, asesino j) de todas las simulaciones de un split. Indices = posicion de la fila en los arrays.
    Una arista significa: si r y j transmiten a la vez, r no llega por culpa de j (regla de captura R1).

    Solo r con mascara y sensibilidad (receivable) pueden ser victimas; 
    solo j con elevSat > 0 pueden ser asesinos.
    
    Las filas de simulaciones distintas nunca se conectan.

    Devuelve receivable (bool por fila), edge_victim y edge_killer (int64).


    receivable = [True, True, True, False, True, False]
        Marca si la fila llegaría sin interferencia. Las filas 3 y 5 nunca llegan, transmita quien transmita.
    
    edge_victim = 
    edge_killer =
    la arista k es el indice entre ambos

    si edge_killer[k] transmite, entonces edge_victim[k] no llega   
    """

    sens       = np.array([SENS_DBM[int(sf)] for sf in lora_sf])

    # Marca las filas q podrian llegar si nadie interfiere
    receivable = (elev >= ELEV_MASK) & (rx_power >= sens)
    end        = tx_start + tx_duration

    # Duracion maxima real del split (= MAX_TOA_S con SF12 y CR del dataset). Se toma de los datos para que la ventana
    # de busqueda nunca deje fuera un interferente si cambia la duracion de las tramas.
    max_toa    = max(float(tx_duration.max()), MAX_TOA_S)
    # Instante en que terminan los 2 primeros simbolos del preambulo (tolerancia de captura)
    critical   = tx_start + PREAMBLE_SYMBOLS * (2.0 ** lora_sf) / BW_HZ

    victims = []
    killers = []
    # Iteramos sobre simulacion
    for sim in np.unique(id_simulation):
        rows = np.flatnonzero(id_simulation == sim)

        # Interferentes de la simulacion ordenados por inicio. np.searchsorted (busqueda binaria sobre un array
        # ordenado) da en O(log n) el rango de interferentes que pueden solaparse con r, sin recorrer toda la simulacion.
        interferers      = rows[elev[rows] > 0]
        interferers      = interferers[np.argsort(tx_start[interferers], kind="stable")]
        interferer_start = tx_start[interferers]

        #(victima r, asesino j)
        for r in rows[receivable[rows]]:
            # Interferentes que empiezan entre (inicio de r - ToA maximo) y el final de r
            lo = np.searchsorted(interferer_start, tx_start[r] - max_toa - 1e-9)
            hi = np.searchsorted(interferer_start, end[r])
            j  = interferers[lo:hi]
            j  = j[j != r]
            if len(j) == 0:
                continue

            # El fin de j es mayor al inicio de r y el inicio de j es menor que el fin de r
            overlap        = (end[j] > tx_start[r]) & (tx_start[j] < end[r])            
            loses_capture  = rx_power[r] - rx_power[j] < NON_ORTH_DELTA[lora_sf[r] - 7, lora_sf[j] - 7]
            # fuera del preambulo LoRaReceiver.cc:300-306:
            after_preamble = critical[r] < end[j]
            # Se cumple overlap, pierde a captura y fuera del preambulo entonces peude matar a la transmision.
            j = j[overlap & loses_capture & after_preamble]
            if len(j) == 0:
                continue

            victims.append(np.full(len(j), r, dtype=np.int64))
            killers.append(j.astype(np.int64))

    if len(victims) > 0:
        edge_victim = np.concatenate(victims)
        edge_killer = np.concatenate(killers)
    else:
        edge_victim = np.zeros(0, dtype=np.int64)
        edge_killer = np.zeros(0, dtype=np.int64)
    return receivable, edge_victim, edge_killer


def count_killers(edge_victim, edge_killer, tx, n_rows):
    """
    Numero de asesinos que estan transmiten para cada fila/copia. 
    np.bincount cuenta por indice en una sola pasada vectorizada.
    
    """
    # para cada transferencia verificio que tenga un killer transmitiendo
    active = tx[edge_killer]

    # Cuatnas activas de killers para la victima
    return np.bincount(edge_victim[active], minlength=n_rows)


def received(edge_victim, edge_killer, receivable, tx):
    """
    ok(r): r transmite, pasa mascara y sensibilidad, y ningun asesino que transmite la destruye.
    """
    n_killers = count_killers(edge_victim, edge_killer, tx, len(tx))
    return tx & receivable & (n_killers == 0)


def network_label(edge_victim, edge_killer, receivable, tx_reference):
    """
    Geneera los nuevos labels y_net. 
    y' (y_net) de cada fila: la copia llegaria si se transmite, con la red tx_reference transmitiendo. (FV_1)
    Es contrafactual: vale tambien para filas que tx_reference no envia ("y si se enviara esta en lugar de la otra").    
    Las copias de un mismo mensaje estan separadas >= 10 s y duran <= 1,65 s, nunca se solapan entre si


    receivable   = [T, T, T, F, T, F]
    edge_victim  = [1, 2, 2, 4]          # 4 aristas
    edge_killer  = [0, 3, 0, 1]
                    aristas: 0→1, 3→2, 0→2, 1→4
    tx_reference = [T, F, F, T, T, F]    # red FV_1: transmiten las filas 0, 3 y 4

    """
    #receivable = [True, True, True, False, True, False] Marca si la fila llegaría sin interferencia
    # edge_victim = array len k 
    # edge_killer = aray len k
    # la arista k es el indice entre ambos
    # si edge_killer[k] transmite, entonces edge_victim[k] no llega   
    # tx_reference Un vector booleano con True en todos que tiene visible_rank = 0 ,que corresponde a FV 1 

    # Cuento cuantos killers tiene
    n_killers = count_killers(edge_victim, edge_killer, tx_reference, len(tx_reference))

    # Returna y_net = [1, 0, 0, 0, 1, 0], verifica que se receivable y q no tenga killers
    return receivable & (n_killers == 0)


# ---------------------------------------------------------------------------------------------------------------------
# Mensajes y politicas (sobre las filas visibles de un split)
# ---------------------------------------------------------------------------------------------------------------------
def message_layout(msg_id, tx_start, n_slots):
    """
    Matriz slot_row [mensajes x n_slots]: fila del array que ocupa la oportunidad s del mensaje m, en orden de tx_start,
    y -1 si el mensaje tiene menos oportunidades. Tambien devuelve msg_index (mensaje de cada fila, de 0 a M-1).

    np.lexsort ordena por varias claves a la vez (la ultima es la principal): primero por mensaje y dentro por tiempo.
    np.unique(return_inverse=True) renumera los mensajes a 0..M-1 sin bucles.

    slot_row: matriz [mensajes × NTX], con NTX = 5
    Guarda el indice de la priemra transmision visible de ese mensaje 
    - Para el mensaje m y la oportunidad s, guarda la fila que corresponde a esa oportunidad.
    - Dentro de cada mensaje, las oportunidades van ordenadas por tx_start
    - Si el mensaje tiene menos de 5 oportunidades visibles, las casillas sobrantes valen −1.
        Ejemplo con 6 filas visibles y 2 mensajes:
        fila  msg_id  tx_start              slot_row (NTX = 5)
        0     B        30                  msg A: [ 2,  4,  1, -1, -1]
        1     A        50                  msg B: [ 3,  0, -1, -1, -1]
        2     A        10
        3     B        20
        4     A        40
    - la primera oportunidad visible del mensaje A, es slot_row[A, 0] = fila 2.

    msg_index: indica a que mensaje idx pertenece dada mensaje
    msg_index = [B,A, A, B, A] ->[1, 0, 0, 1, 0] ordenad por tx_start
    """
    order = np.lexsort((tx_start, msg_id))
    unique_msgs, msg_index = np.unique(msg_id, return_inverse=True)
    sorted_msg = msg_index[order]

    # Posicion de cada fila dentro de su mensaje: indice global menos el indice donde empieza su mensaje
    starts   = np.flatnonzero(np.r_[True, sorted_msg[1:] != sorted_msg[:-1]])
    lengths  = np.diff(np.r_[starts, len(order)])
    position = np.arange(len(order)) - np.repeat(starts, lengths)
    assert position.max() < n_slots, f"un mensaje tiene {position.max() + 1} oportunidades y el maximo es {n_slots}"

    slot_row = np.full((len(unique_msgs), n_slots), -1, dtype=np.int64)
    slot_row[sorted_msg, position] = order
    return slot_row, msg_index


def sequential_budget(slot_row, candidate, budget):
    """
    Politica secuencial por mensaje: 
    recorre las oportunidades en orden de tiempo y transmite las filas candidatas hasta alcanzar budget copias. 
    Es lo que hace FLoRasat con threshold + regla de parada (con stopTarget = 0.01 y budget = 1: 
    tras la 1.a transmision, 1 - cumMissProb = p >= 0.5 >= 0.01 y se saltan las demas).
    
    #slot_row: matriz [mensajes × NTX], con NTX = 5
    #Guarda el orden de las transmisione visible de ese mensaje, por defecto es -1
    slot_row  = [[0, 1],     # A
                 [2, 3],     # B
                 [4, -1]]    # C (solo 1 oportunidad)

    np.cumsum a lo largo de las oportunidades cuenta cuantas copias se han enviado antes de cada una.

    # candidate [n]
    # budget 1
    """    
    
    # Hay transmisiones visibles, por default slot_row -1, descarta las que estan con -1
    valid     = slot_row >= 0 # [mensajes x NTX], de tipo bool ahora
    
    # Reemplaza donde no hay atrnasisiones visibles por 0
    # donde estan las oportunidades para transmitir del set
    safe_rows = np.where(valid, slot_row, 0) # [mensajes x NTX]

    # se obtiene de los candidatos a enviar y que sean validos a traves de slot_row
    # recorre la matriz safe_rows y reemplaza cada numero 'a' por candidate[a]
    # si el modelo quiere trnamistir en esa epoca y si es una oportunidad validad por el test/val set
    cand      = candidate[safe_rows] & valid # [mensajes x NTX]

    # Cuenta cuantas copias se enviaron antes de cada oportunidad y que sea menor que el budget
    # np.cumsum a lo largo de las oportunidades cuenta cuantas copias se han enviado antes de cada una.
    sent_before = np.cumsum(cand, axis=1) - cand

    # Control de budget
    send        = cand & (sent_before < budget) # [mensajes x NTX]

    # vector de zeros 
    tx = np.zeros(len(candidate), dtype=bool)

    # pone True en cada candidato a transmitir
    tx[slot_row[send]] = True
    return tx


def unique_delivered(ok, msg_index):
    """
    Mensajes distintos con al menos una copia recibida.
    """
    return len(np.unique(msg_index[ok]))


def evaluate_policy(data, candidate, budget):
    """
    Esta funcion permite medir el performance sin simular para usarlo como metrica. Se le pasa el set, los dcandidatos a trnasmitir y el budget de copias por mensaje
    
    Mensajes unicos y transmisiones de la politica "candidatas en orden, como mucho `budget` copias por mensaje".
    data: split cargado con load_split (filas visibles, slot_row, msg_index y aristas del grafo).
    Devuelve (unique, tx, unique_by_simulation).    
    """
    #slot_row: matriz [mensajes × NTX], con NTX = 5
    #Guarda el indice de la priemra transmision visible de ese mensaje 
    tx = sequential_budget(data["slot_row"], candidate, budget)
    ok = received(data["edge_victim"], data["edge_killer"], data["receivable"], tx)

    unique_by_simulation = {}
    for sim in np.unique(data["id_simulation"]):
        in_sim = data["id_simulation"] == sim
        unique_by_simulation[str(sim)] = unique_delivered(ok & in_sim, data["msg_index"])
    return unique_delivered(ok, data["msg_index"]), int(tx.sum()), unique_by_simulation

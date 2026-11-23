"""

SequentialMessageLoss
Anteriormente, cada muestra era una transmision y la perdida era BCE por fila (con pesos 1/k y pos_weight). 

Ahora cada muestra es un mensaje con sus hasta NTX oportunidades visibles en orden de tiempo, porque la decision del despliegue es
secuencial: el nodo transmite en la primera oportunidad con p >= 0.5 y despues se detiene (regla de parada con
stopTarget = 0.01). 

La perdida mide directamente lo que esa regla entrega.
"""
import torch
import torch.nn as nn
from torch.utils.data import Dataset


class MyCustomMessageDataset(Dataset):
    """
    Un item = un mensaje.
        x       [NTX, 8]  entradas escaladas de cada oportunidad (0 en las oportunidades que no existen)
        y_net   [NTX]     y' de cada oportunidad (la copia llegaria con la red FV_1 en el aire)
        mask    [NTX]     1 si la oportunidad existe (es visible), 0 si es relleno
    """
    def __init__(self, x, y_net, mask):
        self.x     = torch.as_tensor(x, dtype=torch.float32)
        self.y_net = torch.as_tensor(y_net, dtype=torch.float32)
        self.mask  = torch.as_tensor(mask, dtype=torch.float32)
        self.n     = len(self.y_net)

    def __len__(self):
        return self.n

    def __getitem__(self, idx):
        return {"x": self.x[idx], "y_net": self.y_net[idx], "mask": self.mask[idx]}


class MyCustomMessageDistillationDataset(MyCustomMessageDataset):
    # el dataset de destilacion agrega los logits del maestro de cada oportunidad [NTX] (0 en el relleno)
    def __init__(self, x, y_net, mask, teacher_logits):
        super().__init__(x, y_net, mask)
        self.teacher_logits = torch.as_tensor(teacher_logits, dtype=torch.float32)

        assert self.teacher_logits.shape == self.y_net.shape, \
            f"teacher_logits {tuple(self.teacher_logits.shape)} != y_net {tuple(self.y_net.shape)}"

    def __getitem__(self, idx):
        item = super().__getitem__(idx)
        item["teacher_logits"] = self.teacher_logits[idx]
        return item


class SequentialMessageLoss(nn.Module):
    """
    Perdida secuencial por mensaje. Para cada mensaje con oportunidades s = 1..NTX (en orden de tiempo):

        pi_s     = sigmoid(logit_s) * mask_s                 probabilidad de transmitir en s
        reach_s  = prod_{s' < s} (1 - pi_s')                 probabilidad de llegar a s sin haber transmitido antes
        deliver  = sum_s reach_s * pi_s * target_s           probabilidad de que el mensaje llegue con la 1.a transmision
        cost     = sum_s reach_s * pi_s                      transmisiones esperadas (como mucho 1)

        L = mean_mensajes( -deliver + mu * cost )

    target_s:
        noKD (sin maestro):  y'_s
        KD:                  alpha * y'_s + (1 - alpha) * sigmoid(z_T,s / T)
    La etiqueta suave del maestro entra como probabilidad de entrega dentro de deliver (no como un termino BCE aparte),
    por eso no se multiplica por T^2: aqui T solo suaviza la probabilidad del maestro, no escala un gradiente de BCE.

    
    En comparacion a FV_1:
      - Con target = 1 en todas las oportunidades, el optimo es transmitir en la primera (FV_1).
      - Si la primera oportunidad tiene target bajo (p. ej. SF10 con alt alta, fuera de sensibilidad), compensa
        esperar: reach no se gasta y una oportunidad posterior con target alto aporta mas.
      - mu > 0 penaliza transmitir cuando ninguna oportunidad es buena (ahorra transmisiones e interferencia).    

    
    - El modelo predice por copia. Para cada oportunidad visible de cada mensaje, el MLP da un logit, y de ahi sale una p.
    - El loss se calcula por mensaje. Para cada mensaje se combinan las p de sus hasta 5 oportunidades en un solo numero.
    - Despues se promedia sobre los mensajes del batch.
    De esta forma, la loss se evalua desde el punto de vista de si es la mejor oportuidada para transmitir, sabiendo que si lo haces no vuelves a transmitir?
    """
    def __init__(self, alpha, temperature, mu):
        super().__init__()
        assert 0.0 <= alpha <= 1.0
        assert mu >= 0.0

        self.alpha = alpha
        self.T     = temperature
        # mu - el precio de la transmision medido en mensajes entregados
        # indica cuantos mensajes estad dispuesto a dejar pasar para ahorrarte una transmision, es hyperpaarmtro
        # ejmplo: mu = 0.1, una transmision cuesta 0.1 mensajes. 
        # COmo el modelo ahora nos espera la mejor oportunidad para transmitir puede que nucnca llegue la mejor y 
        # necesita ser penalizado. Mu sirve para esto.
        self.mu    = mu

    def forward(self, student_logits, y_net, mask, teacher_logits=None):
        """
        student_logits tiene forma [mensajes, NTX] 
        """

        # student_logits, y_net, mask, teacher_logits: [mensajes, NTX]
        # pi_s es la probabilidad de que el modelo transmita en la oportunidad s. 
        # La mascara pone 0 en las casillas de relleno, asi que nunca se rtansmite en una oportunidad que no existe.
        pi = torch.sigmoid(student_logits) * mask

        # reach_s = prod_{s' < s} (1 - pi_s'). torch.cumprod hace el producto acumulado en cada posicion en una sola operacion
        # vectorizada; se desplaza una posicion (la primera oportunidad siempre se alcanza: reach_1 = 1).
        # Calcula la probabilida de no transmitir en cada oportunidad 1,2,..., s 
        survive = torch.cumprod(1.0 - pi, dim=1)    # survive = [mensajes x NTX]

        # Calcula la probabilidad de de no trnamistir en cada oportunidad 1, 2,..., s-1 (es decir de llegar hasta s sin haber transmitido)
        reach   = torch.cat([torch.ones_like(survive[:, :1]),  # crea un tensor de unos con esa misma forma, [mensajes, 1],, solo coge la primera columna
                             survive[:, :-1]], # coge todas lasotras columnas 
                             dim=1) # lo concatena. Practicamente lo desplaza
        # reach = [mensajes x NTX]

        if teacher_logits is None:
            target = y_net
        else:
            teacher_soft = torch.sigmoid(teacher_logits / self.T)
            target       = self.alpha * y_net + (1.0 - self.alpha) * teacher_soft

        # probabilidad de q la tranmsiison ocurra exactamente en s. 
        # sent = [mensajes x NTX]
        sent    = reach * pi
        
        # es la probabilida de que el mensaje llegue , suma entre oportunidades
        # el mensaje se tranmite en una sola oportunidad, por eso es valido.Al final se transmite en una sola oportunidad. El loss castigar transmitir alli.
        # deliver = [mensajes]
        # En NoKD, target (y_net) es binario  y anula deliver para donde y_net = 0
        # En KD, los pondera
        deliver = (sent * target).sum(dim=1)

        # cost = [mensajes], representa el numero esperado de transmisiones. Suma las probabilidades de cada s en cada mensaje.
        cost    = sent.sum(dim=1)

        # deliver - probabilidad q el mensaje llegue
        # self.mu * cost - costo de transmitir, y entra el mu para penalizar
        # aplica mean para el backwards
        output = (-deliver + self.mu * cost).mean()
        return output

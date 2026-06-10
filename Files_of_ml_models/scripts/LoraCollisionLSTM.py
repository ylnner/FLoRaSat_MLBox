import pandas as pd
import numpy as np
import os
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, roc_auc_score

import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split
import math
import torch.optim as optim
import itertools
import time
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score

class LoraCollisionLSTM(nn.Module):
    def __init__(self, num_features=9, hidden_size=64, num_layers=2, dropout=0.1, is_bidirectional=True):
        super().__init__()
        
        # Bidirectional LSTM Bidireccional        
        self.lstm = nn.LSTM(
            input_size    = num_features, 
            hidden_size   = hidden_size, 
            num_layers    = num_layers, 
            batch_first   = True,
            dropout       = dropout if num_layers > 1 else 0.0, #dropout,
            bidirectional = is_bidirectional
        )
        
        # factor dinámico
        direction_factor = 2 if is_bidirectional else 1

        # Classifier, similar to Transformer model        
        # Como es Bidireccional, genera el doble de características (hidden_size * 2) # porque concatena lo que aprendió yendo hacia adelante y hacia atrás.        
        self.classifier = nn.Sequential(
            nn.Linear(hidden_size * direction_factor, 32),            
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(32, 1),
            nn.Sigmoid()
        )

    def forward(self, x):
        # Doc oficial.
        # output: tensor of shape (L,D∗Hout)(L,D∗Hout​) for unbatched input, (L,N,D∗Hout)(L,N,D∗Hout​) when batch_first=False 
        # or (N,L,D∗Hout)(N,L,D∗Hout​) when batch_first=True containing the output features (h_t) from the last layer of the LSTM, for each t.
        lstm_out, (hn, cn) = self.lstm(x)
        # lstm_out shape: (Batch, Seq_Length, hidden_size * 2)
        
        # Extraer la conclusión del paquete objetivo (el último de la ventana temporal)
        # El primer : mantiene todos los elementos del batch N
        # El -1 se enfoca en el ultimo elemenot de la secuencia
        # es el hidden size x 2
        target_packet = lstm_out[:, -1, :] # Nos quedamos con la posición -1
        # target_packet shape: (Batch, hidden_size * 2)
        
        # Classification
        output = self.classifier(target_packet)
        return output.squeeze(-1)
    
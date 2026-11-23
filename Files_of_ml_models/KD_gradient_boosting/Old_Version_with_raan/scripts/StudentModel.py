import math
import torch
import torch.nn as nn
import torch.nn.functional as F
from config import STUDENT_HIDDEN, STUDENT_DROPOUT

class StudentModel(nn.Module):
    def __init__(self, n_features = 8, hidden_dims=STUDENT_HIDDEN, dropout=STUDENT_DROPOUT):
        super().__init__()

        # Capas ocultas: Linear -> ReLU -> Dropout
        layers = []
        in_dim = n_features
        for h in hidden_dims:
            layers += [nn.Linear(in_dim, h), nn.ReLU(), nn.Dropout(dropout)]
            in_dim = h
        
        # Se agrega una capa de salida. 
        layers.append(nn.Linear(in_dim, 1))

        self.mlp = nn.Sequential(*layers)


    def forward(self, x):
        return self.mlp(x).squeeze(-1)

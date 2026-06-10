import pandas as pd
import numpy as np
import os

from sklearn.preprocessing import StandardScaler
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split
import math
import torch.optim as optim
import itertools
import time

from TimePositionalEncoding import TimePositionalEncoding

class LoraCollisionTransformer(nn.Module):        
    def __init__(self, num_numerical_features, d_model=64, n_heads=4, n_layers=2, dropout=0.1):
        # num_numerical_features- cantidad de variables fisicas (features)    
        # d_model               - Es la dimensión de representación interna del Transformer. Piensa en esto como la cantidad de "canales de información" que el modelo 
        #                         usará internamente para describir cada paquete. Todo el procesamiento profundo se hará en esta dimensión.
        # n_heads               - Le dice al Transformer en cuántas perspectivas diferentes debe dividir su atención.
        # n_layers              - Cuántas veces se repite el proceso de análisis (cuántos bloques Transformer se apilan).
        # dropout               - 
        super().__init__()
        self.num_num_features = num_numerical_features
        self.d_model = d_model
        
        # 1. Proyección directa de las variables físicas a d_model
        # Entran 'num_numerical_features' (ej. 6) y salen 'd_model' (ej. 64)
        # Recibe tu vector original de 6 variables físicas y lo transforma (lo proyecta) matemáticamente en un vector de tamaño d_model (64 dimensiones).
        # Recives the original input vector and project it to a vector size d_model
        self.input_projection = nn.Linear(num_numerical_features, d_model)
        
        # Apply TimePositionalEncoding 
        self.time_encoding = TimePositionalEncoding(d_model)
        
        # Apply encoder_layer
        encoder_layer = nn.TransformerEncoderLayer(d_model=d_model, nhead=n_heads, 
                                                   dim_feedforward=d_model*4, dropout=dropout, batch_first=True)        
        self.transformer_encoder = nn.TransformerEncoder(encoder_layer, num_layers=n_layers)
        
        self.classifier = nn.Sequential(
            nn.Linear(d_model, 32),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(32, 1),
            nn.Sigmoid()
        )

    def forward(self, x):
        # x shape: (Batch, Seq_Length, Features = 7) -> 6 físicas + 1 Delta_t
        
        # 1. DESEMPAQUETAR EL TENSOR X
        # Tomamos todas las columnas excepto la última (que es el tiempo)
        x_num = x[:, :, :self.num_num_features] 
        
        # La última columna es el Delta de Tiempo
        delta_t = x[:, :, -1] 
        
        # 2. PROYECCIÓN FÍSICA
        # Convertimos las 6 variables físicas en un vector de 64 dimensiones
        x_projected = self.input_projection(x_num)       
        
        # 3. POSITIONAL ENCODING (Inyección del tiempo)
        time_pe = self.time_encoding(delta_t)
        x_encoded = x_projected + time_pe
        
        # 4. TRANSFORMER
        transformer_out = self.transformer_encoder(x_encoded) 
        
        # 5. CLASIFICACIÓN
        target_packet = transformer_out[:, -1, :]             
        output = self.classifier(target_packet)               
        
        return output.squeeze(-1)
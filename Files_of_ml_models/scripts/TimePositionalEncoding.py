import pandas as pd
import numpy as np
import os
import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader, random_split
import math
import torch.optim as optim
import itertools
import time

class TimePositionalEncoding(nn.Module):
    def __init__(self, d_model):
        super().__init__()
        self.d_model = d_model
        # Frecuencias fijas según Attention Is All You Need
        # Fixed frequencies according to Attention is all you need        
        # div_term = torch.exp(torch.arange(0, d_model, 2).float() * (-math.log(10000.0) / d_model))
        
        i = torch.arange(0, d_model//2)
        div_term = torch.exp(-np.log(10000) * (2*i / d_model))
        self.register_buffer('div_term', div_term)

    def forward(self, delta_t):
        # batch x seq x d_model
        pe = torch.zeros(delta_t.size(0), delta_t.size(1), self.d_model, device=delta_t.device)
        # angle = (batch x seq x 1) x ( 1 x 1 x d_model//2)
        angle = delta_t.unsqueeze(-1) * self.div_term.unsqueeze(0).unsqueeze(0)
        pe[:, :, 0::2] = torch.sin(angle)
        pe[:, :, 1::2] = torch.cos(angle)
        return pe
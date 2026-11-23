import math
from dataclasses import dataclass

import torch
import torch.nn as nn
import torch.nn.functional as F


class MLPStatic(nn.Module):

    def __init__(self, static_hidden_dims=(64, 32), dropout = 0.1):
        super().__init__()
        n_static_features = 6 
        # ---------------- Static MLP ----------------
        static_layers = []
        in_dim = n_static_features
        for h in static_hidden_dims:
            static_layers += [nn.Linear(in_dim, h), nn.ReLU(), nn.Dropout(dropout)]
            in_dim = h
        self.static_mlp = nn.Sequential(*static_layers)
        static_out_dim = static_hidden_dims[-1] if len(static_hidden_dims) > 0 else n_static_features


    def forward(self, x):                
        static_repr     = self.static_mlp(x)

        return static_repr
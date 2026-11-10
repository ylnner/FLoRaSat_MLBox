import numpy as np
import torch 
import torch.nn as nn
from torch.utils.data import DataLoader, Dataset


GLOBAL_KEYS = ["seq_feats", "seq_mask", "static_feats", "own_tx", "interf_set", "interf_mask", "context_feats", "y_true", "link_ok"]
#GLOBAL_KEYS = ["two_branches_feats", "own_tx", "interf_set", "interf_mask", "context_feats", "y_true", "link_ok"]

"""
class MyCustomDataset(Dataset):

    def __init__(self, data):
        self.data = {k: torch.as_tensor(v) for k, v in data.items() if k in GLOBAL_KEYS}
        self.n = len(self.data["y_true"])

    def __len__(self):
        return self.n

    def __getitem__(self, idx):
        return {k: v[idx] for k, v in self.data.items()}
"""

class MyCustomStudentDataset(Dataset):
    def __init__(self, x, y_true):
        self.x      = torch.as_tensor(x, dtype=torch.float32)
        self.y_true = torch.as_tensor(y_true, dtype=torch.float32)
        self.n      = len(self.y_true)

    def __len__(self):
        return self.n

    def __getitem__(self, idx):
        return {"x": self.x[idx], "y_true": self.y_true[idx]}

class MyCustonStudentDistillationDataset(MyCustomStudentDataset):
    def __init__(self, x, y_true, teacher_logits):
        super().__init__(x, y_true)
        self.teacher_logits = torch.as_tensor(teacher_logits, dtype=torch.float32)
        
        assert len(self.teacher_logits) == self.n, f"teacher_logits ({len(self.teacher_logits)}) != muestras ({self.n})"

    def __getitem__(self, idx):
        item = super().__getitem__(idx)
        item["teacher_logits"] = self.teacher_logits[idx]
        return item

""" 
class MyCustomDistillationDataset(Dataset):
    def __init__(self, data, teacher_logits = None):
        self.data           = {k: torch.as_tensor(v) for k, v in data.items() if k in GLOBAL_KEYS}
        self.n              = len(self.data["y_true"])
        self.teacher_logits = teacher_logits
        # Los logits del maestro se indexan por posicion: deben corresponder 1 a 1 con las muestras
        if teacher_logits is not None:
            assert len(teacher_logits) == self.n, f"teacher_logits ({len(teacher_logits)}) != muestras ({self.n})"

    def __len__(self):
        return self.n

    def __getitem__(self, idx):
        item = {k: v[idx] for k, v in self.data.items()}
        if self.teacher_logits is not None:
            item["teacher_logits"] = self.teacher_logits[idx]

        return item
"""


class MyDistillationLoss(nn.Module):
    """
    L = alpha * BCE(y_true, student_logits) + (1-alpha) * L_soft(teacher, student)

    L_soft compara las probabilidades "suavizadas" (sigmoid con temperatura T)
    del Maestro y el Estudiante via BCE, reescalado por T^2 (Hinton et al. 2015)    
    """
    def __init__(self, alpha, temperature, pos_weight):
        super().__init__()
        assert 0.0 <= alpha <= 1.0

        self.alpha  = alpha
        self.T      = temperature
        #pos_weight    = torch.tensor([81.38/18.62], device='cpu') # 18.62% vs 81.38%        
        self.bce_hard = nn.BCEWithLogitsLoss(pos_weight = pos_weight)

    def forward(self,  student_logits, teacher_logits, y_true):
        
        L_hard = self.bce_hard(student_logits, y_true,)

        teacher_soft = torch.sigmoid(teacher_logits / self.T)
        student_soft = torch.sigmoid(student_logits / self.T)
        
        L_soft = nn.functional.binary_cross_entropy(student_soft, teacher_soft) * (self.T ** 2)

        output = (self.alpha * L_hard) + ((1 - self.alpha) * L_soft)
        return output
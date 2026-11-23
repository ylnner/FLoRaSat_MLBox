import torch
import torch.nn as nn
import torch.nn.functional as F
import math
import numpy as np

class MAB(nn.Module):
    def __init__(self, dim_Q, dim_K, dim_V, num_heads, ln=False):
        super(MAB, self).__init__()
        self.dim_V = dim_V
        self.num_heads = num_heads
        self.fc_q = nn.Linear(dim_Q, dim_V)
        self.fc_k = nn.Linear(dim_K, dim_V)
        self.fc_v = nn.Linear(dim_K, dim_V)
        if ln:
            self.ln0 = nn.LayerNorm(dim_V)
            self.ln1 = nn.LayerNorm(dim_V)
        self.fc_o = nn.Linear(dim_V, dim_V)

    def forward(self, Q, K, padding_mask = None):
        '''
        N_q             = number of queries
        N_k             = number of keys
        padding_mask    = [B x N_k]

        El padding mask se aplica al calcular Q_.bmm(K_.transpose) (Q x K^T) y antes de softmax
        porque el softmax ya va calcular las probabilidades con una suma total de 1, 
        y si hay un valor random residual en las interferencias que no se hacen padding 
        afecta al calculo de las probabilidades q devuelve softmax. 
        '''
        Q = self.fc_q(Q)                    # Q = [B x N_q x dim_Q] => [B x N_q x dim_V]
        K, V = self.fc_k(K), self.fc_v(K)   # K = [B x N_k x dim_K] => [B x N_k x dim_V] , 
                                            # V = [B x N_v x dim_k] => [B x N_k x dim_V]

        dim_split = self.dim_V // self.num_heads
        # torch.cat concatenates the tensors along the specified dimension.
        # In this case, it concatenates the split tensors along dimension 0 (the batch dimension).
        # Tensor.split: Splits the tensor into chunks. Each chunk is a view of the original tensor.
        Q_ = torch.cat(Q.split(dim_split, 2), 0)    # Q_ = [B * num_heads x N_q x dim_split]
        K_ = torch.cat(K.split(dim_split, 2), 0)    # K_ = [B * num_heads x N_k x dim_split] 
        V_ = torch.cat(V.split(dim_split, 2), 0)    # V_ = [B * num_heads x N_k x dim_split]

        # Del paper original de SetTransformer y Attention is all you need, 
        # en el parametro va 2 porque se aplica softmax sobre las K. 
        # Se aplica el padding_mask a esta operacion, antes de calcular softmax
        
        scores = Q_.bmm(K_.transpose(1,2))/math.sqrt(self.dim_V)   # [B * num_heads x N_q x N_k]
        #print('scores: ', type(scores), scores.shape) #
        #print('padding_mask: ', type(padding_mask), padding_mask.shape) #padding_mask.shape
        #input('Press to continue...')
        if padding_mask is not None:
            #mask    = padding_mask.unsqueeze(1).repeat(1, self.num_heads, 1)    # B x d_model x N_inter
            mask    = padding_mask.repeat(self.num_heads, 1).unsqueeze(1) # B * d_model x 1 x N_inter
            #print('mask: ', mask.shape) 
            #input('Presionar prueba')
            
            B       = Q.size(0)
            #print('B: ', B)
            mask    = mask.reshape(B * self.num_heads, 1, -1)
            scores  = scores.masked_fill(mask, -1e9)


        #A = torch.softmax(Q_.bmm(K_.transpose(1,2))/math.sqrt(self.dim_V), 2)   # A = [B * num_heads x N_q x N_k]
        A = torch.softmax(scores,2)   # A = [B * num_heads x N_q x N_k]
        O = torch.cat((Q_ + A.bmm(V_)).split(Q.size(0), 0), 2)                  # O = [B x N_q x dim_v]
        O = O if getattr(self, 'ln0', None) is None else self.ln0(O)
        O = O + F.relu(self.fc_o(O))
        O = O if getattr(self, 'ln1', None) is None else self.ln1(O)            # O = [B x N_q x dim_v]
        return O

class SAB(nn.Module):
    def __init__(self, dim_in, dim_out, num_heads, ln=False):
        super(SAB, self).__init__()
        self.mab = MAB(dim_in, dim_in, dim_out, num_heads, ln=ln)

    def forward(self, X, key_padding_mask = None):
        return self.mab(X, X, padding_mask = key_padding_mask)

class ISAB(nn.Module):
    def __init__(self, dim_in, dim_out, num_heads, num_inds, ln=False):
        super(ISAB, self).__init__()
        self.I = nn.Parameter(torch.Tensor(1, num_inds, dim_out))
        nn.init.xavier_uniform_(self.I)
        self.mab0 = MAB(dim_out, dim_in, dim_out, num_heads, ln=ln)
        self.mab1 = MAB(dim_in, dim_out, dim_out, num_heads, ln=ln)

    def forward(self, X):
        H = self.mab0(self.I.repeat(X.size(0), 1, 1), X)
        return self.mab1(X, H)

class PMA(nn.Module):
    def __init__(self, dim, num_heads, num_seeds, ln=False):
        super(PMA, self).__init__()
        self.S = nn.Parameter(torch.Tensor(1, num_seeds, dim))
        nn.init.xavier_uniform_(self.S)
        self.mab = MAB(dim, dim, dim, num_heads, ln=ln)

    def forward(self, X):
        return self.mab(self.S.repeat(X.size(0), 1, 1), X)



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
        #print('delta_t: ', delta_t. shape, delta_t)

        pe = torch.zeros(delta_t.size(0), delta_t.size(1), self.d_model, device=delta_t.device)
        #print('pe: ', pe.shape)
        # angle = (batch x seq x 1) x ( 1 x 1 x d_model//2)
        #angle = delta_t.unsqueeze(-1) * self.div_term.unsqueeze(0).unsqueeze(0)
        angle = delta_t * self.div_term.unsqueeze(0).unsqueeze(0)
        #print('angle: ', angle.shape)
        pe[:, :, 0::2] = torch.sin(angle)
        pe[:, :, 1::2] = torch.cos(angle)
        return pe
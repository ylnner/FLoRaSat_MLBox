**Contexto del problema — DtS-IoT Baseline Matemático**

Estoy desarrollando mi tesis de PhD sobre el escenario DtS-IoT (Direct-To-Satellite IoT), donde dispositivos LoRa transmiten directamente a satélites en órbita LEO sin infraestructura terrestre intermedia. El simulador usado es OMNeT++ con INET.

**Modelo ML principal:** Transformer encoder-only que predice si una transmisión LoRa será recibida exitosamente por el satélite (`RcvOk = 0/1`).

**Estructura del dataset:** Cada registro representa una transmisión individual. Los campos disponibles son:
- Dispositivo: `Time_Transmission` (timestamp absoluto del momento de transmisión), `Latitud`, `Longitud`, `LoRaTP` (potencia de transmisión en dBm), `LoRaSF` (spreading factor, valores SF7–SF12)
- Satélite: `Altura` (km), `Doppler` (Hz desplazamiento Doppler medido), `RcvOk` (variable objetivo binaria: 1 = recibido, 0 = no recibido)

**Baseline matemático a implementar:** Modelo probabilístico extendido de tres factores independientes que se multiplican:

$$P(\text{RcvOk}=1) = P_{\text{canal}} \times \eta_{\text{dopp}} \times \eta_{\text{coll}}$$

---

**Factor 1 — Canal (modelo probabilístico con SNR):**

$$P_{\text{canal}} = \frac{1}{2}\left[1 + \text{erf}\left(\frac{\text{SNR}_{\text{rx}} - \text{SNR}_{\text{min}}(\text{SF})}{\sqrt{2}\,\sigma}\right)\right]$$

El SNR recibido: $\text{SNR}_{\text{rx}} = P_{\text{rx}} - N_{\text{floor}}$, donde:

$$P_{\text{rx}} = P_{\text{tx}} + G_{\text{tx}} - L_{\text{fs}} - L_{\text{atm}} - L_{\text{point}} + G_{\text{rx}} \quad \text{[dBm]}$$

Cada término:
- $P_{\text{tx}}$: campo `LoRaTP` directamente en dBm
- $G_{\text{tx}}$: ganancia antena dispositivo, asumir 2 dBi salvo que el simulador especifique otro valor
- $L_{\text{fs}} = 20\log_{10}(d_{\text{km}}) + 20\log_{10}(f_{\text{GHz}}) + 92.45$ dB, donde $d$ es el slant range: $d = \sqrt{(R_E+h)^2 - R_E^2\cos^2(\varepsilon)} - R_E\sin(\varepsilon)$, con $R_E = 6371$ km y $\varepsilon$ el ángulo de elevación calculado a partir de `Latitud`, `Longitud` y la posición orbital del satélite derivada de `Altura` y `Time_Transmission`
- $L_{\text{atm}} = 0.03/\sin(\varepsilon)$ dB, o valor fijo 0.3 dB como aproximación conservadora
- $L_{\text{point}}$: asumir 0 dB si el satélite usa antena de cobertura amplia
- $G_{\text{rx}}$: ganancia antena receptora del satélite, típicamente 0–3 dBi

El piso de ruido:

$$N_{\text{floor}} = -174 + 10\log_{10}(B_{\text{SF}}) + NF \quad \text{[dBm]}$$

donde $B_{\text{SF}} = BW / 2^{\text{SF}}$ Hz con $BW = 125$ kHz típico, y $NF \approx 2$ dB (figura de ruido del receptor). Los umbrales mínimos de SNR por SF son: SF7 = −7.5 dB, SF8 = −10 dB, SF9 = −12.5 dB, SF10 = −15 dB, SF11 = −17.5 dB, SF12 = −20 dB. El parámetro $\sigma$ es el único parámetro libre del modelo, representa la suavidad de la transición SNR y se calibra sobre un subconjunto de validación del dataset buscando en el rango 1–5 dB.

---

**Factor 2 — Doppler:**

$$\eta_{\text{dopp},i} = \max\left(0,\; 1 - \frac{|\Delta f_{D,i}|}{B_{\text{SF}_i}}\right)$$

donde $\Delta f_{D,i}$ es el campo `Doppler` del registro $i$ y $B_{\text{SF}_i} = 125000 / 2^{\text{SF}_i}$ Hz. SF altos son mucho más vulnerables: SF12 tiene $B_{\text{SF}} \approx 30$ Hz mientras que SF7 tiene $B_{\text{SF}} \approx 977$ Hz, por lo que un mismo desplazamiento Doppler puede ser tolerable para SF7 y fatal para SF12.

---

**Factor 3 — Colisiones (estimado empíricamente desde el dataset):**

$$\eta_{\text{coll},i} = e^{-2 \cdot N_i}$$

donde $N_i$ es el número de colisionadores potenciales del registro $i$, calculado directamente desde el dataset sin asumir ninguna tasa de transmisión. Para cada registro $i$ con timestamp $t_i$ y spreading factor $\text{SF}_i$, se cuenta cuántos otros registros $j \neq i$ satisfacen simultáneamente: (a) mismo `LoRaSF` que el registro $i$, ya que en LoRa transmisiones con distinto SF son ortogonales y no se interfieren; y (b) `Time_Transmission` dentro del intervalo $[t_i - T_{\text{on}}(\text{SF}_i),\; t_i + T_{\text{on}}(\text{SF}_i)]$, donde los tiempos en el aire $T_{\text{on}}$ por SF son aproximadamente: SF7 ≈ 0.05 s, SF8 ≈ 0.1 s, SF9 ≈ 0.2 s, SF10 ≈ 0.4 s, SF11 ≈ 0.7 s, SF12 ≈ 1.8 s. Ese conteo es directamente $N_i$, incorporando implícitamente tanto la densidad de transmisiones como la duración del paquete.

---

**Decisión final:** $\hat{y}_i = 1$ si $P(\text{RcvOk}=1) \geq 0.5$, en caso contrario $\hat{y}_i = 0$.

**Calibración:** El único parámetro ajustable es $\sigma$ del Factor 1. Se optimiza sobre un subconjunto de validación maximizando F1-score o Accuracy. Todos los demás términos son deterministas dados los parámetros físicos del sistema.

**Objetivo de comparación:** Las métricas de evaluación son Accuracy, F1-score, Precision y Recall, comparadas contra el Transformer encoder-only entrenado sobre las mismas variables. El baseline matemático no requiere entrenamiento (excepto la calibración de $\sigma$) y sirve para cuantificar cuánto aporta el ML sobre el mejor modelo analítico posible con estos parámetros.

**Limitación conocida del baseline:** No captura interacciones no lineales entre Doppler, SF y colisiones simultáneas, ni patrones temporales complejos. Estas son precisamente las zonas donde se espera que el Transformer supere al modelo matemático, y su identificación constituye parte de la contribución de la tesis.
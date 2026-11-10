# Informe: nueva arquitectura maestro–estudiante con el maestro geométrico (`geo`)

**Proyecto:** predicción, en el propio dispositivo, del éxito de una transmisión LoRa directa a satélite (DtS-IoT), para no transmitir cuando el paquete va a fallar y así ahorrar batería.
**Simulador:** FLoRaSat (OMNeT++ 6.3.0 + INET 4.5), escenario `Test_KD`, 10 repeticiones (seeds 0–9).
**Fecha:** 2026-10-29. **Actualizado:** 2026-10-31, con la versión final del pipeline (sección 7.7).

---

## 0. Resumen ejecutivo

1. **El dataset se corrigió.** Los cambios se hicieron en el simulador, en el preprocesamiento y en el pipeline de entrenamiento (sección 2). Con los datos corregidos, la etiqueta de las transmisiones visibles se reproduce en un **99.93 %** con tres reglas del receptor del satélite: la máscara de elevación y dos reglas de colisión por pares, **R1** (captura) y **R2** (solapamiento en frecuencia).
2. **El maestro antiguo es un buen predictor, pero enseña mal.** Es una red de 3 ramas (SetTransformer + TransformerEncoder + MLP) que ve a los interferentes reales. Predice bien (AUC visible 0.925), pero sobre los datos corregidos su destilación **no mejora de forma significativa** al estudiante (+0.22 %, p_holm = 0.55). La razón es que enseña información que el estudiante nunca podrá observar.
3. **Se propone una arquitectura nueva.** El maestro es un **gradient boosting** sobre las 8 entradas del estudiante más **5 features de vecindad geométrica**, con base física en R1 y R2, y sus logits son **out-of-fold**. El estudiante es el mismo MLP 8-64-32-1, con la misma pérdida de destilación, y el modelo que se despliega en el simulador no cambia.
4. **Resultados (10 seeds, repeticiones no usadas para elegir hiperparámetros).** Con el 2–15 % de los datos etiquetados, el estudiante destilado de `geo` mejora el AP visible en **+1.57 %** frente al estudiante sin destilación (10/10 seeds, p_holm = 0.018). La mejora llega a +2.0 % con el 2 % de los datos. Es **siete veces la ganancia del maestro antiguo**. Un control con la geometría permutada demuestra que las features geométricas aportan información real (+0.39 %, 9/10 seeds, p_holm = 0.023).
5. **El criterio de selección de α/T también cambia.** Elegir α/T por **AP visible de validación**, en lugar de por `val_loss`, triplica la ganancia, y la mejora vale para cualquier maestro.
6. **Las decisiones de implementación del maestro se validaron con ablaciones** (sección 7.6).
   - Destilar logits **in-sample** en vez de **out-of-fold** reduce la ganancia de +1.63 % a +1.00 % (p = 0.010).
   - Obtener el logit con `decision_function` en vez de `logit(clip(predict_proba))` da un resultado igual o algo mejor (+1.63 % frente a +1.50 %).
7. **Versión final del pipeline (sección 7.7).** Tiene tres cambios:
   - logits del maestro con `decision_function`;
   - rejilla ampliada a α ∈ {0, 0.05, …, 0.9} × T ∈ {1, 2, 4, 8};
   - early stopping por AP de validación sobre todos los paquetes.

   Con ella, la ganancia sube a **+2.4 % de AP visible** en el 2–15 % (+2.7 % con todos los paquetes), significativa en las 6 fracciones (p_holm ≤ 0.05), incluido el 100 % (+1.2 %, 10/10 seeds).
   - **No es sesgo de selección:** cualquier α ≤ 0.1 fijado de antemano da entre +1.85 % y +2.60 %.
   - **El origen es la rejilla ampliada:** con α = 0.3, T = 2 se reproduce el +1.4 % de la sección 7.3. El óptimo estaba en α ≤ 0.1, por debajo de la rejilla anterior.
   - **Eficiencia de datos:** el estudiante destilado con el **10 %** de los datos iguala al estudiante sin destilación entrenado con el **100 %**.

---

## 1. Contexto y planteamiento

### 1.1 Objetivo del estudiante

Antes de cada transmisión, el dispositivo calcula `p = P(el satélite recibe el paquete | x)` con un MLP pequeño. Si `p` está por debajo de un umbral, no transmite. Las entradas `x` son 8 magnitudes que el dispositivo conoce o puede calcular con las efemérides:

```
x = [dopplerShift, elevSat, latDev, longDev, loraTP, loraSF, alt, raan]
```

El dispositivo **no** sabe quién más transmite en ese momento. Esa información (los interferentes) solo existe en el simulador, después de los hechos, y por eso es **información privilegiada**: está disponible al entrenar, pero no al desplegar.

### 1.2 Por qué destilación

En la práctica habrá pocas transmisiones etiquetadas. Por eso el estudiante se entrena con fracciones de 2, 3, 5, 10, 15 y 100 % del train. La destilación (*knowledge distillation*, KD) busca que un maestro, entrenado con más datos o con más información, transmita al estudiante una señal más informativa que la etiqueta 0/1.

### 1.3 Métricas

- Alrededor del 65 % de las transmisiones están **bajo la máscara** (elevación < 10°) y fallan siempre. Cualquier modelo las acierta, así que las métricas globales quedan infladas: un baseline simple da AUC 0.94 con todas las filas y 0.76 con las visibles.
- Por eso **todas las comparaciones se hacen sobre las transmisiones visibles** (`belowMask == 0`), con el **AP** (*average precision*, área bajo la curva precisión–recall) como métrica principal.
- La ganancia se expresa en términos relativos: `(AP_KD − AP_noKD) / AP_noKD`.

---

## 2. La corrección del dataset

Los resultados con el dataset anterior (hasta el 2026-10-10) **no son comparables numéricamente** con los actuales. Estas son las correcciones:

### 2.1 Simulador y CSV (dataset regenerado el 2026-10-27)

| Problema | Corrección |
|---|---|
| El separador `;` de `KD_TransmissionStatistics` desalineaba columnas | Separador `,` y sentinels `-999` coherentes en `belowMask` |
| Transmisiones con elevación < 0 (detrás de la Tierra) contaban como interferentes | `LoRaMedium::isInterferingTransmission` las descarta |

### 2.2 Preprocesamiento (cambios B–G)

| Cambio | Qué corrige |
|---|---|
| B | Se excluyen del conjunto de interferentes las transmisiones con elevación < 0, igual que en el simulador |
| C | Un interferente bajo la máscara ya no cuenta como "visible": ~32 % de los "visibles" eran falsos |
| D | Interferentes **no causales** (`CAUSAL = False`): el 40 % de las recepciones rechazadas por R1 se deben solo a paquetes que empiezan *después* del inicio del target, porque el simulador reevalúa R1 durante la recepción |
| E | Flag `interf_below_mask` en cada interferente |
| F | Validaciones automáticas (`validate_cache.py`: 28 comprobaciones × 30 archivos, 0 fallos) |
| G | `belowMask` como metadato y métricas por subconjunto (visibles, fuga de la máscara, transmisiones evitadas, éxitos perdidos) |

### 2.3 Pipeline de entrenamiento (revisión del 2026-09-25)

Se corrigieron errores que invalidaban las comparaciones anteriores:
- **la salida del estudiante pasaba por ReLU y Dropout**, así que el logit nunca era negativo y el modelo predecía siempre "llega";
- **validación y test estaban intercambiados**, lo que filtraba el test en la selección de hiperparámetros y del umbral;
- los scalers se ajustaban incluyendo padding y sentinels −999: la potencia recibida escalada quedaba con std 0.008, prácticamente constante;
- las métricas de validación se calculaban con predicciones 0/1 en lugar de probabilidades.

### 2.4 Consecuencia: ahora se conoce el mecanismo exacto de la etiqueta

Al revisar el receptor del satélite (`LoRaReceiver.cc`) sobre el dataset corregido, se encontró que el satélite actúa como gateway y acepta recepciones concurrentes. Una transmisión visible falla solo por dos reglas por pares, y replicándolas sobre el CSV se acierta `rcvOk` en el **99.93 %** de 46 267 transmisiones visibles (seeds 0–2):

| Regla | Código en FLoRaSat | Condición de fallo para el target x frente a un interferente j | Huella en el CSV | % de los fallos visibles |
|---|---|---|---|---|
| Máscara | – | elevación < 10° | `belowMask = 1` | – |
| **R1**, captura | `isPacketCollided` (l. 190–300) | solapamiento temporal **y** misma portadora **y** `P_x − P_j < nonOrthDelta[SF_x][SF_j]` **y** el interferente cubre los últimos 6 símbolos del preámbulo | `rssi = −999` | 69 % |
| **R2**, frecuencia | `detectCollisionOverlap` (l. 510–590) | solapamiento temporal > 0 **y** `|f_x − f_j| < 1200 Hz`, para cualquier SF y potencia | `collision_flag = 1` | 31 % |

`nonOrthDelta` es la matriz de umbrales de señal/interferencia entre SF de FLoRa (1 dB con el mismo SF; de −8 a −25 dB entre SF distintos), que corresponde a la cuasi-ortogonalidad de LoRa medida por Croce et al. (2018). Como todos los dispositivos usan la misma portadora (868 MHz), `|f_x − f_j| = |doppler_x − doppler_j|` exactamente. El detalle de ambas reglas y de las unidades está en `geo_teacher/CALCULOS_R1_R2.md`.

**Este hallazgo, que solo fue posible con el dataset corregido, es la base del maestro nuevo:** si se sabe qué hace fallar a una transmisión, se pueden diseñar features que lo midan.

---

## 3. Fundamento teórico

### 3.1 Qué puede aprender el estudiante

El estudiante solo observa `x`. Lo máximo que puede aprender es

```
q(x) = P(ok | x)
```

es decir, la probabilidad de éxito promediada sobre todo lo que no ve (quién más transmite, cuándo y con qué potencia). Con las 8 entradas, el techo empírico es un **AUC visible de ~0.73–0.75** (medido con gradient boosting). Ningún maestro puede llevar al estudiante por encima de ese techo, porque no puede darle variables nuevas en el despliegue.

### 3.2 Cuándo ayuda la destilación

Según la perspectiva estadística de Menon et al. (2021), la destilación ayuda cuando la soft label del maestro, `p_T(x)`, **está más cerca de q(x) que la etiqueta dura**. La etiqueta 0/1 es una muestra ruidosa de q(x), y una soft label cercana a q(x) reduce la varianza del objetivo de entrenamiento. Eso pesa mucho con pocos datos.

Dos consecuencias:
- **Un maestro más preciso no es mejor profesor.** Si el maestro usa información que el estudiante no ve (los interferentes reales), sus probabilidades se acercan a la etiqueta realizada y no a q(x). Este efecto está documentado en la literatura (Cho & Hariharan 2019; Stanton et al. 2021).
- **La información privilegiada debe ser "transferible".** En el marco de *learning using privileged information* (Vapnik & Vashist 2009; Lopez-Paz et al. 2016), el maestro ayuda cuando la información privilegiada se puede expresar, al menos en parte, como función de lo que ve el estudiante. La geometría lo cumple: dónde están los demás dispositivos depende del satélite y de las órbitas, que el estudiante sí observa indirectamente (elevación, Doppler, alt, raan).

### 3.3 Logits out-of-fold

Un modelo que predice sobre sus propios datos de entrenamiento los memoriza, y sus probabilidades se pegan a 0 y 1, es decir, a la etiqueta dura. Por eso el maestro nuevo produce logits **out-of-fold**: cada transmisión del train recibe la predicción de un modelo que no la vio. Se usan 5 folds que reparten **simulaciones enteras**, porque las transmisiones de una misma simulación se interfieren entre sí y repartirlas por filas filtraría información entre folds.

Medido en las 10 seeds:

| AUC visible del maestro `geo` | OOF en el train (lo que recibe el estudiante) | Validación | In-sample en el train |
|---|---|---|---|
| | 0.74–0.82 | 0.71–0.78 | 0.85–0.89 (sobreconfiado) |

El OOF se comporta como la validación. El in-sample, en cambio, muestra la sobreconfianza que perjudica a la KD. Se comprobó directamente: destilando los logits in-sample, la ganancia del estudiante cae de +1.63 % a +1.00 % (−0.63 puntos, p = 0.010; sección 7.6).

**Conversión a logit.** En un gradient boosting binario con pérdida logística, el modelo aditivo que construyen los árboles ya está en la escala log-odds (Friedman 2001), y `predict_proba` solo le aplica la sigmoide. Por eso el logit del maestro es la salida cruda del modelo: `decision_function` en scikit-learn, o `output_margin=True` en XGBoost. Se verificó que `logit(predict_proba)` coincide con `decision_function` (diferencia ~10⁻⁸). La temperatura T divide ese logit, como en Hinton et al. (2015).

### 3.4 Diagnóstico: distancia de cada maestro a q(x)

`q(x)` se estimó con un gradient boosting sobre las 8 entradas, entrenado con las **otras 9 seeds**. La tabla muestra el MSE entre cada maestro y q(x) en las transmisiones visibles del train:

| Maestro | MSE a q(x) (menor = mejor profesor) | Accuracy visible |
|---|---|---|
| Etiqueta dura (referencia) | 0.201 ± 0.008 | 1.000 |
| **Antiguo** (3 ramas, in-sample) | 0.168 ± 0.007 | 0.823 |
| **geo** (OOF) | **0.024 ± 0.003** | 0.696 |

El maestro antiguo apenas se separa de la etiqueta dura; `geo` está siete veces más cerca de q(x). Este diagnóstico se hizo **antes** de entrenar los estudiantes, y anticipó el resultado final.

---

## 4. Arquitectura antigua frente a la nueva

### 4.1 Maestro antiguo: `TeacherCollisionPredictor`

```
seq_feats  [8 × 4]   historial del dispositivo ── TransformerEncoder (d=32) ── último token ─┐
static     [6]       lat, lon, TP, SF, alt, raan ── MLP (64, 32) ─────────────────────────────┤
interf_set [15 × 9]  interferentes REALES ── Linear ── SAB × k ── pooling por atención ───────┼─ concat ─ MLP ─ logit
context    [7]       resumen de interferentes ── MLP ─────────────────────────────────────────┘
```

- ~34 700 parámetros, red neuronal entrenada por épocas, logits **in-sample**.
- Ve **las transmisiones que realmente ocurrieron** y se solaparon con el target.
- AUC visible 0.925 ± 0.007.

### 4.2 Maestro nuevo: `geo`

```
                      ┌── 8 entradas del estudiante ─────────────────────────────┐
CSV del train ────────┤                                                          ├─ HistGradientBoosting ─ logits OOF
                      └── add_geo_features: 5 features de vecindad geométrica ───┘   (5 folds por simulación)
```

- `HistGradientBoostingClassifier(max_iter=300)` sobre 13 columnas.
- Las 5 features nuevas **no usan qué transmisiones ocurrieron en el instante del target**. Usan dónde están los demás dispositivos en ese instante y si, por su geometría, podrían provocar R1 o R2.
- ~10 s por seed en CPU, sin red neuronal.

### 4.3 Estudiante y entrenamiento (sin cambios)

```
x [8] ── StandardScaler ── Linear(8,64) ─ ReLU ─ Dropout(0.1) ─ Linear(64,32) ─ ReLU ─ Dropout(0.1) ─ Linear(32,1) ── logit

L = α · BCE_pos_weight(s(x), y)  +  (1 − α) · BCE( σ(s(x)/T), σ(z_T(x)/T) )
```

- Adam, lr 1e-3, batch 32, early stopping por `val_loss` con paciencia 30 y restauración de la mejor época.
- **El modelo desplegado es idéntico**: el mismo `.pth` y el mismo JSON de scalers que consume `Ter_Dlk_KiWan.cc`. Las features geométricas solo existen en el entrenamiento.

### 4.4 Tabla comparativa

| Aspecto | Maestro antiguo | Maestro `geo` |
|---|---|---|
| Modelo | Red de 3 ramas + contexto (SetTransformer, TransformerEncoder, MLP) | Gradient boosting |
| Información privilegiada | Interferentes **realizados** (`interf_set`, 15 × 9) | **Vecindad geométrica** (5 escalares por transmisión) |
| Relación con lo que ve el estudiante | Casi nula: quién transmite es aleatorio para el dispositivo | Parcial: la geometría depende de la órbita, que el estudiante observa indirectamente |
| Logits | In-sample (sobreconfiados) | Out-of-fold, folds por simulación |
| AUC visible del maestro | **0.925** | 0.71–0.78 (validación) |
| MSE a q(x) | 0.168 (≈ etiqueta dura, 0.201) | **0.024** |
| Ganancia del estudiante, 2–15 % | +0.22 % (n.s.) | **+1.57 %** (p_holm = 0.018) |
| Costo de entrenamiento | Red neuronal entrenada por épocas (37 s con solo 15 épocas, seed 0) | ~10 s por seed, CPU, sin épocas |
| Dependencias del preprocesamiento | `interf_set`, `seq_feats`, `context_feats`, `inter_in_dim` | Solo el CSV |
| Interpretabilidad | Baja (atención) | Alta: cada feature es un conteo con significado físico |
| Estudiante desplegado | MLP 8-64-32-1 | **El mismo** |

---

## 5. Las features geométricas y su justificación

### 5.1 Definición

Para cada transmisión x del dispositivo i en el instante t, y para cada **otro** dispositivo j de la misma simulación, se estiman en t la elevación, el Doppler y la potencia recibida de j: `elev_j(t)`, `dopp_j(t)` y `P_j(t)`. Con ellas:

| Feature | Definición (suma sobre j ≠ i) | Regla física que aproxima |
|---|---|---|
| `g_up` | `1[elev_j(t) ≥ 0]` | Dispositivos con línea de vista al satélite: los únicos que pueden interferir (`LoRaMedium`) |
| `g_near_dopp` | `1[up] · 1[|dopp_j(t) − dopp_x| < 1200 Hz]` | Candidatos a **R2** |
| `g_near_dopp_load` | lo mismo · `(dur_x + dur_j)/50` | R2 ponderado por la probabilidad de solaparse en el tiempo |
| `g_kill` | `1[up] · 1[P_x − P_j(t) < nonOrthDelta[SF_x][SF_j]]` | Candidatos a **R1** (sin efecto captura a favor de x) |
| `g_kill_load` | lo mismo · `(dur_x + dur_j)/50` | R1 ponderado por la probabilidad de solaparse en el tiempo |

### 5.2 Justificación de cada componente

**Por qué contar dispositivos "peligrosos" y no transmisiones reales.** La etiqueta depende de que **exista** un interferente letal que se solape en el tiempo. El estudiante no puede saber si j transmite justo en t, pero sí podría inferir, a partir de la órbita, cuántos dispositivos están en posición de ser letales. Las features miden ese riesgo **esperado**: son una información privilegiada "suave", a medio camino entre lo que ve el estudiante (nada sobre j) y lo que ve el maestro antiguo (el j real). Es la condición de transferibilidad de la sección 3.2.

**R2: `|Δdoppler| < 1200 Hz`.**
- El umbral de 1200 Hz es el del simulador (`LoRaReceiver.cc:569-571`), no una elección del maestro.
- El simulador compara frecuencias desplazadas por Doppler, `f = CF + doppler`. Con una sola portadora, la diferencia de frecuencias es exactamente la diferencia de Doppler. Este supuesto se valida al construir el dataset: si hubiera más de una portadora, el script se detiene.
- Físicamente, dos dispositivos con Doppler parecido están en posiciones parecidas respecto del satélite (Doppler ∝ velocidad radial), así que la feature agrupa dispositivos cercanos en el footprint.

**R1: `P_x − P_j < nonOrthDelta[SF_x][SF_j]`.**
- Es la condición de captura de FLoRa: x sobrevive a j solo si lo supera por el margen de la matriz. La matriz es asimétrica y depende del **par** de SF: con el mismo SF basta 1 dB, y con SF distintos la ortogonalidad imperfecta tolera interferentes 8–25 dB más fuertes.
- `P_j` es `rx_power_dbm_at_dev`, la potencia del enlace que calcula el propio dispositivo con el modelo de propagación (FSPL, ganancias, línea de vista). Se comprobó que **coincide exactamente** con la potencia en el satélite (`rx_power_dbm`, diferencia máxima 0.0000 dB), que **nunca vale −999** y que **no depende del resultado de la recepción**: no filtra la etiqueta.
- Todas las potencias están en dBm, así que su diferencia está en dB, la misma unidad que usa `nonOrthDelta`.

**`g_up`: elevación ≥ 0.** Es la condición del simulador para interferir. Un dispositivo entre 0° y 10° no se decodifica, pero **sí interfiere**, y por eso el umbral es 0° y no la máscara de 10°.

**Términos `*_load`: probabilidad de solapamiento.** En un acceso tipo ALOHA, dos paquetes de duraciones `d_x` y `d_j`, enviados en instantes independientes, se solapan si sus inicios están a menos de `d_x + d_j`. La ventana vulnerable mide `d_x + d_j`, y la probabilidad de solape es aproximadamente `(d_x + d_j)/T_j`, donde T_j es el periodo entre transmisiones de j.
- La duración depende del SF: 0.44 s con SF10 y 1.58 s con SF12. Un vecino peligroso con SF alto es más peligroso porque ocupa el canal más tiempo.
- Se usa el solapamiento **esperado** y no el real **a propósito**: el real es la información que el estudiante no puede tener, y usarlo devolvería al maestro hacia la etiqueta realizada.
- La constante 50 es la media del intervalo entre paquetes configurada en el simulador (`timeToNextPacket = exponential(50s)`, sección 5.4). Como el gradient boosting corta por umbrales, una constante global no cambia el ranking; usar el T_j medido de cada vecino es una mejora pendiente (`geo_v2`).

**Por qué interpolar.** La posición de j solo se conoce en los instantes en que j transmite. Para tenerla en t se interpola linealmente entre las dos transmisiones de j más cercanas, sin extrapolar fuera de su rango.
- Se evaluaron alternativas con *leave-one-out* (ocultar una transmisión conocida y reconstruirla). La interpolación lineal resultó la más precisa para el Doppler con huecos normales: error mediano de 101 Hz y p90 de 183 Hz, muy por debajo del umbral de 1200 Hz.
- PCHIP solo mejora con huecos grandes. Vecino más cercano y "valor anterior" son peores.
- La solución exacta sería propagar la órbita.

**Exclusión del propio dispositivo.** Un dispositivo no se interfiere a sí mismo. Sin la exclusión, todas las features visibles subirían en +1 de forma artificial.

**Ausencia de fuga de información.** Ninguna columna derivada del resultado de la recepción (`rssi_dbm`, `snr_db`, `rx_power_dbm`, `collision_flag`, `num_colliding_packets`, `belowMask`, `rcvOk`) entra como feature. Las features de cada split se calculan solo con los dispositivos de ese split.

### 5.3 Ejemplo (dispositivo 1, SF10, t = 30 s, Doppler 1000 Hz, P = −128 dBm)

| Vecino j | elev_j(30) | \|Δdoppler\| | P_x − P_j | Umbral | up | near (R2) | kill (R1) |
|---|---|---|---|---|---|---|---|
| 2 (SF10) | 30° | 500 Hz < 1200 | −2 dB | 1 dB → −2 < 1 | ✔ | ✔ | ✔ |
| 3 (SF12) | 2.5° | 2500 Hz | +8.25 dB | −18 dB → no | ✔ | ✘ | ✘ |

El resultado es `g_up = 2`, `g_near_dopp = 1` y `g_kill = 1`. El vecino 2 es peligroso por las dos reglas. El vecino 3 está sobre el horizonte, pero lejos en frecuencia, y con SF12 frente a SF10 necesitaría ser 18 dB más fuerte para anular la captura. El ejemplo completo y reproducible está en `geo_teacher/ejemplo_add_geo_features.py` y en el `.ipynb` del mismo nombre.

### 5.4 Correspondencia con FLoRaSat: de dónde sale cada feature en el simulador

Cada feature geo es una versión "esperada" de una comprobación que el simulador hace con las transmisiones reales. Las rutas son relativas a `florasat_v4_ult_routing/src/`.

#### a) Magnitudes de entrada: de dónde viene cada columna del CSV

Todas las columnas se escriben en `Global/Statistics/TransmissionStatistics/KD_TransmissionStatistics.cc` (cabecera en l. 84) a partir del tag `CstTransmissionStatisticsTag` que lleva cada paquete.

| Columna usada | Cálculo en FLoRaSat | Equivalente en el receptor del satélite |
|---|---|---|
| `elevSat` | `Ter_Dlk_KiWan::sendData`: `sat->getElevation(*ter)` (l. 566) → `transmissionTag->setElevSat` (l. 634) | Es el mismo valor que lee `LoRaMedium::isInterferingTransmission` (`tag->getElevSat()`) |
| `dopplerShift` | En el satélite, `LoRaAnalogModel::computeDopplerFrequency` (`Global/Channel/LoRaAnalogModel.cc:208-275`): `f_rx = CF·(c + v_r)/c`. Luego `setDopplerShift(f_rx − CF)` (l. 299) y `setDopplerShiftedFrequency(f_rx)` (l. 303). `Sat_Dsl_Phy_LoRaRadio.cc:317/361` lo pasa a `statsMap`, y `KD_TransmissionStatistics.cc:234/274` al tag. Si no hubo recepción (bajo la máscara), lo calcula el propio dispositivo con la misma fórmula (`Ter_Dlk_KiWan.cc:566-631`, `setDoppler`) | R2 compara `getDopplerShiftedFrequency()`, es decir, `CF + dopplerShift` |
| `rx_power_dbm_at_dev` | `Ter_Dlk_KiWan::computeReceptionPower` (`Ter_Dlk_KiWan.cc:432-481`): `P_tx · min(1, G_tx·G_rx·pathLoss·LOS)` → `mW2dBmW` → `setRx_power_dbm_dev` (l. 644) | R1 usa `loRaReception->getPower()` convertida a dBm (`LoRaReceiver.cc:209-211`) y `loRaInterference->getPower()` (l. 255-257). Se comprobó que coinciden exactamente (diferencia 0.0000 dB) |
| `loraSF` | Parámetro del dispositivo (`.ini`) | `getLoRaSF()` del target y del interferente en R1 (`LoRaReceiver.cc:238`, `:258`) |
| `tx_duration` | Tiempo en el aire LoRa en `LoRaTransmitter::createTransmission` (`Global/Transceiver/LoRaTransmitter.cc:79-89`): `Tsym = 2^SF/BW`, preámbulo `(8 + 4.25)·Tsym` + cabecera + payload; se registra con `setTxDuration` (`KD_TransmissionStatistics.cc:200/246`) | `getEndTime() − getStartTime()` de cada recepción en R1 y R2 |
| Periodo entre transmisiones (escala de `*_load`) | `simulations/Test_KD/*.ini`: `**.timeToNextPacket = exponential(50s)`, un proceso de Poisson con media de 50 s | La estadística de llegadas de los interferentes |

#### b) Cada feature frente a la comprobación del simulador

| Feature | Comprobación en FLoRaSat | Qué hace el simulador | Qué hace la feature (diferencia) |
|---|---|---|---|
| `g_up` | `LoRaMedium::isInterferingTransmission` (`Global/Medium/DSL/LoRaMedium.cc:339-350`) | Si `tag->getElevSat() < 0`, la transmisión **no interfiere** (la bloquea la Tierra) | Cuenta los otros dispositivos con `elev_j(t) ≥ 0`. Mismo criterio, aplicado a la posición de j en t y no a una transmisión real de j |
| `g_near_dopp` | `LoRaReceiver::detectCollisionOverlap` (`Global/Transceiver/LoRaReceiver.cc:510-590`), llamada en `computeIsReceptionSuccessful` (l. 490) si `enableCollisionOverlap = true` (`Test_KD_*.ini`, l. 60-63) | Si `overlapDuration > 0` (l. 562-564) **y** `|freq_x − freq_y| < 1200 Hz` (l. 569-571) → `collision_flag = 1` | Exige `up` y `|dopp_j(t) − dopp_x| < 1200`. Con una sola CF, `|freq_x − freq_y| = |dopp_x − dopp_j|` exactamente. **No** exige el solapamiento temporal |
| `g_kill` | `LoRaReceiver::isPacketCollided` (`LoRaReceiver.cc:190-300`), llamada desde `computeIsReceptionAttempted` (l. 152) | Hay colisión (`rssi = −999`) si coinciden: solapamiento `|m_x − m_y| < d_x + d_y` (l. 204-205, 247-250), misma CF (l. 252), **sin captura** `P_x − P_j < nonOrthDelta[SF_x−7][SF_j−7]` (l. 261; matriz en `LoRaReceiver.h:67-74`) y el interferente cubre los últimos 6 símbolos del preámbulo (l. 276-280) | Exige `up` y `P_x − P_j(t) < nonOrthDelta[SF_x][SF_j]`, con la misma matriz y las mismas unidades (dBm → dB). **No** exige el solapamiento ni la condición del preámbulo |
| `g_near_dopp_load`, `g_kill_load` | Las condiciones de solapamiento temporal de R2 (l. 562-564) y de R1 (l. 247-250) | Usan los instantes reales de inicio y fin de ambos paquetes | Sustituyen el solapamiento real por su **probabilidad** con llegadas de Poisson: los inicios deben caer a menos de `d_x + d_j`, así que `P ≈ (d_x + d_j)/T`, con `T = 50 s`, la media de `timeToNextPacket` |

En resumen, **las features evalúan las mismas condiciones físicas que el receptor del satélite, con las mismas constantes (0°, 1200 Hz, `nonOrthDelta`) y magnitudes equivalentes (Doppler, potencia en dBm, SF, tiempo en el aire)**. Solo reemplazan "¿j transmitió realmente y se solapó con x?", que el estudiante no puede saber, por "¿j estaba en posición de matar a x?, y ¿con qué probabilidad se solaparía?".

**Una precisión sobre la escala 50.** Coincide con la media configurada de `timeToNextPacket`, así que `(d_x + d_j)/50` es la probabilidad de solape del modelo ALOHA con la tasa del `.ini`. Sin embargo, el intervalo **medido** en el CSV entre transmisiones de un mismo dispositivo es menor (mediana 13 s, media 28 s, seed 0). Una explicación posible, no verificada, son las ventanas de transmisión de `Ter_Dlk_KiWan` (`startTransmissionWindow`, `checkForWaitingData`), que agrupan los envíos. Por eso `geo_v2`, que divide por el T_j medido de cada vecino, sigue siendo una mejora razonable.

---

## 6. Protocolo experimental

- **Datos:** dataset corregido, 10 seeds, que son 10 repeticiones del simulador con distinta semilla (ver 6.1).
  - El reparto de las 24 configuraciones orbitales (alt, raan) es **el mismo en todas las seeds**: 16 para train, 4 para validación y 4 para test.
  - Los dispositivos de test están en posiciones que no aparecen en el train, así que es una prueba de generalización real.
- **Fracciones del train:** 2, 3, 5, 10, 15 y 100 %.
- **Brazos comparados:**
  - `no_kd`: el estudiante sin destilación;
  - `geo`: el maestro propuesto;
  - el maestro antiguo;
  - tres controles: `self` (GBM OOF solo con las 8 entradas), `geo_perm` (`geo` con las 5 features permutadas entre filas) y label smoothing (ε ∈ {0.1, 0.2, 0.3}).
- **Selección sin sesgo:**
  - en la repetición 0 se prueba la rejilla α ∈ {0.3, 0.5, 0.7} × T ∈ {1, 2} y se elige, por fracción, la configuración con mayor **AP visible de validación**;
  - **los resultados se reportan con las repeticiones 1 y 2**, que usan otros subconjuntos y otras semillas, no se usaron para elegir nada y son iguales para todos los brazos (comparación emparejada).
- **Análisis primario**, fijado antes de ver las repeticiones 1–2:
  - ganancia relativa de AP visible en test, promediada sobre el 2–15 %;
  - Wilcoxon emparejado por seed (n = 10), con corrección de Holm sobre 9 contrastes;
  - intervalo de confianza bootstrap del 95 %.
- **Total:** 3 160 entrenamientos del estudiante.

### 6.1 Seeds del simulador y repeticiones de entrenamiento

El protocolo usa dos niveles de aleatoriedad. Los dos se llaman "repetición" en algún lugar, pero son cosas distintas.

**Seed (0–9): repetición del simulador.** Cada seed es una corrida de FLoRaSat con otra semilla del generador aleatorio de OMNeT++. En los `.ini` de `Test_KD`:

```
repeat    = 10
seed-0-mt = ${23542, 84242, 15937, 42781, 63029, 88143, 27456, 51904, 76328, 92115 ! repetition}
```

`merge_files.ipynb` junta los CSV de cada carpeta `seed_k/` en `seed_k_final_{train,val,test}_data_transmission.csv`.

- **Qué queda igual en las 10 seeds:**
  - las 24 configuraciones orbitales y su reparto en train, validación y test;
  - 60 dispositivos por simulación;
  - 1 199 s de simulación.
- **Qué cambia con la semilla:**
  - las posiciones de los dispositivos. Las 60 ubicaciones base de cada split son fijas (`devices/WorldWide-60-{train,val,test}.ini`). La mobility de FLoRaSat desplaza al azar cada dispositivo unos grados alrededor de su ubicación base; en el CSV, el desplazamiento medio es de ~1.5° y el máximo de ~3–4°. Dentro de una simulación la posición es fija, pero cambia un poco de una seed a otra;
  - el SF (`intuniform(10, 12)`) y la potencia (`intuniform(12, 14)`) de cada dispositivo;
  - los instantes de transmisión (`exponential(50s)`).

  Con eso cambian quién interfiere con quién, las etiquetas y el número de transmisiones: por ejemplo, 35 666 filas de train en la seed 0 y 34 936 en la seed 1.
- **Qué depende de la seed:** cada seed es un dataset distinto. Tiene su propio maestro y sus propios logits OOF, y su propia validación y su propio test.

**Repetición de entrenamiento (rep 0, 1, 2): una nueva tirada del estudiante dentro de la misma seed.** Con los mismos datos, el mismo maestro, la misma validación y el mismo test, se vuelve a entrenar el estudiante con la semilla `seed + 1000·rep`. Esa semilla decide tres cosas:

1. **qué filas forman el subconjunto de train** (qué 2 %, qué 5 %, …);
2. **los pesos iniciales de la red;**
3. **el orden de los batches y las máscaras de dropout.**

Ejemplo con la seed 3 y el 5 % de los datos (unas 1 750 filas):

| rep | semilla | uso |
|---|---|---|
| 0 | 3 | un subconjunto de ~1 750 filas y unos pesos iniciales: **elegir α/T** |
| 1 | 1003 | otro subconjunto y otros pesos: **medir** |
| 2 | 2003 | otro subconjunto y otros pesos: **medir** |

Las tres se evalúan con la misma validación y el mismo test de la seed 3.

**Para qué sirve cada nivel:**

| | Seed (simulador) | Rep (entrenamiento) |
|---|---|---|
| Responde a | ¿el resultado se mantiene con otro despliegue: otras posiciones, SF, potencias y tráfico? | ¿el resultado se mantiene con otro subconjunto de datos y otra inicialización? |
| Cuántas | 10 | 3 (rep 0 para elegir; reps 1 y 2 para medir) |
| Qué cambia | los datos, el maestro y el test | solo el estudiante |
| En la estadística | es la unidad del test de Wilcoxon (n = 10) | se promedia dentro de cada seed |

**Por qué se separan la rep 0 y las reps 1–2.** La combinación α/T que gana en la rep 0 suele ser buena y además haber tenido algo de suerte en esa tirada. Si se reportara su resultado en la misma rep 0, la ganancia saldría inflada (sesgo de selección, o "maldición del ganador"). Las reps 1 y 2 no intervinieron en la elección, así que dan una estimación honesta.

**Comparación emparejada.** Dentro de una misma seed y rep, el estudiante con destilación y el estudiante sin ella comparten:
- el subconjunto de filas;
- los pesos iniciales;
- el orden de los ejemplos.

La única diferencia es la función de pérdida. Por cada fracción hay 10 seeds × 2 reps = 20 pares. Las dos reps de cada seed se promedian, y el contraste final se hace sobre las 10 seeds emparejadas.

**Para la evaluación en closed-loop**, las reps no son necesarias. Basta con elegir α/T con la rep 0 y exportar el par con y sin destilación, porque el simulador ya es una evaluación independiente. Las reps siguen siendo útiles para dos cosas:
- elegir un modelo representativo, no uno que tuvo suerte;
- conocer cuánto varía el resultado entre entrenamientos.

---

## 7. Resultados

### 7.1 Contraste primario: ganancia relativa de AP visible, media sobre el 2–15 %

| Contraste | Ganancia | IC 95 % | Seeds a favor | p_holm |
|---|---|---|---|---|
| **geo − no_kd** | **+1.57 %** | [1.20, 1.96] | **10/10** | **0.018** |
| self − no_kd | +1.28 % | [0.92, 1.64] | 10/10 | 0.018 |
| geo_perm − no_kd | +1.18 % | [0.83, 1.56] | 10/10 | 0.018 |
| antiguo − no_kd | +0.22 % | [−0.12, 0.57] | 7/10 | 0.55 (n.s.) |
| label smoothing − no_kd | −0.25 % | [−0.58, 0.04] | 4/10 | 0.55 (n.s.) |
| **geo − geo_perm** | **+0.39 %** | [0.21, 0.56] | 9/10 | **0.023** |
| geo − self | +0.29 % | [0.11, 0.45] | 8/10 | 0.082 |
| antiguo − self | **−1.03 %** | [−1.25, −0.80] | **0/10** | 0.018 |

Con n = 10, el menor p de Wilcoxon posible es 0.002.

### 7.2 Por fracción de datos (ganancia relativa sobre `no_kd`)

| Fracción | AP visible `no_kd` | **geo** | antiguo |
|---|---|---|---|
| 2 % | 0.668 | **+1.99 %** | +0.82 % |
| 3 % | 0.675 | **+1.79 %** | +0.31 % |
| 5 % | 0.685 | **+1.06 %** | −0.21 % |
| 10 % | 0.690 | **+1.54 %** | +0.21 % |
| 15 % | 0.694 | **+1.47 %** | +0.01 % |
| 100 % | 0.706 | +0.47 % (n.s.) | −0.13 % |

En valor absoluto, +1.6 % relativo equivale a **~+0.011 de AP visible** (por ejemplo, 0.668 → 0.681 con el 2 %). La distancia entre `no_kd` con el 2 % y con el 100 % es de 0.038. Por lo tanto, **`geo` recupera entre un cuarto y un tercio de lo que se gana al pasar de 2 % a 100 % de los datos etiquetados** (0.010–0.013 de 0.038).

### 7.3 Replicación independiente (pipeline autocontenido `geo_teacher/`)

Se reimplementó todo el flujo en una carpeta que no depende del pipeline antiguo. Lee directamente los CSV, y el escalado del estudiante es el mismo que aplica el simulador. Se repitió la corrida completa con 10 seeds (660 entrenamientos):

| Fracción | α / T elegido | AP `no_kd` | AP `geo` | Ganancia | Seeds a favor | p (Wilcoxon) |
|---|---|---|---|---|---|---|
| 2 % | 0.3 / 2 | 0.668 | 0.678 | **+1.52 %** | 7/10 | 0.027 |
| 3 % | 0.3 / 2 | 0.676 | 0.686 | **+1.47 %** | 10/10 | 0.002 |
| 5 % | 0.3 / 2 | 0.682 | 0.694 | **+1.69 %** | 9/10 | 0.004 |
| 10 % | 0.3 / 2 | 0.690 | 0.701 | **+1.69 %** | 8/10 | 0.014 |
| 15 % | 0.3 / 2 | 0.693 | 0.701 | **+1.14 %** | 9/10 | 0.049 |
| 100 % | 0.3 / 1 | 0.706 | 0.710 | +0.52 % | 6/10 | 0.106 |

La media en el 2–15 % es **+1.50 %**, consistente con el +1.57 % de la sección 7.1. En las 30 cachés, las etiquetas y las 8 entradas son idénticas a las del pipeline antiguo.

### 7.4 El criterio de selección de α/T

| Criterio | α elegido | Ganancia de `geo` (2–15 %) |
|---|---|---|
| Menor `val_loss` (pipeline antiguo) | siempre 0.7 | ~+0.4 % |
| **Mayor AP visible de validación** | 0.3 | **~+1.5 %** |

`val_loss` es una BCE contra la etiqueta dura, así que premia al estudiante que más se parece a `no_kd`. Además, el AP debe calcularse **solo sobre las visibles**: las transmisiones bajo la máscara (~65–70 % de la validación) las acierta cualquier modelo y diluyen las diferencias.

### 7.5 Métricas por clase (umbral = mejor F1 de la clase 1 en validación)

Las métricas por clase se reportan para completar el análisis, pero **la ganancia en AP no se traduce de forma consistente en mejores métricas por clase** con el umbral actual:
- con el 2–3 %, el umbral elegido para `geo` es más bajo, el estudiante transmite más (79 % frente a 74–75 %) y bloquea menos fallos;
- con el 5–10 %, ambos brazos quedan prácticamente iguales;
- con el 15 %, `geo` mejora las dos clases (macro-F1 0.600 frente a 0.572);
- con el 100 %, empatan.

El umbral, más que el modelo, determina el punto de operación. Para el objetivo de batería conviene elegir el umbral con un criterio de costo explícito (por ejemplo, máximo de fallos evitados con recall de éxitos ≥ 90 %), no con el F1 de la clase 1.

### 7.6 Ablaciones del maestro: cómo se generan los logits

Dos decisiones de implementación del maestro se probaron directamente. Ambas pruebas usan el pipeline `geo_teacher/`, 10 seeds y las repeticiones 1–2, con los mismos subconjuntos y semillas que en la sección 7.3. Lo único que cambia son los logits del maestro.

| Variante | Cómo se obtiene el logit de cada fila del train | α/T |
|---|---|---|
| **OOF + `clip`** (la de las secciones 7.1–7.5) | 5 modelos por simulación; `logit(clip(predict_proba, 10⁻⁴, 1−10⁻⁴))` | Elegidos en la rep 0 |
| **OOF + `decision_function`** | Los mismos 5 modelos; `decision_function`, el log-odds crudo del GBM, sin recorte | Los mismos que OOF + `clip` |
| **In-sample** | Un solo modelo con el 100 % del train, que predice ese mismo train (`decision_function`) | Elegidos en su propia `grid` (rep 0) |

**Ganancia relativa de AP visible sobre `no_kd`:**

| Fracción | AP `no_kd` | OOF + `clip` | OOF + `decision_function` | In-sample |
|---|---|---|---|---|
| 2 % | 0.668 | +1.52 % | **+1.84 %** | +1.68 % |
| 3 % | 0.676 | +1.47 % | +1.66 % | **+1.68 %** |
| 5 % | 0.682 | **+1.69 %** | +1.67 % | +0.46 % |
| 10 % | 0.690 | +1.69 % | **+1.75 %** | +0.43 % |
| 15 % | 0.693 | +1.15 % | **+1.24 %** | +0.77 % |
| 100 % | 0.706 | +0.52 % | +0.52 % | +0.26 % |
| **Media 2–15 % (por seed)** | | +1.50 % (9/10) | **+1.63 %** (9/10) | +1.00 % (10/10) |

**Contrastes emparejados** (media 2–15 % por seed, Wilcoxon, n = 10):

| Contraste | Diferencia | Seeds a favor | p |
|---|---|---|---|
| OOF + `decision_function` − OOF + `clip` | +0.13 puntos | 8/10 | 0.037 |
| In-sample − OOF + `decision_function` | **−0.63 puntos** | 2/10 | **0.010** |
| In-sample − OOF + `clip` | −0.50 puntos | 2/10 | 0.027 |

En el 5–10 %, el in-sample queda 1.2–1.3 puntos por debajo del OOF, con p = 0.004 y p = 0.010 respectivamente.

**Soft labels del maestro** (transmisiones visibles del train, media de 10 seeds):

| | AUC visible | Accuracy | Distancia media a la etiqueta |
|---|---|---|---|
| OOF | 0.74–0.82 | 0.696 | 0.384 |
| In-sample | 0.85–0.89 | 0.780 | 0.335 |

**Lectura:**
1. **Los logits out-of-fold están justificados empíricamente.**
   - El maestro in-sample predice filas que vio y queda sobreconfiado. Sus soft labels se acercan a la etiqueta y le enseñan menos al estudiante: conserva solo ~60 % de la ganancia del OOF.
   - La pérdida se concentra en el 5–15 %. Allí las filas del estudiante son más numerosas y sus soft labels in-sample se parecen más a la etiqueta que el estudiante ya ve. Coherentemente, su `grid` eligió α = 0.5–0.7, es decir, menos peso al maestro.
   - Con el 2–3 % empatan, porque domina la transferencia desde el resto del train, que ambos maestros tienen.
2. **El in-sample no anula la destilación** (+1.00 %), a diferencia de lo observado con el maestro antiguo. Una explicación plausible, no verificada, es la regularización del `HistGradientBoosting` (hojas de al menos 20 filas y early stopping interno), que limita la memorización.
3. **`decision_function` es equivalente o algo mejor que `logit(clip(·))`.**
   - En las filas no recortadas, los logits coinciden (diferencia < 10⁻⁶).
   - El recorte afecta al 40 % del train, y todas esas filas están bajo la máscara: `decision_function` les asigna su valor real (≈ −10 a −13 en lugar de −9.21).
   - La mejora de +0.13 puntos es pequeña, del orden de la variación entre corridas, y no es significativa en ninguna fracción por separado. Se interpreta como "al menos igual".
   - **Se recomienda adoptar `decision_function`** porque es la salida nativa del modelo y elimina un recorte arbitrario. Antes de adoptarlo hay que repetir la `grid` para confirmar los α/T.

### 7.7 Versión final del pipeline (`my_final_version`, 2026-10-31)

La versión final reúne las recomendaciones de las secciones anteriores y es la que se exportará a FLoRaSat. El detalle completo está en `my_version_KD_gradient/KD_gradient_boosting/my_final_version/INFORME_RESULTADOS_STUDENT_KD.md`.

**Cambios respecto a las secciones 7.1–7.6:**

| | Secciones 7.1–7.6 | Versión final |
|---|---|---|
| Logits del maestro `geo` | OOF + `clip` | **OOF + `decision_function`** (adopta la recomendación de 7.6) |
| Rejilla α/T | α ∈ {0.3, 0.5, 0.7} × T ∈ {1, 2} | **α ∈ {0, 0.05, 0.1, 0.3, 0.5, 0.7, 0.9} × T ∈ {1, 2, 4, 8}** |
| Early stopping | `val_loss` | **AP de validación sobre todos los paquetes**, paciencia 30 |
| Épocas máximas | `ceil(20 / frac)` (20 al 100 %) | `max(ceil(20 / frac), 60)` |
| Selección de α/T | AP visible en la rep 0 | Media de `val_ap_all` sobre las 10 seeds |
| Repeticiones | rep 0 para elegir; reps 1–2 para medir | Una tirada por seed. El test no interviene en la selección. |
| Entrenamientos | 3 160 | 1 740 = 6 fracciones × (1 + 28 combinaciones) × 10 seeds |

**Por qué el early stopping usa AP sobre todos los paquetes y no solo sobre los visibles.** Esto matiza la sección 7.4.

- **Para comparar configuraciones**, las dos AP son equivalentes: ordenan igual las combinaciones (Spearman de 0.995 a 0.998 por fracción), porque los fallos bajo la máscara quedan al fondo del ranking.
- **En el despliegue**, en cambio, el estudiante decide también sobre los paquetes bajo la máscara, y solo la AP con todos los paquetes castiga que el modelo los deje pasar.
- **Con early stopping por `val_loss`**, la BCE dura con `pos_weight` no está alineada con la pérdida soft cuando α ≈ 0. Por eso, 17 runs con el 2 % de los datos, α ≤ 0.1 y T ≥ 4 paraban en la época 1, y dejaban pasar hasta el 98.7 % de los paquetes bajo la máscara.
- **Con early stopping por AP sobre todos los paquetes**, hay 0 colapsos y la fuga máxima en toda la rejilla es del 1.8 %.

**Resultados.** Test, media de 10 seeds, mejor α/T por fracción. Wilcoxon emparejado por seed con corrección de Holm sobre las 6 fracciones.

| Fracción | α / T | AP visible `no_kd` | AP visible KD | Ganancia relativa | Seeds a favor | p_holm | AUC visible `no_kd` → KD |
|---|---|---|---|---|---|---|---|
| 2 % | 0.05 / 1 | 0.670 | 0.687 | **+2.64 %** | 7/10 | 0.049 | 0.700 → 0.718 |
| 3 % | 0.05 / 1 | 0.677 | 0.693 | **+2.33 %** | 9/10 | 0.041 | 0.708 → 0.722 |
| 5 % | 0 / 4 | 0.681 | 0.694 | **+1.88 %** | 9/10 | 0.041 | 0.713 → 0.725 |
| 10 % | 0 / 8 | 0.687 | 0.706 | **+2.76 %** | 10/10 | 0.012 | 0.722 → 0.735 |
| 15 % | 0.05 / 8 | 0.689 | 0.705 | **+2.31 %** | 9/10 | 0.023 | 0.722 → 0.735 |
| 100 % | 0.3 / 4 | 0.708 | 0.716 | **+1.18 %** | 10/10 | 0.012 | 0.739 → 0.748 |

- **Media sobre el 2–15 %:** +2.39 % de AP visible, y +2.67 % con todos los paquetes (10/10 seeds, p = 0.002).
- **Al 100 %**, la destilación ahora es significativa. Antes daba +0.47 %, n.s.; aquel resultado estaba limitado por el tope de 20 épocas.
- **Eficiencia de datos:** KD con el 10 % (0.706) iguala a `no_kd` con el 100 % (0.707) en AP con todos los paquetes. En el criterio de la sección 7.2, `geo` recupera ahora **entre un tercio y la mitad** de lo que se gana al pasar del 2 % al 100 % (0.013–0.019 de 0.038 de AP visible), frente a entre un cuarto y un tercio antes.

**Control del sesgo de selección.** Como esta corrida no tiene repeticiones reservadas, se midió la ganancia de cada combinación α/T **fijada de antemano** e igual para todas las fracciones (AP visible, media sobre el 2–15 %):

| Configuración fija | Ganancia 2–15 % | Seeds a favor | p | Ganancia al 100 % |
|---|---|---|---|---|
| α = 0, T = 1 | +2.60 % | 10/10 | 0.002 | +0.76 % |
| α = 0.05, T = 1 | +2.56 % | 10/10 | 0.002 | +0.87 % |
| α = 0.05, T = 2 | +2.14 % | 10/10 | 0.002 | +0.99 % |
| α = 0.1, T = 4 | +1.85 % | 10/10 | 0.002 | +1.15 % |
| *Rango de las 12 combinaciones con α ≤ 0.1* | *+1.85 % a +2.60 %* | *9–10/10* | *≤ 0.006* | *+0.76 % a +1.36 %* |
| α = 0.3, T = 2 (comparable a la sección 7.3) | +1.37 % | 10/10 | 0.002 | +0.66 % |
| α = 0.7, T = 2 | +0.52 % | 8/10 | 0.014 | +0.07 % |

1. La ganancia no depende de elegir α/T a posteriori: cualquier α ≤ 0.1 fijado de antemano supera el +1.57 % de la sección 7.1.
2. Con α = 0.3, la versión final reproduce la ganancia de la sección 7.3 (+1.37 % frente a +1.47–1.50 %). La mejora respecto al informe original se explica, por tanto, por la rejilla: **el óptimo estaba por debajo de α = 0.3**, es decir, el estudiante debe aprender casi solo del maestro.
3. La ganancia de val sobreestima algo la de test (por ejemplo, +0.023 frente a +0.015 de AP con el 5 %). Por eso se reporta siempre el test.

**Sensibilidad a α y T:**

- **Hasta el 15 %:** α ∈ {0, 0.05, 0.1} son estadísticamente equivalentes (diferencias ≤ 0.003, p ≥ 0.11), y desde α = 0.3 la AP baja de forma monótona.
- **Al 100 %:** todo el rango de α queda dentro de 0.004.
- **T:** importa poco (≤ 0.008). T = 1 es lo mejor con el 2–3 % y T = 4–8 con el 10 % o más.
- **Fiabilidad de la selección:** la correlación entre el orden de las 28 combinaciones en validación y en test es ρ = 0.88–0.96 hasta el 15 %.

**Qué significan α y T.** En `L = α · BCE_dura + (1 − α) · BCE_soft(T) · T²`, el peso efectivo del maestro es ≈ `1 − α`, porque el factor T² compensa la reducción de gradiente al dividir por T.

- **α = 0** es destilación pura: el estudiante nunca ve las etiquetas al entrenar.
- **T = 1** hace que el estudiante imite las *probabilidades* del maestro. El aprendizaje se concentra en los casos dudosos, porque los casos seguros saturan la sigmoide y no dan gradiente.
- **T alta** aproxima una regresión sobre los *logits* (`z_s ≈ z_t`). El estudiante imita también cuán seguro está el maestro, es decir, la estructura completa de su función.

| Fracción | α / T | Peso del maestro | Lectura |
|---|---|---|---|
| 2–3 % | 0.05 / 1 | 95 % | Casi todo del maestro, pero solo sus probabilidades en los casos dudosos. Con ~700–1 000 filas, imitar los logits completos (T ≥ 4) es inestable. |
| 5–10 % | 0 / 4–8 | 100 % | Destilación pura. Las soft labels OOF son mejores "etiquetas" que las propias etiquetas 0/1 para un estudiante con pocos datos. |
| 15 % | 0.05 / 8 | 95 % | Igual que con el 5–10 %: imita casi por completo los logits del maestro. |
| 100 % | 0.3 / 4 | 70 % | Las etiquetas reales vuelven a aportar, aunque cualquier α entre 0.05 y 0.7 da casi lo mismo. |

**Patrón:**

- con pocos datos, el estudiante debe apoyarse casi por completo en el maestro;
- a medida que hay más datos, puede absorber más estructura del maestro (la T preferida sube de 1 a 4–8);
- con todos los datos, el maestro deja de ser imprescindible.

**Efecto colateral:** el `pos_weight` solo está en la parte dura. Con α ≈ 0, el estudiante hereda la calibración del maestro y sus umbrales son más bajos: ≈ 0.30 frente a ≈ 0.65 en `no_kd`.

**Métricas por clase** (visibles, umbral = mejor F1 de la clase 1 en validación):

- **Con el 2–3 %:** se repite lo visto en la sección 7.5. KD conserva más éxitos (recall_1 0.89–0.90 frente a 0.87–0.89) y no bloquea más fallos.
- **Desde el 5 %:** KD bloquea más fallos (recall_0 +0.035 a +0.060; f1_0 +0.035 a +0.055) a cambio de perder entre 1.5 y 2.5 puntos de recall_1. Por ejemplo, con el 15 %: recall_0 de 0.376 → 0.436, macro-F1 de 0.588 → 0.619.
- **Fuga bajo la máscara:** cae a ≈ 0 en todas las fracciones (`no_kd`: 0.1–0.6 %).
- **Dispersión:** recall_0 y f1_0 tienen una std entre seeds de 0.07 a 0.14, así que siguen siendo tendencias. La conclusión de la sección 7.5 se mantiene: el punto de operación lo decide el umbral.

**Elección del modelo para el closed-loop.**

- **La AP no sirve para comparar seeds.** Cada seed tiene su propio test, con una proporción distinta de éxitos visibles (0.41–0.55).
- **La seed 1 gana siempre en AP solo por eso.** Al 100 %, su AUC visible es de 0.723 y bloquea solo el 26 % de los fallos.
- **La seed 4 tiene la mejor AUC visible en las 6 fracciones.** Al 100 %, con α = 0.3 y T = 4, tiene AUC 0.801, recall_0 0.667, evita el 79.5 % de las transmisiones y pierde el 20.6 % de los éxitos.
- **La seed 5 es la alternativa conservadora:** AUC 0.760, evita el 71.1 % de las transmisiones y pierde el 11.3 % de los éxitos.
- **El umbral debe fijarse con un criterio de costo energético** antes de la simulación en closed-loop.

---

## 8. Discusión: por qué la nueva arquitectura funciona mejor

1. **La corrección del dataset hizo visible el problema real.**
   - Con el dataset anterior, las métricas eran globales, sin la métrica visible del cambio G, y estaban infladas por las transmisiones bajo la máscara. Además, la separación validación/test estaba invertida y los interferentes eran causales.
   - Con esos datos, la destilación del maestro antiguo parecía aportar +0.01–0.02 de AUC-PR con el 2–5 %.
   - Sobre los datos corregidos, con métricas visibles y un protocolo sin sesgo de selección, **esa ganancia desaparece** (+0.22 %, n.s.).
   - El dataset corregido también permitió identificar R1 y R2 como causa del 99.93 % de las etiquetas, que es justamente lo que el maestro nuevo modela.
2. **El maestro nuevo enseña lo que el estudiante puede aprender.** Sus soft labels están cerca de q(x) (MSE 0.024 frente a 0.168), como predice la teoría de la sección 3.
3. **La ganancia tiene dos fuentes separables gracias a los controles:**
   - **transferencia desde más datos**, ≈ +1.2 a +1.3 puntos: el maestro ve el 100 % del train, mientras que el estudiante ve el 2–15 %. Es lo que mide `self` o `geo_perm`;
   - **información geométrica privilegiada**, ≈ +0.3 a +0.4 puntos: es `geo − geo_perm`, y crece a +1.32 % con el 15 %, con 10/10 seeds.
4. **No es un efecto de regularización trivial.** Suavizar la etiqueta (label smoothing) da −0.25 %.
5. **Con el 100 % de los datos, la KD aporta poco.** Con el protocolo de las secciones 7.1–7.6, la ganancia era de +0.47 % (n.s.). En la versión final (sección 7.7) es pequeña pero significativa (+1.18 %, 10/10 seeds). La diferencia se debe a dos cambios. Uno es el tope de 20 épocas, que limitaba al estudiante destilado: al quitarlo, la ganancia absoluta pasó de +0.006 a +0.008 de AP. El otro es la rejilla ampliada. Es lo esperado: un estudiante con todos los datos ya está cerca de su techo de información.
6. **El techo lo pone la información del estudiante, no el maestro.** Con 8 entradas, el AUC visible máximo es ~0.73–0.75. El maestro antiguo sigue siendo útil como **cota superior** y como evidencia de que la información de los interferentes existe (AUC 0.925), pero no como profesor.

---

## 9. Limitaciones y trabajo futuro

| Tema | Estado | Próximo paso |
|---|---|---|
| Efecto en el lazo cerrado (batería, transmisiones evitadas) | No medido en simulación | Exportar el estudiante recomendado y simular. **Antes**, corregir el orden del vector de entrada en `Ter_Dlk_KiWan.cc` (hoy `{lat, lon, elev, TP, SF, doppler, alt, raan}`; el estudiante espera `[doppler, elev, lat, lon, TP, SF, alt, raan]`) |
| Umbral de operación | F1 de la clase 1. En la versión final varía entre 0.48 y 0.63 según la seed al 100 %, y algunos runs no seleccionados tienen umbrales < 0.1 | Curvas de fallos evitados frente a éxitos perdidos; umbral por costo energético |
| Elección de la seed para FLoRaSat | La AP de cada seed depende de la proporción de éxitos de su test (sección 7.7). Candidatas: seed 4 (más ahorro) o seed 5 (menos éxitos perdidos) | Evaluar los 10 modelos sobre un conjunto común, o en una corrida piloto de closed-loop |
| `*_load` con escala fija (50 s, la media configurada) | El intervalo medido en el CSV es menor (mediana 13 s) | `geo_v2`: dividir por el T_j medido de cada vecino |
| Interpolación lineal entre pasadas | Error bajo con huecos normales | Limitar el hueco máximo o propagar la órbita |
| Rejilla α/T | **Resuelto en la versión final (7.7).** El óptimo es α ≤ 0.1 con ≤ 15 % de datos, α es indiferente al 100 % y T influye poco | Reportar α/T como rangos, no como un valor puntual |
| Repeticiones reservadas en la versión final | Sin reps: la selección y la medición usan las mismas 10 seeds, con el test separado. El control con configuraciones fijas (7.7) muestra que el sesgo es pequeño | Opcional: reps 1–2 con la configuración elegida |
| Hiperparámetros del GBM | Por defecto | Ajustarlos con la validación del maestro |
| Logits con `decision_function` | **Adoptado en la versión final** (`my_final_version/train_teacher.py`), con la `grid` repetida (7.7) | Ninguno |
| Umbral de 1200 Hz | Se toma del simulador | Justificar físicamente en la descripción del simulador (≈ 10–40 bins LoRa con BW = 125 kHz) |

---

## 10. Reproducibilidad

| Ubicación | Contenido |
|---|---|
| `Fixed_KD_new_model/geo_teacher/` | Pipeline autocontenido del maestro `geo` y su estudiante (`README_GEO.md`) |
| `geo_teacher/CALCULOS_R1_R2.md` | Detalle de R1/R2 en FLoRaSat, unidades, interpolación y constantes |
| `geo_teacher/ejemplo_add_geo_features.{py,ipynb}` | Ejemplo didáctico de las features |
| `Fixed_KD_new_model/maestros_alternativos/` | Validación estadística completa: maestros, controles y análisis (`MAESTROS_ALTERNATIVOS.md`) |
| `Fixed_KD_new_model/MEJORAS_MAESTRO_ESTUDIANTE.md` | Diagnóstico de R1/R2 y réplica de la etiqueta al 99.93 % |
| `my_final_version/EJECUCION.md` | Dataset corregido y decisiones vigentes |
| `my_version_KD_gradient/KD_gradient_boosting/my_final_version/` | **Versión final** (sección 7.7): `train_teacher.py`, `train_student.py`, `config.py`, resultados en `results/` e informe `INFORME_RESULTADOS_STUDENT_KD.md` |

```bash
cd Fixed_KD_new_model/geo_teacher
python prepare_data_geo.py --check-old-cache   # dataset (~2 s), verificado contra la caché antigua
python train_teacher_geo.py                    # maestro geo, 10 seeds (~10 s)
python train_student_geo.py --phase grid       # rejilla alpha x T (rep 0)
python train_student_geo.py --phase reps       # reps 1-2 con la configuración elegida
python summarize_geo.py                        # tablas y modelos recomendados para exportar
```

Entorno: `~/miniconda3/envs/p_new`. El `.pth` resultante se carga sin cambios en `StudentModel` y sirve con la exportación existente al simulador.

---

## Referencias

- Bor, M., Roedig, U., Voigt, T., Alonso, J. (2016). *Do LoRa low-power wide-area networks scale?* MSWiM. (Regla de los 6 símbolos del preámbulo, usada en R1.)
- Croce, D., Gucciardo, M., Mangione, S., Santaromita, G., Tinnirello, I. (2018). *Impact of LoRa imperfect orthogonality: analysis of link-level performance.* IEEE Communications Letters. (Umbrales inter-SF de captura.)
- Breiman, L. (1996). *Stacked regressions.* Machine Learning, 24. (Predicciones de validación cruzada como entrada de otro modelo.)
- Chernozhukov, V. et al. (2018). *Double/debiased machine learning for treatment and structural parameters.* The Econometrics Journal. (*Cross-fitting*.)
- Friedman, J. H. (2001). *Greedy function approximation: a gradient boosting machine.* Annals of Statistics, 29(5). (Salida del boosting binario en escala log-odds.)
- Wolpert, D. (1992). *Stacked generalization.* Neural Networks, 5(2).
- Cho, J. H., Hariharan, B. (2019). *On the efficacy of knowledge distillation.* ICCV.
- Hinton, G., Vinyals, O., Dean, J. (2015). *Distilling the knowledge in a neural network.* NIPS Workshop.
- Lopez-Paz, D., Bottou, L., Schölkopf, B., Vapnik, V. (2016). *Unifying distillation and privileged information.* ICLR.
- Menon, A. K., Rawat, A. S., Reddi, S., Kim, S., Kumar, S. (2021). *A statistical perspective on distillation.* ICML.
- Stanton, S., Izmailov, P., Kirichenko, P., Alemi, A., Wilson, A. G. (2021). *Does knowledge distillation really work?* NeurIPS.
- Vapnik, V., Vashist, A. (2009). *A new learning paradigm: learning using privileged information.* Neural Networks.

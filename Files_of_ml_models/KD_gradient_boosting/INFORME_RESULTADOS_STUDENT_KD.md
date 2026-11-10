# Informe: estudiante MLP con Knowledge Distillation desde Gradient Boosting (versión final)

**Fecha:** 31/10/2026
**Resultados analizados:** `results/student/` (1740 runs), generados el 30/10/2026 entre las 22:59 y las 23:59.
**Scripts:** `train_teacher.py`, `train_student.py` · **Configuración:** `config.py`

Este informe sustituye al anterior del 30/10. Aquel usaba early stopping por `val_loss` y selección por AP de los paquetes visibles. La sección 9 resume qué cambió y por qué.

---

## Resumen

1. **KD supera a no_kd en las 6 fracciones de datos, con diferencia significativa.**
   - En AUC-PR de test (todos los paquetes), la ganancia va de +0.015 a +0.021 con hasta el 15 % de los datos y es de +0.009 con el 100 %.
   - Wilcoxon pareado por seed con corrección de Holm: p_holm ≤ 0.024. KD gana en 9 o 10 de las 10 seeds.
   - En ganancia relativa, la media sobre el 2–15 % es de **+2.67 %** (10/10 seeds, p = 0.002).
2. **KD rinde más con menos datos.** KD con el **10 %** de los datos (0.706) iguala a no_kd con el **100 %** (0.707). KD con el 2 % (0.686) ya alcanza a no_kd con el 10–15 %.
3. **Con pocos datos, conviene que el estudiante aprenda casi solo del maestro.**
   - Hasta el 15 %, cualquier α ∈ {0, 0.05, 0.1} da lo mismo, y a partir de α = 0.3 empeora de forma monótona.
   - Con el 100 %, el efecto de α es mínimo: todos los valores quedan dentro de 0.004.
4. **La temperatura importa poco.** T = 1 es lo mejor con el 2–3 % de los datos y T = 4–8 con el 10 % o más; las diferencias son ≤ 0.008.
5. **El early stopping por AP de validación eliminó los colapsos.** Con el criterio anterior había 17 runs que paraban en la época 1; ahora hay 0. La fuga máxima bajo la máscara bajó del 98.7 % al 1.8 %.
6. **A partir del 5 % de los datos, KD detecta más fallos.** El recall de la clase 0 en paquetes visibles sube entre 4 y 6 puntos, a cambio de perder entre 1.5 y 2.5 puntos de éxitos.
7. **La AP no sirve para elegir la seed que va a FLoRaSat.** La seed 1 gana siempre en AP solo porque su conjunto de datos tiene más éxitos. La seed 4 tiene la mejor AUC-ROC en todas las fracciones.

---

## 1. Configuración del experimento

| Parámetro | Valor |
|---|---|
| Maestro | `HistGradientBoostingClassifier` (300 iteraciones) sobre las 8 entradas del estudiante + 5 features geométricas. Logits out-of-fold con `decision_function`, 5 folds por simulación. |
| Estudiante | MLP 8-64-32-1, dropout 0.1, Adam lr 1e-3, batch 32 |
| Pérdida KD | `L = α · BCE_dura(pos_weight) + (1 − α) · BCE_soft(T) · T²` (α = 0 es destilación pura) |
| Fracciones de train | 0.02, 0.03, 0.05, 0.10, 0.15, 1.0 |
| Grid | α ∈ {0, 0.05, 0.1, 0.3, 0.5, 0.7, 0.9} × T ∈ {1, 2, 4, 8}, más no_kd |
| Seeds | 0–9. Cada una corresponde a una simulación distinta de FLoRaSat con su propio split train/val/test. |
| Épocas máximas | `max(ceil(20 / frac), 60)` |
| Early stopping | **AP de validación sobre todos los paquetes** (`val_ap_all`), paciencia 30 |
| Selección de α/T | Mayor media de `val_ap_all` entre las 10 seeds. El test no interviene en la selección. |
| Umbral de decisión | Mejor F1 de la clase 1 en validación |

Clase 0 = el paquete falla (no transmitir). Clase 1 = el paquete llega (transmitir).

**Por qué AP sobre todos los paquetes.** En el despliegue, el estudiante decide también sobre los paquetes bajo la máscara de elevación. Esos paquetes son el 61–69 % de cada conjunto y fallan todos.

- En un modelo sano, la AP con todos los paquetes y la AP solo con visibles ordenan las configuraciones casi igual (Spearman de 0.995 a 0.998 por fracción), porque esos fallos quedan al fondo del ranking.
- En un modelo que falla, la AP con todos castiga dejar pasar paquetes imposibles; la de visibles no lo detecta. En los colapsos de la corrida anterior, la AP con todos era 0.22 y la de visibles, 0.47.

---

## 2. Validación de los resultados

| Verificación | Resultado |
|---|---|
| Número de runs | 1740 = 6 fracciones × (1 no_kd + 7 α × 4 T) × 10 seeds. Las 174 configuraciones tienen las 10 seeds. |
| Integridad | Sin NaN. 1740 `.pth` y 1740 scalers, con nombres idénticos a los runs. |
| Frescura | Todos los runs son posteriores a la cache (29/10) y a los logits del maestro (29/10), y todos tienen `val_ap_all`. |
| Fuga train/test | 0 simulaciones compartidas entre train y test. |
| Colapsos (mejor época ≤ 3) | 0 (antes, 17) |
| Fuga bajo la máscara | Máximo del 1.8 % en todo el grid. En las configuraciones seleccionadas, del 0 % al 0.2 %. |
| Límite de épocas | Se alcanza en el 0 % de los runs con fracciones ≤ 10 %, en el 0.7 % al 15 % y en el 1.4 % al 100 %. |
| CSV de resumen | Coinciden con el recálculo hecho a partir de los JSON. |
| Código | Probado antes en el scratchpad con 24 runs (2 % y 100 %; no_kd, α=0 T=8 y α=0.3 T=1). |

---

## 3. Resultado principal: mejor α/T por fracción frente a no_kd

Test, media de 10 seeds. La ganancia relativa es la media por seed de `KD / no_kd − 1`.

| frac | α | T | AUC-PR no_kd | AUC-PR KD | ganancia | relativa | KD gana en | p_holm | AUC-ROC visibles no_kd → KD |
|---|---|---|---|---|---|---|---|---|---|
| 0.02 | 0.05 | 1 | 0.665 | 0.686 | **+0.021** | +3.16 % | 9/10 | 0.023 | 0.700 → 0.718 |
| 0.03 | 0.05 | 1 | 0.674 | 0.692 | +0.018 | +2.65 % | 9/10 | 0.023 | 0.708 → 0.722 |
| 0.05 | 0 | 4 | 0.680 | 0.694 | +0.015 | +2.17 % | 9/10 | 0.023 | 0.713 → 0.725 |
| 0.10 | 0 | 8 | 0.686 | 0.706 | **+0.020** | +2.93 % | 10/10 | 0.012 | 0.722 → 0.735 |
| 0.15 | 0.05 | 8 | 0.689 | 0.705 | +0.017 | +2.43 % | 9/10 | 0.023 | 0.722 → 0.735 |
| 1.00 | 0.3 | 4 | 0.707 | 0.716 | +0.009 | +1.22 % | 10/10 | 0.012 | 0.739 → 0.748 |

**Solo paquetes visibles (métrica secundaria, para los casos difíciles):**

| frac | AP visible no_kd | AP visible KD | ganancia | relativa | KD gana en | p_holm |
|---|---|---|---|---|---|---|
| 0.02 | 0.670 | 0.687 | +0.018 | +2.64 % | 7/10 | 0.049 |
| 0.03 | 0.677 | 0.693 | +0.016 | +2.33 % | 9/10 | 0.041 |
| 0.05 | 0.681 | 0.694 | +0.013 | +1.88 % | 9/10 | 0.041 |
| 0.10 | 0.687 | 0.706 | +0.019 | +2.76 % | 10/10 | 0.012 |
| 0.15 | 0.689 | 0.705 | +0.016 | +2.31 % | 9/10 | 0.023 |
| 1.00 | 0.708 | 0.716 | +0.008 | +1.18 % | 10/10 | 0.012 |

Lo que muestran las tablas:

1. **La ganancia es mayor con pocos datos y se reduce al 100 %.** Es el comportamiento esperado de la destilación: el maestro, entrenado con todo el train, aporta más cuanto menos información propia tiene el estudiante.
2. **Rinde más con menos datos.** KD con el 10 % de los datos queda a 0.001 de no_kd con el 100 %.
3. **KD regulariza.** Con fracciones ≤ 15 %, no_kd hace early stopping en la época 28–38 de media, mientras que KD sigue mejorando hasta la época 76–141. Sin maestro, el estudiante sobreajusta enseguida.
4. **Sigue habiendo distancia con el maestro.** El maestro tiene una AP visible de validación de 0.745; el mejor estudiante, 0.725 con el 100 %. El maestro usa 5 features geométricas que el estudiante no ve.

---

## 4. Sensibilidad a α y T

### 4.1 AP de validación (todos los paquetes) por α, con la mejor T para cada α

| frac | α=0 | 0.05 | 0.1 | 0.3 | 0.5 | 0.7 | 0.9 |
|---|---|---|---|---|---|---|---|
| 0.02 | 0.675 | **0.678** | 0.677 | 0.673 | 0.668 | 0.664 | 0.660 |
| 0.03 | 0.684 | **0.687** | 0.685 | 0.679 | 0.676 | 0.673 | 0.669 |
| 0.05 | **0.700** | 0.698 | 0.697 | 0.691 | 0.686 | 0.683 | 0.680 |
| 0.10 | **0.710** | 0.709 | 0.709 | 0.703 | 0.698 | 0.691 | 0.689 |
| 0.15 | 0.714 | **0.714** | 0.713 | 0.710 | 0.704 | 0.702 | 0.700 |
| 1.00 | 0.723 | 0.723 | 0.723 | **0.726** | 0.724 | 0.725 | 0.721 |

### 4.2 AP de validación por T, con el mejor α para cada T

| frac | T=1 | 2 | 4 | 8 |
|---|---|---|---|---|
| 0.02 | **0.678** | 0.676 | 0.671 | 0.670 |
| 0.03 | **0.687** | 0.684 | 0.683 | 0.684 |
| 0.05 | 0.697 | 0.695 | **0.700** | 0.699 |
| 0.10 | 0.704 | 0.708 | 0.709 | **0.710** |
| 0.15 | 0.710 | 0.713 | 0.714 | **0.714** |
| 1.00 | 0.722 | 0.723 | **0.726** | 0.725 |

### 4.3 Diferencia entre la mejor combinación y las tres siguientes

Diferencia pareada por seed en `val_ap_all`.

| frac | mejor | 2.ª | 3.ª | 4.ª |
|---|---|---|---|---|
| 0.02 | α=0.05, T=1 | α=0.1, T=1: +0.001 (p=0.56) | α=0.1, T=2: +0.002 (p=0.38) | α=0.05, T=2: +0.002 (p=0.38) |
| 0.03 | α=0.05, T=1 | α=0.1, T=1: +0.002 (p=0.23) | α=0.05, T=2: +0.003 (p=0.11) | α=0.1, T=2: +0.003 (p=0.19) |
| 0.05 | α=0, T=4 | α=0, T=8: +0.001 (p=0.32) | α=0.05, T=8: +0.002 (p=0.38) | α=0, T=1: +0.003 (p=0.28) |
| 0.10 | α=0, T=8 | α=0.05, T=4: +0.001 (p=1.00) | α=0, T=4: +0.001 (p=0.77) | α=0.05, T=8: +0.001 (p=0.77) |
| 0.15 | α=0.05, T=8 | α=0, T=4: +0.000 (p=0.92) | α=0, T=2: +0.001 (p=0.77) | α=0.1, T=8: +0.001 (p=0.32) |
| 1.00 | α=0.3, T=4 | α=0.7, T=8: +0.001 (p=1.00) | α=0.5, T=8: +0.002 (p=0.23) | α=0.5, T=4: +0.002 (p=0.16) |

### 4.4 Fiabilidad de la selección

| frac | Spearman val/test (28 combinaciones) | test de la elegida | mejor test posible | ganancia en val | ganancia en test |
|---|---|---|---|---|---|
| 0.02 | 0.88 | 0.6857 | 0.6861 | +0.022 | +0.021 |
| 0.03 | 0.96 | 0.6918 | 0.6918 | +0.020 | +0.018 |
| 0.05 | 0.94 | 0.6943 | 0.6998 | +0.023 | +0.015 |
| 0.10 | 0.92 | 0.7058 | 0.7058 | +0.025 | +0.020 |
| 0.15 | 0.92 | 0.7051 | 0.7088 | +0.018 | +0.017 |
| 1.00 | 0.65 | 0.7159 | 0.7171 | +0.009 | +0.009 |

### 4.5 Conclusiones sobre α y T

- **Con ≤ 15 % de los datos, α ∈ {0, 0.05, 0.1} son estadísticamente equivalentes** (diferencias ≤ 0.003, p ≥ 0.11). A partir de α = 0.3 la AP baja de forma monótona.
- **Con el 100 %, α deja de importar:** todo el rango queda dentro de 0.004.
- **La T preferida crece con los datos:** T = 1 con el 2–3 % y T = 4–8 con el 10 % o más. El efecto es pequeño.
- **La selección por validación es fiable.** ρ = 0.88–0.96 hasta el 15 %. Al 100 %, ρ = 0.65, porque todas las combinaciones están muy juntas.
- **La selección es algo optimista.** La ganancia en val sobreestima la de test (al 5 %: +0.023 frente a +0.015), que es el sesgo normal de elegir la mejor de 28 combinaciones. Por eso se reporta siempre la ganancia en test.
- **Para el informe**, lo honesto es presentar rangos: "α ≤ 0.1 con pocos datos, α indiferente con todos los datos, T poco relevante". No conviene presentar un ganador puntual por fracción.

### 4.6 Qué significan α y T: ¿cuánto aprende el estudiante del maestro?

La pérdida de entrenamiento del estudiante KD es:

```
L = α · BCE(y_true, z_s; pos_weight)  +  (1 − α) · BCE(σ(z_t / T), σ(z_s / T)) · T²
    └────── parte dura: etiqueta 0/1 ──┘     └───── parte soft: imitar al maestro ─────┘
```

`z_s` son los logits del estudiante, `z_t` los del maestro (out-of-fold) y `σ` la sigmoide.

**α: cuánto peso tiene la etiqueta real frente al maestro.**

| α | Peso de la etiqueta real | Peso del maestro | Qué significa |
|---|---|---|---|
| 0 | 0 % | 100 % | **Destilación pura.** El estudiante nunca ve las etiquetas al entrenar; solo imita al maestro. Las etiquetas se usan únicamente en validación (early stopping y umbral). |
| 0.05 | 5 % | 95 % | Aprende casi todo del maestro; la etiqueta solo corrige un poco. |
| 0.1 | 10 % | 90 % | Ídem. |
| 0.3 | 30 % | 70 % | El maestro domina, pero la etiqueta aporta de forma apreciable. |
| 0.5 | 50 % | 50 % | Mismo peso para ambos. |
| 0.7–0.9 | 70–90 % | 30–10 % | Aprende sobre todo de la etiqueta; el maestro es un regularizador suave. |
| (no_kd) | 100 % | 0 % | Sin maestro. |

El factor `T²` compensa que, al dividir los logits por T, los gradientes de la parte soft se hacen más pequeños (Hinton et al., 2015). Así, el peso efectivo del maestro es aproximadamente `1 − α` para cualquier T.

**T: qué aspecto de la salida del maestro se imita.**

- **T = 1:** el estudiante imita las **probabilidades** del maestro tal como son.
  - Los paquetes en los que el maestro está muy seguro (p ≈ 0 o p ≈ 1, por ejemplo los que están bajo la máscara) apenas generan gradiente.
  - El aprendizaje se concentra en los **casos dudosos**, cerca de p = 0.5, que son los que deciden el ranking.
- **T alta (4–8):** las probabilidades se "aplanan" y la pérdida se parece cada vez más a una regresión sobre los **logits** (`z_s ≈ z_t`).
  - El estudiante intenta reproducir también **cuán seguro está el maestro** en los casos fáciles, es decir, la estructura completa de su función: el "conocimiento oscuro" (*dark knowledge*).
  - Es más información por ejemplo, pero exige más capacidad y más datos para imitarla bien.

**Efecto colateral de α bajo: calibración y umbral.** El `pos_weight`, que compensa el desbalance de clases, solo está en la parte dura. Con α ≈ 0, el estudiante hereda la calibración del maestro, sin `pos_weight`, y sus probabilidades son más bajas. Por eso los umbrales cambian:

- con α ≤ 0.05 los umbrales son de 0.29–0.37;
- con α = 0.3 (al 100 %), el umbral es de 0.54;
- en no_kd, de 0.54–0.71.

**Interpretación de cada configuración seleccionada:**

| frac | α / T | Peso del maestro | Lectura |
|---|---|---|---|
| 0.02 | 0.05 / 1 | 95 % | Con solo unas 700 filas, el estudiante aprende casi todo del maestro, pero imitando sus probabilidades y no sus logits: se concentra en los casos dudosos. Imitar los logits completos (T ≥ 4) con tan pocos datos es inestable; ahí aparecieron los colapsos (corrida 2) y los umbrales < 0.1. |
| 0.03 | 0.05 / 1 | 95 % | Igual que con el 2 %. |
| 0.05 | 0 / 4 | 100 % | Destilación pura. Con unas 1 750 filas, la etiqueta no añade nada a lo que ya da el maestro, y el estudiante ya puede imitar parte de la estructura de los logits. |
| 0.10 | 0 / 8 | 100 % | Destilación pura con T alta: el estudiante hace prácticamente una regresión sobre los logits del maestro. Es la fracción con la mayor ganancia relativa (+2.93 %). |
| 0.15 | 0.05 / 8 | 95 % | Igual que con el 10 %. |
| 1.00 | 0.3 / 4 | 70 % | Con unas 35 000 filas, la etiqueta real vuelve a aportar (30 %), aunque el maestro sigue dominando. Cualquier α entre 0.05 y 0.7 da casi lo mismo (sección 4.3): con todos los datos, el estudiante llega a la misma solución por cualquier camino. |

**Patrón general:**

1. **Cuantos menos datos, más debe apoyarse el estudiante en el maestro.** Con ≤ 15 % de los datos, el maestro aporta el 90–100 % de la señal. La etiqueta 0/1 de unos pocos cientos de paquetes es ruidosa, mientras que el maestro resume lo aprendido del 100 % del train y de las 5 features geométricas.
2. **Cuantos más datos, más estructura del maestro puede absorber el estudiante.** La T preferida crece de 1 (2–3 %) a 4–8 (≥ 5 %): primero imita solo los casos dudosos y después la función completa del maestro.
3. **Con todos los datos, el maestro deja de ser imprescindible.** α deja de importar y la ganancia baja a +0.009: el estudiante ya tiene información suficiente por sí mismo y el maestro solo aporta las features geométricas que el estudiante no puede observar.
4. **Que α = 0 funcione tan bien con el 5–10 %** significa que el maestro OOF es un "etiquetador" mejor que las propias etiquetas para un estudiante con pocos datos. Sus soft labels son una versión suavizada y menos ruidosa de la probabilidad de éxito, que es lo que el estudiante puede aprender con sus 8 entradas.

---

## 5. Métricas por clase

Test, umbral elegido en validación, media de 10 seeds.

### 5.1 Paquetes visibles (los casos difíciles)

| frac | precision_0 no_kd → KD | recall_0 no_kd → KD | f1_0 no_kd → KD | recall_1 no_kd → KD | f1_1 no_kd → KD | macro_f1 no_kd → KD |
|---|---|---|---|---|---|---|
| 0.02 | 0.739 → 0.766 | 0.322 → 0.311 | 0.446 → 0.433 | 0.872 → 0.891 | 0.659 → 0.666 | 0.552 → 0.550 |
| 0.03 | 0.764 → 0.779 | 0.318 → 0.295 | 0.443 → 0.421 | 0.886 → 0.904 | 0.666 → 0.669 | 0.554 → 0.545 |
| 0.05 | 0.761 → 0.777 | 0.325 → 0.360 | 0.451 → 0.486 | 0.882 → 0.880 | 0.665 → 0.675 | 0.558 → 0.581 |
| 0.10 | 0.778 → 0.777 | 0.362 → 0.415 | 0.482 → 0.535 | 0.878 → 0.863 | 0.676 → 0.683 | 0.579 → 0.609 |
| 0.15 | 0.778 → 0.772 | 0.376 → 0.436 | 0.499 → 0.553 | 0.875 → 0.852 | 0.677 → 0.684 | 0.588 → 0.619 |
| 1.00 | 0.804 → 0.784 | 0.414 → 0.474 | 0.529 → 0.584 | 0.872 → 0.847 | 0.689 → 0.694 | 0.609 → 0.639 |

La std entre seeds de recall_0 y f1_0 es de 0.07 a 0.14.

### 5.2 Todos los paquetes (`best_vs_no_kd_metrics.csv`)

| métrica | 0.02 | 0.03 | 0.05 | 0.10 | 0.15 | 1.00 |
|---|---|---|---|---|---|---|
| auc_pr | 0.665 → 0.686 | 0.674 → 0.692 | 0.679 → 0.694 | 0.686 → 0.706 | 0.689 → 0.705 | 0.707 → 0.716 |
| precision_0 | 0.967 → 0.972 | 0.971 → 0.975 | 0.970 → 0.970 | 0.970 → 0.966 | 0.969 → 0.964 | 0.969 → 0.963 |
| recall_0 | 0.824 → 0.824 | 0.825 → 0.821 | 0.828 → 0.839 | 0.838 → 0.853 | 0.842 → 0.858 | 0.852 → 0.868 |
| f1_0 | 0.890 → 0.892 | 0.892 → 0.891 | 0.893 → 0.899 | 0.899 → 0.906 | 0.900 → 0.908 | 0.906 → 0.913 |
| precision_1 | 0.524 → 0.532 | 0.530 → 0.529 | 0.533 → 0.548 | 0.548 → 0.567 | 0.552 → 0.573 | 0.571 → 0.590 |
| recall_1 | 0.872 → 0.891 | 0.886 → 0.904 | 0.882 → 0.880 | 0.878 → 0.863 | 0.875 → 0.852 | 0.872 → 0.847 |
| f1_1 | 0.654 → 0.665 | 0.662 → 0.667 | 0.664 → 0.675 | 0.674 → 0.683 | 0.676 → 0.684 | 0.688 → 0.694 |
| macro_f1 | 0.772 → 0.778 | 0.777 → 0.779 | 0.779 → 0.787 | 0.786 → 0.794 | 0.788 → 0.796 | 0.797 → 0.803 |
| belowmask_leak | 0.006 → 0.002 | 0.004 → 0.002 | 0.002 → 0.000 | 0.002 → 0.000 | 0.001 → 0.000 | 0.001 → 0.000 |

Con todos los paquetes, precision_0 y recall_0 están dominadas por el 61–69 % de paquetes bajo la máscara, que son fallos fáciles. Para analizar la calidad real del modelo hay que mirar la tabla 5.1.

### 5.3 Interpretación

- **Las métricas de ranking (AUC-PR y AUC-ROC) son la evidencia sólida.** Mejoran en todas las fracciones y su std entre seeds (≈ 0.027) es menor que la ganancia.
- **Las métricas por clase dependen del umbral y tienen mucho ruido.** Son tendencias, no pruebas.
- **Con el 2–3 % de los datos**, KD ordena mejor los paquetes, pero su umbral favorece transmitir: conserva más éxitos (recall_1 +0.02) y no detecta más fallos.
- **A partir del 5 %**, KD detecta más fallos (recall_0 entre +0.035 y +0.060 en visibles) a cambio de perder algunos éxitos (recall_1 entre −0.002 y −0.025). Es decir, desplaza el punto de operación hacia ahorrar batería.
- **KD reduce la fuga bajo la máscara** a casi cero en todas las fracciones.
- **No se deben reportar métricas con umbral 0.5.** Los umbrales óptimos de KD (0.29–0.54) y de no_kd (0.54–0.71) están calibrados de forma distinta: no_kd usa `pos_weight` en toda su pérdida y la parte soft de KD no lo lleva.

### 5.4 Métricas operativas (todos los paquetes de test)

| frac | modelo | umbral (media, mín.–máx.) | transmisiones evitadas | éxitos perdidos | fuga bajo la máscara | época |
|---|---|---|---|---|---|---|
| 0.02 | no_kd | 0.54 (0.38–0.68) | 69.7 % | 12.8 % | 0.6 % | 33 |
| 0.02 | KD | 0.30 (0.16–0.39) | 69.4 % | 10.9 % | 0.2 % | 76 |
| 0.05 | no_kd | 0.65 (0.42–0.74) | 69.9 % | 11.8 % | 0.2 % | 28 |
| 0.05 | KD | 0.29 (0.21–0.34) | 70.8 % | 12.0 % | 0.0 % | 141 |
| 0.10 | no_kd | 0.65 (0.39–0.74) | 70.7 % | 12.2 % | 0.2 % | 30 |
| 0.10 | KD | 0.31 (0.23–0.35) | 72.2 % | 13.7 % | 0.0 % | 101 |
| 0.15 | no_kd | 0.68 (0.48–0.78) | 71.1 % | 12.5 % | 0.1 % | 38 |
| 0.15 | KD | 0.37 (0.26–0.45) | 72.9 % | 14.8 % | 0.0 % | 88 |
| 1.00 | no_kd | 0.71 (0.55–0.78) | 72.0 % | 12.8 % | 0.1 % | 29 |
| 1.00 | KD | 0.54 (0.48–0.63) | 73.7 % | 15.3 % | 0.0 % | 39 |

**Umbrales extremos.** En el grid completo hay 46 runs con umbral < 0.1, sobre todo al 2–3 % con T ≥ 4 y α ≤ 0.1. Ninguno pertenece a una configuración seleccionada, pero indican que, en esa zona, las probabilidades del estudiante están muy comprimidas. Un umbral tan bajo sería frágil en el closed-loop.

---

## 6. Elección de la seed para FLoRaSat

Cada seed es una simulación distinta, con su propio conjunto de val y de test y una proporción de éxitos diferente. La AUC-PR depende de esa proporción, así que **comparar la AP entre seeds compara tareas distintas, no modelos**.

| frac | seed con mayor `val_ap_all` | seeds con mayor AUC-ROC de test (visibles) |
|---|---|---|
| 0.02 | 1 | 4, 5, 2 |
| 0.03 | 1 | 4, 5, 0 |
| 0.05 | 1 | 4, 9, 5 |
| 0.10 | 1 | 4, 5, 0 |
| 0.15 | 1 | 4, 0, 5 |
| 1.00 | 1 | 4, 5, 0 |

**Detalle al 100 % (α=0.3, T=4):**

| seed | val AP | test AP | AUC-ROC test (visibles) | f1_0 | recall_0 | transmisiones evitadas | éxitos perdidos | umbral | época |
|---|---|---|---|---|---|---|---|---|---|
| 0 | 0.720 | 0.721 | 0.756 | 0.633 | 0.536 | 75.8 % | 18.4 % | 0.515 | 35 |
| 1 | **0.794** | **0.754** | 0.723 | 0.386 | 0.257 | 67.1 % | 6.0 % | 0.486 | 50 |
| 2 | 0.733 | 0.715 | 0.751 | 0.641 | 0.554 | 77.0 % | 20.9 % | 0.608 | 37 |
| 3 | 0.705 | 0.683 | 0.709 | 0.584 | 0.485 | 74.3 % | 18.7 % | 0.626 | 35 |
| 4 | 0.714 | 0.735 | **0.801** | **0.735** | **0.667** | **79.5 %** | 20.6 % | 0.588 | 54 |
| 5 | 0.724 | 0.713 | 0.760 | 0.560 | 0.426 | 71.1 % | 11.3 % | 0.480 | 16 |
| 6 | 0.763 | 0.738 | 0.749 | 0.616 | 0.520 | 74.4 % | 17.6 % | 0.606 | 41 |
| 7 | 0.673 | 0.673 | 0.751 | 0.602 | 0.480 | 74.5 % | 16.4 % | 0.503 | 44 |
| 8 | 0.697 | 0.693 | 0.727 | 0.520 | 0.389 | 71.6 % | 11.9 % | 0.479 | 27 |
| 9 | 0.731 | 0.734 | 0.755 | 0.559 | 0.427 | 71.9 % | 11.2 % | 0.534 | 51 |

**La seed 1 no es el mejor modelo:**

- su conjunto de datos tiene la mayor proporción de éxitos (0.57 en val y 0.55 en test entre los visibles, frente a una media de 0.46);
- tiene una de las AUC-ROC más bajas;
- detecta solo el 26 % de los fallos, así que ahorraría poca batería.

**La seed 4 es la que mejor discrimina en todas las fracciones.**

**Recomendaciones:**

1. **No elegir la seed por AP**, y menos por la AP de test, que además sesga al alza el resultado offline.
2. **Candidatas al 100 % de datos (α=0.3, T=4):**
   - **seed 4**, si el objetivo principal es ahorrar batería: evita el 79.5 % de las transmisiones y pierde el 20.6 % de los éxitos;
   - **seed 5**, si se prioriza no perder éxitos: evita el 71.1 % y pierde solo el 11.3 %.
3. **Lo ideal es comparar los 10 modelos sobre un conjunto común** (val+test unidos, o una corrida piloto de closed-loop) antes de decidir.
4. **Fijar el umbral de forma explícita para el closed-loop.** Varía entre 0.48 y 0.63 según la seed y decide cuánta batería se ahorra frente a cuántos éxitos se pierden. Se puede elegir en validación a partir de un máximo aceptable de éxitos perdidos.

---

## 7. Conclusiones

1. La destilación desde el gradient boosting mejora de forma consistente y significativa la calidad de ranking del estudiante (AUC-PR y AUC-ROC) en todas las fracciones de datos. La ganancia relativa media es de +2.67 % sobre el 2–15 % y de +1.22 % al 100 %.
2. KD hace un uso mucho más eficiente de los datos: con el 10 % de ellos, el estudiante destilado iguala al estudiante sin destilación entrenado con el 100 %.
3. Con pocos datos, lo mejor es que el estudiante aprenda casi solo del maestro (α ≤ 0.1). Con todos los datos, α apenas importa. La temperatura tiene poco efecto.
4. A partir del 5 % de los datos, KD detecta más fallos a costa de perder algunos éxitos, y reduce a cero la fuga bajo la máscara. Las métricas por clase tienen mucha varianza entre seeds.
5. El early stopping por AP de validación sobre todos los paquetes es el criterio correcto. Está alineado con la selección de α/T y con el despliegue, y eliminó los colapsos del criterio por `val_loss`.

---

## 8. Próximos pasos

1. **Elegir la seed y el umbral sobre un conjunto común** y exportar el modelo a FLoRaSat. Antes, verificar que el orden del vector de entrada del simulador coincide con `STUDENT_INPUTS`.
2. **Definir el umbral según el costo energético** (fallos evitados frente a éxitos perdidos), en lugar de con el F1 de la clase 1.
3. **Reportar α/T como rangos.**
4. **Limpiar las cuestiones menores de código** de la sección 10. No afectan a los resultados.

---

## 9. Historial: qué cambió respecto a las corridas anteriores

| Corrida | Grid | Early stopping | Selección | Problemas |
|---|---|---|---|---|
| 1 (30/10, ≈ 19:00) | α ∈ {0.1–0.9} | `val_loss`, máximo de 20 épocas al 100 % | `val_ap_visible` | α en el borde del grid; el 100 % no alcanzaba la convergencia |
| 2 (30/10, ≈ 21:30) | + α ∈ {0, 0.05}; máximo de 60 épocas | `val_loss` | `val_ap_visible` | 17 colapsos al 2 % (mejor época 1, fuga hasta del 98.7 %) |
| **3 (30/10, ≈ 23:30)** | igual que la 2 | **`val_ap_all`** | **`val_ap_all`** | ninguno detectado |

**AP visible de test, comparada entre las corridas 2 y 3:**

| frac | no_kd (corrida 2 → 3) | KD (corrida 2 → 3) |
|---|---|---|
| 0.02 | 0.659 → 0.670 | 0.684 → 0.687 |
| 0.03 | 0.672 → 0.677 | 0.693 → 0.693 |
| 0.05 | 0.679 → 0.681 | 0.694 → 0.694 |
| 0.10 | 0.686 → 0.687 | 0.702 → 0.706 |
| 0.15 | 0.689 → 0.689 | 0.705 → 0.705 |
| 1.00 | 0.707 → 0.708 | 0.715 → 0.716 |

El nuevo criterio mejoró también a no_kd con pocos datos (+0.010 al 2 %), así que ahí la ganancia baja un poco (+0.025 → +0.018). En el resto se mantiene o sube. **La conclusión no depende del criterio de parada.**

---

## 10. Observaciones sobre el código

Revisión de `train_student.py` y `config.py` en su estado al 31/10/2026. **Ninguna de estas observaciones afecta a los resultados de este informe.** Son textos desactualizados tras el cambio de criterio, restos de código o detalles de legibilidad. Los números de línea corresponden a la versión actual de `train_student.py`.

### 10.1 Textos que se muestran en consola o se guardan con información desactualizada

Son las más importantes, porque confunden al leer la salida:

| Línea | Actual | Debería decir |
|---|---|---|
| 367 | `print("\nMejor alpha/T por fraccion (criterio: media de val_ap_visible)")` | `(criterio: media de val_ap_all)` |
| 377 | `print("\nMetricas de test (visibles, umbral de validacion), ...")` | `Metricas de test (todos los paquetes, umbral de validacion), ...`: `metrics_full` usa ahora `best_th_all_*` |

### 10.2 Comentarios y docstrings desactualizados

| Línea | Actual | Corrección |
|---|---|---|
| 86–87 | Docstring de `train_student`: `t_tr = logits del maestro (None = no_kd).` y `Return model, val_loss, p_val.` | `t_tr` ya no existe (los logits vienen en `batch["teacher_logits"]`), y la función devuelve 4 valores: `model, best_val_loss, best_p_val, best_epoch` |
| 242 | `val_ap_visible = ...  # criterio de seleccion` | Ahora es secundaria: `# secundaria: solo paquetes visibles` |
| 243 | `val_ap_all = ...  # new criterio de seleccion` | `# criterio de early stopping y de seleccion` |
| 245 | `test_ap_visible = ...  # metrica principal` | `# secundaria: casos dificiles`. La principal es `test_ap_all`. |
| 273 | Docstring de `select_best_configs`: `mayor media de val_ap_visible entre seeds` | `mayor media de val_ap_all` |
| 295 | `# Por cada fraccion, la fila con mayor val_ap_visible` | `... mayor val_ap_all` |
| `config.py`, 40 | `PATIENCE = 30  # early stopping sobre val_loss (BCE con pos_weight)` | `# early stopping sobre val_ap_all` |

### 10.3 Restos de código comentado o duplicado

| Línea | Qué hay | Sugerencia |
|---|---|---|
| 122–124 | `# Validation` duplicado y las dos líneas antiguas comentadas (`#y_val, logits_val = ...`, `#val_loss = ...`) | Borrarlas: el bloque correcto empieza en la 126 |
| 133 | `#if val_loss < best_val_loss:` | Borrarla |
| 211–212 | `# Gettin val visible` y `#vis_va, vis_te = ...` | Corregir a `# Mascaras de paquetes visibles` y borrar la línea comentada |

### 10.4 Pequeñas mejoras de claridad

| Línea | Qué hay | Sugerencia |
|---|---|---|
| 130 | `average_precision_score(y_val, p_val)` recibe `y_val` como tensor de PyTorch | Funciona, pero es más explícito `y_val.numpy()` |
| 138 | `best_p_val = torch.sigmoid(logits_val).numpy()` vuelve a calcular la sigmoide | Usar `best_p_val = p_val`, ya calculado en la línea 129 |
| 97 | `best_val_loss = float("inf")` | Ya no se compara con nada; se puede iniciar en `None`, como `best_state` |

### 10.5 Imports y parámetros sin uso

| Dónde | Qué | Sugerencia |
|---|---|---|
| `train_student.py`, 18 | `precision_recall_curve` | No se usa en este archivo (lo usa `find_best_threshold` en `global_funcs.py`) |
| `train_student.py`, 21 | `TensorDataset` | No se usa: los datasets son `MyCustomStudentDataset` y `MyCustonStudentDistillationDataset` |
| `train_student.py`, 23–24 | `STUDENT_DROPOUT`, `STUDENT_HIDDEN` | Se importan pero no se usan aquí: `StudentModel` ya los lee directamente de `config.py`, así que el modelo es correcto. Se pueden quitar del import. |
| `config.py`, 20 | `EPOCHS = 20` | Ningún script de `my_final_version` lo usa (el estudiante usa `BASE_EPOCHS`). Se puede borrar. |
| `config.py`, 27 | `SELECTION_METRIC = 'auc_pr'` | Lo usa `common.py`, no `train_student.py`, que selecciona siempre por `val_ap_all`. Mantenerlo, pero aclarar en un comentario que no rige al estudiante. |

### 10.6 Decisiones de diseño a tener presentes (no son errores)

1. **`pos_weight` solo está en la parte dura de la pérdida** (`MyDistillationLoss`). Con α ≈ 0, el estudiante no compensa el desbalance de clases y hereda la calibración del maestro. Por eso los umbrales de KD (≈ 0.30) son tan distintos de los de no_kd (≈ 0.65), como se explica en la sección 4.6. No afecta al ranking (AUC-PR, AUC-ROC), pero sí a la interpretación de las probabilidades en el simulador.
2. **El umbral maximiza el F1 de la clase 1** (`find_best_threshold` en `global_funcs.py`). Es un criterio que favorece transmitir, no ahorrar batería. Para el closed-loop conviene sustituirlo por un criterio de costo (sección 8).
3. **El nombre `MyCustonStudentDistillationDataset`** tiene una errata ("Custon"). Es inofensiva, pero si se corrige hay que cambiarla en `MyCustomClasses.py` y en el import de `train_student.py` a la vez.
4. **`train_one` salta los runs cuyo JSON ya existe.** Al cambiar cualquier aspecto del entrenamiento, hay que mover o borrar `results/student` antes de volver a correr; si no, se mezclan runs de versiones distintas.

---

## Archivos

| Archivo | Contenido |
|---|---|
| `results/teacher_gradient_boosting/logits/` | Logits out-of-fold del maestro, uno por seed |
| `results/teacher_gradient_boosting/metrics/` | Métricas del maestro (OOF, in-sample y val) |
| `results/student/runs/*.json` | Métricas de cada run (1740) |
| `results/student/models/*.pth` | Pesos del estudiante |
| `results/student/scalers/*.json` | Media, escala y umbral para el simulador |
| `results/student/grid_alpha_temp.csv` | Media y std de las 168 combinaciones KD |
| `results/student/best_alpha_temp.csv` | Mejor α/T por fracción, con la referencia no_kd |
| `results/student/best_vs_no_kd_metrics.csv` | Métricas por clase (todos los paquetes): mejor KD frente a no_kd |

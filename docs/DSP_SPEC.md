# DSP_SPEC — Contrato técnico y musical
**Versión:** 0.2 · Alineado al rediseño UI (`UMBRAL Duck — Rediseño UI.pdf`).

## 1. Señal, Controles y Ecuación de Ganancia
- **Unificación Mix/Depth:** Para evitar redundancia matemática, el control único de intensidad se denomina **`MIX`** ($M \in [0, 1]$, mostrado como `0%..100%`, por defecto `70%` / `0.70`).
- **Curva base:** $C(\phi) \in [0, 1]$, donde $0$ representa máxima atenuación (suelo visual en $-24\text{ dB}$ o silencio según curva) y $1$ representa ganancia unitaria ($0\text{ dB}$).
- **Desplazamiento de fase (`SHIFT`):** Parámetro $S \in [0, 1]$ (`0%..100%`, default `0%`). Desplaza horizontalmente la lectura de la tabla/función de curva con wrap cíclico:
  $$\phi_{\text{shifted}} = \text{frac}(\phi - S)$$
- **Inclinación/Tensión (`SLOPE`):** Parámetro $K \in [0, 1]$ (`0%..100%`, default `50%`). En $K = 0.5$ la curva mantiene su forma nominal; valores $< 0.5$ comprimen la recuperación (más rápida/cóncava) y valores $> 0.5$ expanden la retención del ducking (más lenta/convexa) aplicando una función de potencia continua sobre cada segmento:
  $$p = 2^{(K - 0.5) \cdot 2}, \quad t' = t^p$$
- **Ganancia efectiva instantánea:**
  $$G(\phi) = (1 - M) + M \cdot C_{\text{slope}}(\phi_{\text{shifted}})$$
- **Lectura en decibelios (UI Readout y Eje Y):**
  $$\text{Gain}_{\text{dB}}(\phi) = 20 \cdot \log_{10}(\max(G(\phi), 10^{-3}))$$
  Clampeado visualmente en el rango $[0\text{ dB}, -24\text{ dB}]$ para el área de trazado.
- **Modo de canales (`CHANNELS: Stereo / Mono`):**
  - `Stereo`: aplica $G[n]$ idéntico a L y R preservando la imagen estéreo original: $y_L[n] = G[n] \cdot x_L[n]$, $y_R[n] = G[n] \cdot x_R[n]$.
  - `Mono`: suma la entrada a mono con compensación de potencia antes de aplicar la envolvente: $m[n] = 0.5 \cdot (x_L[n] + x_R[n]) \cdot G[n]$, escribiendo $m[n]$ en ambos canales de salida.
- **Bypass:** Rampa lineal/exponencial libre de clics de 10 ms hacia $G = 1.0$ y preservación del canal original.

## 2. Tiempo Musical y Divisiones (`RATE`)
La posición PPQ de JUCE se mide en negras métricas. Las 4 divisiones de `RATE` del diseño son:
- `1/8` = $0.5$ negras.
- `1/4` = $1.0$ negra.
- `1/2` = $2.0$ negras.
- `1/1` = $N \cdot (4 / D)$ negras según el compás $N/D$ del host (en 4/4 equivale a $4.0$ negras = 1 compás completo). Fallback a $4.0$ si el host no reporta compás.

Para un ciclo de $L$ negras, BPM $B$ y frecuencia de muestreo $f_s$:
$$\Delta\phi = \frac{B}{60 \cdot f_s \cdot L}, \quad \phi[n] = \text{frac}\left(\frac{\text{PPQ}_0 - \text{anchor}}{L} + n \cdot \Delta\phi\right)$$
Donde $\text{frac}(x) = x - \lfloor x \rfloor$ (soporta PPQ negativos).

## 3. Política de Transporte y `SYNC`
- **`SYNC` Activo (Locked to DAW):**
  - Host reproduciendo con datos válidos: sincroniza fase y BPM al DAW; actualiza el indicador `1.2.3` (Compás.Tiempo.Subdivisión).
  - Host detenido (`Stopped`): ganancia retorna suavemente a $1.0$ ($0\text{ dB}$) con rampa anti-clic.
- **`SYNC` Inactivo (Free / Own BPM):**
  - El acumulador de fase corre continuamente gobernado por el parámetro `bpm` (`40.0` a `240.0`, default `120.0`), permitiendo ducking libre o uso en hosts sin datos de transporte.

## 4. Banco de Curvas Originales (`CURVE BANK` 01–12)
Motor por segmentos con nodos $(\phi, \text{valor})$ ordenados en $[0, 1]$, interpolados con `smoothstep(t) = t^2(3 - 2t)` modulado por `SLOPE`, o tramos lineales/escalón según el preset:
1. **`01 Classic`:** Bombeo estándar equilibrado `[(0,0), (0.12,0), (0.35,0.30), (0.65,0.85), (1,1)]`.
2. **`02 Sharp`:** Caída inmediata y ascenso rápido anguloso `[(0,0), (0.05,0), (0.25,0.90), (1,1)]`.
3. **`03 Smooth`:** Curva en campana suave tipo coseno invertido (como se muestra en el PDF con valle en $1/2$) `[(0,1), (0.15,0.80), (0.42,0.0), (0.58,0.0), (0.85,0.80), (1,1)]`.
4. **`04 Tight`:** Ducking corto para kicks secos `[(0,0), (0.04,0), (0.18,1), (1,1)]`.
5. **`05 Reverse`:** Rampa ascendente invertida con caída al final del ciclo `[(0,1), (0.65,0.85), (0.92,0), (1,0)]`.
6. **`06 Rhythmic`:** Doble modulación dentro de un mismo ciclo `[(0,0), (0.25,1), (0.50,0.35), (0.75,1), (1,1)]`.
7. **`07 Deep`:** Retención prolongada en atenuación máxima `[(0,0), (0.45,0), (0.80,0.75), (1,1)]`.
8. **`08 Light`:** Atenuación sutil y recuperación temprana `[(0,0.35), (0.15,0.35), (0.45,1), (1,1)]`.
9. **`09 Pump`:** Efecto sidechain exagerado con curva exponencial tardía `[(0,0), (0.25,0.05), (0.65,0.40), (0.92,1), (1,1)]`.
10. **`10 Flow`:** Onda sinusoidal continua suave `[(0,0.5), (0.25,0), (0.75,1), (1,0.5)]`.
11. **`11 Step`:** Compuerta rítmica por escalones (interpolación step/rampa de 2 ms anti-clic) `[(0,0), (0.50,0), (0.501,1), (1,1)]`.
12. **`12 Custom`:** En el MVP delega de forma segura a la curva `01 Classic` (o mantiene el último preset válido) mostrando el aviso en UI *"Custom: editor coming in a later version"*.

## 5. Medición y Suavizado en Tiempo Real
- **Suavizado:** Cambios en `MIX`, `SHIFT`, `SLOPE`, `RATE` o selección de curva aplican rampa/crossfade de 5–10 ms para evitar clics. En discontinuidades de salto de ciclo ($C(1) \neq C(0)$), aplicar micro-rampa de 1 ms.
- **Medidores `IN` / `OUT`:** Cálculo de picos por bloque en dBFS con decaimiento balístico suave, exportado mediante `std::atomic<float>` para consumo exclusivo del hilo de UI.
- **Restricción absoluta:** Cero asignaciones dinámicas (`new`/`malloc`), locks, I/O ni llamadas a UI dentro de `processBlock`.
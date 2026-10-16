# Registro de decisiones

## 2026-10-13 — Dirección de producto B
**Decisión:** UMBRAL Duck priorizará rapidez de uso con presets originales y un camino claro a curvas editables. **Motivo:** Kickstart 2 demuestra el valor del flujo rápido; LFOTool/Duck 2 la utilidad de edición, y STFU muestra competencia gratuita real. **Alternativas descartadas por ahora:** clon minimalista sin personalización (A), suite avanzada con MIDI/audio trigger y multibanda desde el inicio (C). **Implicación:** editor interactivo en etapa 2; MVP primero con motor fiable, presets y sincronía. Fuente y límites: `docs/RESEARCH.md`.

## 2026-10-14 — Entorno y licencia (T-01)
- **JUCE:** FetchContent fijado al tag `8.0.15`. **Catch2:** v3 `v3.9.1` por FetchContent. Ambos configurables en `CMakeLists.txt`.
- **Licencia:** el proyecto se documenta bajo GPLv3/AGPLv3, compatible con JUCE 8 Open Source. La auditoría final queda para T-14.
- **Rutas con espacio:** la carpeta raíz `UMBRAL- Duck` no se renombra; todas las rutas de CMake van entre comillas.
- **`DesignTokens.h`:** incluido en T-01 (`Source/Common/`).

## 2026-10-14 — Resolución de contradicciones de especificación
- **Curvas y mezcla:** 11 curvas operativas (01–11) + slot 12 `Custom`. Solo existe `MIX`; no hay `Depth`. Tokens y diseño aprobados.
- **Clics:** se usa "suavizado para minimizar discontinuidades audibles"; no se promete cero clics.
- **Mono:** promedio lineal `0.5·(L+R)` (-6 dB por canal) para evitar clipping en fase.
- **`12 Custom`:** delega siempre a `01 Classic` en v1.
- **`SLOPE`:** `t' = t^p` sobre el tiempo normalizado del segmento, suba o baje el tramo.
- **`11 Step`:** escalón ideal en `0.50`; el anti-clic lo da `GainSmoother` (1–2 ms). **`10 Flow`:** `smoothstep` por tramo.
- **dB:** el cálculo interno llega a -60 dB; la gráfica recorta el eje Y en -24 dB. Medidores `IN/OUT` solo Pico con decaimiento balístico.
- **Tokens derivados:** texto activo `Umbra`; hover = 50% `Hairline` + 50% `Muted`; disabled `0.40`; playhead `Bone`; relleno de curva `Signal` alpha `0.15`.
- **Fuentes:** se embebe también `Archivo-Light.ttf` (peso 300); fallback a fuentes del sistema mientras falten los `.ttf`.

## 2026-10-15 — Curvas puras (T-03)
- **Interpolación:** `smoothstep` en todos los segmentos; `lerp` queda como helper puro en `MathUtils.h`. Segmentos con `dx <= 1e-6` saltan al valor del segundo nodo (sin división). `11 Step`: `phi < 0.5 -> 0`, `phi >= 0.5 -> 1`.
- **Responsabilidades:** `evaluateNodes(nodes, phi, slope)` acota `phi` a [0,1] (`phi = 1` devuelve el último nodo exacto); `evaluate(curve, phi, shift, slope)` aplica `frac(phi - shift)` antes (por tanto `phi = 1` se envuelve a 0).
- **Seguridad:** `phi`/`shift` no finitos -> `0.0f`; `slope` no finito -> `0.5f`. Índice de curva fuera de rango se acota a 0..11.

## 2026-10-15 — Ganancia y parámetros (T-04)
- **Aislamiento DSP:** `DuckingEngine` (`Source/DSP/`) es 100% libre de cabeceras de JUCE; procesa punteros/spans estándar de C++ (`float* const* channels, int numChannels, int numSamples`). `PluginProcessor` extrae los punteros del buffer y los pasa a `DuckingEngine`.
- **Contrato de 9 parámetros APVTS:** `rate`, `sync`, `bpm`, `curve`, `mix`, `shift`, `slope`, `channels`, `bypass` registrados en `ParameterLayout.cpp` con IDs en `ParameterIDs.h` (`ParameterID { id, 1 }`). Punteros atómicos raw cacheados en el procesador para lectura RT-safe lock-free.
- **Fase temporal en T-04:** mientras no exista `PhaseClock` (T-05), `PluginProcessor::processBlock` evalúa en `phase = 0.0f` por defecto. Dado que la curva por defecto `03 Smooth` tiene `C(0.0f) == 1.0f`, `PassThroughTests` sigue pasando bit-identical.
- **Canales:** modo `Stereo` escala canales L y R de forma independiente; modo `Mono` calcula el promedio lineal `0.5 * (L + R) * G` hacia ambos canales para evitar clipping con señales en fase.

## Pendientes
- Auditoría de licencia JUCE y del código propio antes de publicar (T-14).
- Pruebas comparativas y DAW de referencia.


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

## 2026-10-16 — Fase matemática (T-05)
- **Aislamiento y precisión:** `PhaseClock` (`Source/DSP/PhaseClock.h/.cpp`) es un módulo matemático puro (`noexcept`, sin heap, 100% libre de JUCE) que opera internamente en `double` para evitar deriva con PPQ grandes o bloques largos y entrega fases normalizadas `float` en `[0.0f, 1.0f)`.
- **Subdivisiones y anclaje (`anchor`):** `1/8` (`0.5`), `1/4` (`1.0`), `1/2` (`2.0`) se anclan a la cuadrícula `PPQ = 0.0`. `1/1` (`OneBar`, `L = N * 4.0 / D` con fallback a `4/4` si `N <= 0` o `D <= 0`) se ancla a `barStartPpq` cuando `hasBarStartPpq` es verdadero y finito, o a `0.0` en su ausencia.
- **Lectura musical (`BarBeatTick`):** calcula `bar.beat.subdivision` (1-indexado, pulso de longitud `4.0 / D` negras y subdivisión en 4 partes por pulso) soportando compases `4/4`, `3/4`, `6/8` y PPQ negativos.

## 2026-10-16 — Transporte (T-06)
- **Módulo:** `TransportClock` (`Source/DSP/TransportClock.h/.cpp`), puro y `noexcept`, sin JUCE. `PluginProcessor` convierte `juce::AudioPlayHead::PositionInfo` a `HostTransport` y delega en `fillBlockPhases` (T-05).
- **Modos:** `Locked` (SYNC on, host reproduciendo con ppq válido), `Free` (SYNC off: acumulador `double` interno, `dt = 1/fs`, solo parámetro `bpm`; independiente del host) y `Bypass` (SYNC on con host detenido, sin PlayHead o sin ppq finito).
- **Bypass:** fase congelada en la última válida; el Processor fuerza `mix = 0` hacia `DuckingEngine` (G = 1.0 exacto, salida Stereo bit-identical). No se usa `params.bypass` para conservar el downmix del modo `Mono`. La rampa anti-clic queda para T-07.
- **BPM:** en `Locked` manda el tempo del host; si no es válido (ausente, NaN, <= 0) se usa el parámetro `bpm`.
- **Continuidad:** `Locked` actualiza el acumulador libre, así que `Locked -> Free` no salta. Seek y loop se resuelven desde el ppq del host en cada bloque.
- **Memoria:** `phaseBuffer` se preasigna (4096 en el constructor, `prepareToPlay` lo ajusta a `samplesPerBlock`). Si el host entrega un bloque mayor, `processBlock` lo procesa en tramos sin redimensionar; `sampleOffset` mantiene el ppq continuo entre tramos.

## 2026-10-16 — Suavizado de ganancia (T-07)
- **Módulo:** `GainSmoother` (`Source/DSP/GainSmoother.h/.cpp`), filtro IIR de un polo por muestra (`y[n] = y[n-1] + alpha * (x[n] - y[n-1])`), 100% libre de JUCE, `noexcept` y sin asignaciones dinámicas.
- **Calibración temporal:** el coeficiente $\alpha = 1 - \exp(-\ln(100) / (f_s \cdot T_{\text{ms}} \cdot 10^{-3}))$ se calibra para alcanzar el $99\%$ del escalón en `1.5 ms` por defecto (dentro de la ventana contractual de `1.0 ms` a `2.0 ms`).
- **Snap y arranque limpio:** cuando $|x[n] - y[n]| \le 10^{-6}$, el estado hace *snap* exacto al objetivo (preservando ganancia `1.0f` bit-idéntica en reposo/bypass). `resetToNextSample()` permite que la primera muestra tras `prepareToPlay` o `reset` fije el nivel inicial sin rampa artificial de arranque.
- **Integración y métricas:** `DuckingEngine` aplica `GainSmoother` muestra a muestra sobre `computeGain` y en las rampas de `bypass` (preservando los canales originales cuando `bypass` está activo). Las pruebas en `Tests/GainSmootherTests.cpp` verifican cuantitativamente el tiempo de asentamiento (`1–2 ms`) y la cota de diferencia máxima entre muestras consecutivas ($\Delta G_{\max} \le \alpha$) en `11 Step`, wrap de `05 Reverse`, cambios de `mix`/`bypass` y paradas de transporte entre `44.1 kHz` y `192 kHz` y `40–240 BPM`.

## 2026-10-16 — Estado, automatización y snapshot atómico (T-08)
- **Snapshot lock-free (`Processor -> UI`):** `UiStateSnapshot` (`Source/Common/UiStateSnapshot.h`), libre de JUCE y `noexcept`, transfiere `currentPhase` ($[0, 1)$), `currentGainDb` ($[-60, 0]\text{ dB}$), `barBeatTick` (`BarBeatTick` empaquetado en `std::atomic<uint64_t>` para evitar *torn reads*) y niveles `inputLevelDb` / `outputLevelDb` ($[-60, 0]\text{ dB}$) con semántica `memory_order_release` / `memory_order_acquire` en el cierre del bloque.
- **Balística de medidores Peak (`IN` / `OUT`):** `LevelMeterSource` (`Source/DSP/LevelMeterSource.h`), 100% libre de JUCE y `noexcept`, mide el pico lineal por bloque antes y después del procesamiento de ducking con ataque instantáneo y decaimiento exponencial suave ($\tau = 300\text{ ms}$, $\approx 28.9\text{ dB/s}$), con *snap* al piso de $-60\text{ dB}$ ($10^{-3}$).
- **Persistencia APVTS:** verificado mediante pruebas de roundtrip XML (`getStateInformation` / `setStateInformation`) que los 9 parámetros del contrato y las propiedades de UI en el `ValueTree` (como `compactMode`) se restauran produciendo audio y snapshots idénticos.

## 2026-10-16 — Tokens de diseño y LookAndFeel base (T-09)
- **Tokens canónicos (`Source/UI/DesignTokens.h`):** paleta oficial confirmada según `docs/DESIGN.md` (`Umbra #131311`, `Panel #1A1A17`, `Raised #22221E`, `Hairline #33322C`, `Muted #8C8A7E`, `Bone #ECE8DC`, `Signal #C9F04B`), opacidades derivadas y constantes geométricas/tipográficas. `Source/Common/DesignTokens.h` redirige a `Source/UI/DesignTokens.h` para mantener una única fuente de verdad y compatibilidad con includes previos.
- **`UmbralLookAndFeel` (`Source/UI/UmbralLookAndFeel.h/.cpp`):** hereda de `juce::LookAndFeel_V4`, registra el esquema de color oficial y sobrescribe el dibujado plano sin sombras ni degradados para `drawRotarySlider` (cuerpo circular `Raised`, borde `1 px Hairline` y aguja indicadora radial `Bone` de `1.5 px` en barrido de `270°`), `drawButtonBackground`/`drawButtonText` (esquinas redondeadas `4 px`, estado activo relleno `Signal`/`Umbra` y variante `kOutlineActiveStyleProperty` con fondo `Raised` + borde/texto `Signal` para controles como `SYNC`) y `drawComboBox`.
- **Tipografía fallback JUCE 8:** construida con `juce::FontOptions` (libre de constructores deprecados de `juce::Font` bajo `/W4 /WX`) empleando las familias sans-serif y monoespaciadas del sistema con la jerarquía exacta de alturas de `docs/DESIGN.md` mientras se integran los binarios `.ttf` de `Archivo` e `IBM Plex Mono`.

## Pendientes
- Auditoría de licencia JUCE y del código propio antes de publicar (T-14).
- Pruebas comparativas y DAW de referencia.


## T-10 - UI funcional (GAIN RESPONSE y LEVEL)
- Eje vertical de GainResponseView mapeado a [-24, 0] dB con recorte visual; la lectura numerica del snapshot llega hasta -60 dB.
- Curva efectiva G(phi) = (1 - M) + M * C_slope(frac(phi - S)) via DuckingEngine::computeGain y math::gainToDb: misma funcion que el audio.
- Compact oficial = 560 x 360 px (token windowCompactHeight 368 -> 360; PRD y ARCHITECTURE actualizados). Nota: AGENTS.md aun menciona 560 x 368 y debe actualizarse a mano.
- LevelMetersComponent: el snapshot expone un unico nivel IN y uno OUT, por lo que se dibuja una barra por canal (no L/R); escala [-48, 0] dB.
- El panel LEVEL ocupa todo el panel derecho hasta que exista el selector CHANNELS.
- Timer 60 Hz lee el snapshot lock-free; Path y textos se reconstruyen en refresh()/resized(), paint() no reserva memoria.

## 2026-10-16 — Doble vista Full/Compact y persistencia de ventana (T-11)
- **Dos dimensiones fijas y constrainer:** `UmbralDuckEditor` bloquea el redimensionamiento libre (`setResizable(false, false)` y `setResizeLimits(w, h, w, h)`) alternando únicamente entre **Full (`960 x 600 px`)** y **Compact (`560 x 360 px`)**.
- **Comportamiento en Compact:** oculta `LevelMetersComponent` (`setVisible(false)`), oculta la pastilla `1.2.3` del playhead en `GainResponseView` (`setBarPillVisible(false)`) y reposiciona `GainResponseView` en la cuadrícula Compact (`x = 16`, `y = 65`, `w = 350`, `h = 279` según `docs/DESIGN.md §5`).
- **Persistencia en `APVTS`:** el estado de vista se almacena en la propiedad `isCompactView` (`UmbralDuckProcessor::kIsCompactViewProperty`) del `ValueTree` de `apvts.state` para no contaminar la lista de parámetros automatizables del DAW. `UmbralDuckEditor` escucha `apvts.state` mediante `juce::ValueTree::Listener` (`valueTreePropertyChanged` y `valueTreeRedirected`), sincronizando inmediatamente el tamaño y la visibilidad tanto al reabrir el editor como cuando el host invoca `setStateInformation` con el editor abierto.

## 2026-10-16 — Panel de controles (`ControlStrip` y `ChannelsSelector`) (T-12)
- **`ControlStrip` (`Source/UI/ControlStrip.h/.cpp`):** agrupa el selector desplegable `CURVE` (12 opciones del contrato, altura `36 px`), la perilla principal `MIX` (diámetro `80 px`, barrido `270°`, lectura numérica `21 px` en `IBM Plex Mono`) y las dos perillas secundarias `SHIFT` y `SLOPE` en columnas simétricas (diámetro `44 px`, barrido `270°`, lecturas `13 px`), enlazados bidireccionalmente mediante `APVTS::ComboBoxAttachment` y `APVTS::SliderAttachment` con orden de destrucción explícito libre de fugas.
- **Adaptabilidad Full/Compact:** en modo Full se ubica a la izquierda (`20, 77, 193, 368`) con sus 3 secciones separadas por líneas `1 px Hairline`; en modo Compact (`setCompactMode(true)`) se reubica al panel derecho (`382, 65, 160, 279`), ocultando `SHIFT` y `SLOPE` y dejando visibles únicamente `CURVE` y `MIX`.
- **`ChannelsSelector` y panel derecho unificado:** implementado como control segmentado mutuamente excluyente (`Stereo` / `Mono`, altura `32 px`) enlazado al parámetro `channels` mediante `juce::ParameterAttachment`. Se aloja en la franja superior (`82 px`) del contenedor derecho (`787, 77, 153, 368`) sobre `LevelMetersComponent` (que reserva `channelsSectionHeight` y dibuja el separador `1 px Hairline`), ocultándose junto con `LevelMetersComponent` en vista Compact.

## 2026-10-16 — Cabecera (`HeaderBar`) y banco de curvas (`CurveBankGrid`) (T-13)
- **`HeaderBar` (`Source/UI/HeaderBar.h/.cpp`):** concentra el logotipo vectorial y tipográfico `UMBRAL duck` (`getBrandFont(true/false)`), el selector segmentado de 4 botones `RATE` (`1/8`, `1/4`, `1/2`, `1/1`), el readout/control arrastrable de `BPM` (`40.0..240.0` en `IBM Plex Mono` `20 px`, habilitado únicamente cuando `SYNC` está en modo `Free`), el botón `SYNC` (`kOutlineActiveStyleProperty`), el botón `BYPASS` y el botón de alternancia `Full/Compact`. En modo Compact (`48 px`) oculta `BYPASS` y las etiquetas secundarias `RATE`/`BPM`.
- **`CurveBankGrid` (`Source/UI/CurveBankGrid.h/.cpp`):** renderiza la fila inferior (`920 x 114 px` en `x = 20, y = 466` en vista Full; oculto en Compact) con cabecera `CURVE BANK`, nota informativa de la ranura 12 y 12 celdas interactivas (`01 Classic` .. `12 Custom`) enlazadas bidireccionalmente al parámetro `curve` de `APVTS` mediante `juce::ParameterAttachment`. Las miniaturas vectoriales se precalculan en `resized()` evaluando `dsp::getNodes()` + `dsp::evaluateNodes()` (forma nominal `shift = 0`, `slope = 0.5`), y la celda `12 Custom` genera su trazo discontinuo mediante `juce::PathStrokeType::createDashedStroke` sin asignaciones en `paint()`.

## 2026-10-16 — Compilación Release y Validación pluginval (T-14)
- **Flags de punto flotante en `Release`:** se mantienen los flags recomendados por JUCE (`juce::juce_recommended_config_flags` y `juce::juce_recommended_lto_flags`, que activan `/O2` y `/GL` bajo `/fp:precise` en MSVC) sin forzar `/fp:fast`, garantizando que las guardas IEEE-754 (`std::isfinite` ante `NaN`/`Inf`) exigidas por `docs/DSP_SPEC.md` no sean eliminadas por el optimizador.
- **Verificación CTest en `Release`:** compilación limpia con `/W4 /WX` (0 warnings) y **73/73 pruebas superadas al 100 %** (`2.58 s` totales) sobre `build/Release/UmbralDuckTests.exe`.
- **Validación `pluginval` (Strictness Level 5):** ejecutado `pluginval.exe --strictness-level 5 --validate-in-process` sobre `build/UmbralDuck_artefacts/Release/VST3/UMBRAL Duck.vst3` con resultado **`SUCCESS`** (código de salida `0`; log en `build/pluginval-logs/UMBRAL Duck.vst3_20261016T182211.989-0400.txt`). Superadas todas las pruebas de escaneo, apertura en frío/caliente, editor abierto durante procesamiento, audio en 44.1 / 48 / 96 kHz con bloques de 64 a 1024 muestras, estado APVTS, automatización de parámetros (sub-bloques de 32 muestras), automatización desde el editor y layouts de buses `Mono` / `Stereo`.
- **Artefactos `Release` generados:**
  - Bundle VST3: `build\UmbralDuck_artefacts\Release\VST3\UMBRAL Duck.vst3` (binario x64 interno de `6.87 MB`).
  - Ejecutable Standalone: `build\UmbralDuck_artefacts\Release\Standalone\UMBRAL Duck.exe` (`6.74 MB`).

## 2026-10-17 — Corrección de tiempo real: Modo Free-Running en Stop, Disparo MIDI (`NoteOn`) y Continuidad Cíclica $C^1$
- **Modo Free-Running en `Stop` (`TransportClock` y `PluginProcessor`):** cuando el DAW está detenido (`!host.isPlaying`) o no reporta PPQ válido, `TransportClock::process` avanza continuamente en `TransportMode::Free` empleando `host.bpm` (si `sync == true` y el BPM del host es válido) o `paramBpm` como respaldo. `PluginProcessor` mantiene `params.mix = baseMix` en modo `Free`, permitiendo modular instrumentos virtuales en vivo con el transporte detenido.
- **Disparo por MIDI (`NoteOn` sample-accurate):** habilitado `NEEDS_MIDI_INPUT TRUE` en `CMakeLists.txt` y `acceptsMidi() -> true` en `UmbralDuckProcessor`. En `processBlock`, el bloque de audio se segmenta en las posiciones exactas de muestra donde ocurren eventos `NoteOn`, invocando `transportClock.triggerNoteOn(...)` (que reinicia `freePhase = 0.0` o fija `lockedPhaseOffset` en modo `Locked`) y `duckingEngine.triggerAntiPopRamp()`.
- **Micro-rampa cúbica Hermite $C^1$ (`2.0 ms` en `GainSmoother` / `DuckingEngine`):** `triggerAntiPopRamp()` aplica una interpolación `smoothstep` ($t^2(3 - 2t)$, derivada nula en ambos extremos) de `2.0 ms` desde la ganancia suavizada actual hacia la ganancia objetivo tanto al recibir un `NoteOn` como al detectar el wrap cíclico de fase ($\phi_{\text{shifted}}: 1.0^- \to 0.0^+$) en curvas con $C(1) \neq C(0)$, eliminando chasquidos al final del compás.
- **Eliminación de la caída vertical en $\phi = 1.0$ (`MathUtils::applyShift` y `GainResponseGeometry`):** `applyShift(1.0f, shift)` preserva `1.0f` cuando el desplazamiento efectivo es `0.0f` en lugar de envolverlo a `0.0f`, eliminando la línea vertical artificial en el extremo derecho del gráfico `GAIN RESPONSE`.


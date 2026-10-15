# ARCHITECTURE — UMBRAL Duck
**Versión:** 0.2 · Alineado al rediseño UI y a la Guía de planeación.

## 1. Tabla de Stack Cerrada
| Capa | Decisión registrada |
|---|---|
| **Lenguaje y estándar** | C++20 (`CMAKE_CXX_STANDARD 20`) |
| **Framework principal** | JUCE 8.x (submódulo Git o FetchContent fijado a tag exacto en T-01) |
| **Formato de plugin** | VST3 (`x86_64` Windows como target primario; Standalone solo para desarrollo/test) |
| **Build system** | CMake >= 3.22 + Ninja / MSVC (Visual Studio 2022 Build Tools) |
| **UI y Renderizado** | `juce::Graphics` / `juce::Path` nativo (sin WebView ni frameworks externos) |
| **Fuentes tipográficas** | `Archivo` (Light, Regular, SemiBold, ExtraBold) e `IBM Plex Mono` (Regular, Medium) embebidas vía `juce_add_binary_data`; fallback a fuentes del sistema mientras falten los `.ttf` |
| **Estado y Parámetros** | `juce::AudioProcessorValueTreeState` (APVTS) con serialización XML en estado del plugin |
| **Testing** | CTest + Catch2 v3 (pruebas unitarias/integración) + `pluginval` (strictness level 5) |
| **Dependencias prohibidas** | Librerías de terceros para UI, Boost, motores de scripting, SDKs de red/telemetría |

## 2. Estructura del Repositorio
```text
UMBRAL-Duck/
  README.md
  AGENTS.md
  CMakeLists.txt
  docs/
    PRD.md
    ARCHITECTURE.md
    DSP_SPEC.md
    DESIGN.md
    TASKS.md
    DECISIONS.md
    RESEARCH.md
    refs/
      UMBRAL Duck — Rediseño UI.pdf
  Assets/
    Fonts/
      Archivo-Light.ttf
      Archivo-Regular.ttf
      Archivo-SemiBold.ttf
      Archivo-ExtraBold.ttf
      IBMPlexMono-Regular.ttf
      IBMPlexMono-Medium.ttf
  Source/
    Common/
      AudioDefines.h
      DesignTokens.h
      MathUtils.h
    Parameters/
      ParameterIDs.h
      ParameterLayout.h
      ParameterLayout.cpp
    DSP/
      CurveBank.h
      CurveBank.cpp
      PhaseClock.h
      PhaseClock.cpp
      DuckingEngine.h
      DuckingEngine.cpp
      GainSmoother.h
      GainSmoother.cpp
      LevelMeterSource.h
    UI/
      UmbralLookAndFeel.h
      UmbralLookAndFeel.cpp
      HeaderBar.h
      HeaderBar.cpp
      GainResponseView.h
      GainResponseView.cpp
      CurveBankGrid.h
      CurveBankGrid.cpp
      ControlStrip.h
      ControlStrip.cpp
      LevelMetersComponent.h
      LevelMetersComponent.cpp
      PluginEditor.h
      PluginEditor.cpp
    PluginProcessor.h
    PluginProcessor.cpp
  Tests/
    DSPTests.cpp
    IntegrationTests.cpp
```

## 3. Capas y Flujo de Datos
- **Dirección de dependencia:** `UI (60 Hz Timer) ──(lee atomics / APVTS)──> PluginProcessor <──(valor por bloque)──> DSP puro`.
- **Regla de aislamiento:** La carpeta `Source/DSP/` no incluye cabeceras de GUI ni depende de componentes visuales.
- **Curva compartida:** `CurveBank` expone funciones matemáticas puras `constexpr`/`noexcept` que tanto `DuckingEngine` (audio thread) como `GainResponseView` (UI thread) evalúan sin compartir memoria mutable.
- **Snapshots atómicos (`Processor -> UI`):**
  - `currentPhase` (`std::atomic<float>` en `[0, 1)`)
  - `currentGainDb` (`std::atomic<float>` en `[-24.0, 0.0]`)
  - `barBeatTick` (estructura atómica para mostrar `1.2.3`)
  - `inputLevelDb` y `outputLevelDb` (`std::atomic<float>` para los medidores `IN`/`OUT`)

## 4. Contrato de Parámetros (`APVTS`)
| ID v1 | Nombre UI | Tipo | Rango / Opciones | Default |
|---|---|---|---|---|
| `rate` | `RATE` | Choice | `1/8`, `1/4`, `1/2`, `1/1` | `1/4` |
| `sync` | `SYNC` | Bool | `false` (Free), `true` (Locked to DAW) | `true` |
| `bpm` | `BPM` | Float | `40.0` .. `240.0` (paso `0.1`) | `120.0` |
| `curve` | `CURVE BANK` | Choice | `01 Classic` .. `12 Custom` (12 opciones) | `03 Smooth` |
| `mix` | `MIX` | Float | `0.0` .. `100.0` % | `70.0` % |
| `shift` | `SHIFT` | Float | `0.0` .. `100.0` % | `0.0` % |
| `slope` | `SLOPE` | Float | `0.0` .. `100.0` % | `50.0` % |
| `channels` | `CHANNELS` | Choice | `Stereo`, `Mono` | `Stereo` |
| `bypass` | `BYPASS` | Bool | `false`, `true` | `false` |

*Nota: El estado de tamaño de ventana (`Window 960x600` vs `Compact 560x368`) se guarda en el `ValueTree` de propiedades de UI del `APVTS` para no contaminar la lista de automatización del DAW.*

## 5. Reglas de Tiempo Real y Distribución
- En `processBlock`: cero asignaciones en heap (`new`, `resize`), cero `std::mutex`, cero acceso a disco o llamadas a UI.
- CI en Windows: compilar y correr CTest en cada PR una vez que T-01 establezca los comandos reproducibles.
# UMBRAL Duck

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![JUCE 8](https://img.shields.io/badge/JUCE-8.0.15-orange.svg)](https://juce.com/)
[![Format](https://img.shields.io/badge/Format-VST3%20%7C%20Standalone-purple.svg)](https://www.steinberg.net/technologies/vst/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)](https://microsoft.com/windows)
[![Tests](https://img.shields.io/badge/CTest-86%2F86%20Passing-brightgreen.svg)](Tests/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**UMBRAL Duck** is a modern, high-precision rhythm ducking (sidechain pump) audio effect plugin built with **C++20** and **JUCE 8**. Designed with an uncompromising brutalist aesthetic and an ultra-clean, click-free $C^1$ continuous DSP engine, it delivers surgical volume modulation without relying on complex sidechain routing.

---

## Key Features

- **Pristine, Click-Free DSP Engine:**
  - **Acoustic Pre-Release Window ($\phi \in [0.97, 1.0]$):** Hermite $C^1$ smoothstep lookahead guarantees exact periodic continuity ($G(1.0^-) = G(0.0^+)$ with zero derivative), eliminating cyclic boundary pops on wrap-around.
  - **Dynamic Step Detection:** Sample-accurate Hermite anti-pop smoothing ($4.5\text{ ms}$) on sharp internal edges (e.g., Trance Gate) and a calibrated exponential tracking filter ($2.5\text{ ms}$) prevent audible clicks across all tempo subdivisions and sample rates.
  - **Zero Allocations & Lock-Free:** Strict audio-thread isolation (`noexcept`, zero heap allocations, zero mutexes, zero disk/OS calls).

- **16-Slot Curve Bank ($2 \times 8$ Matrix):**
  - Instant selection across 15 curated mathematical curves covering classic sidechain pumping, sharp transients, gentle inverted bells, reverse sweeps, trance gating, and step modulations.
  - Clean vector miniature previews with dynamic state highlighting.

- **Interactive Custom Curve Editor (Slot 16):**
  - Visual breakpoint curve editor directly on the main gain response graph.
  - Supports 2 to 16 monotonic nodes with clamped boundaries ($\phi = 0.0$ and $\phi = 1.0$) and real-time safe triple-buffered synchronization with the DSP engine.
  - Intuitive mouse workflow: hover glow, drag with neighbor-bounding, double-click to add nodes, right-click / Alt+Click to remove intermediate nodes.
  - Vector thumbnail in the 16th bank cell updates dynamically with dashed rendering.

- **Native VST3 Host Context Menu Integration:**
  - Full right-click support on parameter knobs (`MIX`, `SHIFT`, `SLOPE`).
  - Seamlessly exposes DAW-native context actions (e.g., *Create automation clip*, *Link to controller...* in FL Studio; automation lanes in Cubase, Reaper, and Studio One) with fallback to local parameter reset.

- **Rhythm & Transport Synchronization:**
  - **Sync Mode (Locked to Host):** Locked to DAW PPQ position and tempo with metric subdivisions (`1/8`, `1/4`, `1/2`, `1/1` OneBar with time-signature anchoring).
  - **Free-Running Mode:** Internal phase accumulator running at custom manual BPM ($40.0$ to $240.0$) when transport is stopped or sync is disabled.
  - **Musical Bar.Beat.Subdivision Indicator:** Live atomic readout (`1.2.3`).
  - **Static 'ACTIVE' Status Badge:** High-contrast visual pill indicating live gain modulation status synchronized with transport and bypass states.

- **Monitoring & Channels:**
  - Dual vertical peak level meters (`IN` / `OUT`) with smooth ballistic decay ($\tau = 300\text{ ms}$) and instant attack.
  - Channel processing modes: `Stereo` (independent channel processing) and `Mono` (phase-coherent linear sum).
  - Global `BYPASS` control with smooth anti-click ramp.

- **Brutalist Technical Interface:**
  - Optimized fixed-dimension window ($780 \times 540\text{ px}$) tailored for production workflows.
  - Strict high-contrast industrial palette (`Umbra`, `Panel`, `Raised`, `Hairline`, `Muted`, `Bone`, `Signal`).
  - Hardware-accelerated vector drawing with embedded typography (*Archivo* and *IBM Plex Mono*).

---

## Architecture Overview

```text
UMBRAL Duck/
├── CMakeLists.txt          # Root CMake build definition (JUCE 8.0.15 & Catch2 v3.9.1)
├── CMakePresets.json       # Visual Studio and Ninja Multi-Config presets
├── Assets/Fonts/           # Embedded Archivo & IBM Plex Mono typeface assets
├── Source/
│   ├── Common/             # Design tokens, math helpers, audio definitions & UI atomic snapshots
│   ├── Parameters/         # APVTS parameter layout & contract constants
│   ├── DSP/                # Pure DSP engine (DuckingEngine, PhaseClock, TransportClock, CurveBank, etc.)
│   └── UI/                 # Custom LookAndFeel, HeaderBar, GainResponseView, CurveBankGrid, ControlStrip
└── Tests/                  # Comprehensive Catch2 test suite (86/86 automated unit & integration tests)
```

For detailed technical specifications, consult the project documentation:
- [`docs/PRD.md`](docs/PRD.md) — Product requirements and architectural constraints.
- [`docs/DSP_SPEC.md`](docs/DSP_SPEC.md) — Mathematical formulations, curve equations, and smoothing filters.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — APVTS parameter contract, data flow, and thread isolation.
- [`docs/DESIGN.md`](docs/DESIGN.md) — Visual design tokens, typography, and layout specifications.
- [`docs/DECISIONS.md`](docs/DECISIONS.md) — Engineering decision records and implementation history.

---

## Building from Source

### Prerequisites

- **Operating System:** Windows 10/11 64-bit (`x86_64`).
- **Compiler:** Microsoft Visual C++ (MSVC) with **C++20** support (Visual Studio 2022 or VS Build Tools v17 / v18).
- **Build System:** [CMake](https://cmake.org/download/) $\ge 3.22$ and [Ninja](https://ninja-build.org/) (optional, included with VS C++ tools).
- **Git:** Required for fetching JUCE 8 and Catch2 dependencies via CMake `FetchContent`.

### Build Steps (Visual Studio MSVC Preset)

Open a terminal or developer prompt in the repository root directory:

```powershell
# 1. Configure the project using the MSVC preset
cmake --preset windows-msvc

# 2. Build Release binaries (VST3, Standalone, and Test suite)
cmake --build --preset release

# 3. Run the automated test suite
ctest --preset release --output-on-failure
```

### Build Steps (Ninja Multi-Config)

If building from the `x64 Native Tools Command Prompt for VS`:

```powershell
cmake --preset windows-ninja
cmake --build --preset ninja-release
ctest --preset ninja-release --output-on-failure
```

Upon successful compilation:
- **VST3 Plugin Bundle:** `build/UmbralDuck_artefacts/Release/VST3/UMBRAL Duck.vst3`
- **Standalone Executable:** `build/UmbralDuck_artefacts/Release/Standalone/UMBRAL Duck.exe`
- **Test Executable:** `build/Release/UmbralDuckTests.exe`

---

## Installation

### Windows VST3 Location

To install the plugin manually on Windows, copy the compiled `.vst3` bundle directory into your system VST3 directory:

```powershell
# Default Windows 64-bit VST3 folder:
# %CommonProgramFiles%\VST3  ->  C:\Program Files\Common Files\VST3

Copy-Item -Recurse "build\UmbralDuck_artefacts\Release\VST3\UMBRAL Duck.vst3" "$env:CommonProgramFiles\VST3\"
```

Rescan for plugins in your digital audio workstation (FL Studio, Ableton Live, Reaper, Cubase, Studio One, Bitwig, etc.).

---

## Verification & Testing

The project maintains 100% test pass rates across all mathematical and UI state logic:
- **CTest Suite:** 86 automated test cases covering curve equations, phase calculations, clock continuity, smoother ballistics, atomic state snapshots, and UI layout.
- **Pluginval:** Verified at strictness level 5 for VST3 specification compliance.

---

## Attributions & Third-Party Notice

- **Framework:** Developed using the [JUCE](https://juce.com/) framework (v8.0.15), licensed under open-source terms.
- **Testing:** Unit and integration testing powered by [Catch2](https://github.com/catchorg/Catch2) (v3.9.1, BSL-1.0).
- **Typography:**
  - *Archivo* is designed by Omnibus-Type, licensed under the [SIL Open Font License 1.1](http://scripts.sil.org/OFL).
  - *IBM Plex Mono* is designed by Mike Abbink and Bold Monday, licensed under the [SIL Open Font License 1.1](http://scripts.sil.org/OFL).

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

Copyright (c) 2026 Sergio Barazarte.

---

## Legal & Trademark Disclaimers

- **VST** is a trademark of **Steinberg Media Technologies GmbH**, registered in Europe and other countries.
- **FL Studio** is a registered trademark of **Image-Line Software NV**.
- **JUCE** is a registered trademark of **Raw Material Software Pty Ltd**.
- All other company, brand, and product names mentioned herein are trademarks or registered trademarks of their respective holders. This project is an independent open-source audio plugin and is not affiliated with, endorsed by, or sponsored by Steinberg, Image-Line, or any other trademark owner.

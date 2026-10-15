# UMBRAL Duck

Plugin gratuito de ducking rítmico para producción musical. Proyecto original inspirado en la categoría de herramientas de bombeo sincronizado al tempo; no es una copia de marca, interfaz, código ni presets propietarios de terceros.

**Estado:** especificación previa a implementación. Ningún resultado de compilación, rendimiento o compatibilidad se declara verificado.

## Objetivo inicial
Windows x86_64, VST3, JUCE 8.x, C++20 y CMake >=3.22. Un plugin que modula la ganancia con curvas reproducibles y sincronización al DAW, sin procesamiento en red ni telemetría.

## Documentos y orden de lectura
1. `docs/PRD.md`: qué construir y qué excluir.
   `docs/RESEARCH.md`: investigación oficial de competencia y referencias (apartado 2 de la guía).
2. `docs/DSP_SPEC.md`: contrato matemático y casos límite.
3. `docs/ARCHITECTURE.md`: capas, parámetros, implementación.
4. `docs/DESIGN.md`: experiencia e interfaz.
5. `AGENTS.md`: normas de trabajo para IA.
6. `docs/TASKS.md`: secuencia de tareas y aceptación.
7. `docs/DECISIONS.md`: decisiones, alternativas y asuntos abiertos.

## Construcción
Requisitos (Windows x64): Visual Studio Build Tools con C++ (verificado con MSVC 19.51 / VS 18 Build Tools), CMake >= 3.22 y Git. JUCE `8.0.15` y Catch2 `v3.9.1` se descargan por FetchContent (versiones fijadas en `CMakeLists.txt`). La primera configuración tarda varios minutos.

Local (verificado), desde una terminal con el entorno de VS cargado (`VsDevCmd.bat -arch=x64`):

```powershell
cmake --preset windows-ninja
cmake --build --preset ninja-release
ctest --preset ninja-release
```

El `.vst3` queda en `build-ninja/UmbralDuck_artefacts/Release/VST3/`. El preset `windows-msvc` (generador Visual Studio 17 2022) es el que usa el CI y **aún no se ha verificado**.

## Política de distribución
Gratuito para el usuario final. El proyecto se documenta bajo GPLv3/AGPLv3, compatible con JUCE 8 Open Source; la auditoría final de licencias y avisos de terceros queda para T-14. No utilizar nombres, logos, capturas ni curvas extraídas de productos comerciales.

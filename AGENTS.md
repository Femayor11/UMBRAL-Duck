# AGENTS.md — Reglas permanentes del agente

## Proyecto
**UMBRAL Duck:** Plugin VST3 gratuito de ducking rítmico sincronizado al tempo (C++20 / JUCE 8).
Antes de escribir código, lee siempre:
- `docs/PRD.md` (alcance y capacidades)
- `docs/DSP_SPEC.md` (ecuaciones, curvas 01–12, Shift, Slope, Mix)
- `docs/ARCHITECTURE.md` (capas, parámetros APVTS, estructura)
- `docs/DESIGN.md` y `docs/refs/UMBRAL Duck — Rediseño UI.pdf` (sistema visual y medidas)
- `docs/TASKS.md` (tarea activa y criterios de aceptación)

## Stack y Límites Técnicos
- C++20, JUCE 8.x, CMake >= 3.22, Catch2 v3.
- **Prohibido en `processBlock` (Audio Thread):** `new`, `delete`, `malloc`, `std::vector::push_back/resize`, `std::mutex`, locks, I/O de disco, logging o llamadas a componentes UI.
- **Prohibido en general:** Añadir librerías externas sin autorización explícita, cambiar IDs de parámetros publicados o acoplar `Source/DSP/` con `Source/UI/`.

## Reglas de Diseño UI (Estrictas)
- Usar **exclusivamente** los tokens definidos en `docs/DESIGN.md` (`Source/Common/DesignTokens.h`):
  - `Umbra` (`#131311`), `Panel` (`#1A1A17`), `Raised` (`#22221E`), `Hairline` (`#333220`), `Muted` (`#8C8A7E`), `Bone` (`#ECE8DC`), `Signal` (`#C9F04B`).
- Nunca escribir colores hexadecimales ni tamaños de fuente sueltos en los métodos `paint()`.
- Respetar las dos dimensiones oficiales de ventana: **Full (`960 x 600`)** y **Compact (`560 x 368`)**.
- Cero sombras (`none`) y cero degradados en controles (`none`). Bordes de `1 px`, radio de `4 px`, trazo de curva de `2.5 px`, barrido de perilla de `270°`.

## Forma de trabajo
1. Lee la tarea asignada en `docs/TASKS.md`, resume tu entendimiento y propón un plan breve de archivos a crear/modificar. **Espera confirmación antes de programar.**
2. Ejecuta **una sola tarea por sesión**. No adelantes trabajo de tareas futuras ni refactorices archivos no relacionados.
3. Escribe pruebas unitarias en `Tests/` para toda lógica matemática o de estado antes de dar la tarea por terminada.
4. Verifica que el proyecto compile sin warnings y pasen todos los tests de CTest.

## Definición de "Terminado"
- Cumple todos los criterios de aceptación de la tarea en `docs/TASKS.md`.
- Compila limpio y pasa las pruebas automatizadas.
- Si es una tarea de UI, respeta exactamente los tokens, tipografías (`Archivo` / `IBM Plex Mono`) y proporciones del PDF de diseño.
- `docs/TASKS.md` y `docs/DECISIONS.md` actualizados si corresponde.
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

## Construcción (cuando exista código)
La configuración, compilación y pruebas se definirán y comprobarán en T-01. No se presentan comandos no verificados como si ya funcionaran. Dependencias externas: JUCE 8.x y un SDK VST3 compatible a través de JUCE; revisar condiciones de licencia antes de publicar.

## Política de distribución
Gratuito para el usuario final. La elección definitiva entre las modalidades de licencia de JUCE, la licencia del código propio y los avisos de terceros es una decisión de publicación pendiente. No utilizar nombres, logos, capturas ni curvas extraídas de productos comerciales.

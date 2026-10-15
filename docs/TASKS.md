# TASKS — plan de MVP
**Estado:** solo T-09 (aprobación de diseño, solo documentación, sin código de UI) está marcada como completada. Una tarea por sesión/cambio. No saltar dependencias.

- [x] **T-01 Entorno reproducible.** Fijar JUCE 8.x exacto, compilador, CMake, dependencias/licencias, comandos de build/test, `DesignTokens.h` y README. **Aceptar:** proyecto VST3 vacío compila desde clon limpio en Windows; CI básico ejecuta build.
- [x] **T-02 Pass-through.** Processor con buses mono/estéreo, sin DSP. **Aceptar:** audio in/out idéntico en condiciones nominales; prueba en host.
- [ ] **T-03 Curvas puras.** Motor de nodos, interpolación (`smoothstep`, `SLOPE` como `t' = t^p`) y las 11 curvas operativas (01–11) más el slot 12 `Custom` que delega a `01 Classic`, según DSP_SPEC. **Aceptar:** extremos, valores finitos, límites y puntos esperados cubiertos por tests.
- [ ] **T-04 Ganancia y parámetros.** Implementar la ganancia con `MIX` (sin Depth, decisión cerrada); layout APVTS de 9 parámetros. **Aceptar:** identidad en cero, fórmula y automatización verificadas.
- [ ] **T-05 Fase matemática.** PPQ, BPM, subdivisiones, 1 Bar y compases no 4/4. **Aceptar:** tests 3/4, 6/8, posiciones negativas, tasas y tamaños de bloque.
- [ ] **T-06 Transporte.** Host playing/stopped/unknown, seek, loop y reloj interno explícito. **Aceptar:** pruebas deterministas y salida segura sin datos.
- [ ] **T-07 Suavizado.** Rampas para cambios de curva, ganancia, bypass y wrap, con suavizado para minimizar discontinuidades audibles. **Aceptar:** mediciones y escucha en múltiples BPM/sample rates; no prometer cero clics sin pruebas.
- [ ] **T-08 Estado y automatización.** Guardado/restauración APVTS e IDs. **Aceptar:** reabrir proyecto reproduce parámetros; automatización no bloquea callback.
- [x] **T-09 Wireframe e identidad.** Distribución, tokens y estados aprobados (`DESIGN.md` + PDF de rediseño). **Aceptar:** `DESIGN.md` actualizado.
- [ ] **T-10 UI funcional.** Controles, curva efectiva y fase atómica. **Aceptar:** vista coincide con DSP, sin acoplamiento UI/audio y funciona a DPI alto.
- [ ] **T-11 Pruebas integradas.** Validar bloques variables, canales, transporte, offline y casos límite. **Aceptar:** informe reproducible de tests y fallos resueltos o explícitos.
- [ ] **T-12 Validación DAW.** pluginval strictness 5 y pruebas manuales en al menos dos hosts disponibles. **Aceptar:** logs y matriz de compatibilidad reales.
- [ ] **T-13 Rendimiento.** Medir CPU/memoria en equipo de referencia y comparar tamaños de bloque. **Aceptar:** métricas documentadas; sin calificativos de rendimiento sin evidencia.
- [ ] **T-14 Auditoría de publicación.** Licencia JUCE, dependencias, assets, marca, empaquetado e instrucciones. **Aceptar:** checklist legal/técnico cerrado antes de distribuir.

## Reglas de aceptación transversales
No marcar tarea por mera generación de código. Guardar comandos y resultados, revisar diff, documentar decisiones, no ampliar alcance sin aprobar PRD. El editor de curvas, MIDI, sidechain externo y macOS quedan fuera de este backlog MVP.

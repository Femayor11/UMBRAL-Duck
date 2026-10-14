# Investigación y referencias — UMBRAL Duck
**Fecha de consulta:** 2026-10-13 · **Estado:** investigación documental, sin pruebas A/B de plugins ni entrevistas. **Dirección aprobada:** opción B: rapidez de Kickstart con personalización progresiva. Este documento recoge hechos publicados por fabricantes por separado de nuestras decisiones de diseño.

## 1. Pregunta de investigación
¿Cómo ofrecer ducking rítmico gratuito, inmediato y editable sin replicar la identidad ni los recursos propietarios de terceros? Usuario objetivo: productor que necesita espacio entre kick y bajo o bombeo rítmico en sintetizadores, pads y buses.

## 2. Comparación documentada
| Producto | Hechos documentados por el fabricante | Aprendizaje para UMBRAL | Evitar en MVP | Fuente |
|---|---|---|---|---|
| Kickstart 2 (Cableguys/Nicky Romero) | 16 curvas, Mix, Slope, longitudes 1/8, 1/4, 1/2 y 1 bar, disparo Sync/MIDI/Audio, Band Split, Shift; Sync sin latencia declarada | Resultado musical en pocos pasos, controles visibles, curva como protagonista | Copiar curvas, nombres, identidad o prometer paridad de disparos | https://downloads.cableguys.com/Nicky-Romero-Kickstart-2-Manual.pdf?v=2.0.8 |
| LFOTool (Xfer) | Editor de puntos/tensión, múltiples gráficos, presets y sincronía BPM | Editor visual legible y reutilización de motor de curvas | Múltiples destinos de modulación y complejidad de enrutamiento | https://xferrecords.com/products/lfo-tool |
| Duck 2 (Devious Machines) | Envolventes multisegmento, Repeat/MIDI/Sidechain/Input, multibanda y edición avanzada | Feedback claro del editor y control sobre la envolvente | Multibanda, disparos externos y herramientas avanzadas prematuras | https://deviousmachines.com/product/duck2/ ; https://deviousmachines.com/manuals/duck2/html/editing-envelopes.html |
| STFU (ZEEK) | Herramienta gratuita de volume shaping, sync a BPM, MIDI trigger, dos envolventes con crossfade | Existe competencia gratuita real; editable por sí solo no es diferenciador suficiente | Afirmar que UMBRAL es el primer plugin gratuito de este tipo | https://zeeks.app/ |
| TAL-Filter-2 (TAL) | Módulo sincronizado con filtro, panorama y modulación de volumen; página oficial documenta restricciones de demo | Mantener foco en ducking, sin convertir el plugin en una suite de modulación | Clasificarlo sin reservas como freeware sin restricciones | https://tal-software.com/products/tal-filter |

**Límite de la evidencia:** los hechos anteriores provienen de documentación comercial; no son mediciones comparativas de CPU, calidad de sonido, compatibilidad ni usabilidad. No hay base para declarar que UMBRAL superará a estos productos.

## 3. Decisiones de producto derivadas (propuestas propias)
1. **Promesa:** abrir, elegir curva, ajustar intensidad y obtener ducking en segundos. La interfaz mostrará una curva grande y controles esenciales.
2. **MVP real:** motor de curvas original, presets originales, Depth/Mix con semántica documentada, divisiones musicales, VST3 Windows, sincronía de host, guardado de estado y pruebas. No prometer latencia cero sin medición.
3. **Personalización:** arquitectura y representación de curvas por puntos desde el núcleo si resulta viable; editor interactivo de puntos y presets guardados por usuario en etapa 2, después de validar el MVP. Esta secuencia respeta la opción B sin disparar el alcance inicial.
4. **Diferenciación buscada (hipótesis, no hecho comprobado):** interfaz menos cargada que las herramientas multipropósito, comportamiento claro y predecible en transporte, documentación abierta de las curvas propias, descarga gratuita sin cuentas ni telemetría.
5. **Identidad:** nombre, iconos, curvas, colores y textos propios; las capturas de terceros son referencias internas, no assets distribuibles.
6. **Fuera del MVP:** audio/MIDI trigger, multibanda, edición avanzada, compatibilidad multiplataforma y funciones de filtro/pan.

## 4. Referencias visuales para `docs/refs/`
Guardar manualmente capturas **solo como referencia interna** (con URL, fecha y nota; no redistribuirlas dentro del plugin):
- `kickstart2-reference`: https://www.cableguys.com/kickstart-2 — jerarquía visual y flujo rápido.
- `lfotool-reference`: https://xferrecords.com/products/lfo-tool — puntos y tensión del editor.
- `duck2-reference`: https://deviousmachines.com/product/duck2/ — visualización y controles progresivos.
- `stfu-reference`: https://zeeks.app/ — alternativa gratuita y comparación de facilidad.
- `tal-filter-reference`: https://tal-software.com/products/tal-filter — diseño de modulador enfocado.
Las URL no implican permiso para reutilizar imágenes, marcas, presets o diseños.

## 5. Referencias técnicas y límites
- JUCE: https://juce.com/get-juce/ ; licencia: https://juce.com/legal/juce-8-licence/ . JUCE 8 distingue licencia comercial Starter y alternativa de código abierto AGPLv3; no confundir con GPLv3. Verificar elegibilidad y condiciones antes de distribuir.
- JUCE API: https://docs.juce.com/ ; JUCE GitHub: https://github.com/juce-framework/JUCE .
- CMake: https://cmake.org/documentation/ .
- pluginval: https://github.com/Tracktion/pluginval .
- VST3 SDK: https://github.com/steinbergmedia/vst3sdk . Revisar las licencias aplicables a la combinación elegida.
- No hay API externa, base de datos, cuentas, servicio en nube ni datos personales previstos para el MVP.

## 6. Riesgos, preguntas abiertas y validaciones
| Asunto | Riesgo | Acción antes de darlo por cerrado |
|---|---|---|
| Mix y Depth | Pueden resultar matemáticamente redundantes si se implementan ambos como mezcla lineal de la misma curva | Definir semántica y test de valores extremos en DSP_SPEC; valorar simplificar UI si no aportan control audible diferente |
| Editor de puntos | Alcance y gestos de interacción crecen rápidamente | Diseñar wireframe y especificación después de validar presets y motor |
| Sincronía | Diferencias de información del host, saltos, loops, offline y compases no 4/4 | Matriz de pruebas y dos hosts reales como mínimo |
| Calidad audible | Clics en discontinuidades y cambios de parámetros | Tests de continuidad, escucha A/B y mediciones a distintas tasas/BPM |
| Licencias | Gratuito no equivale a ausencia de obligaciones | Revisar EULA vigente de JUCE, dependencias, fuentes y assets |
| Compatibilidad | El hecho de compilar no demuestra funcionar en todos los DAW | pluginval y registrar versiones/resultado en hosts concretos |
| Diferenciación | STFU ya cubre parte de la propuesta gratis | Pruebas con 3–5 productores, medir tiempo hasta primer resultado y facilidad de edición |

## 7. Criterios de cierre del apartado 2
- [x] Cinco referencias investigadas con fuentes oficiales y enlaces.
- [x] Qué adoptar/evitar y límites de la evidencia documentados.
- [x] Dirección B traducida a decisiones de producto.
- [x] Riesgos técnicos, legales y de competencia enumerados.
- [ ] Capturas internas seleccionadas y anotadas por el responsable del proyecto.
- [ ] Prueba práctica comparativa y entrevistas de usuario (no realizadas).
- [ ] Verificación final de licencia para la modalidad de publicación escogida.

**Próximo paso recomendado:** cerrar `docs/DESIGN.md` con wireframe de una ventana, estados y flujo de editor futuro; después revisar la semántica de Depth/Mix y el plan de tareas antes de programar.

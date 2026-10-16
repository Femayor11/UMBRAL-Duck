# PRD — UMBRAL Duck
**Versión:** 0.2 · **Estado:** Especificación alineada al sistema de diseño definitivo.

## 1. Problema y usuario
Productores musicales necesitan aplicar ducking rítmico sincronizado al tempo de manera inmediata, visual y gratuita, sin configurar rutas complejas de compresión sidechain.

## 2. Propuesta de valor
Plugin VST3 gratuito, local y ligero con estética oscura de alta precisión (`Umbra`/`Signal`), 11 curvas originales listas para usar (+ slot reservado para edición futura), ajustes de fase (`SHIFT`) e inclinación (`SLOPE`), medidores `IN/OUT` y modo de vista compacta (`560x360`).

## 3. MVP (ocho capacidades cerradas)
1. **Formato y Canales:** VST3 para Windows `x86_64`, con selector `CHANNELS` (`Stereo` / `Mono`) y soporte de buses mono/estéreo.
2. **Sincronía y Reloj (`RATE`, `BPM`, `SYNC`):** Divisiones `1/8`, `1/4`, `1/2` y `1/1`, modo sincronizado al DAW (`SYNC` activo) y modo libre con `BPM` propio (`40.0–240.0`).
3. **Banco de Curvas (`CURVE BANK`):** 12 celdas interactivas (`01 Classic` a `11 Step` operativas en DSP + `12 Custom` reservada con aviso informativo para versión futura).
4. **Modelado y Mezcla:** Controles de `MIX` (`0–100%`), `SHIFT` (`0–100%`), `SLOPE` (`0–100%`) y `BYPASS` suave con suavizado para minimizar discontinuidades audibles.
5. **Visualizador `GAIN RESPONSE`:** Gráfica en tiempo real con escala en dB (`0` a `-24 dB`), eje temporal (`0`, `1/4`, `1/2`, `3/4`, `1`), aguja de playhead, readout numérico (`-14.2 dB`) y posición de compás (`1.2.3`).
6. **Medición (`LEVEL`):** Medidores de nivel de entrada (`IN`) y salida (`OUT`) integrados con escala `-6`, `-12`, `-24`, `-48 dB`.
7. **Doble Vista:** Ventana completa (`960 x 600 px`) y vista compacta (`560 x 360 px`) para pantallas pequeñas o sesiones con múltiples plugins abiertos.
8. **Calidad y Estabilidad:** Persistencia de estado en sesiones DAW, seguridad total en el hilo de audio y validación con `pluginval` (strictness 5).

## 4. No-objetivos del MVP
- Editor interactivo de nodos para el slot `12 Custom` (explícitamente pospuesto: *"Custom: editor coming in a later version"*).
- Disparo por notas MIDI o sidechain de audio externo.
- Procesamiento multibanda (Band Split).
- Versiones AU/AAX o compilación oficial para macOS/Linux en esta primera entrega.
- Cuentas de usuario, telemetría o conexión a internet.

## 5. Flujo principal
Insertar VST3 en pista o bus → reproducir DAW con `SYNC` activo → elegir división en `RATE` (`1/8`, `1/4`, `1/2`, `1/1`) y curva en `CURVE BANK` (`01` a `11`) → ajustar `MIX`, `SHIFT` y `SLOPE` → observar la reducción en dB en `GAIN RESPONSE` y los niveles `IN/OUT` → guardar sesión y reabrir obteniendo el mismo estado audible y visual.

## 6. Criterios de éxito
- La salida coincide con la fórmula de ganancia definida en `docs/DSP_SPEC.md` y es finita, sin clipping añadido.
- Cero asignaciones dinámicas, I/O ni bloqueos en `processBlock`.
- Reproducibilidad al cambiar tamaño de bloque y en render offline.
- Guardado/restauración correctos de los 9 parámetros y del modo de ventana (`960x600` / `560x360`).
- Validación `pluginval` strictness 5 y pruebas manuales en al menos dos DAW disponibles.

## 7. Historias de usuario clave
- Como productor, abro UMBRAL Duck, elijo `1/4` y la curva `03 Smooth`, ajusto `MIX` al `70%` y escucho el bombeo sincronizado mientras veo la atenuación exacta en dB (`-14.2 dB`).
- Como productor, ajusto `SHIFT` y `SLOPE` para adaptar el bolsillo rítmico de la curva al groove de mi batería sin tener que dibujar nodos uno por uno.
- Como productor trabajando en una laptop, cambio a la **Vista Compacta (`560x360`)** para conservar en pantalla solo la gráfica, `RATE`, `SYNC`, selector de curva y `MIX`.
- Como productor en un host detenido o sin datos de transporte, desactivo `SYNC` y ajusto el `BPM` manualmente para seguir diseñando sonido.
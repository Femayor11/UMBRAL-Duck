# DESIGN — UMBRAL Duck
**Estado:** Sistema de diseño y wireframes aprobados (alineado a `UMBRAL Duck — Rediseño UI.pdf`).

## 1. Principios visuales
Comprensión inmediata, estética técnica oscura de alto contraste, un único color de acento (`Signal`), cero sombras/resplandores (`none`) y cero degradados en controles (`none`). Coherencia total entre la curva dibujada y la ganancia real del DSP.

## 2. Dimensiones y Modos de Ventana
El plugin soporta dos modos de vista conmutables sin alterar el procesamiento de audio:
- **Vista Completa (Window):** `960 x 600 px`. Muestra cabecera, panel principal `GAIN RESPONSE`, controles de `SHIFT`, `SLOPE`, `CURVE BANK` (12 celdas), `CHANNELS` (`Stereo`/`Mono`) y medidores `LEVEL` (`IN`/`OUT`).
- **Vista Compacta (Compact):** `560 x 368 px`. Muestra cabecera compacta (`RATE`, `BPM`, `SYNC`), visor `GAIN RESPONSE` con readout en dB y pie reducido con selector de curva (`03 Smooth`) y knob `MIX`.

## 3. Tokens de Color
| Token | Hex | Uso | Contraste / Nota |
|---|---|---|---|
| `Umbra` | `#131311` | Fondo principal de la ventana (`window`) | Base oscura |
| `Panel` | `#1A1A17` | Módulos y contenedores (`modules`) | Superficie secundaria |
| `Raised` | `#22221E` | Perillas, campos interactivos y botones inactivos (`knobs, fields`) | Superficie elevada |
| `Hairline` | `#333220` | Bordes de 1 px y líneas divisoras (`borders`) | Delimitación sutil |
| `Muted` | `#8C8A7E` | Etiquetas secundarias y unidades (`labels`) | Ratio 5.8:1 |
| `Bone` | `#ECE8DC` | Texto principal y valores activos (`text`) | Ratio 15.2:1 |
| `Signal` | `#C9F04B` | Único acento activo: curva, estado activo, foco (`the only accent`) | Verde lima de alta visibilidad |

*Alternativas de acento documentadas para Tweaks futuros (no activas en MVP): `#F2C94C`, `#EBE4D4`, `#FF6857`.*

## 4. Tipografía
Fuentes con licencia abierta (OFL) embebidas en el binario:
- **UI y Etiquetas (`Archivo`):**
  - Marca `UMBRAL`: `19 px`, peso `800 / 300`, tracking `+0.04em`.
  - Encabezados de sección (ej. `CURVE BANK`, `GAIN RESPONSE`): `10 px`, peso `600`, mayúsculas (`caps`), tracking `+0.12em`.
  - Nombres de curvas activas (ej. `Smooth`): `13 px`, peso `600`.
  - Botones de modo (ej. `Stereo`): `11 px`, peso `600`.
- **Números y Lecturas (`IBM Plex Mono`):**
  - Readout principal de ganancia (ej. `-14.2 dB`): `28–32 px` (jerarquía principal).
  - Valor de BPM (ej. `120.0`): `20 px`, peso `400`.
  - Subdivisiones y valores medios (ej. `1/4`): `13 px`, peso `400 / 500`.
  - Indicador de posición de compás (ej. `1.2.3`): `11 px`, peso `400`.
  - Escalas de ejes (`0`, `-6`, `-12`, `-18`, `-24`): `10 px`, peso `400`.

## 5. Medidas y Geometría
- `Radius`: `4 px` en paneles, botones y celdas.
- `Border`: `1 px` sólido color `Hairline` (`#333220`).
- `Window padding`: `20 px`.
- `Panel gap`: `20 px`.
- `Panel padding`: `14 px`.
- `Control height`: `32 px` / `36 px`.
- `Knob sweep`: `270°`.
- `Curve stroke`: `2.5 px` en color `Signal` (`#C9F04B`), con relleno semitransparente bajo la atenuación hacia `0 dB`.
- `Shadows / glow`: `none`.
- `Gradients`: `none`.

## 6. Estados de Componentes
- **Rate Button (`1/8`, `1/4`, `1/2`, `1/1`):**
  - `Normal`: fondo `Raised`, borde `Hairline`, texto `Bone`.
  - `Hover`: borde iluminado suave, texto `Bone`.
  - `Active`: fondo sólido `Signal` (`#C9F04B`), texto oscuro `#131311`.
  - `Focus`: anillo exterior de `1 px` en color `Signal`.
  - `Disabled`: opacidad reducida, texto `Muted`.
- **Sync Toggle (`SYNC`):**
  - `Free (own BPM)`: fondo `Raised`, texto `Muted`, campo numérico de `BPM` editable manualmente.
  - `Locked to DAW`: borde en `Signal`, texto en `Signal`, `BPM` gobernado por el host.
- **Curve Tile (`01` a `12`):**
  - `Default`: fondo `Raised`, miniatura de curva en `Muted`, borde `Hairline`.
  - `Hover`: borde más claro, curva en `Bone`.
  - `Selected`: borde en `Signal`, miniatura de curva en `Signal`, fondo oscurecido.
  - `Slot 12 (Custom)`: muestra etiqueta *"Custom: editor coming in a later version"* al seleccionarse o pasar el cursor; no abre editor en v1.

## 7. Visualizador (`GAIN RESPONSE`) y Medidores (`LEVEL`)
- **Eje Y:** Escala en decibelios con marcas en `0`, `-6`, `-12`, `-18` y `-24 dB`.
- **Eje X:** Ciclo normalizado con marcas en `0`, `1/4`, `1/2`, `3/4` y `1`.
- **Playhead:** Línea vertical blanca/hueso de `1 px` que recorre la fase actual `[0, 1)` mediante lectura de snapshot atómico a 60 Hz, más lectura de posición musical (`1.2.3`) y reducción instantánea en dB (`-14.2 dB`).
- **Level Meters (`IN` / `OUT`):** Medidores verticales de pico/RMS con marcas en `-6`, `-12`, `-24`, `-48 dB`, alimentados mediante variables atómicas libres de bloqueo desde el procesador.
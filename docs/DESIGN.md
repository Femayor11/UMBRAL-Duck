# DESIGN — UMBRAL Duck
**Estado:** Sistema de diseño y especificaciones visuales de referencia aprobadas (fuente de verdad absoluta para T-09, T-10 y T-11, extraída de `docs/refs/Sistema de diseño@1x.png`, `Plugin - ventana principal@2x.png` y `Plugin - vista compacta@2x.png`).

## 1. Principios visuales
- **Comprensión inmediata:** Interfaz técnica de audio orientada a precisión rítmica.
- **Estética técnica oscura:** Fondo de alto contraste, superficies planas jerarquizadas sin sombras (`none`) ni degradados en controles (`none`).
- **Acento único de alta visibilidad:** Un único color de acento (`Signal` `#C9F04B`), reservado exclusivamente para elementos activos, visualización de reducción activa, foco y medidor de salida procesada.
- **Coherencia matemática estricta:** Correspondencia 1:1 entre el trazo de la curva visualizada y la ganancia aplicada en el hilo de audio del DSP.

---

## 2. Dimensiones y Modos de Ventana
El plugin soporta dos vistas conmutables sin reinicializar el estado de audio ni alterar la coherencia de parámetros:
- **Vista Completa (Window):** `960 x 600 px`.
  - Muestra la cabecera completa (`Logo`, `RATE`, `BPM`, `SYNC`, `BYPASS`, `Menú`), 3 paneles superiores (`CURVE/KNOBS`, `GAIN RESPONSE`, `CHANNELS/LEVEL`) y panel inferior `CURVE BANK` con las 12 celdas.
- **Vista Compacta (Compact):** `560 x 360 px`.
  - Cabecera simplificada (`Logo`, `RATE`, `BPM`, `SYNC`, `Menú` — sin etiquetas de texto ni botón `BYPASS`), panel principal `GAIN RESPONSE` y panel lateral con selector `CURVE` y perilla grande `MIX`.
  - Oculta `SHIFT`, `SLOPE`, selector `CHANNELS`, medidores `LEVEL` y el `CURVE BANK` para uso enfocado en directos o pantallas reducidas.

---

## 3. Tokens de Color Oficiales
| Token | Hex | Uso en UI | Contraste / Nota |
|---|---|---|---|
| `Umbra` | `#131311` | Fondo general de la ventana (`window`); texto sobre fondos activos `Signal` | Base más profunda |
| `Panel` | `#1A1A17` | Superficie de paneles contenedores y módulos (`modules`) | Nivel 1 de elevación |
| `Raised` | `#22221E` | Superficie de botones inactivos, perillas, campos numéricos y fondos de medidor | Nivel 2 de elevación |
| `Hairline` | `#33322C` | Bordes exteriores de 1 px y líneas divisorias internas (`borders`) | Delimitación técnica sutil |
| `Muted` | `#8C8A7E` | Etiquetas secundarias, ejes, unidades y elementos inactivos (`labels`) | Ratio 5.3:1 (accesible) |
| `Bone` | `#ECE8DC` | Texto principal, agujas de perillas, playhead y barras de medidor `IN` (`text`) | Ratio 15.2:1 (máximo contraste) |
| `Signal` | `#C9F04B` | Único color de acento: curva activa, readout dB, botones activos, borde de selección y barras `OUT` | Verde lima eléctrico |

### Colores específicos de medidores (`LEVEL`)
- **Canal de Entrada (`IN`):** Barras activas en color `Bone` (`#ECE8DC`), carril de fondo en `Raised` (`#22221E`).
- **Canal de Salida (`OUT`):** Barras activas en color `Signal` (`#C9F04B`), carril de fondo en `Raised` (`#22221E`).
- **Escala central dB:** Texto en `Muted` (`#8C8A7E`).

### Alternativas de acento (Tweaks > Color — paleta documentada para versiones posteriores)
- `Signal (Default)`: `#C9F04B`
- `Amber`: `#F2C94C`
- `Warm Bone`: `#E8E4D4`
- `Coral`: `#FF6B57`

---

## 4. Tipografía y Jerarquía
Tipografías con licencia de código abierto (SIL Open Font License) integradas como binarios en el plugin:

### A. Archivo (Etiquetas, Títulos y Marca)
- **Logotipo `UMBRAL duck`:** `19 px`, tracking `+0.04em`. Palabra `UMBRAL` en peso `800` (ExtraBold), palabra `duck` en peso `300` (Light), color `Bone`.
- **Encabezados de sección (`RATE`, `BPM`, `CURVE`, `MIX`, `SHIFT`, `SLOPE`, `CHANNELS`, `LEVEL`, `CURVE BANK`, `GAIN RESPONSE`):** `10 px`, peso `600` (SemiBold), mayúsculas (`ALL CAPS`), tracking `+0.12em`, color `Muted`.
- **Nombres de curvas activas (`Classic`, `Smooth`, etc.):** `11–13 px`, peso `600`, color `Bone`.
- **Botones de modo y canales (`Stereo`, `Mono`, `BYPASS`, `SYNC`):** `11 px`, peso `600`.
- **Etiquetas de canal de medidores (`IN`, `OUT`):** `11 px`, peso `700` (Bold), mayúsculas, color `Bone`.

### B. IBM Plex Mono (Números, Métricas y Valores Técnicos)
- **Lectura principal de knob `MIX`:** `20–22 px`, peso `500` / `600`, color `Bone` (ej. `70%`).
- **Lectura de Tempo (`BPM`):** `20 px`, peso `400`, color `Bone` (ej. `120.0`).
- **Reducción instantánea en cabecera de `GAIN RESPONSE`:** `13–14 px`, peso `500` / `600`, color `Signal` (`#C9F04B`) (ej. `-14.2 dB`).
- **Subdivisiones rítmicas (`Rate`):** `13 px`, peso `400` / `500` (ej. `1/8`, `1/4`, `1/2`, `1/1`).
- **Valores secundarios de perillas (`SHIFT`, `SLOPE`):** `11–13 px`, peso `400`, color `Bone` (ej. `0%`, `50%`).
- **Posición musical del compás (Pastilla flotante `Playhead`):** `11 px`, peso `400`, color `Bone` (ej. `1.2.3`).
- **Texto informativo en cabecera de Curve Bank:** `11 px`, peso `400`, color `Muted` (`12 — Custom: editor coming in a later version`).
- **Números de ranura en Curve Bank (`01` a `12`):** `10 px`, peso `400`, color `Muted` (color `Signal` si la celda está seleccionada).
- **Escalas de decibelios y tiempo (Ejes y Medidores):** `10 px`, peso `400`, color `Muted` (`0`, `-6`, `-12`, `-18`, `-24`, `-48`, `0`, `¼`, `½`, `¾`, `1`).

---

## 5. Medidas, Cuadrícula y Espaciado (Layout)

### Medidas Generales
- `Radius`: `4 px` en esquinas de paneles contenedores, botones, cajas de texto y celdas de curva.
- `Border`: `1 px` sólido color `Hairline` (`#33322C`).
- `Shadows / glow`: `none` (estrictamente prohibido añadir sombras proyectadas o resplandores exteriores).
- `Gradients`: `none` (todas las superficies, rellenos de botones y trazos son de color sólido).
- `Knob sweep`: `270°` (arco de rotación estándar desde 135° hasta 405° / -135° a +135°).
- `Curve stroke`: `2.5 px` de grosor continuo en color `Signal` (`#C9F04B`).
- `Curve fill`: Relleno vertical entre la curva y la línea de `0 dB` con color `Signal` y factor alpha `0.15` (`signal.withAlpha(0.15f)`).
- `Playhead width`: `1 px` vertical continuo en color `Bone` (`#ECE8DC`).

### Cuadrícula en Vista Completa (`960 x 600 px`)
- `Window padding`: `20 px` exterior en los 4 bordes.
- `Panel gap`: `20 px` entre módulos adyacentes.
- `Panel padding`: `14 px` interior.
- `Header height`: ~`56 px` de zona de controles (separada del cuerpo mediante una línea divisoria horizontal `1 px Hairline` a `y ≈ 76 px`).
- **Fila Media de Paneles (altura: `368 px`, de `y = 77 px` a `445 px`):**
  - **Panel Izquierdo (`CURVE & KNOBS`):** Ancho `193 px`. Dividido internamente en 3 niveles por separadores `1 px Hairline`:
    1. Desplegable `CURVE` (altura de control `36 px`).
    2. Perilla grande de `MIX` (diámetro ~`80 px`) + lectura `70%`.
    3. Perillas dobles `SHIFT` y `SLOPE` (diámetro ~`44 px`) + lecturas `0%` y `50%`.
  - **Panel Central (`GAIN RESPONSE`):** Ancho `534 px`.
    - Cabecera: `GAIN RESPONSE` a la izquierda, `-14.2 dB` a la derecha en `Signal`.
    - Gráfico: Retícula de líneas tenues en decibelios (`0`, `-6`, `-12`, `-18`, `-24`) y fases rítmicas (`0`, `¼`, `½`, `¾`, `1`).
    - Pastilla flotante superior de compás: Caja `Raised` de `1.2.3` anclada sobre el playhead.
  - **Panel Derecho (`CHANNELS & LEVEL`):** Ancho `153 px`. Dividido internamente en 2 niveles por separador `1 px Hairline`:
    1. Selector `CHANNELS` con botones segmentados `Stereo` y `Mono` (altura `32 px`).
    2. Medidores estéreo verticales `LEVEL`: par `IN` (`Bone`) y par `OUT` (`Signal`) flanqueando la escala numérica central.
- **Fila Inferior (`CURVE BANK`):** Ancho total `920 px`, altura ~`114 px` (de `y = 466 px` a `580 px`).
  - Cabecera con título `CURVE BANK` y nota informativa sobre la celda 12 `Custom`.
  - Matriz de 12 celdas rítmicas en hilera horizontal con separación de `~8 px` entre celdas (altura de celda `~82 px`, ancho `~69 px`).

### Cuadrícula en Vista Compacta (`560 x 360 px`)
- `Window padding`: `16 px`.
- `Panel gap`: `16 px`.
- `Header height`: ~`48 px` (separada por línea `1 px Hairline` a `y = 48 px`).
- **Paneles (`y = 65 px` a `344 px`, altura `279 px`):**
  - **Panel Izquierdo (`GAIN RESPONSE`):** Ancho `350 px`. Muestra cabecera (`GAIN RESPONSE` y `-14.2 dB` en `Signal`), ejes `0` a `-24 dB` y fases `0` a `1`, playhead de 1 px y curva activa con relleno. Oculta la pastilla flotante `1.2.3`.
  - **Panel Derecho (`CURVE & MIX`):** Ancho `160 px`.
    1. Sección superior: encabezado `CURVE` y selector desplegable `03 Smooth`.
    2. Línea divisoria horizontal `1 px Hairline`.
    3. Sección inferior: encabezado `MIX`, perilla grande `MIX` y lectura numérica `70%`.

---

## 6. Estados Visuales de Componentes

### Botones de Subdivisión Rítmica (`RATE`: `1/8`, `1/4`, `1/2`, `1/1`)
- **Normal (Inactivo):** Fondo `Raised` (`#22221E`), borde `Hairline` (`#33322C`), texto `Bone` (`#ECE8DC`).
- **Hover:** Fondo `Raised`, borde con luminancia intermedia (mezcla 50% `Hairline` + 50% `Muted` o `Bone` al 35%), texto `Bone`.
- **Active (Seleccionado):** Fondo sólido `Signal` (`#C9F04B`), texto `Umbra` (`#131311`), sin borde adicional.
- **Focus:** Borde perimetral exterior de `1 px` en color `Signal` (`#C9F04B`).
- **Disabled:** Opacidad reducida al `40%` (`0.40`), texto en `Muted` (`#8C8A7E`).

### Selector de Sincronización (`SYNC`)
- **Free (BPM Propio):** Fondo `Raised`, borde `Hairline`, texto `Muted`. Campo numérico `BPM` habilitado para edición manual directa.
- **Locked to DAW (Sincronizado):** Fondo `Raised`, borde `1 px Signal` (`#C9F04B`), texto `Signal` (`#C9F04B`). Campo `BPM` bloqueado en modo de solo lectura gobernado por el transporte del host.

### Selector de Canales (`Stereo` / `Mono`)
- Control segmentado de 2 opciones.
- Opción activa: Fondo sólido `Signal` (`#C9F04B`), texto `Umbra` (`#131311`).
- Opción inactiva: Fondo `Raised` (`#22221E`), texto `Bone` (`#ECE8DC`).

### Celdas de Curva en `CURVE BANK` (`01` a `12`)
- **Default (Inactivo):** Superficie `Raised`, borde `1 px Hairline`, miniatura de curva en trazo `Muted`, número `Muted`, nombre `Bone`.
- **Hover:** Borde más claro (mezcla Hairline/Bone), miniatura de curva en trazo `Bone`, nombre `Bone`.
- **Selected (Activo):** Borde `1 px Signal` (`#C9F04B`), miniatura de curva en trazo `Signal` (`#C9F04B`), número de ranura en `Signal`, nombre de curva en `Bone`.
- **Ranura 12 (`Custom`):** Miniatura dibujada con línea discontinua (`dashed stroke`). Al seleccionarse, no abre ventana modal ni editor en la v1; opera bajo la curva de fallback `01 Classic` según la especificación DSP.

### Desplegable de Curva (`CURVE Dropdown`)
- Fondo `Raised`, borde `Hairline`, radio `4 px`.
- Contenido en fila: icono miniatura en `Signal`, número de dos dígitos en `Muted`, nombre de curva en `Bone` y chevron hacia abajo (`v`) en `Muted`.

### Perillas Giratorias (`Knobs`)
- **Pista de fondo:** Arco de `270°` en color `Hairline` (`#33322C`), grosor de trazo `2.5 px`.
- **Arco de valor:** Trazo circular continuo de `2.5 px` en color `Signal` (`#C9F04B`) proporcional al valor normalizado `[0.0, 1.0]`.
- **Cuerpo central:** Círculo sólido plano en color `Raised` (`#22221E`).
- **Aguja indicadora:** Línea radial de `1.5 px` en color `Bone` (`#ECE8DC`) que apunta hacia el valor angular actual.

---

## 7. Directrices de Implementación para T-10 (UI Funcional)
1. **Separación estricta de responsabilidades:**
   - Todo dibujo en `paint()` debe emplear exclusivamente las constantes de `Source/Common/DesignTokens.h`.
   - Queda prohibido escribir colores hex literales, números mágicos de padding o fuentes con tamaños arbitrarios en los componentes UI.
2. **Coordinación con el Audio Thread:**
   - La posición del playhead, valores de gain response y niveles peak de medidores `IN`/`OUT` deben consumirse a 60 Hz mediante variables atómicas (`std::atomic<float>`) libres de bloqueos procedentes de `UmbralDuckProcessor`.
3. **Escalabilidad y Alta Densidad (HiDPI):**
   - El dibujo vectorial de curvas, rejillas y perillas debe realizarse mediante primitivas de punto flotante (`juce::Path`, `juce::Graphics::strokePath`) respetando `scaleFactor` del entorno sin distorsiones ni artefactos de pixelado.
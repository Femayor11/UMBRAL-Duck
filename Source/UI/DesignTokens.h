#pragma once

#include <juce_graphics/juce_graphics.h>

// Fuente única de tokens visuales aprobados en docs/DESIGN.md.
// Prohibido usar colores hexadecimales o tamaños de fuente sueltos en paint().
namespace umbral::tokens
{
    // --- Paleta Oficial de Colores (docs/DESIGN.md §3) ---
    inline const juce::Colour umbra    { 0xFF131311 };  // Fondo general de ventana; texto sobre fondo activo Signal
    inline const juce::Colour panel    { 0xFF1A1A17 };  // Superficie de paneles contenedores y módulos
    inline const juce::Colour raised   { 0xFF22221E };  // Superficie de botones inactivos, perillas, campos y medidores
    inline const juce::Colour hairline { 0xFF33322C };  // Bordes exteriores de 1 px y líneas divisorias internas
    inline const juce::Colour muted    { 0xFF8C8A7E };  // Etiquetas secundarias, ejes, unidades y elementos inactivos
    inline const juce::Colour bone     { 0xFFECE8DC };  // Texto principal, agujas de perillas, playhead y barras IN
    inline const juce::Colour signal   { 0xFFC9F04B };  // Único acento: curva activa, readout dB, botones activos y barras OUT

    // --- Opacidades y Colores Derivados ---
    inline constexpr float disabledOpacity = 0.40f;  // Estado Disabled
    inline constexpr float hoverBoneAlpha  = 0.35f;  // Borde hover alternativo: Bone con alpha
    inline constexpr float curveFillAlpha  = 0.15f;  // Relleno vertical bajo la curva: Signal con alpha 0.15

    // Borde hover: mezcla 50% Hairline + 50% Muted
    inline juce::Colour hoverBorder() noexcept { return hairline.interpolatedWith (muted, 0.5f); }
    inline juce::Colour curveFill() noexcept   { return signal.withAlpha (curveFillAlpha); }

    // --- Geometría y Layout (px) (docs/DESIGN.md §2 y §5) ---
    inline constexpr int   windowFullWidth      = 960;
    inline constexpr int   windowFullHeight     = 600;
    inline constexpr int   windowCompactWidth   = 560;
    inline constexpr int   windowCompactHeight  = 360;
    inline constexpr float radius               = 4.0f;
    inline constexpr float borderWidth          = 1.0f;
    inline constexpr int   windowPadding        = 20;
    inline constexpr int   windowPaddingCompact = 16;
    inline constexpr int   panelGap             = 20;
    inline constexpr int   panelGapCompact      = 16;
    inline constexpr int   panelPadding         = 14;
    inline constexpr int   controlHeightSmall   = 32;
    inline constexpr int   controlHeightLarge   = 36;
    inline constexpr float knobSweepDegrees     = 270.0f;
    inline constexpr float knobTrackStroke      = 2.5f;
    inline constexpr float knobNeedleStroke     = 1.5f;
    inline constexpr float curveStroke          = 2.5f;
    inline constexpr float playheadWidth        = 1.0f;

    // --- Layout vista Full (docs/DESIGN.md §5) ---
    inline constexpr int   fullHeaderDividerY   = 76;    // Linea divisoria horizontal Hairline en Full
    inline constexpr int   fullContentTop       = 77;    // Inicio de la fila media de paneles
    inline constexpr int   fullPanelRowHeight   = 368;   // Altura de la fila media (y = 77 .. 445)
    inline constexpr int   fullLeftPanelWidth   = 193;   // CURVE & KNOBS
    inline constexpr int   fullCenterPanelWidth = 534;   // GAIN RESPONSE
    inline constexpr int   fullRightPanelWidth  = 153;   // CHANNELS & LEVEL
    inline constexpr int   fullBottomRowTop     = 466;   // Inicio de la fila inferior CURVE BANK (y = 466 .. 580)
    inline constexpr int   fullBottomRowWidth   = 920;   // Ancho total del CURVE BANK en Full
    inline constexpr int   fullBottomRowHeight  = 114;   // Altura total del CURVE BANK en Full

    // --- Layout vista Compact (docs/DESIGN.md §5) ---
    inline constexpr int   compactHeaderDividerY  = 48;  // Linea divisoria horizontal Hairline en Compact
    inline constexpr int   compactContentTop      = 65;  // Inicio de paneles en Compact
    inline constexpr int   compactPanelRowHeight  = 279; // Altura de paneles en Compact (y = 65 .. 344)
    inline constexpr int   compactLeftPanelWidth  = 350; // GAIN RESPONSE en Compact
    inline constexpr int   compactRightPanelWidth = 160; // CURVE & MIX en Compact
    inline constexpr int   viewToggleWidth        = 72;  // Ancho del boton de alternancia de vista en cabecera
    inline constexpr int   viewToggleHeight       = 28;  // Alto del boton de alternancia de vista en cabecera

    // --- Cabecera (HeaderBar, docs/DESIGN.md §4, §5 y §6) ---
    inline constexpr int   headerLogoIconSize         = 24;
    inline constexpr int   headerLogoTextGap          = 10;
    inline constexpr int   headerLogoWidth            = 168;
    inline constexpr int   headerLogoWidthCompact     = 150;
    inline constexpr int   headerRateLabelWidth       = 38;
    inline constexpr int   headerRateButtonWidth      = 44;
    inline constexpr int   headerRateButtonCompactW   = 38;
    inline constexpr int   headerBpmWidth             = 68;
    inline constexpr int   headerBpmCompactWidth      = 58;
    inline constexpr int   headerBpmLabelHeight       = 12;
    inline constexpr int   headerBpmValueHeight       = 24;
    inline constexpr int   headerSyncButtonWidth      = 54;
    inline constexpr int   headerBypassButtonWidth    = 84;
    inline constexpr int   headerControlGap           = 8;

    // --- Banco de Curvas (CurveBankGrid, docs/DESIGN.md §5 y §6) ---
    inline constexpr int   curveBankHeaderHeight      = 24;  // Cabecera CURVE BANK + nota 12 Custom
    inline constexpr int   curveBankCellGap           = 8;   // Separacion horizontal entre las 12 celdas
    inline constexpr int   curveBankCellHeight        = 82;  // Altura de cada celda (y = 498 .. 580)
    inline constexpr int   curveBankCellPadding       = 7;   // Padding interior de cada celda
    inline constexpr int   curveBankPreviewHeight     = 36;  // Zona superior para la miniatura vectorial
    inline constexpr int   curveBankNumberHeight      = 13;  // Fila para el numero de ranura 01..12
    inline constexpr int   curveBankNameHeight        = 16;  // Fila para el nombre de la curva
    inline constexpr float curveBankStrokeWidth       = 1.5f;

    // --- Panel GAIN RESPONSE ---
    inline constexpr int   graphHeaderHeight    = 20;
    inline constexpr int   graphAxisLabelWidth  = 26;    // Ancho de la columna de etiquetas dB
    inline constexpr int   graphAxisLabelHeight = 12;
    inline constexpr int   graphAxisGap         = 6;     // Separacion entre ejes y etiquetas
    inline constexpr int   graphTimeLabelWidth  = 24;
    inline constexpr float graphPillWidth       = 44.0f; // Pastilla "1.2.3" sobre el playhead
    inline constexpr float graphPillHeight      = 18.0f;

    // --- Panel LEVEL & CHANNELS ---
    inline constexpr float meterMinDb           = -48.0f;  // Base de la escala de los medidores
    inline constexpr int   meterHeaderHeight    = 20;
    inline constexpr int   meterLabelHeight     = 16;      // Fila de etiquetas IN / OUT
    inline constexpr int   meterBarWidth        = 14;
    inline constexpr int   meterScaleWidth      = 34;      // Columna central de escala dB
    inline constexpr int   channelsSectionHeight = 82;     // Altura del bloque superior CHANNELS en panel derecho

    // --- Panel CURVE & KNOBS (ControlStrip) ---
    inline constexpr int   curveSectionHeight    = 82;     // Altura del bloque superior CURVE (Full y Compact)
    inline constexpr int   mixSectionHeightFull  = 166;    // Altura del bloque central MIX en vista Full
    inline constexpr int   knobMixDiameter       = 80;     // Diametro de perilla principal MIX (DESIGN.md §5)
    inline constexpr int   knobSecondaryDiameter = 44;     // Diametro de perillas SHIFT y SLOPE (DESIGN.md §5)
    inline constexpr int   knobHeaderHeight      = 16;     // Altura de fila de encabezado de seccion
    inline constexpr int   mixReadoutHeight      = 24;     // Altura de lectura numerica de MIX (21 px)
    inline constexpr int   secondaryReadoutHeight = 16;    // Altura de lectura numerica de SHIFT / SLOPE (13 px)
    inline constexpr int   segmentedButtonGap    = 4;      // Separacion entre botones Stereo y Mono

    // --- Jerarquía Tipográfica (px) (docs/DESIGN.md §4) ---
    inline constexpr float brandSize            = 19.0f;  // Archivo 800/300, tracking +0.04em
    inline constexpr float sectionHeaderSize    = 10.0f;  // Archivo 600 ALL CAPS, tracking +0.12em
    inline constexpr float curveNameSize        = 13.0f;  // Archivo 600 (11-13 px)
    inline constexpr float modeButtonSize       = 11.0f;  // Archivo 600 (Stereo, Mono, BYPASS, SYNC)
    inline constexpr float meterLabelSize       = 11.0f;  // Archivo 700 (IN, OUT)
    inline constexpr float mixReadoutSize       = 21.0f;  // IBM Plex Mono 500/600 (20-22 px)
    inline constexpr float bpmValueSize         = 20.0f;  // IBM Plex Mono 400
    inline constexpr float gainReadoutSize      = 14.0f;  // IBM Plex Mono 500/600 (13-14 px, ej. -14.2 dB)
    inline constexpr float midValueSize         = 13.0f;  // IBM Plex Mono 400/500 (Rate 13 px, Shift/Slope 11-13 px)
    inline constexpr float barPositionSize      = 11.0f;  // IBM Plex Mono 400 (Playhead 1.2.3, nota Custom)
    inline constexpr float axisScaleSize        = 10.0f;  // IBM Plex Mono 400 (Escalas dB/tiempo y slots 01-12)
} // namespace umbral::tokens

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
    inline constexpr int   windowFullWidth      = 780;
    inline constexpr int   windowFullHeight     = 540;
    inline constexpr int   windowCompactWidth   = 560;
    inline constexpr int   windowCompactHeight  = 360;
    inline constexpr float radius               = 4.0f;
    inline constexpr float borderWidth          = 1.0f;
    inline constexpr int   windowPadding        = 16;
    inline constexpr int   windowPaddingCompact = 16;
    inline constexpr int   panelGap             = 14;
    inline constexpr int   panelGapCompact      = 16;
    inline constexpr int   panelPadding         = 14;
    inline constexpr int   controlHeightSmall   = 32;
    inline constexpr int   controlHeightLarge   = 36;
    inline constexpr float knobSweepDegrees     = 270.0f;
    inline constexpr float knobTrackStroke      = 2.5f;
    inline constexpr float knobNeedleStroke     = 1.5f;
    inline constexpr float curveStroke          = 2.5f;
    inline constexpr float playheadWidth        = 1.0f;

    // --- Layout vista Full (780 x 540 px) ---
    inline constexpr int   fullHeaderDividerY   = 56;    // Linea divisoria horizontal Hairline en Full
    inline constexpr int   fullContentTop       = 71;    // Inicio de la fila media de paneles (56 + 1 + 14)
    inline constexpr int   fullPanelRowHeight   = 290;   // Altura de la fila media (y = 71 .. 361)
    inline constexpr int   fullLeftPanelWidth   = 176;   // MIX & SHIFT/SLOPE
    inline constexpr int   fullCenterPanelWidth = 408;   // GAIN RESPONSE
    inline constexpr int   fullRightPanelWidth  = 136;   // CHANNELS & LEVEL
    inline constexpr int   fullBottomRowTop     = 375;   // Inicio de la fila inferior CURVE BANK (y = 375 .. 524)
    inline constexpr int   fullBottomRowWidth   = 748;   // Ancho total del CURVE BANK en Full (780 - 2 * 16)
    inline constexpr int   fullBottomRowHeight  = 149;   // Altura total del CURVE BANK en Full (21 + 2 * 60 + 8)

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

    // --- Banco de Curvas (CurveBankGrid, matriz 2x8 en 748 x 149 px) ---
    inline constexpr int   curveBankHeaderHeight      = 21;  // Cabecera CURVE BANK
    inline constexpr int   curveBankCellGap           = 8;   // Separacion horizontal y vertical entre celdas
    inline constexpr int   curveBankCellHeight        = 60;  // Altura de cada celda en la matriz 2x8
    inline constexpr int   curveBankCellPadding       = 8;   // Padding interior de cada celda
    inline constexpr int   curveBankPreviewHeight     = 44;  // Altura util de la miniatura vectorial
    inline constexpr int   curveBankNumberHeight      = 13;
    inline constexpr int   curveBankNameHeight        = 16;
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

    // --- Panel MIX & KNOBS (ControlStrip) ---
    inline constexpr int   curveSectionHeight    = 82;
    inline constexpr int   mixSectionHeightFull  = 174;    // Altura del bloque superior MIX en vista Full (290 -> 174 + 1 + 115)
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

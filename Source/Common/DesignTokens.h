#pragma once

#include <juce_graphics/juce_graphics.h>

// Fuente única de tokens visuales. Valores de docs/DESIGN.md.
// Prohibido usar hex o tamaños sueltos en paint().
namespace umbral::tokens
{
    // --- Color ---
    inline const juce::Colour umbra    { 0xFF131311 };  // Fondo de ventana; texto sobre Signal activo
    inline const juce::Colour panel    { 0xFF1A1A17 };  // Módulos
    inline const juce::Colour raised   { 0xFF22221E };  // Perillas, campos, botones inactivos
    inline const juce::Colour hairline { 0xFF333220 };  // Bordes 1 px
    inline const juce::Colour muted    { 0xFF8C8A7E };  // Etiquetas secundarias
    inline const juce::Colour bone     { 0xFFECE8DC };  // Texto principal; playhead
    inline const juce::Colour signal   { 0xFFC9F04B };  // Único acento

    // --- Opacidades derivadas ---
    inline constexpr float disabledOpacity   = 0.40f;  // Estado Disabled
    inline constexpr float hoverBoneAlpha    = 0.35f;  // Borde hover: Bone con alpha
    inline constexpr float curveFillAlpha    = 0.15f;  // Relleno bajo la curva: Signal

    // Borde hover: mezcla 50% Hairline + 50% Muted
    inline juce::Colour hoverBorder() { return hairline.interpolatedWith (muted, 0.5f); }
    inline juce::Colour curveFill()   { return signal.withAlpha (curveFillAlpha); }

    // --- Geometría (px) ---
    inline constexpr int   windowFullWidth     = 960;
    inline constexpr int   windowFullHeight    = 600;
    inline constexpr int   windowCompactWidth  = 560;
    inline constexpr int   windowCompactHeight = 368;
    inline constexpr float radius              = 4.0f;
    inline constexpr float borderWidth         = 1.0f;
    inline constexpr int   windowPadding       = 20;
    inline constexpr int   panelGap            = 20;
    inline constexpr int   panelPadding        = 14;
    inline constexpr int   controlHeightSmall  = 32;
    inline constexpr int   controlHeightLarge  = 36;
    inline constexpr float knobSweepDegrees    = 270.0f;
    inline constexpr float curveStroke         = 2.5f;
    inline constexpr float playheadWidth       = 1.0f;

    // --- Tipografía (px) ---
    inline constexpr float brandSize           = 19.0f;  // Archivo 800/300, tracking +0.04em
    inline constexpr float sectionHeaderSize   = 10.0f;  // Archivo 600 caps, tracking +0.12em
    inline constexpr float curveNameSize       = 13.0f;  // Archivo 600
    inline constexpr float modeButtonSize      = 11.0f;  // Archivo 600
    inline constexpr float gainReadoutSize     = 30.0f;  // Plex Mono (rango 28-32)
    inline constexpr float bpmValueSize        = 20.0f;  // Plex Mono 400
    inline constexpr float midValueSize        = 13.0f;  // Plex Mono 400/500
    inline constexpr float barPositionSize     = 11.0f;  // Plex Mono 400
    inline constexpr float axisScaleSize       = 10.0f;  // Plex Mono 400
}

#pragma once

#include <span>

// Banco de 16 curvas musicales (01–16). Funciones puras, noexcept, sin heap.
// No depende de JUCE ni de UI: lo comparten DuckingEngine y GainResponseView.
namespace umbral::dsp
{
    inline constexpr int numCurves         = 16;
    inline constexpr int defaultCurveIndex = 3;  // "04 Classic"
    inline constexpr int customCurveIndex  = 15; // "16 Custom" delega a 3 ("04 Classic")

    enum class SegmentShape : int
    {
        Smoothstep = 0, // s = t'^2 * (3 - 2 * t')
        Linear     = 1, // s = t'
        PowerIn    = 2, // s = (t')^exponent (curva concava / codo marcado)
        PowerOut   = 3  // s = 1 - (1 - t')^exponent (curva convexa / subida inicial rapida)
    };

    struct Node
    {
        float phi;
        float value;
        SegmentShape shape = SegmentShape::Smoothstep;
        float exponent     = 1.0f;
    };

    // Nodos de una curva. curveIndex 0..15 (se acota); 15 devuelve los de 04 Classic.
    [[nodiscard]] std::span<const Node> getNodes (int curveIndex) noexcept;

    // Evalua una lista de nodos ordenados. Asume phi en [0, 1] (se acota con clamp01),
    // por lo que phi = 1 devuelve exactamente el ultimo nodo. Segmentos con
    // dx <= 1e-6 no dividen: saltan al valor del segundo nodo.
    // Salida siempre finita y en [0, 1]. NaN/Inf: phi -> 0, slope -> 0.5.
    [[nodiscard]] float evaluateNodes (std::span<const Node> nodes, float phi, float slope) noexcept;

    // C_slope(frac(phi - shift)). Wrap ciclico del shift y luego evaluateNodes.
    // NaN/Inf: phi/shift -> 0, slope -> 0.5.
    [[nodiscard]] float evaluate (int curveIndex, float phi, float shift, float slope) noexcept;
}

#pragma once

#include <span>

// Banco de curvas 01–12 (DSP_SPEC.md §4). Funciones puras, noexcept, sin heap.
// No depende de JUCE ni de UI: lo comparten DuckingEngine y GainResponseView.
namespace umbral::dsp
{
    inline constexpr int numCurves = 12;
    inline constexpr int customCurveIndex = 11; // "12 Custom" delega a 0 ("01 Classic")

    struct Node
    {
        float phi;
        float value;
    };

    // Nodos de una curva. curveIndex 0..11 (se acota); 11 devuelve los de Classic.
    [[nodiscard]] std::span<const Node> getNodes (int curveIndex) noexcept;

    // Evalúa una lista de nodos ordenados. Asume phi en [0, 1] (se acota con clamp01),
    // por lo que phi = 1 devuelve exactamente el último nodo. Segmentos con
    // dx <= 1e-6 no dividen: saltan al valor del segundo nodo.
    // Salida siempre finita y en [0, 1]. NaN/Inf: phi -> 0, slope -> 0.5.
    [[nodiscard]] float evaluateNodes (std::span<const Node> nodes, float phi, float slope) noexcept;

    // C_slope(frac(phi - shift)). Wrap cíclico del shift y luego evaluateNodes.
    // NaN/Inf: phi/shift -> 0, slope -> 0.5.
    [[nodiscard]] float evaluate (int curveIndex, float phi, float shift, float slope) noexcept;
}

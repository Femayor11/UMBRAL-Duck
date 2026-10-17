#pragma once

#include <algorithm>
#include <cmath>

// Utilidades matemáticas puras y seguras para tiempo real (DSP_SPEC.md §1, §4).
// Sin asignaciones en heap, sin locks, sin dependencias de JUCE ni de UI.
namespace umbral::math
{
    // Umbral de segmento de ancho cero (DSP_SPEC.md §4, regla de dx <= 1e-6).
    inline constexpr float zeroWidthEpsilon = 1.0e-6f;

    // Piso de ganancia para la lectura en dB (-60 dB).
    inline constexpr float gainFloor = 1.0e-3f;

    // Fallbacks seguros ante valores no finitos.
    inline constexpr float fallbackPhase = 0.0f;
    inline constexpr float fallbackShift = 0.0f;
    inline constexpr float fallbackSlope = 0.5f;

    // Reemplaza NaN/Inf por un valor de respaldo.
    [[nodiscard]] inline float finiteOr (float x, float fallback) noexcept
    {
        return std::isfinite (x) ? x : fallback;
    }

    [[nodiscard]] constexpr float clamp01 (float x) noexcept
    {
        return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    }

    [[nodiscard]] constexpr float lerp (float a, float b, float t) noexcept
    {
        return a + (b - a) * t;
    }

    // smoothstep(t) = t^2 (3 - 2t), con t acotado a [0, 1].
    [[nodiscard]] constexpr float smoothstep (float t) noexcept
    {
        const float c = clamp01 (t);
        return c * c * (3.0f - 2.0f * c);
    }

    // frac(x) = x - floor(x), resultado en [0, 1). Soporta negativos.
    [[nodiscard]] inline float frac (float x) noexcept
    {
        if (! std::isfinite (x))
            return 0.0f;

        const float r = x - std::floor (x);
        // Para x negativo muy pequeño, x - floor(x) puede redondear a 1.0f.
        return r >= 1.0f ? 0.0f : r;
    }

    // phi_shifted = max(0.0f, phi - S), sin wrap circular envolvente.
    // Durante [0, S], phi_shifted permanece en 0.0f (nivel base inicial de la curva),
    // comenzando la evolucion de la curva en phi >= S.
    [[nodiscard]] inline float applyShift (float phi, float shift) noexcept
    {
        const float rawPhi    = finiteOr (phi, fallbackPhase);
        const float safePhi   = (rawPhi == 1.0f) ? 1.0f : frac (rawPhi);
        const float safeShift = clamp01 (finiteOr (shift, fallbackShift));
        return clamp01 (safePhi - safeShift);
    }

    // p = 2^((K - 0.5) * 2), con K saneado y acotado a [0, 1].
    [[nodiscard]] inline float slopeExponent (float slope) noexcept
    {
        const float k = clamp01 (finiteOr (slope, fallbackSlope));
        return std::exp2 ((k - 0.5f) * 2.0f);
    }

    // t' = t^p
    [[nodiscard]] inline float applySlope (float t, float slope) noexcept
    {
        return std::pow (clamp01 (finiteOr (t, 0.0f)), slopeExponent (slope));
    }

    // G = (1 - M) + M * C
    [[nodiscard]] inline float gainFromCurve (float curveValue, float mix) noexcept
    {
        const float m = clamp01 (finiteOr (mix, 0.0f));
        const float c = clamp01 (finiteOr (curveValue, 1.0f));
        return (1.0f - m) + m * c;
    }

    // Gain_dB = 20 log10(max(G, 1e-3))
    [[nodiscard]] inline float gainToDb (float gain) noexcept
    {
        return 20.0f * std::log10 (std::max (finiteOr (gain, 1.0f), gainFloor));
    }
}

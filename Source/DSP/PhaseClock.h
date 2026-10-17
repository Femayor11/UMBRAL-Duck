#pragma once

#include <span>

// Reloj matematico de fase ritmica y posicion musical (DSP_SPEC.md §2).
// Modulo 100% puro, noexcept, sin asignaciones en heap ni dependencias de JUCE o UI.
namespace umbral::dsp
{
    // Subdivisiones de RATE (DSP_SPEC.md §2, ARCHITECTURE.md §4).
    enum class RateDivision : int
    {
        Eighth  = 0, // 1/8 = 0.5 negras
        Quarter = 1, // 1/4 = 1.0 negra (default)
        Half    = 2, // 1/2 = 2.0 negras
        OneBar  = 3  // 1/1 = N * (4.0 / D) negras segun compas
    };

    struct TimeSignature
    {
        int numerator   = 4;
        int denominator = 4;
    };

    struct MusicalContext
    {
        double ppqPosition   = 0.0;
        double bpm           = 120.0;
        double sampleRate    = 44100.0;
        TimeSignature timeSig { 4, 4 };
        bool hasBarStartPpq  = false;
        double barStartPpq   = 0.0;
        double phaseOffset   = 0.0;
    };

    // Posicion musical 1-indexada para el readout "1.2.3" (Bar.Beat.Subdivision).
    struct BarBeatTick
    {
        int bar         = 1;
        int beat        = 1;
        int subdivision = 1;

        constexpr bool operator== (const BarBeatTick&) const noexcept = default;
    };

    // Convierte un indice de parametro (0..3) a RateDivision con acotado seguro.
    [[nodiscard]] constexpr RateDivision rateFromIndex (int index) noexcept
    {
        if (index <= 0)
            return RateDivision::Eighth;
        if (index == 1)
            return RateDivision::Quarter;
        if (index == 2)
            return RateDivision::Half;
        return RateDivision::OneBar;
    }

    // Valida el compas N/D; aplica fallback a 4/4 si N <= 0 o D <= 0.
    [[nodiscard]] constexpr TimeSignature sanitizeTimeSignature (TimeSignature ts) noexcept
    {
        if (ts.numerator <= 0 || ts.denominator <= 0)
            return { 4, 4 };
        return ts;
    }

    // Longitud de un compas completo en negras metricas: N * (4.0 / D).
    [[nodiscard]] constexpr double barLengthQuarterNotes (TimeSignature ts) noexcept
    {
        const auto valid = sanitizeTimeSignature (ts);
        return static_cast<double> (valid.numerator) * (4.0 / static_cast<double> (valid.denominator));
    }

    // Longitud del ciclo L en negras metricas para la division RATE seleccionada.
    [[nodiscard]] constexpr double cycleLengthQuarterNotes (RateDivision rate, TimeSignature ts = {}) noexcept
    {
        switch (rate)
        {
            case RateDivision::Eighth:  return 0.5;
            case RateDivision::Quarter: return 1.0;
            case RateDivision::Half:    return 2.0;
            case RateDivision::OneBar:  return barLengthQuarterNotes (ts);
        }
        return 1.0;
    }

    // Avance de fase por muestra: deltaPhi = B / (60 * fs * L).
    // Si bpm o sampleRate son invalidos (<= 0 o NaN/Inf), usa fallbacks seguros (120 BPM, 44100 Hz).
    [[nodiscard]] double computeDeltaPhi (double bpm, double sampleRate,
                                          RateDivision rate, TimeSignature ts = {}) noexcept;

    // Evalua phi[n] = frac((PPQ0 - anchor) / L + n * deltaPhi) en [0, 1).
    // En modo 1/1 (OneBar) usa barStartPpq como anchor cuando hasBarStartPpq es true;
    // en el resto de divisiones (o si falta barStartPpq) usa anchor = 0.0.
    [[nodiscard]] float computePhaseAtSample (const MusicalContext& ctx,
                                              RateDivision rate,
                                              int sampleIndex = 0) noexcept;

    // Llena outPhases[0 .. N-1] con la fase de cada muestra del bloque y devuelve
    // la fase proyectada al final del bloque (muestra N) en [0, 1).
    float fillBlockPhases (const MusicalContext& ctx,
                           RateDivision rate,
                           std::span<float> outPhases) noexcept;

    // Calcula la posicion musical (Bar.Beat.Subdivision) para alimentar el snapshot de UI.
    [[nodiscard]] BarBeatTick computeBarBeatTick (const MusicalContext& ctx) noexcept;
}

#include "DSP/PhaseClock.h"

#include <algorithm>
#include <cmath>

namespace umbral::dsp
{
    namespace
    {
        inline constexpr double kDefaultBpm        = 120.0;
        inline constexpr double kDefaultSampleRate = 44100.0;

        [[nodiscard]] inline double sanitizeBpm (double bpm) noexcept
        {
            return (std::isfinite (bpm) && bpm > 0.0) ? bpm : kDefaultBpm;
        }

        [[nodiscard]] inline double sanitizeSampleRate (double sampleRate) noexcept
        {
            return (std::isfinite (sampleRate) && sampleRate > 0.0) ? sampleRate : kDefaultSampleRate;
        }

        [[nodiscard]] inline double sanitizePpq (double ppq) noexcept
        {
            return std::isfinite (ppq) ? ppq : 0.0;
        }

        // Parte fraccionaria en precision doble en [0.0, 1.0), soportando negativos.
        [[nodiscard]] inline double fracDouble (double x) noexcept
        {
            if (! std::isfinite (x))
                return 0.0;

            const double r = x - std::floor (x);
            return (r >= 1.0 || r < 0.0) ? 0.0 : r;
        }

        // Convierte una fase normalizada double [0.0, 1.0) a float garantizando [0.0f, 1.0f).
        [[nodiscard]] inline float toUnitPhaseFloat (double phaseDouble) noexcept
        {
            const float f = static_cast<float> (fracDouble (phaseDouble));
            return (f >= 1.0f || f < 0.0f) ? 0.0f : f;
        }

        [[nodiscard]] inline double resolveAnchor (const MusicalContext& ctx, RateDivision rate) noexcept
        {
            if (rate == RateDivision::OneBar && ctx.hasBarStartPpq && std::isfinite (ctx.barStartPpq))
                return ctx.barStartPpq;

            return 0.0;
        }
    }

    double computeDeltaPhi (double bpm, double sampleRate,
                            RateDivision rate, TimeSignature ts) noexcept
    {
        const double safeBpm = sanitizeBpm (bpm);
        const double safeFs  = sanitizeSampleRate (sampleRate);
        const double length  = cycleLengthQuarterNotes (rate, ts);

        return safeBpm / (60.0 * safeFs * length);
    }

    float computePhaseAtSample (const MusicalContext& ctx,
                                RateDivision rate,
                                int sampleIndex) noexcept
    {
        const double ppq0     = sanitizePpq (ctx.ppqPosition);
        const double anchor   = resolveAnchor (ctx, rate);
        const double length   = cycleLengthQuarterNotes (rate, ctx.timeSig);
        const double deltaPhi = computeDeltaPhi (ctx.bpm, ctx.sampleRate, rate, ctx.timeSig);

        const double rawPhase = (ppq0 - anchor) / length + static_cast<double> (sampleIndex) * deltaPhi;
        return toUnitPhaseFloat (rawPhase);
    }

    float fillBlockPhases (const MusicalContext& ctx,
                           RateDivision rate,
                           std::span<float> outPhases) noexcept
    {
        const double ppq0      = sanitizePpq (ctx.ppqPosition);
        const double anchor    = resolveAnchor (ctx, rate);
        const double length    = cycleLengthQuarterNotes (rate, ctx.timeSig);
        const double deltaPhi  = computeDeltaPhi (ctx.bpm, ctx.sampleRate, rate, ctx.timeSig);
        const double basePhase = (ppq0 - anchor) / length;

        const std::size_t numSamples = outPhases.size();
        for (std::size_t n = 0; n < numSamples; ++n)
        {
            const double rawPhase = basePhase + static_cast<double> (n) * deltaPhi;
            outPhases[n] = toUnitPhaseFloat (rawPhase);
        }

        const double endRawPhase = basePhase + static_cast<double> (numSamples) * deltaPhi;
        return toUnitPhaseFloat (endRawPhase);
    }

    BarBeatTick computeBarBeatTick (const MusicalContext& ctx) noexcept
    {
        const TimeSignature ts   = sanitizeTimeSignature (ctx.timeSig);
        const int N              = ts.numerator;
        const int D              = ts.denominator;
        const double barLenPpq   = static_cast<double> (N) * (4.0 / static_cast<double> (D));
        const double beatLenPpq  = 4.0 / static_cast<double> (D);
        const double ppqPosition = sanitizePpq (ctx.ppqPosition);

        int bar = 1;
        double relPpq = 0.0;

        if (ctx.hasBarStartPpq && std::isfinite (ctx.barStartPpq))
        {
            relPpq = ppqPosition - ctx.barStartPpq;
            if (relPpq < 0.0 || relPpq >= barLenPpq)
                relPpq = fracDouble (relPpq / barLenPpq) * barLenPpq;

            bar = static_cast<int> (std::floor (ctx.barStartPpq / barLenPpq + 1.0e-6)) + 1;
        }
        else
        {
            const double barIndex = std::floor (ppqPosition / barLenPpq);
            bar = static_cast<int> (barIndex) + 1;
            relPpq = ppqPosition - barIndex * barLenPpq;
            if (relPpq < 0.0 || relPpq >= barLenPpq)
                relPpq = fracDouble (ppqPosition / barLenPpq) * barLenPpq;
        }

        const int beatZero = std::clamp (static_cast<int> (std::floor (relPpq / beatLenPpq + 1.0e-12)), 0, N - 1);
        const int beat = beatZero + 1;

        const double withinBeat = std::clamp ((relPpq - static_cast<double> (beatZero) * beatLenPpq) / beatLenPpq, 0.0, 1.0);
        const int subdivision = std::clamp (static_cast<int> (std::floor (withinBeat * 4.0 + 1.0e-12)), 0, 3) + 1;

        return { bar, beat, subdivision };
    }
}

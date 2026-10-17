#include "DSP/TransportClock.h"

#include <algorithm>
#include <cmath>

namespace umbral::dsp
{
    namespace
    {
        inline constexpr double kDefaultBpm = 120.0;

        [[nodiscard]] inline bool validBpm (double bpm) noexcept
        {
            return std::isfinite (bpm) && bpm > 0.0;
        }

        [[nodiscard]] inline double fracDouble (double x) noexcept
        {
            if (! std::isfinite (x))
                return 0.0;

            const double r = x - std::floor (x);
            return (r >= 1.0 || r < 0.0) ? 0.0 : r;
        }
    }

    void TransportClock::prepare (double newSampleRate) noexcept
    {
        sampleRate = (std::isfinite (newSampleRate) && newSampleRate > 0.0) ? newSampleRate : 44100.0;
        reset();
    }

    void TransportClock::reset() noexcept
    {
        freePhase         = 0.0;
        lockedPhaseOffset = 0.0;
        wasLocked         = false;
        lastPhase         = 0.0f;
        lastTick          = {};
    }

    void TransportClock::triggerNoteOn (bool sync, const HostTransport& host, double paramBpm,
                                        RateDivision rate, int sampleOffset) noexcept
    {
        freePhase = 0.0;
        lastPhase = 0.0f;

        const bool canLock = sync && host.hasInfo && host.isPlaying && host.hasPpq && std::isfinite (host.ppq);
        if (canLock)
        {
            const double fallbackBpm = validBpm (paramBpm) ? paramBpm : kDefaultBpm;
            const double bpm         = (host.hasBpm && validBpm (host.bpm)) ? host.bpm : fallbackBpm;
            const TimeSignature ts   = host.hasTimeSig ? sanitizeTimeSignature (host.timeSig) : TimeSignature {};

            MusicalContext ctx;
            ctx.bpm            = bpm;
            ctx.sampleRate     = sampleRate;
            ctx.timeSig        = ts;
            ctx.hasBarStartPpq = host.hasBarStartPpq && std::isfinite (host.barStartPpq);
            ctx.barStartPpq    = ctx.hasBarStartPpq ? host.barStartPpq : 0.0;
            ctx.ppqPosition    = host.ppq + static_cast<double> (std::max (sampleOffset, 0)) * bpm / (60.0 * sampleRate);
            ctx.phaseOffset    = 0.0;

            const float rawPhase = computePhaseAtSample (ctx, rate, 0);
            lockedPhaseOffset    = -static_cast<double> (rawPhase);
            wasLocked            = true;
        }
        else
        {
            lockedPhaseOffset = 0.0;
        }
    }

    TransportResult TransportClock::process (bool sync, const HostTransport& host, double paramBpm,
                                             RateDivision rate, std::span<float> outPhases,
                                             int sampleOffset) noexcept
    {
        const double fallbackBpm = validBpm (paramBpm) ? paramBpm : kDefaultBpm;
        const TimeSignature ts   = host.hasTimeSig ? sanitizeTimeSignature (host.timeSig) : TimeSignature {};
        const auto numSamples    = static_cast<double> (outPhases.size());

        TransportResult result;

        const bool canLock = sync && host.hasInfo && host.isPlaying && host.hasPpq && std::isfinite (host.ppq);

        if (canLock)
        {
            if (! wasLocked)
                lockedPhaseOffset = 0.0;

            MusicalContext ctx;
            const double bpm = (host.hasBpm && validBpm (host.bpm)) ? host.bpm : fallbackBpm;

            ctx.bpm            = bpm;
            ctx.sampleRate     = sampleRate;
            ctx.timeSig        = ts;
            ctx.hasBarStartPpq = host.hasBarStartPpq && std::isfinite (host.barStartPpq);
            ctx.barStartPpq    = ctx.hasBarStartPpq ? host.barStartPpq : 0.0;
            ctx.ppqPosition    = host.ppq + static_cast<double> (std::max (sampleOffset, 0)) * bpm / (60.0 * sampleRate);
            ctx.phaseOffset    = lockedPhaseOffset;

            result.endPhase = fillBlockPhases (ctx, rate, outPhases);
            result.mode     = TransportMode::Locked;
            result.tick     = computeBarBeatTick (ctx);

            // Mantiene el acumulador libre continuo para un cambio Locked -> Free sin salto.
            freePhase = static_cast<double> (result.endPhase);
            lastPhase = result.endPhase;
            lastTick  = result.tick;
            wasLocked = true;
            return result;
        }

        // Modo Free-Running (SYNC off o DAW en Stop / sin PPQ):
        // Si sync es true y el host reporta un BPM valido, usa host.bpm; de lo contrario usa paramBpm.
        wasLocked         = false;
        lockedPhaseOffset = 0.0;

        const double activeBpm = (sync && host.hasInfo && host.hasBpm && validBpm (host.bpm))
                                     ? host.bpm
                                     : fallbackBpm;
        const double length = cycleLengthQuarterNotes (rate, ts);

        MusicalContext ctx;
        ctx.bpm         = activeBpm;
        ctx.sampleRate  = sampleRate;
        ctx.timeSig     = ts;
        ctx.ppqPosition = freePhase * length;

        fillBlockPhases (ctx, rate, outPhases);

        const double deltaPhi = computeDeltaPhi (activeBpm, sampleRate, rate, ts);
        freePhase = fracDouble (freePhase + numSamples * deltaPhi);

        result.mode     = TransportMode::Free;
        result.endPhase = static_cast<float> (freePhase);
        if (result.endPhase >= 1.0f)
            result.endPhase = 0.0f;

        MusicalContext tickCtx = ctx;
        tickCtx.ppqPosition = freePhase * length;
        result.tick = computeBarBeatTick (tickCtx);

        lastPhase = result.endPhase;
        lastTick  = result.tick;
        return result;
    }
}

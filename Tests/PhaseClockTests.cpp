#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/PhaseClock.h"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;
namespace dsp = umbral::dsp;

static_assert (noexcept (dsp::rateFromIndex (0)));
static_assert (noexcept (dsp::sanitizeTimeSignature ({})));
static_assert (noexcept (dsp::barLengthQuarterNotes ({})));
static_assert (noexcept (dsp::cycleLengthQuarterNotes (dsp::RateDivision::Quarter, {})));
static_assert (noexcept (dsp::computeDeltaPhi (120.0, 44100.0, dsp::RateDivision::Quarter, {})));
static_assert (noexcept (dsp::computePhaseAtSample ({}, dsp::RateDivision::Quarter, 0)));
static_assert (noexcept (dsp::fillBlockPhases ({}, dsp::RateDivision::Quarter, {})));
static_assert (noexcept (dsp::computeBarBeatTick ({})));

TEST_CASE ("PhaseClock: RateDivision and cycle lengths for standard and non-4/4 time signatures", "[dsp][phase]")
{
    SECTION ("Rate index clamping and mapping")
    {
        CHECK (dsp::rateFromIndex (-5) == dsp::RateDivision::Eighth);
        CHECK (dsp::rateFromIndex (0)  == dsp::RateDivision::Eighth);
        CHECK (dsp::rateFromIndex (1)  == dsp::RateDivision::Quarter);
        CHECK (dsp::rateFromIndex (2)  == dsp::RateDivision::Half);
        CHECK (dsp::rateFromIndex (3)  == dsp::RateDivision::OneBar);
        CHECK (dsp::rateFromIndex (99) == dsp::RateDivision::OneBar);
    }

    SECTION ("Fixed subdivisions 1/8, 1/4, 1/2 are independent of time signature")
    {
        const dsp::TimeSignature ts34 { 3, 4 };
        const dsp::TimeSignature ts68 { 6, 8 };

        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::Eighth, ts34),  WithinAbs (0.5, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::Quarter, ts34), WithinAbs (1.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::Half, ts68),    WithinAbs (2.0, 1e-12));
    }

    SECTION ("1/1 (OneBar) adapts to 4/4, 3/4, 6/8, 5/4 and 7/8")
    {
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 4, 4 }), WithinAbs (4.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 3, 4 }), WithinAbs (3.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 6, 8 }), WithinAbs (3.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 5, 4 }), WithinAbs (5.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 7, 8 }), WithinAbs (3.5, 1e-12));
    }

    SECTION ("Invalid time signatures fallback to 4/4 (4.0 quarter notes)")
    {
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 0, 4 }),  WithinAbs (4.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 4, 0 }),  WithinAbs (4.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { -3, 4 }), WithinAbs (4.0, 1e-12));
        CHECK_THAT (dsp::cycleLengthQuarterNotes (dsp::RateDivision::OneBar, { 4, -8 }), WithinAbs (4.0, 1e-12));
    }
}

TEST_CASE ("PhaseClock: deltaPhi and sample progression across sample rates and BPMs", "[dsp][phase]")
{
    constexpr std::array<double, 4> sampleRates { 44100.0, 48000.0, 96000.0, 192000.0 };
    constexpr std::array<double, 3> bpms        { 40.0, 120.0, 240.0 };
    constexpr std::array<dsp::RateDivision, 4> rates {
        dsp::RateDivision::Eighth,
        dsp::RateDivision::Quarter,
        dsp::RateDivision::Half,
        dsp::RateDivision::OneBar
    };

    for (const double fs : sampleRates)
    {
        for (const double bpm : bpms)
        {
            for (const auto rate : rates)
            {
                const dsp::TimeSignature ts { 4, 4 };
                const double L = dsp::cycleLengthQuarterNotes (rate, ts);
                const double expectedDelta = bpm / (60.0 * fs * L);
                const double actualDelta   = dsp::computeDeltaPhi (bpm, fs, rate, ts);

                CHECK_THAT (actualDelta, WithinAbs (expectedDelta, 1e-15));

                dsp::MusicalContext ctx;
                ctx.ppqPosition = 0.0;
                ctx.bpm         = bpm;
                ctx.sampleRate  = fs;
                ctx.timeSig     = ts;

                // At sample 0 phase is 0.0
                CHECK_THAT (dsp::computePhaseAtSample (ctx, rate, 0), WithinAbs (0.0f, 1e-6f));

                // At half cycle in samples, phase is ~0.5
                const double samplesPerCycle = (60.0 * fs * L) / bpm;
                const int halfCycleSamples   = static_cast<int> (std::llround (samplesPerCycle * 0.5));
                const float halfPhase        = dsp::computePhaseAtSample (ctx, rate, halfCycleSamples);
                CHECK_THAT (halfPhase, WithinAbs (0.5f, 1e-3f));
            }
        }
    }
}

TEST_CASE ("PhaseClock: negative PPQ positions and continuity across zero", "[dsp][phase]")
{
    dsp::MusicalContext ctx;
    ctx.bpm        = 120.0;
    ctx.sampleRate = 48000.0;

    SECTION ("Quarter note rate at negative PPQ values")
    {
        ctx.timeSig = { 4, 4 };

        ctx.ppqPosition = -0.25;
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Quarter, 0), WithinAbs (0.75f, 1e-6f));

        ctx.ppqPosition = -1.5;
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Quarter, 0), WithinAbs (0.50f, 1e-6f));

        ctx.ppqPosition = -4.0;
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Quarter, 0), WithinAbs (0.0f, 1e-6f));

        // Tiny negative PPQ must never round up to 1.0f
        ctx.ppqPosition = -1.0e-9;
        const float tinyNegPhase = dsp::computePhaseAtSample (ctx, dsp::RateDivision::Quarter, 0);
        CHECK (tinyNegPhase >= 0.0f);
        CHECK (tinyNegPhase < 1.0f);
    }

    SECTION ("OneBar rate in 3/4 and 6/8 at negative PPQ")
    {
        ctx.timeSig     = { 3, 4 }; // L = 3.0
        ctx.ppqPosition = -0.75;    // (-0.75 / 3.0) = -0.25 -> frac = 0.75
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 0), WithinAbs (0.75f, 1e-6f));

        ctx.timeSig     = { 6, 8 }; // L = 3.0
        ctx.ppqPosition = -1.5;     // (-1.5 / 3.0) = -0.5 -> frac = 0.50
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 0), WithinAbs (0.50f, 1e-6f));
    }

    SECTION ("Continuous phase progression crossing from negative to positive PPQ")
    {
        // At 120 BPM and 48000 Hz, 1 quarter note = 24000 samples (deltaPhi = 1 / 24000).
        // Start 128 samples before PPQ = 0.0 and process 256 samples across 0.0.
        constexpr int numSamples = 256;
        const double deltaPpqPerSample = 120.0 / (60.0 * 48000.0);
        ctx.timeSig     = { 4, 4 };
        ctx.ppqPosition = -128.0 * deltaPpqPerSample;

        std::array<float, numSamples> phases {};
        dsp::fillBlockPhases (ctx, dsp::RateDivision::Quarter, phases);

        const float expectedStep = static_cast<float> (deltaPpqPerSample);
        for (int i = 0; i < numSamples; ++i)
        {
            CHECK (phases[static_cast<std::size_t> (i)] >= 0.0f);
            CHECK (phases[static_cast<std::size_t> (i)] < 1.0f);
        }

        // Sample 128 corresponds to PPQ = 0.0 -> phase = 0.0f
        CHECK_THAT (phases[128], WithinAbs (0.0f, 1e-6f));
        // Sample 127 is right before wrap (~1.0 - deltaPhi), Sample 129 is +deltaPhi
        CHECK_THAT (phases[127], WithinAbs (1.0f - expectedStep, 1e-5f));
        CHECK_THAT (phases[129], WithinAbs (expectedStep, 1e-6f));
    }
}

TEST_CASE ("PhaseClock: barStartPpq anchoring in OneBar vs sub-bar rates", "[dsp][phase]")
{
    // Simulate a project where a time signature change caused the current 4/4 bar
    // to start at PPQ = 3.0 (not a multiple of 4.0).
    dsp::MusicalContext ctx;
    ctx.bpm            = 120.0;
    ctx.sampleRate     = 48000.0;
    ctx.timeSig        = { 4, 4 };
    ctx.hasBarStartPpq = true;
    ctx.barStartPpq    = 3.0;
    ctx.ppqPosition    = 4.0; // 1 beat into the bar (PPQ 4.0 - barStart 3.0 = 1.0)

    SECTION ("OneBar (1/1) anchors to barStartPpq when available")
    {
        // (4.0 - 3.0) / 4.0 = 0.25
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 0), WithinAbs (0.25f, 1e-6f));
    }

    SECTION ("OneBar (1/1) falls back to PPQ 0.0 grid when hasBarStartPpq is false")
    {
        ctx.hasBarStartPpq = false;
        // (4.0 - 0.0) / 4.0 = 1.0 -> frac = 0.0
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 0), WithinAbs (0.0f, 1e-6f));
    }

    SECTION ("Sub-bar rates (1/8, 1/4, 1/2) ignore barStartPpq and anchor to PPQ 0.0")
    {
        ctx.ppqPosition = 3.5; // half beat after 3.0
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Quarter, 0), WithinAbs (0.50f, 1e-6f));
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Eighth, 0),  WithinAbs (0.0f, 1e-6f));
        CHECK_THAT (dsp::computePhaseAtSample (ctx, dsp::RateDivision::Half, 0),    WithinAbs (0.75f, 1e-6f));
    }
}

TEST_CASE ("PhaseClock: variable block sizes produce identical phase trajectory", "[dsp][phase]")
{
    constexpr int totalSamples = 4096;
    constexpr double bpm       = 128.0;
    constexpr double fs        = 44100.0;
    const dsp::TimeSignature ts { 3, 4 };

    dsp::MusicalContext singleCtx;
    singleCtx.ppqPosition    = 1.25;
    singleCtx.bpm            = bpm;
    singleCtx.sampleRate     = fs;
    singleCtx.timeSig        = ts;
    singleCtx.hasBarStartPpq = true;
    singleCtx.barStartPpq    = 0.0;

    std::vector<float> referencePhases (totalSamples, 0.0f);
    const float refEndPhase = dsp::fillBlockPhases (singleCtx, dsp::RateDivision::OneBar, referencePhases);

    // Process the same 4096 samples using variable block sizes: 1, 64, 127, 256, 512, 1024, etc.
    constexpr std::array<int, 7> blockSizes { 1, 64, 127, 256, 512, 1024, 300 };
    std::vector<float> chunkedPhases (totalSamples, 0.0f);

    const double ppqPerSample = bpm / (60.0 * fs);
    int offset = 0;
    std::size_t sizeIdx = 0;
    float lastEndPhase = 0.0f;

    while (offset < totalSamples)
    {
        const int blockSize = std::min (blockSizes[sizeIdx % blockSizes.size()], totalSamples - offset);
        ++sizeIdx;

        dsp::MusicalContext blockCtx = singleCtx;
        blockCtx.ppqPosition = singleCtx.ppqPosition + static_cast<double> (offset) * ppqPerSample;

        std::span<float> subSpan (chunkedPhases.data() + offset, static_cast<std::size_t> (blockSize));
        lastEndPhase = dsp::fillBlockPhases (blockCtx, dsp::RateDivision::OneBar, subSpan);

        offset += blockSize;
    }

    CHECK_THAT (lastEndPhase, WithinAbs (refEndPhase, 1e-6f));

    for (std::size_t i = 0; i < totalSamples; ++i)
    {
        CHECK_THAT (chunkedPhases[i], WithinAbs (referencePhases[i], 1e-6f));
    }
}

TEST_CASE ("PhaseClock: computeBarBeatTick in 4/4, 3/4, 6/8 and negative PPQ", "[dsp][phase]")
{
    dsp::MusicalContext ctx;
    ctx.bpm        = 120.0;
    ctx.sampleRate = 48000.0;

    SECTION ("4/4 readout including 1.2.3")
    {
        ctx.timeSig        = { 4, 4 };
        ctx.hasBarStartPpq = true;
        ctx.barStartPpq    = 0.0;

        ctx.ppqPosition = 0.0;
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 1, 1 });

        // Bar 1, Beat 2 (1.0 PPQ), 3rd sixteenth (+0.5 PPQ) -> 1.5 PPQ => 1.2.3
        ctx.ppqPosition = 1.5;
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 2, 3 });

        // Bar 2 (starts at PPQ 4.0), Beat 4 (+3.0 PPQ), 4th sixteenth (+0.75 PPQ) -> 7.75 PPQ => 2.4.4
        ctx.barStartPpq = 4.0;
        ctx.ppqPosition = 7.75;
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 2, 4, 4 });
    }

    SECTION ("3/4 readout with and without barStartPpq")
    {
        ctx.timeSig        = { 3, 4 }; // barLenPpq = 3.0, beatLenPpq = 1.0
        ctx.hasBarStartPpq = false;

        ctx.ppqPosition = 0.0;
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 1, 1 });

        ctx.ppqPosition = 2.25; // Bar 1, Beat 3, Sub 2
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 3, 2 });

        ctx.ppqPosition = 3.0; // Bar 2, Beat 1, Sub 1
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 2, 1, 1 });
    }

    SECTION ("6/8 readout where each eighth-note pulse is 0.5 PPQ")
    {
        ctx.timeSig        = { 6, 8 }; // barLenPpq = 3.0, beatLenPpq = 0.5, N = 6
        ctx.hasBarStartPpq = true;
        ctx.barStartPpq    = 0.0;

        ctx.ppqPosition = 0.0;
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 1, 1 });

        ctx.ppqPosition = 0.5; // 2nd eighth note -> 1.2.1
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 2, 1 });

        ctx.ppqPosition = 2.75; // 6th eighth note (2.5) + half eighth (0.25) -> 1.6.3
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 1, 6, 3 });
    }

    SECTION ("Negative PPQ without barStartPpq")
    {
        ctx.timeSig        = { 4, 4 };
        ctx.hasBarStartPpq = false;
        ctx.ppqPosition    = -1.0; // Last beat of bar 0 -> 0.4.1
        CHECK (dsp::computeBarBeatTick (ctx) == dsp::BarBeatTick { 0, 4, 1 });
    }
}

TEST_CASE ("PhaseClock: resilience against NaN, Inf and non-positive inputs", "[dsp][phase]")
{
    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    constexpr double kInf = std::numeric_limits<double>::infinity();

    dsp::MusicalContext ctx;
    ctx.ppqPosition    = kNaN;
    ctx.bpm            = -50.0;
    ctx.sampleRate     = 0.0;
    ctx.timeSig        = { 0, -4 };
    ctx.hasBarStartPpq = true;
    ctx.barStartPpq    = kInf;

    const float p0 = dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 0);
    const float p1 = dsp::computePhaseAtSample (ctx, dsp::RateDivision::OneBar, 512);
    CHECK (std::isfinite (p0));
    CHECK (std::isfinite (p1));
    CHECK (p0 >= 0.0f);
    CHECK (p0 < 1.0f);
    CHECK (p1 >= 0.0f);
    CHECK (p1 < 1.0f);

    const auto bbt = dsp::computeBarBeatTick (ctx);
    CHECK (bbt == dsp::BarBeatTick { 1, 1, 1 });
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/CurveBank.h"
#include "DSP/GainSmoother.h"
#include "DSP/DuckingEngine.h"
#include "DSP/PhaseClock.h"
#include "PluginProcessor.h"
#include "Parameters/ParameterIDs.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;
namespace dsp = umbral::dsp;

static_assert (noexcept (std::declval<dsp::GainSmoother&>().prepare (48000.0, 1.5f)));
static_assert (noexcept (std::declval<dsp::GainSmoother&>().reset (1.0f)));
static_assert (noexcept (std::declval<dsp::GainSmoother&>().resetToNextSample()));
static_assert (noexcept (std::declval<dsp::GainSmoother&>().processSample (1.0f)));
static_assert (noexcept (std::declval<dsp::GainSmoother&>().processBlock (std::span<float> {})));

namespace
{
    class TestPlayHead final : public juce::AudioPlayHead
    {
    public:
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setIsPlaying (playing);
            info.setPpqPosition (ppq);
            info.setBpm (bpm);
            return info;
        }

        bool playing = true;
        double ppq   = 0.0;
        double bpm   = 120.0;
    };

    void setProcessorParam (UmbralDuckProcessor& p, const char* id, float normalised)
    {
        p.getAPVTS().getParameter (id)->setValueNotifyingHost (normalised);
    }
}

TEST_CASE ("GainSmoother: settling time is within 1 ms to 2 ms across sample rates", "[dsp][smoother]")
{
    const double sampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };

    SECTION ("Default 1.5 ms configuration settles to 99 percent between 1 ms and 2 ms")
    {
        for (const double fs : sampleRates)
        {
            dsp::GainSmoother smoother;
            smoother.prepare (fs);
            smoother.reset (0.0f);

            int samplesTo99Rise = -1;
            const int maxSamples = static_cast<int> (fs * 0.010); // 10 ms window
            for (int n = 1; n <= maxSamples; ++n)
            {
                const float g = smoother.processSample (1.0f);
                if (g >= 0.99f)
                {
                    samplesTo99Rise = n;
                    break;
                }
            }

            REQUIRE (samplesTo99Rise > 0);
            const double riseTimeMs = (static_cast<double> (samplesTo99Rise) / fs) * 1000.0;
            CHECK (riseTimeMs >= 1.0);
            CHECK (riseTimeMs <= 2.0);

            smoother.reset (1.0f);
            int samplesTo99Fall = -1;
            for (int n = 1; n <= maxSamples; ++n)
            {
                const float g = smoother.processSample (0.0f);
                if (g <= 0.01f)
                {
                    samplesTo99Fall = n;
                    break;
                }
            }

            REQUIRE (samplesTo99Fall > 0);
            const double fallTimeMs = (static_cast<double> (samplesTo99Fall) / fs) * 1000.0;
            CHECK (fallTimeMs >= 1.0);
            CHECK (fallTimeMs <= 2.0);
            CHECK (std::abs (samplesTo99Fall - samplesTo99Rise) <= 1);
        }
    }

    SECTION ("Explicit 1.0 ms and 2.0 ms bounds match target settling time")
    {
        for (const float targetMs : { 1.0f, 2.0f })
        {
            const double fs = 48000.0;
            dsp::GainSmoother smoother;
            smoother.prepare (fs, targetMs);
            smoother.reset (0.0f);

            int samplesTo99 = -1;
            for (int n = 1; n <= 500; ++n)
            {
                if (smoother.processSample (1.0f) >= 0.99f)
                {
                    samplesTo99 = n;
                    break;
                }
            }

            const double expectedSamples = fs * (static_cast<double> (targetMs) * 1.0e-3);
            CHECK_THAT (static_cast<double> (samplesTo99), WithinAbs (expectedSamples, 1.5));
        }
    }
}

TEST_CASE ("GainSmoother: step response 0 to 1 and 1 to 0 is monotonic and finite", "[dsp][smoother]")
{
    constexpr double kFs = 48000.0;
    dsp::GainSmoother smoother;
    smoother.prepare (kFs);

    SECTION ("Step 0 to 1 increases monotonically without overshoot and snaps to 1.0")
    {
        smoother.reset (0.0f);
        float prev = 0.0f;

        for (int n = 0; n < 500; ++n)
        {
            const float g = smoother.processSample (1.0f);
            REQUIRE (std::isfinite (g));
            REQUIRE (g >= prev);
            REQUIRE (g <= 1.0f);
            prev = g;
        }

        CHECK (prev == 1.0f);
    }

    SECTION ("Step 1 to 0 decreases monotonically without undershoot and snaps to 0.0")
    {
        smoother.reset (1.0f);
        float prev = 1.0f;

        for (int n = 0; n < 500; ++n)
        {
            const float g = smoother.processSample (0.0f);
            REQUIRE (std::isfinite (g));
            REQUIRE (g <= prev);
            REQUIRE (g >= 0.0f);
            prev = g;
        }

        CHECK (prev == 0.0f);
    }

    SECTION ("resetToNextSample latches first sample without ramp and smooths subsequent jumps")
    {
        smoother.resetToNextSample();
        REQUIRE_FALSE (smoother.isPrimed());

        const float first = smoother.processSample (0.25f);
        REQUIRE (smoother.isPrimed());
        CHECK (first == 0.25f);

        const float second = smoother.processSample (1.0f);
        CHECK (second > 0.25f);
        CHECK (second < 1.0f);
    }

    SECTION ("processBlock matches per-sample processing and sanitises invalid inputs")
    {
        dsp::GainSmoother a, b;
        a.prepare (kFs);
        b.prepare (kFs);
        a.reset (0.0f);
        b.reset (0.0f);

        std::vector<float> block (64, 1.0f);
        a.processBlock (block);

        for (size_t i = 0; i < block.size(); ++i)
            CHECK (block[i] == b.processSample (1.0f));

        // Non-finite parameters and inputs stay finite in [0, 1]
        const float qnan = std::numeric_limits<float>::quiet_NaN();
        const float inf  = std::numeric_limits<float>::infinity();
        a.prepare (-100.0, qnan);
        const float s1 = a.processSample (qnan);
        const float s2 = a.processSample (inf);
        const float s3 = a.processSample (-5.0f);
        CHECK (std::isfinite (s1));
        CHECK (std::isfinite (s2));
        CHECK (std::isfinite (s3));
        CHECK (s3 >= 0.0f);
        CHECK (s3 <= 1.0f);
    }
}

TEST_CASE ("GainSmoother: maximum consecutive sample delta stays below threshold", "[dsp][smoother]")
{
    SECTION ("Direct full-scale step deltaG_max equals alpha and stays within quantitative bound")
    {
        for (const double fs : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        {
            dsp::GainSmoother smoother;
            smoother.prepare (fs);
            smoother.reset (0.0f);

            float prev     = 0.0f;
            float maxDelta = 0.0f;
            for (int n = 0; n < 256; ++n)
            {
                const float g = smoother.processSample (1.0f);
                maxDelta      = std::max (maxDelta, std::abs (g - prev));
                prev          = g;
            }

            CHECK_THAT (maxDelta, WithinAbs (smoother.getAlpha(), 1.0e-6f));
            // At 44.1 kHz with 1.5 ms 99% settling, alpha is ~0.0672 (<= 0.08)
            const float expectedBound = static_cast<float> (1.0 - std::exp (-4.605170186 / (fs * 0.001))) + 1.0e-4f;
            CHECK (maxDelta <= expectedBound);
        }
    }

    SECTION ("Curve 11 Step and Curve 05 Reverse discontinuities are bounded across BPM and sample rates")
    {
        const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
        const double bpms[]        = { 40.0, 120.0, 240.0 };

        for (const double fs : sampleRates)
        {
            for (const double bpm : bpms)
            {
                for (const int curveIndex : { 10, 4 }) // 10 = "11 Step", 4 = "05 Reverse" (wrap C(1)=0 -> C(0)=1)
                {
                    // Un ciclo completo en Quarter note
                    const int numSamples = static_cast<int> ((60.0 / bpm) * fs) + 64;
                    std::vector<float> phases (static_cast<size_t> (numSamples));
                    dsp::MusicalContext ctx;
                    ctx.ppqPosition = 0.0;
                    ctx.bpm         = bpm;
                    ctx.sampleRate  = fs;
                    dsp::fillBlockPhases (ctx, dsp::RateDivision::Quarter, phases);

                    std::vector<float> chL (static_cast<size_t> (numSamples), 1.0f);
                    std::vector<float> chR (static_cast<size_t> (numSamples), 1.0f);
                    float* channels[2] = { chL.data(), chR.data() };

                    dsp::DuckingEngine engine;
                    engine.prepare (fs);

                    dsp::DuckingParameters params;
                    params.curveIndex = curveIndex;
                    params.mix        = 1.0f;
                    params.shift      = 0.0f;
                    params.slope      = 0.5f;
                    params.channels   = dsp::ChannelsMode::Stereo;
                    params.bypass     = false;

                    engine.process (channels, 2, numSamples, params, phases);

                    float maxDelta = 0.0f;
                    for (int i = 1; i < numSamples; ++i)
                    {
                        const float d = std::abs (chL[static_cast<size_t> (i)] - chL[static_cast<size_t> (i - 1)]);
                        maxDelta      = std::max (maxDelta, d);
                    }

                    // Sin suavizador el salto de 11 Step o el wrap de 05 Reverse daria maxDelta ~ 1.0.
                    // Con GainSmoother (1.5 ms), maxDelta nunca supera alpha (~0.067 a 44.1 kHz).
                    CHECK (maxDelta <= engine.getGainSmoother().getAlpha() + 1.0e-4f);
                }
            }
        }
    }
}

TEST_CASE ("GainSmoother: processor transitions smooth curve, mix, bypass and transport stops", "[dsp][smoother][processor]")
{
    constexpr double kFs    = 48000.0;
    constexpr int kBlockLen = 256;

    UmbralDuckProcessor processor;
    processor.prepareToPlay (kFs, kBlockLen);

    TestPlayHead playHead;
    playHead.playing = true;
    playHead.bpm     = 120.0;
    playHead.ppq     = 0.5; // Fase 0.5: valle de 03 Smooth (C = 0)
    processor.setPlayHead (&playHead);

    setProcessorParam (processor, umbral::params::rawIDs::mix, 1.0f); // Mix 100% -> G = 0 en el valle

    juce::MidiBuffer midi;

    auto makeUnityBuffer = [] {
        juce::AudioBuffer<float> b (2, kBlockLen);
        for (int i = 0; i < kBlockLen; ++i)
        {
            b.setSample (0, i, 1.0f);
            b.setSample (1, i, 1.0f);
        }
        return b;
    };

    SECTION ("Toggling bypass while ducking ramps smoothly to 1.0 and then stays bit-identical")
    {
        auto b1 = makeUnityBuffer();
        processor.processBlock (b1, midi);
        const float lastDucked = b1.getSample (0, kBlockLen - 1);
        REQUIRE (lastDucked < 0.05f);

        // Activar bypass en caliente
        setProcessorParam (processor, umbral::params::rawIDs::bypass, 1.0f);
        auto b2 = makeUnityBuffer();
        processor.processBlock (b2, midi);

        // La primera muestra del bloque de bypass debe continuar suavemente desde lastDucked
        CHECK (b2.getSample (0, 0) < 0.2f);
        // Al final de 256 muestras a 48 kHz (~5.33 ms > 1.5 ms), la rampa ya asento exactamente en 1.0f
        CHECK (b2.getSample (0, kBlockLen - 1) == 1.0f);

        float maxDelta = std::abs (b2.getSample (0, 0) - lastDucked);
        for (int i = 1; i < kBlockLen; ++i)
            maxDelta = std::max (maxDelta, std::abs (b2.getSample (0, i) - b2.getSample (0, i - 1)));

        CHECK (maxDelta < 0.08f);
    }

    SECTION ("Stopping host transport continues smoothly in Free-Running mode without click")
    {
        auto b1 = makeUnityBuffer();
        processor.processBlock (b1, midi);
        const float lastDucked = b1.getSample (0, kBlockLen - 1);
        REQUIRE (lastDucked < 0.05f);

        // Detener el host: TransportMode pasa a Free continuando desde la ultima fase sin salto
        playHead.playing = false;
        auto b2 = makeUnityBuffer();
        processor.processBlock (b2, midi);

        CHECK (b2.getSample (0, 0) < 0.05f);

        float maxDelta = std::abs (b2.getSample (0, 0) - lastDucked);
        for (int i = 1; i < kBlockLen; ++i)
            maxDelta = std::max (maxDelta, std::abs (b2.getSample (0, i) - b2.getSample (0, i - 1)));

        CHECK (maxDelta < 0.01f);
    }

    SECTION ("Abrupt mix change from 100 percent to 0 percent is smoothed without click")
    {
        auto b1 = makeUnityBuffer();
        processor.processBlock (b1, midi);
        const float lastDucked = b1.getSample (0, kBlockLen - 1);
        REQUIRE (lastDucked < 0.05f);

        setProcessorParam (processor, umbral::params::rawIDs::mix, 0.0f);
        auto b2 = makeUnityBuffer();
        processor.processBlock (b2, midi);

        CHECK (b2.getSample (0, 0) < 0.2f);
        CHECK (b2.getSample (0, kBlockLen - 1) == 1.0f);

        float maxDelta = std::abs (b2.getSample (0, 0) - lastDucked);
        for (int i = 1; i < kBlockLen; ++i)
            maxDelta = std::max (maxDelta, std::abs (b2.getSample (0, i) - b2.getSample (0, i - 1)));

        CHECK (maxDelta < 0.08f);
    }

    processor.setPlayHead (nullptr);
}

TEST_CASE ("DuckingEngine: cyclic wrap across phi 1.0 to 0.0 maintains C1 continuity without clicks", "[dsp][smoother][wrap]")
{
    constexpr double kFs = 48000.0;
    constexpr double kBpm = 120.0;
    // Dos ciclos completos en Quarter note para cruzar phi = 1.0 -> 0.0 en medio del buffer
    const int numSamples = static_cast<int> ((60.0 / kBpm) * kFs * 2.0);

    std::vector<float> phases (static_cast<size_t> (numSamples));
    dsp::MusicalContext ctx;
    ctx.ppqPosition = 0.0;
    ctx.bpm         = kBpm;
    ctx.sampleRate  = kFs;
    dsp::fillBlockPhases (ctx, dsp::RateDivision::Quarter, phases);

    // Verifica para todas las curvas (excluyendo 11 Step que tiene un escalon interno en 0.5)
    for (int curveIndex = 0; curveIndex < dsp::numCurves; ++curveIndex)
    {
        if (curveIndex == 10)
            continue;

        std::vector<float> chL (static_cast<size_t> (numSamples), 1.0f);
        std::vector<float> chR (static_cast<size_t> (numSamples), 1.0f);
        float* channels[2] = { chL.data(), chR.data() };

        dsp::DuckingEngine engine;
        engine.prepare (kFs);

        dsp::DuckingParameters params;
        params.curveIndex = curveIndex;
        params.mix        = 1.0f;
        params.shift      = 0.0f;
        params.slope      = 0.5f;
        params.channels   = dsp::ChannelsMode::Stereo;
        params.bypass     = false;

        engine.process (channels, 2, numSamples, params, phases);

        // El cruce de ciclo ocurre en la mitad del buffer (sample ~ 24000)
        const int wrapCenter = numSamples / 2;
        float maxWrapDelta   = 0.0f;
        for (int i = wrapCenter - 32; i < wrapCenter + 256; ++i)
        {
            const float d = std::abs (chL[static_cast<size_t> (i)] - chL[static_cast<size_t> (i - 1)]);
            maxWrapDelta  = std::max (maxWrapDelta, d);
        }

        // Con la micro-rampa Hermite C1 de 2.0 ms (96 muestras a 48 kHz), el salto maximo por muestra
        // en el wrap es <= 1.5 / 96 ~ 0.0156 (muy por debajo del salto IIR de ~0.0915)
        CHECK (maxWrapDelta <= 0.020f);
    }
}

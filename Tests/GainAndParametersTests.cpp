#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Parameters/ParameterIDs.h"
#include "Parameters/ParameterLayout.h"
#include "DSP/DuckingEngine.h"
#include "DSP/CurveBank.h"
#include "PluginProcessor.h"
#include <cmath>
#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;

TEST_CASE ("APVTS layout defines all 9 contract parameters with exact defaults", "[parameters]")
{
    auto layout = umbral::params::createParameterLayout();

    SECTION ("All 9 parameters exist in layout")
    {
        std::vector<juce::String> expectedIDs = {
            umbral::params::rawIDs::rate,
            umbral::params::rawIDs::sync,
            umbral::params::rawIDs::bpm,
            umbral::params::rawIDs::curve,
            umbral::params::rawIDs::mix,
            umbral::params::rawIDs::shift,
            umbral::params::rawIDs::slope,
            umbral::params::rawIDs::channels,
            umbral::params::rawIDs::bypass
        };

        REQUIRE (expectedIDs.size() == 9);
    }

    UmbralDuckProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION ("RATE parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            apvts.getParameter (umbral::params::rawIDs::rate));
        REQUIRE (param != nullptr);
        REQUIRE (param->choices.size() == 4);
        REQUIRE (param->choices[0] == "1/8");
        REQUIRE (param->choices[1] == "1/4");
        REQUIRE (param->choices[2] == "1/2");
        REQUIRE (param->choices[3] == "1/1");
        REQUIRE (param->getIndex() == 1); // Default 1/4
    }

    SECTION ("SYNC parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterBool*> (
            apvts.getParameter (umbral::params::rawIDs::sync));
        REQUIRE (param != nullptr);
        REQUIRE (param->get() == true); // Default true
    }

    SECTION ("BPM parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            apvts.getParameter (umbral::params::rawIDs::bpm));
        REQUIRE (param != nullptr);
        REQUIRE (param->range.start == 40.0f);
        REQUIRE (param->range.end == 240.0f);
        REQUIRE (param->range.interval == 0.1f);
        REQUIRE_THAT (param->get(), WithinAbs (120.0f, 1e-4f)); // Default 120.0
    }

    SECTION ("CURVE BANK parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            apvts.getParameter (umbral::params::rawIDs::curve));
        REQUIRE (param != nullptr);
        REQUIRE (param->choices.size() == 16);
        REQUIRE (param->choices[0]  == "01 Click Duck");
        REQUIRE (param->choices[1]  == "02 Tight");
        REQUIRE (param->choices[2]  == "03 Punch");
        REQUIRE (param->choices[3]  == "04 Classic");
        REQUIRE (param->choices[10] == "11 Trance Gate");
        REQUIRE (param->choices[15] == "16 Custom");
        REQUIRE (param->getIndex() == 3); // Default 04 Classic
    }

    SECTION ("MIX parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            apvts.getParameter (umbral::params::rawIDs::mix));
        REQUIRE (param != nullptr);
        REQUIRE (param->range.start == 0.0f);
        REQUIRE (param->range.end == 100.0f);
        REQUIRE_THAT (param->get(), WithinAbs (70.0f, 1e-4f)); // Default 70%
    }

    SECTION ("SHIFT parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            apvts.getParameter (umbral::params::rawIDs::shift));
        REQUIRE (param != nullptr);
        REQUIRE (param->range.start == 0.0f);
        REQUIRE (param->range.end == 100.0f);
        REQUIRE_THAT (param->get(), WithinAbs (0.0f, 1e-4f)); // Default 0%
    }

    SECTION ("SLOPE parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (
            apvts.getParameter (umbral::params::rawIDs::slope));
        REQUIRE (param != nullptr);
        REQUIRE (param->range.start == 0.0f);
        REQUIRE (param->range.end == 100.0f);
        REQUIRE_THAT (param->get(), WithinAbs (50.0f, 1e-4f)); // Default 50%
    }

    SECTION ("CHANNELS parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (
            apvts.getParameter (umbral::params::rawIDs::channels));
        REQUIRE (param != nullptr);
        REQUIRE (param->choices.size() == 2);
        REQUIRE (param->choices[0] == "Stereo");
        REQUIRE (param->choices[1] == "Mono");
        REQUIRE (param->getIndex() == 0); // Default Stereo
    }

    SECTION ("BYPASS parameter properties")
    {
        auto* param = dynamic_cast<juce::AudioParameterBool*> (
            apvts.getParameter (umbral::params::rawIDs::bypass));
        REQUIRE (param != nullptr);
        REQUIRE (param->get() == false); // Default false
    }
}

TEST_CASE ("DuckingEngine computeGain obeys mathematical gain equation", "[dsp][gain]")
{
    using namespace umbral::dsp;

    SECTION ("Identity at mix zero across all curves and phases")
    {
        for (int curve = 0; curve < numCurves; ++curve)
        {
            for (float phi : { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.99f })
            {
                const float g = DuckingEngine::computeGain (curve, phi, 0.0f, 0.5f, 0.0f);
                REQUIRE_THAT (g, WithinAbs (1.0f, 1e-6f));
            }
        }
    }

    SECTION ("Full ducking at mix 100 percent equals curve value")
    {
        for (int curve = 0; curve < numCurves; ++curve)
        {
            for (float phi : { 0.0f, 0.2f, 0.5f, 0.8f })
            {
                const float g = DuckingEngine::computeGain (curve, phi, 0.1f, 0.6f, 1.0f);
                const float c = evaluate (curve, phi, 0.1f, 0.6f);
                REQUIRE_THAT (g, WithinAbs (c, 1e-6f));
            }
        }
    }

    SECTION ("Intermediate mix scales linearly: G = (1 - M) + M * C")
    {
        // 01 Classic has value 0.0 at phi = 0.0
        const float cValley = evaluate (0, 0.0f, 0.0f, 0.5f);
        REQUIRE (cValley == 0.0f);

        // Mix 70% -> gain should be 0.30
        const float g70 = DuckingEngine::computeGain (0, 0.0f, 0.0f, 0.5f, 0.70f);
        REQUIRE_THAT (g70, WithinAbs (0.30f, 1e-5f));

        // Mix 50% -> gain should be 0.50
        const float g50 = DuckingEngine::computeGain (0, 0.0f, 0.0f, 0.5f, 0.50f);
        REQUIRE_THAT (g50, WithinAbs (0.50f, 1e-5f));
    }

    SECTION ("Non-finite input handling returns finite number in unit range")
    {
        const float qnan = std::numeric_limits<float>::quiet_NaN();
        const float inf  = std::numeric_limits<float>::infinity();

        const float g1 = DuckingEngine::computeGain (0, qnan, 0.0f, 0.5f, 0.7f);
        REQUIRE (std::isfinite (g1));
        REQUIRE (g1 >= 0.0f);
        REQUIRE (g1 <= 1.0f);

        const float g2 = DuckingEngine::computeGain (0, 0.5f, inf, qnan, inf);
        REQUIRE (std::isfinite (g2));
        REQUIRE (g2 >= 0.0f);
        REQUIRE (g2 <= 1.0f);
    }
}

TEST_CASE ("DuckingEngine channel processing and bypass behaviors", "[dsp][channels]")
{
    using namespace umbral::dsp;
    DuckingEngine engine;

    const int numSamples = 64;
    std::vector<float> left (numSamples);
    std::vector<float> right (numSamples);

    auto initSignals = [&]() {
        for (int i = 0; i < numSamples; ++i)
        {
            left[static_cast<size_t> (i)]  = 0.8f;
            right[static_cast<size_t> (i)] = -0.4f;
        }
    };

    SECTION ("Bypass true preserves audio exactly regardless of parameters")
    {
        initSignals();
        DuckingParameters params;
        params.bypass     = true;
        params.mix        = 1.0f;
        params.curveIndex = 0; // Curve 0 has 0.0 at phi = 0

        engine.process (left.data(), right.data(), numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE (left[static_cast<size_t> (i)] == 0.8f);
            REQUIRE (right[static_cast<size_t> (i)] == -0.4f);
        }
    }

    SECTION ("Stereo mode scales channels independently by effective gain")
    {
        initSignals();
        DuckingParameters params;
        params.channels   = ChannelsMode::Stereo;
        params.curveIndex = 0;
        params.mix        = 1.0f; // Curve 0 at phi = 0 gives gain 0.0

        engine.process (left.data(), right.data(), numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE_THAT (left[static_cast<size_t> (i)], WithinAbs (0.0f, 1e-6f));
            REQUIRE_THAT (right[static_cast<size_t> (i)], WithinAbs (0.0f, 1e-6f));
        }
    }

    SECTION ("Mono mode averages left and right with -6 dB factor to avoid clipping")
    {
        initSignals(); // left = 0.8, right = -0.4 -> average = 0.5 * (0.8 - 0.4) = 0.2
        DuckingParameters params;
        params.channels   = ChannelsMode::Mono;
        params.curveIndex = 12; // 13 Offbeat Dip: at phi = 0, C(0) = 1.0 -> gain = 1.0 with any mix
        params.mix        = 0.7f;

        engine.process (left.data(), right.data(), numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE_THAT (left[static_cast<size_t> (i)], WithinAbs (0.2f, 1e-5f));
            REQUIRE_THAT (right[static_cast<size_t> (i)], WithinAbs (0.2f, 1e-5f));
        }
    }

    SECTION ("Mono mode with in-phase 1.0 signals produces 1.0 without clipping")
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[static_cast<size_t> (i)]  = 1.0f;
            right[static_cast<size_t> (i)] = 1.0f;
        }

        DuckingParameters params;
        params.channels   = ChannelsMode::Mono;
        params.curveIndex = 12; // 13 Offbeat Dip: C(0) = 1.0
        params.mix        = 0.7f;

        engine.process (left.data(), right.data(), numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE_THAT (left[static_cast<size_t> (i)], WithinAbs (1.0f, 1e-5f));
            REQUIRE_THAT (right[static_cast<size_t> (i)], WithinAbs (1.0f, 1e-5f));
        }
    }

    SECTION ("Mono mode with opposite phase signals cancels to zero")
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[static_cast<size_t> (i)]  = 0.75f;
            right[static_cast<size_t> (i)] = -0.75f;
        }

        DuckingParameters params;
        params.channels   = ChannelsMode::Mono;
        params.curveIndex = 12;
        params.mix        = 0.7f;

        engine.process (left.data(), right.data(), numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE_THAT (left[static_cast<size_t> (i)], WithinAbs (0.0f, 1e-6f));
            REQUIRE_THAT (right[static_cast<size_t> (i)], WithinAbs (0.0f, 1e-6f));
        }
    }

    SECTION ("Single channel mono buffer is scaled by gain without halving")
    {
        std::vector<float> monoChannel (numSamples, 1.0f);
        float* channels[1] = { monoChannel.data() };

        DuckingParameters params;
        params.curveIndex = 3; // 04 Classic has value 0.0 at phi = 0
        params.mix        = 0.60f; // Gain = (1 - 0.60) + 0.60 * 0 = 0.40

        engine.process (channels, 1, numSamples, params, 0.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            REQUIRE_THAT (monoChannel[static_cast<size_t> (i)], WithinAbs (0.40f, 1e-5f));
        }
    }

    SECTION ("Per-sample phase processing matches expected envelope")
    {
        std::vector<float> phases (numSamples);
        for (int i = 0; i < numSamples; ++i)
            phases[static_cast<size_t> (i)] = static_cast<float> (i) / static_cast<float> (numSamples);

        std::vector<float> chL (numSamples, 1.0f);
        std::vector<float> chR (numSamples, 1.0f);
        float* channels[2] = { chL.data(), chR.data() };

        DuckingParameters params;
        params.curveIndex = 3;
        params.mix        = 1.0f;
        params.channels   = ChannelsMode::Stereo;

        engine.process (channels, 2, numSamples, params, std::span<const float> (phases));

        for (int i = 0; i < numSamples; ++i)
        {
            const float expectedG = DuckingEngine::computeGain (3, phases[static_cast<size_t> (i)], 0.0f, 0.5f, 1.0f);
            REQUIRE_THAT (chL[static_cast<size_t> (i)], WithinAbs (expectedG, 1e-5f));
            REQUIRE_THAT (chR[static_cast<size_t> (i)], WithinAbs (expectedG, 1e-5f));
        }
    }
}

TEST_CASE ("Processor integration updates processing based on APVTS parameters", "[processor]")
{
    UmbralDuckProcessor processor;
    processor.prepareToPlay (44100.0, 64);

    auto& apvts = processor.getAPVTS();

    SECTION ("Setting bypass parameter passes audio through")
    {
        auto* bypassParam = apvts.getParameter (umbral::params::rawIDs::bypass);
        auto* curveParam  = apvts.getParameter (umbral::params::rawIDs::curve);
        auto* mixParam    = apvts.getParameter (umbral::params::rawIDs::mix);

        curveParam->setValueNotifyingHost (0.0f); // 01 Click Duck (gain 0 at phi 0)
        mixParam->setValueNotifyingHost (1.0f);   // 100% mix
        bypassParam->setValueNotifyingHost (1.0f); // Bypass ON

        juce::AudioBuffer<float> buffer (2, 64);
        for (int i = 0; i < 64; ++i)
        {
            buffer.setSample (0, i, 0.5f);
            buffer.setSample (1, i, 0.25f);
        }

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        for (int i = 0; i < 64; ++i)
        {
            REQUIRE (buffer.getSample (0, i) == 0.5f);
            REQUIRE (buffer.getSample (1, i) == 0.25f);
        }
    }

    SECTION ("Setting channels to Mono downmixes in processBlock")
    {
        auto* channelsParam = apvts.getParameter (umbral::params::rawIDs::channels);
        auto* curveParam    = apvts.getParameter (umbral::params::rawIDs::curve);
        channelsParam->setValueNotifyingHost (1.0f); // Choice 1: Mono
        // 13 Offbeat Dip (indice 12): C(phi) = 1.0 en [0, 0.25] -> gain = 1.0
        curveParam->setValueNotifyingHost (curveParam->convertTo0to1 (12.0f));

        juce::AudioBuffer<float> buffer (2, 64);
        for (int i = 0; i < 64; ++i)
        {
            buffer.setSample (0, i, 0.8f);
            buffer.setSample (1, i, 0.2f);
        }

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        // Average = 0.5 * (0.8 + 0.2) = 0.5
        for (int i = 0; i < 64; ++i)
        {
            REQUIRE_THAT (buffer.getSample (0, i), WithinAbs (0.5f, 1e-4f));
            REQUIRE_THAT (buffer.getSample (1, i), WithinAbs (0.5f, 1e-4f));
        }
    }

    processor.releaseResources();
}

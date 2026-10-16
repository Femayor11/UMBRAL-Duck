#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Common/UiStateSnapshot.h"
#include "DSP/LevelMeterSource.h"
#include "Parameters/ParameterIDs.h"
#include "PluginProcessor.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

using Catch::Matchers::WithinAbs;

static_assert (noexcept (std::declval<umbral::UiStateSnapshot&>().publish (0.0f, 0.0f, {}, -60.0f, -60.0f)));
static_assert (noexcept (std::declval<const umbral::UiStateSnapshot&>().read()));
static_assert (noexcept (std::declval<umbral::dsp::LevelMeterSource&>().processInput (nullptr, 0, 0)));
static_assert (noexcept (std::declval<umbral::dsp::LevelMeterSource&>().processOutput (nullptr, 0, 0)));

namespace
{
    class SnapshotTestPlayHead final : public juce::AudioPlayHead
    {
    public:
        juce::Optional<PositionInfo> getPosition() const override
        {
            if (! available)
                return {};

            PositionInfo info;
            info.setIsPlaying (playing);
            info.setPpqPosition (ppq);
            info.setBpm (bpm);
            info.setTimeSignature (TimeSignature { numerator, denominator });
            info.setPpqPositionOfLastBarStart (barStartPpq);
            return info;
        }

        bool available     = true;
        bool playing       = true;
        double ppq         = 0.0;
        double bpm         = 120.0;
        int numerator      = 4;
        int denominator    = 4;
        double barStartPpq = 0.0;
    };
}

TEST_CASE ("UiStateSnapshot default state and range sanitization", "[snapshot]")
{
    umbral::UiStateSnapshot snapshot;

    SECTION ("Default values are finite and within contract bounds")
    {
        const auto data = snapshot.read();
        CHECK_THAT (data.currentPhase, WithinAbs (0.0f, 1.0e-6f));
        CHECK_THAT (data.currentGainDb, WithinAbs (0.0f, 1.0e-6f));
        CHECK (data.barBeatTick == umbral::dsp::BarBeatTick { 1, 1, 1 });
        CHECK_THAT (data.inputLevelDb, WithinAbs (-60.0f, 1.0e-6f));
        CHECK_THAT (data.outputLevelDb, WithinAbs (-60.0f, 1.0e-6f));
    }

    SECTION ("Valid values roundtrip accurately")
    {
        snapshot.publish (0.375f, -14.2f, { 12, 3, 4 }, -6.5f, -18.7f);
        const auto data = snapshot.read();

        CHECK_THAT (data.currentPhase, WithinAbs (0.375f, 1.0e-6f));
        CHECK_THAT (data.currentGainDb, WithinAbs (-14.2f, 1.0e-5f));
        CHECK (data.barBeatTick == umbral::dsp::BarBeatTick { 12, 3, 4 });
        CHECK_THAT (data.inputLevelDb, WithinAbs (-6.5f, 1.0e-5f));
        CHECK_THAT (data.outputLevelDb, WithinAbs (-18.7f, 1.0e-5f));
    }

    SECTION ("Out of bounds, negative and non-finite inputs are sanitized")
    {
        const float qnan = std::numeric_limits<float>::quiet_NaN();
        const float inf  = std::numeric_limits<float>::infinity();

        snapshot.publish (1.25f, -120.0f, { 0, -2, -5 }, 12.0f, -95.0f);
        auto data = snapshot.read();

        CHECK_THAT (data.currentPhase, WithinAbs (0.25f, 1.0e-6f));
        CHECK_THAT (data.currentGainDb, WithinAbs (-60.0f, 1.0e-6f));
        CHECK (data.barBeatTick == umbral::dsp::BarBeatTick { 1, 1, 1 });
        CHECK_THAT (data.inputLevelDb, WithinAbs (0.0f, 1.0e-6f));
        CHECK_THAT (data.outputLevelDb, WithinAbs (-60.0f, 1.0e-6f));

        snapshot.publish (qnan, inf, { -10, 999999, 0 }, qnan, -inf);
        data = snapshot.read();

        CHECK (std::isfinite (data.currentPhase));
        CHECK (data.currentPhase >= 0.0f);
        CHECK (data.currentPhase < 1.0f);
        CHECK (std::isfinite (data.currentGainDb));
        CHECK (data.currentGainDb >= -60.0f);
        CHECK (data.currentGainDb <= 0.0f);
        CHECK (data.barBeatTick.bar == 1);
        CHECK (data.barBeatTick.beat == 65535);
        CHECK (data.barBeatTick.subdivision == 1);
        CHECK_THAT (data.inputLevelDb, WithinAbs (-60.0f, 1.0e-6f));
        CHECK_THAT (data.outputLevelDb, WithinAbs (-60.0f, 1.0e-6f));
    }

    SECTION ("publishFromLinearGain converts linear gain in [0, 1] to [-60, 0] dB")
    {
        snapshot.publishFromLinearGain (0.5f, 1.0f, { 2, 1, 1 }, -3.0f, -3.0f);
        CHECK_THAT (snapshot.getGainDb(), WithinAbs (0.0f, 1.0e-5f));

        snapshot.publishFromLinearGain (0.5f, 0.5f, { 2, 1, 1 }, -3.0f, -9.0f);
        CHECK_THAT (snapshot.getGainDb(), WithinAbs (20.0f * std::log10 (0.5f), 1.0e-4f));

        snapshot.publishFromLinearGain (0.5f, 0.0f, { 2, 1, 1 }, -3.0f, -60.0f);
        CHECK_THAT (snapshot.getGainDb(), WithinAbs (-60.0f, 1.0e-5f));
    }
}

TEST_CASE ("Peak meter ballistics instant attack and smooth exponential decay", "[snapshot][meter]")
{
    umbral::dsp::LevelMeterSource meter;
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize     = 480; // 10 ms por bloque
    meter.prepare (sampleRate, 300.0f);

    std::vector<float> left (blockSize, 0.0f);
    std::vector<float> right (blockSize, 0.0f);
    const float* channels[2] = { left.data(), right.data() };

    SECTION ("Silence produces -60 dB floor")
    {
        const float inDb  = meter.processInput (channels, 2, blockSize);
        const float outDb = meter.processOutput (channels, 2, blockSize);
        CHECK_THAT (inDb, WithinAbs (-60.0f, 1.0e-5f));
        CHECK_THAT (outDb, WithinAbs (-60.0f, 1.0e-5f));
    }

    SECTION ("Instant attack captures single-sample peak immediately")
    {
        left[123]  = -0.5f; // -6.0206 dB
        right[200] = 0.25f;

        const float inDb = meter.processInput (channels, 2, blockSize);
        CHECK_THAT (inDb, WithinAbs (20.0f * std::log10 (0.5f), 1.0e-4f));

        right[10] = 1.0f; // 0.0 dB
        const float outDb = meter.processOutput (channels, 2, blockSize);
        CHECK_THAT (outDb, WithinAbs (0.0f, 1.0e-5f));
    }

    SECTION ("Exponential decay falls monotonically and reaches -60 dB floor")
    {
        left[0] = 1.0f;
        float prevDb = meter.processInput (channels, 2, blockSize);
        CHECK_THAT (prevDb, WithinAbs (0.0f, 1.0e-5f));

        // Pasa a silencio: debe caer suavemente bloque a bloque (~28.9 dB/s para tau = 300 ms).
        left[0] = 0.0f;

        for (int b = 0; b < 30; ++b) // 30 bloques de 10 ms = 300 ms (1 tau -> -8.686 dB)
        {
            const float currentDb = meter.processInput (channels, 2, blockSize);
            CHECK (currentDb < prevDb);
            CHECK (currentDb > -60.0f);
            prevDb = currentDb;
        }

        // En t = tau (300 ms), 20*log10(exp(-1)) = -8.68589 dB
        CHECK_THAT (prevDb, WithinAbs (-8.68589f, 0.05f));

        // Tras ~2.5 segundos adicionales de silencio debe asentar exactamente en -60 dB
        for (int b = 0; b < 250; ++b)
            prevDb = meter.processInput (channels, 2, blockSize);

        CHECK_THAT (prevDb, WithinAbs (-60.0f, 1.0e-5f));
    }
}

TEST_CASE ("Processor updates UiStateSnapshot across transport and ducking modes", "[snapshot][processor]")
{
    UmbralDuckProcessor processor;
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize     = 480; // 10 ms
    processor.prepareToPlay (sampleRate, blockSize);

    SnapshotTestPlayHead playHead;
    playHead.available   = true;
    playHead.playing     = true;
    playHead.bpm         = 120.0;
    playHead.ppq         = 0.0; // Inicio de negra: curva 01 Classic vale 0.0 en phi = 0
    playHead.numerator   = 4;
    playHead.denominator = 4;
    processor.setPlayHead (&playHead);

    auto& apvts = processor.getAPVTS();
    apvts.getParameter (umbral::params::rawIDs::curve)->setValueNotifyingHost (0.0f); // 01 Classic
    apvts.getParameter (umbral::params::rawIDs::mix)->setValueNotifyingHost (1.0f);   // 100% mix
    apvts.getParameter (umbral::params::rawIDs::sync)->setValueNotifyingHost (1.0f);  // SYNC ON

    juce::AudioBuffer<float> buffer (2, blockSize);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.8f);

    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    const auto snap = processor.getUiStateSnapshot();

    // En 120 BPM (2 negras/s), 10 ms avanzan 0.02 negras en modo 1/4 -> endPhase = 0.02
    CHECK_THAT (snap.currentPhase, WithinAbs (0.02f, 1.0e-4f));
    CHECK (snap.barBeatTick == umbral::dsp::BarBeatTick { 1, 1, 1 });

    // Entrada tiene pico 0.8 (-1.938 dB); salida fue atenuada al maximo (01 Classic vale 0 en [0, 0.12])
    CHECK_THAT (snap.inputLevelDb, WithinAbs (20.0f * std::log10 (0.8f), 1.0e-3f));
    CHECK (snap.outputLevelDb < snap.inputLevelDb);
    CHECK_THAT (snap.currentGainDb, WithinAbs (-60.0f, 1.0e-3f));

    // Posicion musical en compas 3, tiempo 2, subdivision 3 (ppq = 8.0 + 1.0 + 0.5 = 9.5 en 4/4)
    playHead.ppq         = 9.5;
    playHead.barStartPpq = 8.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            buffer.setSample (ch, i, 0.5f);

    processor.processBlock (buffer, midi);
    const auto snap2 = processor.getUiStateSnapshot();
    CHECK (snap2.barBeatTick == umbral::dsp::BarBeatTick { 3, 2, 3 });

    processor.setPlayHead (nullptr);
    processor.releaseResources();
}

TEST_CASE ("Lock-free thread safety stress test between audio producer and UI reader", "[snapshot][threads]")
{
    umbral::UiStateSnapshot snapshot;
    std::atomic<bool> running { true };
    std::atomic<int> readerIterations { 0 };
    std::atomic<bool> invariantFailed { false };

    std::thread producer ([&]() {
        for (int i = 0; i < 50000; ++i)
        {
            const float phase  = static_cast<float> (i % 1000) * 0.001f;
            const float gainDb = -static_cast<float> (i % 61);
            const int bar      = 1 + (i % 128);
            const int beat     = 1 + (i % 4);
            const int sub      = 1 + (i % 4);
            const float inDb   = -static_cast<float> (i % 48);
            const float outDb  = -static_cast<float> ((i + 12) % 60);

            snapshot.publish (phase, gainDb, { bar, beat, sub }, inDb, outDb);
        }
        running.store (false, std::memory_order_release);
    });

    std::thread consumer ([&]() {
        while (running.load (std::memory_order_acquire) || readerIterations.load (std::memory_order_relaxed) < 1000)
        {
            const auto d = snapshot.read();
            const bool ok = std::isfinite (d.currentPhase) && d.currentPhase >= 0.0f && d.currentPhase < 1.0f
                         && std::isfinite (d.currentGainDb) && d.currentGainDb >= -60.0f && d.currentGainDb <= 0.0f
                         && d.barBeatTick.bar >= 1 && d.barBeatTick.bar <= 128
                         && d.barBeatTick.beat >= 1 && d.barBeatTick.beat <= 4
                         && d.barBeatTick.subdivision >= 1 && d.barBeatTick.subdivision <= 4
                         && std::isfinite (d.inputLevelDb) && d.inputLevelDb >= -60.0f && d.inputLevelDb <= 0.0f
                         && std::isfinite (d.outputLevelDb) && d.outputLevelDb >= -60.0f && d.outputLevelDb <= 0.0f;

            if (! ok)
                invariantFailed.store (true, std::memory_order_relaxed);

            readerIterations.fetch_add (1, std::memory_order_relaxed);
            if (! running.load (std::memory_order_acquire) && readerIterations.load (std::memory_order_relaxed) >= 1000)
                break;
        }
    });

    producer.join();
    consumer.join();

    CHECK_FALSE (invariantFailed.load());
    CHECK (readerIterations.load() >= 1000);
}

TEST_CASE ("APVTS state persistence roundtrip preserves all 9 parameters and UI properties", "[state][parameters]")
{
    UmbralDuckProcessor sourceProcessor;
    auto& srcApvts = sourceProcessor.getAPVTS();

    // Modifica los 9 parametros respecto a sus defaults
    dynamic_cast<juce::AudioParameterChoice*> (srcApvts.getParameter (umbral::params::rawIDs::rate))->operator= (3);     // 1/1
    dynamic_cast<juce::AudioParameterBool*>   (srcApvts.getParameter (umbral::params::rawIDs::sync))->operator= (false); // Free
    dynamic_cast<juce::AudioParameterFloat*>  (srcApvts.getParameter (umbral::params::rawIDs::bpm))->operator= (174.5f);
    dynamic_cast<juce::AudioParameterChoice*> (srcApvts.getParameter (umbral::params::rawIDs::curve))->operator= (6);    // 07 Deep
    dynamic_cast<juce::AudioParameterFloat*>  (srcApvts.getParameter (umbral::params::rawIDs::mix))->operator= (42.5f);
    dynamic_cast<juce::AudioParameterFloat*>  (srcApvts.getParameter (umbral::params::rawIDs::shift))->operator= (25.0f);
    dynamic_cast<juce::AudioParameterFloat*>  (srcApvts.getParameter (umbral::params::rawIDs::slope))->operator= (80.0f);
    dynamic_cast<juce::AudioParameterChoice*> (srcApvts.getParameter (umbral::params::rawIDs::channels))->operator= (1); // Mono
    dynamic_cast<juce::AudioParameterBool*>   (srcApvts.getParameter (umbral::params::rawIDs::bypass))->operator= (false);

    // Guarda tambien una propiedad de UI en el ValueTree (ej. vista compacta) segun ARCHITECTURE.md §4
    srcApvts.state.setProperty ("compactMode", true, nullptr);

    juce::MemoryBlock stateBlock;
    sourceProcessor.getStateInformation (stateBlock);
    REQUIRE (stateBlock.getSize() > 0);

    UmbralDuckProcessor restoredProcessor;
    restoredProcessor.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));

    auto& dstApvts = restoredProcessor.getAPVTS();

    CHECK (dynamic_cast<juce::AudioParameterChoice*> (dstApvts.getParameter (umbral::params::rawIDs::rate))->getIndex() == 3);
    CHECK (dynamic_cast<juce::AudioParameterBool*>   (dstApvts.getParameter (umbral::params::rawIDs::sync))->get() == false);
    CHECK_THAT (dynamic_cast<juce::AudioParameterFloat*> (dstApvts.getParameter (umbral::params::rawIDs::bpm))->get(), WithinAbs (174.5f, 0.05f));
    CHECK (dynamic_cast<juce::AudioParameterChoice*> (dstApvts.getParameter (umbral::params::rawIDs::curve))->getIndex() == 6);
    CHECK_THAT (dynamic_cast<juce::AudioParameterFloat*> (dstApvts.getParameter (umbral::params::rawIDs::mix))->get(), WithinAbs (42.5f, 1.0e-3f));
    CHECK_THAT (dynamic_cast<juce::AudioParameterFloat*> (dstApvts.getParameter (umbral::params::rawIDs::shift))->get(), WithinAbs (25.0f, 1.0e-3f));
    CHECK_THAT (dynamic_cast<juce::AudioParameterFloat*> (dstApvts.getParameter (umbral::params::rawIDs::slope))->get(), WithinAbs (80.0f, 1.0e-3f));
    CHECK (dynamic_cast<juce::AudioParameterChoice*> (dstApvts.getParameter (umbral::params::rawIDs::channels))->getIndex() == 1);
    CHECK (dynamic_cast<juce::AudioParameterBool*>   (dstApvts.getParameter (umbral::params::rawIDs::bypass))->get() == false);
    CHECK (static_cast<bool> (dstApvts.state.getProperty ("compactMode", false)) == true);

    // Verifica que ambas instancias producen salida de audio y snapshot identicos
    sourceProcessor.prepareToPlay (48000.0, 256);
    restoredProcessor.prepareToPlay (48000.0, 256);

    juce::AudioBuffer<float> bufA (2, 256);
    juce::AudioBuffer<float> bufB (2, 256);
    for (int i = 0; i < 256; ++i)
    {
        const float sL = 0.6f * std::sin (0.05f * static_cast<float> (i));
        const float sR = 0.4f * std::cos (0.05f * static_cast<float> (i));
        bufA.setSample (0, i, sL);
        bufA.setSample (1, i, sR);
        bufB.setSample (0, i, sL);
        bufB.setSample (1, i, sR);
    }

    juce::MidiBuffer midiA, midiB;
    sourceProcessor.processBlock (bufA, midiA);
    restoredProcessor.processBlock (bufB, midiB);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            CHECK_THAT (bufA.getSample (ch, i), WithinAbs (bufB.getSample (ch, i), 1.0e-6f));

    const auto snapA = sourceProcessor.getUiStateSnapshot();
    const auto snapB = restoredProcessor.getUiStateSnapshot();
    CHECK_THAT (snapA.currentPhase, WithinAbs (snapB.currentPhase, 1.0e-6f));
    CHECK_THAT (snapA.currentGainDb, WithinAbs (snapB.currentGainDb, 1.0e-6f));
    CHECK (snapA.barBeatTick == snapB.barBeatTick);
    CHECK_THAT (snapA.inputLevelDb, WithinAbs (snapB.inputLevelDb, 1.0e-6f));
    CHECK_THAT (snapA.outputLevelDb, WithinAbs (snapB.outputLevelDb, 1.0e-6f));
}

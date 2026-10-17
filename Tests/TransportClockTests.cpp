#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/TransportClock.h"
#include "PluginProcessor.h"
#include "Parameters/ParameterIDs.h"

#include <cmath>
#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;
namespace dsp = umbral::dsp;

static_assert (noexcept (std::declval<dsp::TransportClock&>().process (true, {}, 120.0, dsp::RateDivision::Quarter, {}, 0)));

namespace
{
    constexpr double kFs = 48000.0;

    dsp::HostTransport playingHost (double ppq, double bpm = 120.0)
    {
        dsp::HostTransport h;
        h.hasInfo   = true;
        h.isPlaying = true;
        h.hasPpq    = true;
        h.ppq       = ppq;
        h.hasBpm    = true;
        h.bpm       = bpm;
        return h;
    }

    bool allUnitFinite (const std::vector<float>& v)
    {
        for (const auto p : v)
            if (! std::isfinite (p) || p < 0.0f || p >= 1.0f)
                return false;
        return true;
    }

    // PlayHead falso para pruebas de integracion con el Processor.
    class FakePlayHead final : public juce::AudioPlayHead
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
            return info;
        }

        bool available = true;
        bool playing   = true;
        double ppq     = 0.0;
        double bpm     = 120.0;
    };

    void setParam (UmbralDuckProcessor& p, const char* id, float normalised)
    {
        p.getAPVTS().getParameter (id)->setValueNotifyingHost (normalised);
    }
}

TEST_CASE ("TransportClock: Locked follows host ppq and BPM changes", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);
    std::vector<float> ph (480);

    SECTION ("Locked uses host ppq as phase origin")
    {
        const auto r = clock.process (true, playingHost (0.25), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK (r.mode == dsp::TransportMode::Locked);
        CHECK_THAT (ph[0], WithinAbs (0.25, 1.0e-6));
    }

    SECTION ("Host BPM change between blocks changes phase advance")
    {
        clock.process (true, playingHost (0.0, 120.0), 77.0, dsp::RateDivision::Quarter, ph);
        const double slope120 = static_cast<double> (ph[1] - ph[0]);
        clock.process (true, playingHost (0.0, 60.0), 77.0, dsp::RateDivision::Quarter, ph);
        const double slope60 = static_cast<double> (ph[1] - ph[0]);

        CHECK_THAT (slope120, WithinAbs (120.0 / (60.0 * kFs), 1.0e-7));
        CHECK_THAT (slope60, WithinAbs (60.0 / (60.0 * kFs), 1.0e-7));
    }

    SECTION ("Host BPM extremes 40 and 240 stay finite and in range")
    {
        for (const double bpm : { 40.0, 240.0 })
        {
            clock.process (true, playingHost (3.7, bpm), 120.0, dsp::RateDivision::Eighth, ph);
            CHECK (allUnitFinite (ph));
        }
    }

    SECTION ("Invalid host BPM falls back to parameter BPM")
    {
        auto h = playingHost (0.0, std::numeric_limits<double>::quiet_NaN());
        clock.process (true, h, 90.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (static_cast<double> (ph[1] - ph[0]), WithinAbs (90.0 / (60.0 * kFs), 1.0e-7));

        h.hasBpm = false;
        clock.process (true, h, 60.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (static_cast<double> (ph[1] - ph[0]), WithinAbs (60.0 / (60.0 * kFs), 1.0e-7));
    }
}

TEST_CASE ("TransportClock: seek and loop jumps are resolved from ppq", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);
    std::vector<float> ph (256);

    SECTION ("Forward and backward seek")
    {
        clock.process (true, playingHost (10.0), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.0, 1.0e-6));

        clock.process (true, playingHost (123.5), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.5, 1.0e-6));

        clock.process (true, playingHost (1.75), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.75, 1.0e-6));
    }

    SECTION ("Negative ppq wraps into [0,1)")
    {
        clock.process (true, playingHost (-0.25), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.75, 1.0e-6));
        CHECK (allUnitFinite (ph));
    }

    SECTION ("Loop back to start resets phase")
    {
        clock.process (true, playingHost (15.9), 120.0, dsp::RateDivision::Quarter, ph);
        clock.process (true, playingHost (8.0), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.0, 1.0e-6));
    }

    SECTION ("Sample offset continues the ppq inside a split block")
    {
        std::vector<float> whole (512), a (256), b (256);
        clock.process (true, playingHost (0.3), 120.0, dsp::RateDivision::Quarter, whole);
        clock.process (true, playingHost (0.3), 120.0, dsp::RateDivision::Quarter, a, 0);
        clock.process (true, playingHost (0.3), 120.0, dsp::RateDivision::Quarter, b, 256);

        for (size_t i = 0; i < 256; ++i)
        {
            CHECK_THAT (a[i], WithinAbs (static_cast<double> (whole[i]), 1.0e-5));
            CHECK_THAT (b[i], WithinAbs (static_cast<double> (whole[i + 256]), 1.0e-5));
        }
    }
}

TEST_CASE ("TransportClock: Free mode accumulates internally with dt = 1/fs", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);

    const double delta = 100.0 / (60.0 * kFs); // Quarter a 100 BPM

    SECTION ("Phase matches analytic value after N samples regardless of block partition")
    {
        std::vector<float> big (4800);
        std::vector<float> last (1);

        clock.process (false, {}, 100.0, dsp::RateDivision::Quarter, big);

        dsp::TransportClock split;
        split.prepare (kFs);
        const int sizes[] = { 64, 512, 1, 1000, 3223 };
        int consumed = 0;
        std::vector<float> tmp;
        for (const auto s : sizes)
        {
            tmp.assign (static_cast<size_t> (s), 0.0f);
            split.process (false, {}, 100.0, dsp::RateDivision::Quarter, tmp);
            for (int i = 0; i < s; ++i)
                CHECK_THAT (tmp[static_cast<size_t> (i)],
                            WithinAbs (static_cast<double> (big[static_cast<size_t> (consumed + i)]), 1.0e-5));
            consumed += s;
        }

        // Fase en la muestra 4800 (inicio del siguiente bloque)
        clock.process (false, {}, 100.0, dsp::RateDivision::Quarter, last);
        const double expected = 4800.0 * delta - std::floor (4800.0 * delta);
        CHECK_THAT (static_cast<double> (last[0]), WithinAbs (expected, 1.0e-5));
    }

    SECTION ("Free ignores host transport state and host tempo")
    {
        std::vector<float> a (128), b (128);
        dsp::TransportClock other;
        other.prepare (kFs);

        clock.process (false, {}, 100.0, dsp::RateDivision::Quarter, a);
        auto h = playingHost (50.0, 200.0);
        h.isPlaying = false;
        other.process (false, h, 100.0, dsp::RateDivision::Quarter, b);

        for (size_t i = 0; i < a.size(); ++i)
            CHECK (a[i] == b[i]);

        const auto r = clock.process (false, playingHost (9.0, 33.0), 100.0, dsp::RateDivision::Quarter, a);
        CHECK (r.mode == dsp::TransportMode::Free);
    }

    SECTION ("Free BPM change affects slope only")
    {
        std::vector<float> a (16);
        clock.process (false, {}, 60.0, dsp::RateDivision::Quarter, a);
        CHECK_THAT (static_cast<double> (a[1] - a[0]), WithinAbs (60.0 / (60.0 * kFs), 1.0e-7));
        clock.process (false, {}, 240.0, dsp::RateDivision::Quarter, a);
        CHECK_THAT (static_cast<double> (a[1] - a[0]), WithinAbs (240.0 / (60.0 * kFs), 1.0e-7));
    }

    SECTION ("Invalid parameter BPM is sanitised")
    {
        std::vector<float> a (64);
        clock.process (false, {}, std::numeric_limits<double>::quiet_NaN(), dsp::RateDivision::Quarter, a);
        CHECK (allUnitFinite (a));
        CHECK (a[1] > a[0]);
    }
}

TEST_CASE ("TransportClock: Free-running advances when host is stopped even with SYNC enabled", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);
    std::vector<float> ph (128);

    SECTION ("No PlayHead info advances in Free mode using paramBpm")
    {
        const auto r = clock.process (true, {}, 120.0, dsp::RateDivision::Quarter, ph);
        CHECK (r.mode == dsp::TransportMode::Free);
        CHECK (allUnitFinite (ph));
        CHECK_THAT (ph[0], WithinAbs (0.0, 1.0e-6));
        CHECK_THAT (static_cast<double> (ph[1] - ph[0]), WithinAbs (120.0 / (60.0 * kFs), 1.0e-7));
    }

    SECTION ("Stopped host continues advancing from last valid phase using host BPM")
    {
        clock.process (true, playingHost (0.5, 140.0), 120.0, dsp::RateDivision::Quarter, ph);
        const float lastValid = clock.getLastPhase();

        auto h = playingHost (7.0, 140.0);
        h.isPlaying = false;
        const auto r = clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph);

        CHECK (r.mode == dsp::TransportMode::Free);
        CHECK_THAT (ph[0], WithinAbs (static_cast<double> (lastValid), 1.0e-6));
        CHECK_THAT (static_cast<double> (ph[1] - ph[0]), WithinAbs (140.0 / (60.0 * kFs), 1.0e-7));
    }

    SECTION ("Playing without valid ppq falls back to Free-running")
    {
        auto h = playingHost (0.0);
        h.hasPpq = false;
        CHECK (clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph).mode == dsp::TransportMode::Free);

        h.hasPpq = true;
        h.ppq    = std::numeric_limits<double>::infinity();
        const auto r = clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph);
        CHECK (r.mode == dsp::TransportMode::Free);
        CHECK (allUnitFinite (ph));
        CHECK (ph[1] > ph[0]);
    }
}

TEST_CASE ("TransportClock: mode transitions stay continuous and finite", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);
    std::vector<float> ph (480);

    SECTION ("Locked to Free continues from the last locked phase")
    {
        clock.process (true, playingHost (0.0), 120.0, dsp::RateDivision::Quarter, ph);
        const auto locked = clock.process (true, playingHost (1.0), 120.0, dsp::RateDivision::Quarter, ph);

        std::vector<float> next (4);
        clock.process (false, {}, 120.0, dsp::RateDivision::Quarter, next);
        CHECK_THAT (next[0], WithinAbs (static_cast<double> (locked.endPhase), 1.0e-6));
        CHECK (allUnitFinite (next));
    }

    SECTION ("Stopped to Playing resumes from host ppq")
    {
        auto stopped = playingHost (2.0);
        stopped.isPlaying = false;
        CHECK (clock.process (true, stopped, 120.0, dsp::RateDivision::Quarter, ph).mode == dsp::TransportMode::Free);
        clock.process (true, playingHost (2.25), 120.0, dsp::RateDivision::Quarter, ph);
        CHECK_THAT (ph[0], WithinAbs (0.25, 1.0e-6));
    }

    SECTION ("Time signatures 3/4 and 6/8 with 1/1")
    {
        auto h = playingHost (1.5);
        h.hasTimeSig = true;
        h.timeSig    = { 3, 4 };
        clock.process (true, h, 120.0, dsp::RateDivision::OneBar, ph);
        CHECK_THAT (ph[0], WithinAbs (0.5, 1.0e-6)); // 1.5 / 3.0

        h.timeSig = { 6, 8 };
        clock.process (true, h, 120.0, dsp::RateDivision::OneBar, ph);
        CHECK_THAT (ph[0], WithinAbs (0.5, 1.0e-6)); // 1.5 / 3.0
    }
}

TEST_CASE ("Processor transport: Locked and Free-Running modulate live audio when playing or stopped", "[transport][processor]")
{
    UmbralDuckProcessor processor;
    constexpr int blockSize = 256;
    processor.prepareToPlay (kFs, blockSize);

    auto makeBuffer = [] {
        juce::AudioBuffer<float> b (2, blockSize);
        for (int i = 0; i < blockSize; ++i)
        {
            b.setSample (0, i, 0.5f);
            b.setSample (1, i, -0.25f);
        }
        return b;
    };

    auto isIdentical = [&] (const juce::AudioBuffer<float>& b) {
        for (int i = 0; i < blockSize; ++i)
            if (b.getSample (0, i) != 0.5f || b.getSample (1, i) != -0.25f)
                return false;
        return true;
    };

    // Curva 04 Classic (default, indice 3): fase 0.0 es el valle (C = 0 en [0.00, 0.10]). Mix 100%.
    setParam (processor, "mix", 1.0f);

    FakePlayHead playHead;
    processor.setPlayHead (&playHead);
    juce::MidiBuffer midi;

    SECTION ("Stopped host with SYNC enabled modulates audio in Free-Running mode")
    {
        playHead.playing = false;
        playHead.ppq     = 0.0;
        bool ducked      = false;
        for (int n = 0; n < 100 && ! ducked; ++n)
        {
            auto buf = makeBuffer();
            processor.processBlock (buf, midi);
            ducked = ! isIdentical (buf);
        }
        CHECK (ducked);
    }

    SECTION ("No position available modulates audio in Free-Running mode")
    {
        playHead.available = false;
        bool ducked        = false;
        for (int n = 0; n < 100 && ! ducked; ++n)
        {
            auto buf = makeBuffer();
            processor.processBlock (buf, midi);
            ducked = ! isIdentical (buf);
        }
        CHECK (ducked);
    }

    SECTION ("Playing at the curve valley attenuates")
    {
        playHead.ppq = 0.0; // fase 0.0 en 1/4: valle de 04 Classic (C = 0)
        auto buf = makeBuffer();
        processor.processBlock (buf, midi);
        CHECK_FALSE (isIdentical (buf));
        CHECK (std::abs (buf.getSample (0, 0)) < 0.05f);
    }

    SECTION ("Free mode ducks even when host is stopped")
    {
        setParam (processor, "sync", 0.0f);
        playHead.playing = false;
        processor.prepareToPlay (kFs, blockSize);

        bool ducked = false;
        for (int n = 0; n < 200 && ! ducked; ++n)
        {
            auto buf = makeBuffer();
            processor.processBlock (buf, midi);
            ducked = ! isIdentical (buf);
        }
        CHECK (ducked);
    }

    SECTION ("Block larger than prepared size is processed without errors")
    {
        processor.prepareToPlay (kFs, 32);
        juce::AudioBuffer<float> big (2, 1000);
        for (int i = 0; i < 1000; ++i)
        {
            big.setSample (0, i, 0.5f);
            big.setSample (1, i, 0.5f);
        }
        playHead.ppq = 0.0;
        processor.processBlock (big, midi);
        for (int i = 0; i < 1000; ++i)
            REQUIRE (std::isfinite (big.getSample (0, i)));
    }

    processor.setPlayHead (nullptr);
}

TEST_CASE ("Processor MIDI trigger: NoteOn resets phase at exact sample offset with anti-pop ramp", "[midi][transport][processor]")
{
    UmbralDuckProcessor processor;
    REQUIRE (processor.acceptsMidi());

    constexpr int blockSize = 256;
    processor.prepareToPlay (kFs, blockSize);

    // Selecciona 04 Classic (default, C(0)=0 en [0, 0.10], C(1)=1 en [0.78, 1.00]) con mix 100%
    setParam (processor, "mix", 1.0f);
    setParam (processor, "sync", 0.0f);

    FakePlayHead playHead;
    playHead.playing = false;
    processor.setPlayHead (&playHead);

    auto makeUnityBuffer = [] {
        juce::AudioBuffer<float> b (2, blockSize);
        for (int i = 0; i < blockSize; ++i)
        {
            b.setSample (0, i, 1.0f);
            b.setSample (1, i, 1.0f);
        }
        return b;
    };

    // Avanza hasta la parte alta de 01 Classic (cerca de phi ~ 0.85, donde G ~ 0.98)
    juce::MidiBuffer emptyMidi;
    for (int b = 0; b < 80; ++b)
    {
        auto buf = makeUnityBuffer();
        processor.processBlock (buf, emptyMidi);
    }

    const float phaseBeforeNote = processor.getUiStateSnapshot().currentPhase;
    REQUIRE (phaseBeforeNote > 0.75f);

    SECTION ("Mid-block NoteOn resets phase at exact sample offset and ramps smoothly without click")
    {
        constexpr int triggerSample = 100;
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), triggerSample);

        auto buf = makeUnityBuffer();
        processor.processBlock (buf, midi);

        // Antes de triggerSample la ganancia sigue cerca de 1.0
        CHECK (buf.getSample (0, triggerSample - 1) > 0.90f);

        // Tras triggerSample (quedan 156 muestras > 96 muestras de rampa de 2.0 ms a 48 kHz),
        // la ganancia desciende hasta 0.0 (valle inicial de 01 Classic en [0, 0.12])
        CHECK (buf.getSample (0, blockSize - 1) < 0.01f);

        // Verifica que la fase final del bloque corresponde exactamente a (blockSize - triggerSample) muestras desde 0.0
        const double expectedEndPhase = static_cast<double> (blockSize - triggerSample) * (120.0 / (60.0 * kFs));
        CHECK_THAT (static_cast<double> (processor.getUiStateSnapshot().currentPhase),
                    WithinAbs (expectedEndPhase, 1.0e-5));

        // Continuidad C1 anti-pop: ningun salto muestra a muestra supera el umbral seguro (1.5 / 96 ~ 0.0156)
        float maxDelta = 0.0f;
        for (int i = 1; i < blockSize; ++i)
            maxDelta = std::max (maxDelta, std::abs (buf.getSample (0, i) - buf.getSample (0, i - 1)));

        CHECK (maxDelta < 0.025f);
    }

    SECTION ("NoteOff and CC events do not reset phase")
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOff (1, 60, static_cast<juce::uint8> (64)), 64);
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 120);

        auto buf = makeUnityBuffer();
        processor.processBlock (buf, midi);

        // La fase continua avanzando por encima de phaseBeforeNote sin reiniciarse a 0
        CHECK (processor.getUiStateSnapshot().currentPhase > phaseBeforeNote);
    }

    processor.setPlayHead (nullptr);
}

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

TEST_CASE ("TransportClock: stopped or missing host yields frozen phase and Bypass mode", "[dsp][transport]")
{
    dsp::TransportClock clock;
    clock.prepare (kFs);
    std::vector<float> ph (128);

    SECTION ("No PlayHead info")
    {
        const auto r = clock.process (true, {}, 120.0, dsp::RateDivision::Quarter, ph);
        CHECK (r.mode == dsp::TransportMode::Bypass);
        CHECK (allUnitFinite (ph));
        for (const auto p : ph)
            CHECK (p == 0.0f);
    }

    SECTION ("Stopped host keeps last valid phase")
    {
        clock.process (true, playingHost (0.5), 120.0, dsp::RateDivision::Quarter, ph);
        const float lastValid = clock.getLastPhase();

        auto h = playingHost (7.0);
        h.isPlaying = false;
        const auto r = clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph);

        CHECK (r.mode == dsp::TransportMode::Bypass);
        for (const auto p : ph)
            CHECK (p == lastValid);
    }

    SECTION ("Playing without ppq is Bypass")
    {
        auto h = playingHost (0.0);
        h.hasPpq = false;
        CHECK (clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph).mode == dsp::TransportMode::Bypass);

        h.hasPpq = true;
        h.ppq    = std::numeric_limits<double>::infinity();
        const auto r = clock.process (true, h, 120.0, dsp::RateDivision::Quarter, ph);
        CHECK (r.mode == dsp::TransportMode::Bypass);
        CHECK (allUnitFinite (ph));
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
        CHECK (clock.process (true, stopped, 120.0, dsp::RateDivision::Quarter, ph).mode == dsp::TransportMode::Bypass);
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

TEST_CASE ("Processor transport: Bypass keeps output bit-identical, Locked and Free duck", "[transport][processor]")
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

    // Curva 03 Smooth (default): fase 0.5 es el valle (C = 0). Mix 100%.
    setParam (processor, "mix", 1.0f);

    FakePlayHead playHead;
    processor.setPlayHead (&playHead);
    juce::MidiBuffer midi;

    SECTION ("Stopped host: output is bit-identical")
    {
        playHead.playing = false;
        playHead.ppq     = 0.5;
        auto buf = makeBuffer();
        processor.processBlock (buf, midi);
        CHECK (isIdentical (buf));
    }

    SECTION ("No position available: output is bit-identical")
    {
        playHead.available = false;
        auto buf = makeBuffer();
        processor.processBlock (buf, midi);
        CHECK (isIdentical (buf));
    }

    SECTION ("Playing at the curve valley attenuates")
    {
        playHead.ppq = 0.5; // fase 0.5 en 1/4
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

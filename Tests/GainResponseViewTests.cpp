#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Common/UiStateSnapshot.h"
#include "UI/DesignTokens.h"
#include "UI/GainResponseGeometry.h"
#include "UI/GainResponseView.h"
#include "UI/LevelMetersComponent.h"

#include <atomic>
#include <cmath>
#include <cstring>

using Catch::Matchers::WithinAbs;
namespace geo = umbral::ui::geometry;

namespace
{
    const juce::Rectangle<float> kArea { 10.0f, 20.0f, 400.0f, 200.0f };

    int countPixels (const juce::Image& image, juce::Colour colour)
    {
        int count = 0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelAt (x, y).getARGB() == colour.getARGB())
                    ++count;
        return count;
    }

    void render (juce::Component& component, juce::Image& image)
    {
        juce::Graphics g (image);
        component.paintEntireComponent (g, false);
    }
}

TEST_CASE ("GainResponse coordinate mapping is exact at the edges", "[ui][gainresponse]")
{
    SECTION ("phase maps to x across the plot")
    {
        CHECK_THAT (geo::phaseToX (kArea, 0.0f),  WithinAbs (10.0f, 1.0e-4f));
        CHECK_THAT (geo::phaseToX (kArea, 0.5f),  WithinAbs (210.0f, 1.0e-4f));
        CHECK_THAT (geo::phaseToX (kArea, 1.0f),  WithinAbs (410.0f, 1.0e-4f));
    }

    SECTION ("db maps to y, 0 dB on top and -24 dB at the bottom")
    {
        CHECK_THAT (geo::dbToY (kArea, 0.0f),   WithinAbs (20.0f, 1.0e-4f));
        CHECK_THAT (geo::dbToY (kArea, -12.0f), WithinAbs (120.0f, 1.0e-4f));
        CHECK_THAT (geo::dbToY (kArea, -24.0f), WithinAbs (220.0f, 1.0e-4f));
    }

    SECTION ("values below -24 dB are visually clipped")
    {
        CHECK_THAT (geo::dbToY (kArea, -60.0f), WithinAbs (220.0f, 1.0e-4f));
        CHECK_THAT (geo::dbToY (kArea, 6.0f),   WithinAbs (20.0f, 1.0e-4f));
    }

    SECTION ("inverse mappings round trip")
    {
        for (const float p : { 0.0f, 0.1f, 0.25f, 0.77f, 1.0f })
            CHECK_THAT (geo::xToPhase (kArea, geo::phaseToX (kArea, p)), WithinAbs (p, 1.0e-5f));

        for (const float db : { 0.0f, -3.0f, -6.0f, -17.5f, -24.0f })
            CHECK_THAT (geo::yToDb (kArea, geo::dbToY (kArea, db)), WithinAbs (db, 1.0e-4f));
    }

    SECTION ("non finite input and empty areas stay finite")
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const juce::Rectangle<float> empty;

        CHECK (std::isfinite (geo::phaseToX (kArea, nan)));
        CHECK (std::isfinite (geo::dbToY (kArea, nan)));
        CHECK (std::isfinite (geo::xToPhase (kArea, nan)));
        CHECK (std::isfinite (geo::yToDb (kArea, nan)));
        CHECK (geo::xToPhase (empty, 5.0f) == 0.0f);
        CHECK (geo::yToDb (empty, 5.0f) == 0.0f);
    }
}

TEST_CASE ("GainResponse grid has the specified lines and marks", "[ui][gainresponse]")
{
    REQUIRE (geo::gridDb.size() == 5);
    REQUIRE (geo::gridPhase.size() == 5);

    const float expectedDb[]    = { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f };
    const float expectedPhase[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    for (std::size_t i = 0; i < 5; ++i)
    {
        CHECK (geo::gridDb[i] == expectedDb[i]);
        CHECK (geo::gridPhase[i] == expectedPhase[i]);
    }
}

TEST_CASE ("GainResponse paths are valid for every curve", "[ui][gainresponse]")
{
    juce::Path curve;
    juce::Path fill;

    for (int curveIndex = 0; curveIndex < umbral::dsp::numCurves; ++curveIndex)
    {
        for (const float mix : { 0.0f, 0.7f, 1.0f })
        {
            for (const float shift : { 0.0f, 0.5f, 1.0f })
            {
                for (const float slope : { 0.0f, 0.5f, 1.0f })
                {
                    const geo::CurveParams params { curveIndex, mix, shift, slope };
                    geo::buildPaths (kArea, params, curve, fill);

                    REQUIRE_FALSE (curve.isEmpty());
                    REQUIRE_FALSE (fill.isEmpty());

                    const auto curveBounds = curve.getBounds();
                    const auto fillBounds  = fill.getBounds();
                    CHECK (kArea.expanded (1.0e-3f).contains (curveBounds));
                    CHECK (kArea.expanded (1.0e-3f).contains (fillBounds));
                    CHECK_THAT (fillBounds.getY(), WithinAbs (kArea.getY(), 1.0e-3f)); // cierra en 0 dB
                    CHECK (std::isfinite (curveBounds.getWidth()));
                    CHECK (std::isfinite (curveBounds.getHeight()));
                }
            }
        }
    }
}

TEST_CASE ("GainResponse curve matches the DSP gain point by point", "[ui][gainresponse]")
{
    const geo::CurveParams params { 4, 0.8f, 0.25f, 0.3f };

    juce::Path curve;
    juce::Path fill;
    geo::buildPaths (kArea, params, curve, fill);

    int index = 0;
    for (juce::Path::Iterator it (curve); it.next();)
    {
        REQUIRE (index < geo::curvePointCount);
        REQUIRE ((it.elementType == juce::Path::Iterator::startNewSubPath
                  || it.elementType == juce::Path::Iterator::lineTo));

        const float phi = static_cast<float> (index) / static_cast<float> (geo::curvePointCount - 1);
        const float g   = umbral::dsp::DuckingEngine::computeGain (params.curveIndex, phi, params.shift,
                                                                   params.slope, params.mix);
        const float expectedY = geo::dbToY (kArea, umbral::math::gainToDb (g));

        CHECK_THAT (it.x1, WithinAbs (geo::phaseToX (kArea, phi), 1.0e-3f));
        CHECK_THAT (it.y1, WithinAbs (expectedY, 1.0e-3f));
        ++index;
    }

    CHECK (index == geo::curvePointCount);
}

TEST_CASE ("GainResponse degenerate areas produce empty paths", "[ui][gainresponse]")
{
    juce::Path curve;
    juce::Path fill;
    geo::buildPaths ({}, geo::CurveParams {}, curve, fill);

    CHECK (curve.isEmpty());
    CHECK (fill.isEmpty());
}

TEST_CASE ("GainResponseGeometry: no vertical drop at phi 1.0 on curves ending at unity", "[ui][gainresponse]")
{
    // Curvas que terminan en C(1.0) = 1.0 (0 dB) pero inician en C(0.0) < 1.0:
    // Grupo A (0..7) y 16 Custom (15)
    const int unityEndCurves[] = { 0, 1, 2, 3, 4, 5, 6, 7, 15 };
    const float topY           = geo::dbToY (kArea, 0.0f);

    for (const int curveIndex : unityEndCurves)
    {
        const geo::CurveParams params { curveIndex, 1.0f, 0.0f, 0.5f };

        // En phi = 1.0f con shift = 0.0f la ganancia en dB debe ser exactamente 0.0 dB (sin envolver a phi = 0.0f)
        CHECK_THAT (geo::effectiveGainDb (params, 1.0f), WithinAbs (0.0f, 1.0e-4f));

        juce::Path curve;
        juce::Path fill;
        geo::buildPaths (kArea, params, curve, fill);

        float prevY = topY;
        float lastY = topY;
        int idx     = 0;
        for (juce::Path::Iterator it (curve); it.next();)
        {
            if (idx == geo::curvePointCount - 2)
                prevY = it.y1;
            if (idx == geo::curvePointCount - 1)
                lastY = it.y1;
            ++idx;
        }

        // El ultimo punto (phi = 1.0) termina en 0 dB (topY) sin caida vertical respecto al penultimo punto
        CHECK_THAT (lastY, WithinAbs (topY, 1.0e-3f));
        CHECK (std::abs (lastY - prevY) < 2.0f);
    }
}

TEST_CASE ("GainResponse text formatting", "[ui][gainresponse]")
{
    char buffer[32] {};

    SECTION ("gain readout")
    {
        geo::formatGainDb (-14.2f, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "-14.2 dB") == 0);

        geo::formatGainDb (0.0f, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "0.0 dB") == 0);

        geo::formatGainDb (-0.04f, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "0.0 dB") == 0);

        geo::formatGainDb (-60.0f, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "-60.0 dB") == 0);

        geo::formatGainDb (-100.0f, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "-60.0 dB") == 0);

        geo::formatGainDb (std::numeric_limits<float>::quiet_NaN(), buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "0.0 dB") == 0);
    }

    SECTION ("bar beat pill")
    {
        geo::formatBarBeat ({ 1, 2, 3 }, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "1.2.3") == 0);

        geo::formatBarBeat ({ 128, 4, 4 }, buffer, sizeof (buffer));
        CHECK (std::strcmp (buffer, "128.4.4") == 0);
    }
}

TEST_CASE ("GainResponseView refresh reads the snapshot and parameters", "[ui][gainresponse]")
{
    juce::ScopedJuceInitialiser_GUI gui;

    umbral::UiStateSnapshot snapshot;
    std::atomic<float> curve { 5.0f };
    std::atomic<float> mix { 100.0f };
    std::atomic<float> shift { 25.0f };
    std::atomic<float> slope { 50.0f };

    umbral::ui::GainResponseView view (snapshot, { &curve, &mix, &shift, &slope });
    view.setBounds (0, 0, 534, 368);

    SECTION ("snapshot values are mirrored")
    {
        snapshot.publish (0.25f, -14.2f, { 2, 3, 4 }, -10.0f, -12.0f);
        view.refresh();

        CHECK (view.getGainText() == "-14.2 dB");
        CHECK (view.getBarBeatText() == "2.3.4");
        CHECK_THAT (view.getPlayheadPhase(), WithinAbs (0.25f, 1.0e-6f));
    }

    SECTION ("readout reaches -60 dB")
    {
        snapshot.publish (0.0f, -60.0f, { 1, 1, 1 }, -60.0f, -60.0f);
        view.refresh();
        CHECK (view.getGainText() == "-60.0 dB");
    }

    SECTION ("parameter changes rebuild the effective curve")
    {
        const auto before = view.getCurveParams();
        CHECK (before.curveIndex == 5);
        CHECK_THAT (before.mix, WithinAbs (1.0f, 1.0e-6f));
        CHECK_THAT (before.shift, WithinAbs (0.25f, 1.0e-6f));

        mix.store (0.0f);
        view.refresh();

        CHECK_THAT (view.getCurveParams().mix, WithinAbs (0.0f, 1.0e-6f));

        // mix = 0 -> G = 1 -> toda la curva queda en 0 dB (borde superior)
        const auto bounds = view.getCurvePath().getBounds();
        CHECK_THAT (bounds.getHeight(), WithinAbs (0.0f, 1.0e-3f));
        CHECK_THAT (bounds.getY(), WithinAbs (view.getPlotArea().getY(), 1.0e-3f));
    }
}

TEST_CASE ("GainResponseView renders offscreen without hanging", "[ui][gainresponse]")
{
    juce::ScopedJuceInitialiser_GUI gui;

    umbral::UiStateSnapshot snapshot;
    umbral::ui::GainResponseView view (snapshot, {});

    SECTION ("full size draws Signal pixels for curve and readout")
    {
        view.setBounds (0, 0, 534, 368);
        snapshot.publish (0.5f, -14.2f, { 1, 2, 3 }, -20.0f, -22.0f);
        view.refresh();

        juce::Image image (juce::Image::ARGB, 534, 368, true, juce::SoftwareImageType {});
        render (view, image);

        CHECK (countPixels (image, umbral::tokens::signal) > 0);
        CHECK (countPixels (image, umbral::tokens::bone) > 0);   // playhead + pastilla
        CHECK (countPixels (image, umbral::tokens::panel) > 0);
    }

    SECTION ("extreme snapshot states do not break painting")
    {
        view.setBounds (0, 0, 350, 279);
        view.setBarPillVisible (false);

        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float states[][2] = { { 0.0f, 0.0f }, { 0.999f, -60.0f }, { nan, nan }, { 3.7f, 5.0f } };

        juce::Image image (juce::Image::ARGB, 350, 279, true, juce::SoftwareImageType {});
        for (const auto& s : states)
        {
            snapshot.publish (s[0], s[1], { 1, 1, 1 }, nan, nan);
            view.refresh();
            render (view, image);
        }

        SUCCEED();
    }

    SECTION ("tiny and zero sizes are safe")
    {
        for (const int size : { 0, 1, 20, 40 })
        {
            view.setBounds (0, 0, size, size);
            view.refresh();

            juce::Image image (juce::Image::ARGB, juce::jmax (1, size), juce::jmax (1, size), true,
                               juce::SoftwareImageType {});
            render (view, image);
        }

        SUCCEED();
    }
}

TEST_CASE ("LevelMetersComponent maps levels and renders", "[ui][levelmeters]")
{
    using umbral::ui::LevelMetersComponent;

    SECTION ("level to fraction")
    {
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (0.0f),   WithinAbs (1.0f, 1.0e-6f));
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (-24.0f), WithinAbs (0.5f, 1.0e-6f));
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (-48.0f), WithinAbs (0.0f, 1.0e-6f));
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (-100.0f), WithinAbs (0.0f, 1.0e-6f));
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (10.0f),  WithinAbs (1.0f, 1.0e-6f));
        CHECK_THAT (LevelMetersComponent::levelDbToFraction (std::numeric_limits<float>::quiet_NaN()),
                    WithinAbs (0.0f, 1.0e-6f));
    }

    SECTION ("refresh mirrors the snapshot and paints Bone and Signal bars")
    {
        juce::ScopedJuceInitialiser_GUI gui;

        umbral::UiStateSnapshot snapshot;
        LevelMetersComponent meters (snapshot);
        meters.setBounds (0, 0, 153, 368);

        snapshot.publish (0.0f, 0.0f, { 1, 1, 1 }, -6.0f, -3.0f);
        meters.refresh();

        CHECK_THAT (meters.getInputLevelDb(),  WithinAbs (-6.0f, 1.0e-6f));
        CHECK_THAT (meters.getOutputLevelDb(), WithinAbs (-3.0f, 1.0e-6f));

        juce::Image image (juce::Image::ARGB, 153, 368, true, juce::SoftwareImageType {});
        render (meters, image);

        // Una columna de pixeles dentro de cada barra, cerca de la base
        const auto inBar  = meters.getBarArea (true);
        const auto outBar = meters.getBarArea (false);
        const int y       = static_cast<int> (inBar.getBottom()) - 4;

        CHECK (image.getPixelAt (static_cast<int> (inBar.getCentreX()), y).getARGB()
               == umbral::tokens::bone.getARGB());
        CHECK (image.getPixelAt (static_cast<int> (outBar.getCentreX()), y).getARGB()
               == umbral::tokens::signal.getARGB());
    }

    SECTION ("tiny sizes are safe")
    {
        juce::ScopedJuceInitialiser_GUI gui;

        umbral::UiStateSnapshot snapshot;
        LevelMetersComponent meters (snapshot);

        for (const int size : { 0, 1, 30 })
        {
            meters.setBounds (0, 0, size, size);
            juce::Image image (juce::Image::ARGB, juce::jmax (1, size), juce::jmax (1, size), true,
                               juce::SoftwareImageType {});
            render (meters, image);
        }

        SUCCEED();
    }
}

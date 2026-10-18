#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/CustomCurve.h"
#include "DSP/DuckingEngine.h"
#include "PluginProcessor.h"
#include "UI/GainResponseGeometry.h"
#include "UI/GainResponseView.h"
#include "UI/CurveBankGrid.h"

using namespace umbral;
using namespace Catch::Matchers;

TEST_CASE ("CustomCurve default model and validity", "[custom_curve]")
{
    SECTION ("Default custom curve matches 04 Classic and is valid")
    {
        const auto curve = dsp::CustomCurveData::createDefault();
        REQUIRE (curve.isValid());
        REQUIRE (curve.numNodes >= 2);
        REQUIRE (curve.numNodes <= dsp::CustomCurveData::kMaxNodes);

        // Nodos extremos anclados
        REQUIRE_THAT (curve.nodes[0].phi, WithinAbs (0.0f, 1e-5f));
        REQUIRE_THAT (curve.nodes[static_cast<size_t> (curve.numNodes - 1)].phi, WithinAbs (1.0f, 1e-5f));
    }

    SECTION ("Validation catches invalid node bounds or non-monotonic ordering")
    {
        dsp::CustomCurveData invalidCurve;
        invalidCurve.numNodes = 1;
        REQUIRE_FALSE (invalidCurve.isValid());

        invalidCurve.numNodes = 2;
        invalidCurve.nodes[0] = { 0.1f, 0.0f }; // Primer nodo no esta en 0.0
        invalidCurve.nodes[1] = { 1.0f, 1.0f };
        REQUIRE_FALSE (invalidCurve.isValid());

        invalidCurve.nodes[0] = { 0.0f, 0.0f };
        invalidCurve.nodes[1] = { 0.9f, 1.0f }; // Ultimo nodo no esta en 1.0
        REQUIRE_FALSE (invalidCurve.isValid());

        invalidCurve.numNodes = 3;
        invalidCurve.nodes[0] = { 0.0f, 0.0f };
        invalidCurve.nodes[1] = { 0.002f, 0.5f }; // Muy cerca del nodo 0 (< kMinSpacing)
        invalidCurve.nodes[2] = { 1.0f, 1.0f };
        REQUIRE_FALSE (invalidCurve.isValid());
    }
}

TEST_CASE ("CustomCurve insertion, removal, and movement constraints", "[custom_curve]")
{
    dsp::CustomCurveData curve = dsp::CustomCurveData::createDefault();
    const int initialCount = curve.numNodes;

    SECTION ("Inserting nodes within valid ranges succeeds and maintains monotonic ordering")
    {
        const int insertedIdx = curve.insertNode (0.35f, 0.8f);
        REQUIRE (insertedIdx > 0);
        REQUIRE (curve.numNodes == initialCount + 1);
        REQUIRE (curve.isValid());
        REQUIRE_THAT (curve.nodes[static_cast<size_t> (insertedIdx)].phi, WithinAbs (0.35f, 1e-5f));
        REQUIRE_THAT (curve.nodes[static_cast<size_t> (insertedIdx)].value, WithinAbs (0.8f, 1e-5f));

        // Insercion demasiado cerca de un nodo existente debe fallar
        const int failIdx = curve.insertNode (0.352f, 0.5f);
        REQUIRE (failIdx == -1);
        REQUIRE (curve.numNodes == initialCount + 1);
    }

    SECTION ("Removal of intermediate nodes succeeds while boundaries are protected")
    {
        // No se puede eliminar el nodo 0 ni el ultimo nodo
        REQUIRE_FALSE (curve.removeNode (0));
        REQUIRE_FALSE (curve.removeNode (curve.numNodes - 1));

        // Eliminar nodo intermedio
        REQUIRE (curve.removeNode (1));
        REQUIRE (curve.numNodes == initialCount - 1);
        REQUIRE (curve.isValid());
    }

    SECTION ("Moving nodes clamps to neighboring limits")
    {
        // Mover nodo 0 solo altera su valor, phi permanece en 0.0
        REQUIRE (curve.moveNode (0, 0.2f, 0.5f));
        REQUIRE_THAT (curve.nodes[0].phi, WithinAbs (0.0f, 1e-5f));
        REQUIRE_THAT (curve.nodes[0].value, WithinAbs (0.5f, 1e-5f));

        // Mover ultimo nodo solo altera su valor, phi permanece en 1.0
        const int lastIdx = curve.numNodes - 1;
        REQUIRE (curve.moveNode (lastIdx, 0.8f, 0.25f));
        REQUIRE_THAT (curve.nodes[static_cast<size_t> (lastIdx)].phi, WithinAbs (1.0f, 1e-5f));
        REQUIRE_THAT (curve.nodes[static_cast<size_t> (lastIdx)].value, WithinAbs (0.25f, 1e-5f));

        // Mover nodo intermedio respeta espaciado minimo
        const float prevPhi = curve.nodes[0].phi;
        const float nextPhi = curve.nodes[2].phi;
        REQUIRE (curve.moveNode (1, -0.5f, 0.7f)); // Intento de mover a la izquierda del limite
        REQUIRE (curve.nodes[1].phi >= prevPhi + dsp::CustomCurveData::kMinSpacing);
        REQUIRE (curve.nodes[1].phi <= nextPhi - dsp::CustomCurveData::kMinSpacing);
        REQUIRE (curve.isValid());
    }
}

TEST_CASE ("CustomCurve DSP evaluation and pre-release lookahead", "[custom_curve][dsp]")
{
    dsp::CustomCurveData curve;
    curve.numNodes = 3;
    curve.nodes[0] = { 0.0f, 0.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    curve.nodes[1] = { 0.5f, 1.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    curve.nodes[2] = { 1.0f, 0.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    REQUIRE (curve.isValid());

    SECTION ("DuckingEngine evaluates custom curve in slot 15 with custom nodes")
    {
        // En phi = 0.0, gain con mix = 1.0 debe ser 0.0
        const float g0 = dsp::DuckingEngine::computeGain (dsp::customCurveIndex, 0.0f, 0.0f, 0.5f, 1.0f, curve.getSpan());
        REQUIRE_THAT (g0, WithinAbs (0.0f, 1e-4f));

        // En phi = 0.5, gain con mix = 1.0 debe ser 1.0
        const float gMid = dsp::DuckingEngine::computeGain (dsp::customCurveIndex, 0.5f, 0.0f, 0.5f, 1.0f, curve.getSpan());
        REQUIRE_THAT (gMid, WithinAbs (1.0f, 1e-4f));

        // En phi = 1.0, gain con mix = 1.0 debe ser 0.0
        const float g1 = dsp::DuckingEngine::computeGain (dsp::customCurveIndex, 1.0f, 0.0f, 0.5f, 1.0f, curve.getSpan());
        REQUIRE_THAT (g1, WithinAbs (0.0f, 1e-4f));
    }

    SECTION ("Pre-release lookahead in computeAcousticGain smoothly connects to cycle start")
    {
        // Curva donde C(1.0) = 1.0 pero C(0.0) = 0.0
        dsp::CustomCurveData jumpCurve;
        jumpCurve.numNodes = 2;
        jumpCurve.nodes[0] = { 0.0f, 0.0f, dsp::SegmentShape::Linear, 1.0f };
        jumpCurve.nodes[1] = { 1.0f, 1.0f, dsp::SegmentShape::Linear, 1.0f };
        REQUIRE (jumpCurve.isValid());

        const float gPre = dsp::DuckingEngine::computeAcousticGain (dsp::customCurveIndex, 0.96f, 0.0f, 0.5f, 1.0f, jumpCurve.getSpan());
        const float gNearEnd = dsp::DuckingEngine::computeAcousticGain (dsp::customCurveIndex, 0.999f, 0.0f, 0.5f, 1.0f, jumpCurve.getSpan());
        const float gStart = dsp::DuckingEngine::computeAcousticGain (dsp::customCurveIndex, 0.0f, 0.0f, 0.5f, 1.0f, jumpCurve.getSpan());

        REQUIRE (gPre > 0.9f);
        // Hacia el final del tramo [0.97, 1.0], la ganancia acustica converge a gStart (0.0)
        REQUIRE (gNearEnd < 0.1f);
        REQUIRE_THAT (gStart, WithinAbs (0.0f, 1e-4f));
    }
}

TEST_CASE ("CustomCurve ValueTree roundtrip and processor triple buffer", "[custom_curve][processor]")
{
    UmbralDuckProcessor processor;

    SECTION ("Custom curve is persisted and restored via getStateInformation and setStateInformation")
    {
        dsp::CustomCurveData customCurve;
        customCurve.numNodes = 3;
        customCurve.nodes[0] = { 0.0f, 0.25f, dsp::SegmentShape::Linear, 1.0f };
        customCurve.nodes[1] = { 0.4f, 0.75f, dsp::SegmentShape::PowerIn, 1.8f };
        customCurve.nodes[2] = { 1.0f, 0.10f, dsp::SegmentShape::Smoothstep, 1.0f };
        REQUIRE (customCurve.isValid());

        processor.setCustomCurve (customCurve);
        REQUIRE (processor.getCustomCurve() == customCurve);

        juce::MemoryBlock stateBlock;
        processor.getStateInformation (stateBlock);

        UmbralDuckProcessor restoredProcessor;
        restoredProcessor.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));

        const auto restoredCurve = restoredProcessor.getCustomCurve();
        REQUIRE (restoredCurve.numNodes == 3);
        REQUIRE (restoredCurve == customCurve);
    }
}

TEST_CASE ("GainResponseView interactive custom curve mouse operations", "[gain_response_view][custom_curve]")
{
    umbral::UiStateSnapshot snapshot;
    ui::GainResponseView::ParameterSource params;
    std::atomic<float> curveVal { static_cast<float> (dsp::customCurveIndex) }; // Slot 16 Custom
    std::atomic<float> mixVal   { 100.0f };
    std::atomic<float> shiftVal { 0.0f };
    std::atomic<float> slopeVal { 50.0f };

    params.curve = &curveVal;
    params.mix   = &mixVal;
    params.shift = &shiftVal;
    params.slope = &slopeVal;

    ui::GainResponseView view (snapshot, params);
    view.setBounds (0, 0, 408, 290);
    view.resized();

    dsp::CustomCurveData initialCurve;
    initialCurve.numNodes = 3;
    initialCurve.nodes[0] = { 0.0f, 0.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    initialCurve.nodes[1] = { 0.5f, 1.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    initialCurve.nodes[2] = { 1.0f, 0.0f, dsp::SegmentShape::Smoothstep, 1.0f };
    view.setCustomCurveData (initialCurve);

    SECTION ("Hover detection tracks nodes")
    {
        const auto plot = view.getPlotArea();
        const float midX = plot.getX() + 0.5f * plot.getWidth();
        const float midY = ui::geometry::dbToY (plot, 0.0f); // value = 1.0 -> 0 dB

        juce::MouseEvent moveEvent (
            juce::Desktop::getInstance().getMainMouseSource(),
            juce::Point<float> (midX, midY),
            juce::ModifierKeys(),
            0.0f, 0.0f, 0, 0, 0, &view, &view,
            juce::Time::getCurrentTime(),
            juce::Point<float> (midX, midY),
            juce::Time::getCurrentTime(),
            1, false);

        view.mouseMove (moveEvent);
        REQUIRE (view.getHoveredNodeIndex() == 1);
    }

    SECTION ("Double click inserts new node")
    {
        const auto plot = view.getPlotArea();
        const float targetX = plot.getX() + 0.25f * plot.getWidth();
        const float targetY = ui::geometry::dbToY (plot, -12.0f);

        bool callbackFired = false;
        view.onCustomCurveChanged = [&callbackFired] (const dsp::CustomCurveData&)
        {
            callbackFired = true;
        };

        juce::MouseEvent dblClickEvent (
            juce::Desktop::getInstance().getMainMouseSource(),
            juce::Point<float> (targetX, targetY),
            juce::ModifierKeys::leftButtonModifier,
            0.0f, 0.0f, 0, 0, 0, &view, &view,
            juce::Time::getCurrentTime(),
            juce::Point<float> (targetX, targetY),
            juce::Time::getCurrentTime(),
            2, false);

        view.mouseDoubleClick (dblClickEvent);
        REQUIRE (callbackFired);
        REQUIRE (view.getCustomCurveData().numNodes == 4);
    }

    SECTION ("Right click deletes intermediate node")
    {
        const auto plot = view.getPlotArea();
        const float midX = plot.getX() + 0.5f * plot.getWidth();
        const float midY = ui::geometry::dbToY (plot, 0.0f);

        bool callbackFired = false;
        view.onCustomCurveChanged = [&callbackFired] (const dsp::CustomCurveData&)
        {
            callbackFired = true;
        };

        juce::MouseEvent rightClickEvent (
            juce::Desktop::getInstance().getMainMouseSource(),
            juce::Point<float> (midX, midY),
            juce::ModifierKeys::rightButtonModifier,
            0.0f, 0.0f, 0, 0, 0, &view, &view,
            juce::Time::getCurrentTime(),
            juce::Point<float> (midX, midY),
            juce::Time::getCurrentTime(),
            1, false);

        view.mouseDown (rightClickEvent);
        REQUIRE (callbackFired);
        REQUIRE (view.getCustomCurveData().numNodes == 2);
    }
}

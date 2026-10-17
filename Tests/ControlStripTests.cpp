#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters/ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/ControlStrip.h"
#include "UI/DesignTokens.h"

#include <limits>
#include <memory>

using Catch::Matchers::WithinAbs;

namespace
{
    void setNormalizedParam (juce::AudioProcessorValueTreeState& apvts,
                             const char* rawId,
                             float plainValue)
    {
        auto* param = apvts.getParameter (rawId);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }
}

TEST_CASE ("ControlStrip default initialization matches APVTS parameter defaults", "[ui][controls][t12]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& strip    = editor->getControlStrip();
    auto& channels = editor->getChannelsSelector();

    SECTION ("MIX knob defaults to 70 percent in upper section with 80 px diameter and 270 degree sweep")
    {
        auto& mix   = strip.getMixSlider();
        auto& shift = strip.getShiftSlider();

        CHECK_THAT (static_cast<float> (mix.getValue()), WithinAbs (70.0f, 1.0e-3f));
        CHECK (strip.getMixReadoutText() == "70%");
        CHECK (mix.getWidth() == t::knobMixDiameter);
        CHECK (mix.getHeight() == t::knobMixDiameter);
        CHECK (mix.getY() < shift.getY());

        const auto rotaryParams = mix.getRotaryParameters();
        const float sweepRad = rotaryParams.endAngleRadians - rotaryParams.startAngleRadians;
        CHECK_THAT (juce::radiansToDegrees (sweepRad), WithinAbs (t::knobSweepDegrees, 1.0e-3f));
        CHECK (rotaryParams.stopAtEnd);
    }

    SECTION ("SHIFT and SLOPE knobs default to 0 and 50 percent in symmetric columns")
    {
        auto& shift = strip.getShiftSlider();
        auto& slope = strip.getSlopeSlider();

        CHECK_THAT (static_cast<float> (shift.getValue()), WithinAbs (0.0f, 1.0e-3f));
        CHECK_THAT (static_cast<float> (slope.getValue()), WithinAbs (50.0f, 1.0e-3f));
        CHECK (strip.getShiftReadoutText() == "0%");
        CHECK (strip.getSlopeReadoutText() == "50%");

        CHECK (shift.getWidth() == t::knobSecondaryDiameter);
        CHECK (shift.getHeight() == t::knobSecondaryDiameter);
        CHECK (slope.getWidth() == t::knobSecondaryDiameter);
        CHECK (slope.getHeight() == t::knobSecondaryDiameter);
        CHECK (shift.getY() == slope.getY());
        CHECK (shift.getX() < slope.getX());
    }

    SECTION ("CHANNELS segmented selector defaults to Stereo")
    {
        CHECK (channels.getSelectedChannelIndex() == 0);
        CHECK (channels.getStereoButton().getToggleState());
        CHECK_FALSE (channels.getMonoButton().getToggleState());
        CHECK (channels.getStereoButton().getHeight() == t::controlHeightSmall);
        CHECK (channels.getMonoButton().getHeight() == t::controlHeightSmall);
    }
}

TEST_CASE ("ControlStrip UI changes propagate to APVTS parameters", "[ui][controls][t12]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;
    auto& apvts = processor.getAPVTS();

    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& strip    = editor->getControlStrip();
    auto& channels = editor->getChannelsSelector();

    SECTION ("Moving MIX SHIFT and SLOPE sliders updates APVTS and readouts")
    {
        strip.getMixSlider().setValue (35.0, juce::sendNotificationSync);
        strip.getShiftSlider().setValue (62.0, juce::sendNotificationSync);
        strip.getSlopeSlider().setValue (18.0, juce::sendNotificationSync);

        CHECK_THAT (apvts.getRawParameterValue (ids::mix)->load(),   WithinAbs (35.0f, 0.2f));
        CHECK_THAT (apvts.getRawParameterValue (ids::shift)->load(), WithinAbs (62.0f, 0.2f));
        CHECK_THAT (apvts.getRawParameterValue (ids::slope)->load(), WithinAbs (18.0f, 0.2f));

        CHECK (strip.getMixReadoutText()   == "35%");
        CHECK (strip.getShiftReadoutText() == "62%");
        CHECK (strip.getSlopeReadoutText() == "18%");
    }

    SECTION ("Clicking Stereo and Mono buttons updates APVTS channels parameter")
    {
        REQUIRE (channels.getMonoButton().onClick != nullptr);
        channels.getMonoButton().onClick();

        CHECK (channels.getSelectedChannelIndex() == 1);
        CHECK_FALSE (channels.getStereoButton().getToggleState());
        CHECK (channels.getMonoButton().getToggleState());
        CHECK (juce::roundToInt (apvts.getRawParameterValue (ids::channels)->load()) == 1);

        // Hacer clic de nuevo en Mono mantiene Mono activo sin desactivar ambos botones
        channels.getMonoButton().onClick();
        CHECK (channels.getMonoButton().getToggleState());
        CHECK_FALSE (channels.getStereoButton().getToggleState());

        REQUIRE (channels.getStereoButton().onClick != nullptr);
        channels.getStereoButton().onClick();

        CHECK (channels.getSelectedChannelIndex() == 0);
        CHECK (channels.getStereoButton().getToggleState());
        CHECK_FALSE (channels.getMonoButton().getToggleState());
        CHECK (juce::roundToInt (apvts.getRawParameterValue (ids::channels)->load()) == 0);
    }
}

TEST_CASE ("ControlStrip responds to programmatic APVTS parameter changes", "[ui][controls][t12]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;
    auto& apvts = processor.getAPVTS();

    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& strip    = editor->getControlStrip();
    auto& channels = editor->getChannelsSelector();

    setNormalizedParam (apvts, ids::mix,      85.0f);
    setNormalizedParam (apvts, ids::shift,    25.0f);
    setNormalizedParam (apvts, ids::slope,    75.0f);
    setNormalizedParam (apvts, ids::channels, 1.0f);  // Mono

    CHECK_THAT (static_cast<float> (strip.getMixSlider().getValue()),   WithinAbs (85.0f, 0.2f));
    CHECK_THAT (static_cast<float> (strip.getShiftSlider().getValue()), WithinAbs (25.0f, 0.2f));
    CHECK_THAT (static_cast<float> (strip.getSlopeSlider().getValue()), WithinAbs (75.0f, 0.2f));

    CHECK (strip.getMixReadoutText()   == "85%");
    CHECK (strip.getShiftReadoutText() == "25%");
    CHECK (strip.getSlopeReadoutText() == "75%");

    CHECK (channels.getSelectedChannelIndex() == 1);
    CHECK_FALSE (channels.getStereoButton().getToggleState());
    CHECK (channels.getMonoButton().getToggleState());

    // Formato defensivo ante valores fuera de rango o no finitos
    CHECK (umbral::ui::ControlStrip::formatPercentage (-10.0) == "0%");
    CHECK (umbral::ui::ControlStrip::formatPercentage (150.0) == "100%");
    CHECK (umbral::ui::ControlStrip::formatPercentage (std::numeric_limits<double>::quiet_NaN()) == "0%");
}

TEST_CASE ("ControlStrip Full and Compact layout and conditional visibility", "[ui][controls][window][t12]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& strip    = editor->getControlStrip();
    auto& channels = editor->getChannelsSelector();

    SECTION ("Full mode places ControlStrip on the left and shows all controls")
    {
        CHECK_FALSE (editor->isCompactView());
        CHECK_FALSE (strip.isCompactMode());

        CHECK (strip.getBounds() == juce::Rectangle<int> (t::windowPadding,
                                                          t::fullContentTop,
                                                          t::fullLeftPanelWidth,
                                                          t::fullPanelRowHeight));

        CHECK (strip.getMixSlider().isVisible());
        CHECK (strip.getShiftSlider().isVisible());
        CHECK (strip.getSlopeSlider().isVisible());
        CHECK (channels.isVisible());
    }

    SECTION ("Compact mode moves ControlStrip to the right panel and hides SHIFT SLOPE and CHANNELS")
    {
        editor->setCompactView (true);

        CHECK (editor->isCompactView());
        CHECK (strip.isCompactMode());

        const int expectedCompactRightX = t::windowPaddingCompact + t::compactLeftPanelWidth + t::panelGapCompact;
        CHECK (strip.getBounds() == juce::Rectangle<int> (expectedCompactRightX,
                                                          t::compactContentTop,
                                                          t::compactRightPanelWidth,
                                                          t::compactPanelRowHeight));

        CHECK (strip.getMixSlider().isVisible());
        CHECK_FALSE (strip.getShiftSlider().isVisible());
        CHECK_FALSE (strip.getSlopeSlider().isVisible());
        CHECK_FALSE (channels.isVisible());

        // Volver a Full restaura visibilidad y posicion de todos los controles
        editor->setCompactView (false);

        CHECK_FALSE (strip.isCompactMode());
        CHECK (strip.getBounds() == juce::Rectangle<int> (t::windowPadding,
                                                          t::fullContentTop,
                                                          t::fullLeftPanelWidth,
                                                          t::fullPanelRowHeight));
        CHECK (strip.getShiftSlider().isVisible());
        CHECK (strip.getSlopeSlider().isVisible());
        CHECK (channels.isVisible());
    }
}

TEST_CASE ("ControlStrip lifecycle and editor reopen without memory leaks", "[ui][controls][lifecycle][t12]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;

    // 1. Primera instancia del editor: modifica parametros y renderiza en Full y Compact
    {
        std::unique_ptr<juce::AudioProcessorEditor> firstEditorBase (processor.createEditor());
        auto* firstEditor = dynamic_cast<UmbralDuckEditor*> (firstEditorBase.get());
        REQUIRE (firstEditor != nullptr);

        firstEditor->getControlStrip().getMixSlider().setValue (42.0, juce::sendNotificationSync);
        firstEditor->getControlStrip().getShiftSlider().setValue (15.0, juce::sendNotificationSync);
        firstEditor->getControlStrip().getSlopeSlider().setValue (80.0, juce::sendNotificationSync);
        firstEditor->getChannelsSelector().getMonoButton().onClick();

        juce::Image fullCanvas (juce::Image::ARGB, firstEditor->getWidth(), firstEditor->getHeight(), true,
                                juce::SoftwareImageType {});
        {
            juce::Graphics g (fullCanvas);
            firstEditor->paintEntireComponent (g, false);
        }

        firstEditor->setCompactView (true);
        juce::Image compactCanvas (juce::Image::ARGB, firstEditor->getWidth(), firstEditor->getHeight(), true,
                                   juce::SoftwareImageType {});
        {
            juce::Graphics g (compactCanvas);
            firstEditor->paintEntireComponent (g, false);
        }
    }

    // 2. Segunda instancia tras destruir la primera: sincroniza el estado actual de APVTS sin leaks
    {
        std::unique_ptr<juce::AudioProcessorEditor> secondEditorBase (processor.createEditor());
        auto* secondEditor = dynamic_cast<UmbralDuckEditor*> (secondEditorBase.get());
        REQUIRE (secondEditor != nullptr);

        auto& strip    = secondEditor->getControlStrip();
        auto& channels = secondEditor->getChannelsSelector();

        CHECK (secondEditor->isCompactView());
        CHECK_THAT (static_cast<float> (strip.getMixSlider().getValue()),   WithinAbs (42.0f, 0.2f));
        CHECK_THAT (static_cast<float> (strip.getShiftSlider().getValue()), WithinAbs (15.0f, 0.2f));
        CHECK_THAT (static_cast<float> (strip.getSlopeSlider().getValue()), WithinAbs (80.0f, 0.2f));
        CHECK (strip.getMixReadoutText() == "42%");
        CHECK (channels.getSelectedChannelIndex() == 1);
        CHECK (channels.getMonoButton().getToggleState());
        CHECK (juce::roundToInt (processor.getAPVTS().getRawParameterValue (ids::channels)->load()) == 1);
    }
}

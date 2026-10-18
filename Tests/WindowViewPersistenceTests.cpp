#include <catch2/catch_test_macros.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/DesignTokens.h"

#include <memory>

TEST_CASE ("WindowView default fixed dimensions and constraints", "[ui][window][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    SECTION ("Editor opens strictly in 780x540 and is not resizable")
    {
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK_FALSE (editor->isResizable());

        auto* constrainer = editor->getConstrainer();
        REQUIRE (constrainer != nullptr);
        CHECK (constrainer->getMinimumWidth() == t::windowFullWidth);
        CHECK (constrainer->getMaximumWidth() == t::windowFullWidth);
        CHECK (constrainer->getMinimumHeight() == t::windowFullHeight);
        CHECK (constrainer->getMaximumHeight() == t::windowFullHeight);

        // Attempting to resize to arbitrary bounds is restricted strictly to 780x540
        juce::Rectangle<int> arbitraryBounds (0, 0, 900, 650);
        constrainer->checkBounds (arbitraryBounds,
                                  editor->getBounds(),
                                  juce::Rectangle<int> (0, 0, 3840, 2160),
                                  false, false, true, true);
        CHECK (arbitraryBounds.getWidth() == t::windowFullWidth);
        CHECK (arbitraryBounds.getHeight() == t::windowFullHeight);
    }
}

TEST_CASE ("WindowView subcomponent visibility and fixed layout bounds", "[ui][window][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& gainView   = editor->getGainResponseView();
    auto& meters     = editor->getLevelMeters();
    auto& control    = editor->getControlStrip();
    auto& channels   = editor->getChannelsSelector();
    auto& curveBank  = editor->getCurveBankGrid();

    SECTION ("All modules are visible and layed out in 780x540 window")
    {
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (gainView.isVisible());
        CHECK (meters.isVisible());
        CHECK (control.isVisible());
        CHECK (channels.isVisible());
        CHECK (curveBank.isVisible());

        const int expectedCenterX = t::windowPadding + t::fullLeftPanelWidth + t::panelGap;
        const int expectedRightX  = expectedCenterX + t::fullCenterPanelWidth + t::panelGap;

        CHECK (control.getBounds() == juce::Rectangle<int> (t::windowPadding, t::fullContentTop,
                                                            t::fullLeftPanelWidth, t::fullPanelRowHeight));
        CHECK (gainView.getBounds() == juce::Rectangle<int> (expectedCenterX, t::fullContentTop,
                                                             t::fullCenterPanelWidth, t::fullPanelRowHeight));
        CHECK (meters.getBounds() == juce::Rectangle<int> (expectedRightX, t::fullContentTop,
                                                           t::fullRightPanelWidth, t::fullPanelRowHeight));
        CHECK (channels.getBounds() == juce::Rectangle<int> (expectedRightX, t::fullContentTop,
                                                             t::fullRightPanelWidth, t::channelsSectionHeight));
        CHECK (curveBank.getBounds() == juce::Rectangle<int> (t::windowPadding, t::fullBottomRowTop,
                                                              t::fullBottomRowWidth, t::fullBottomRowHeight));

        // Offscreen paint without crashes
        juce::Image canvas (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true,
                            juce::SoftwareImageType {});
        juce::Graphics g (canvas);
        editor->paintEntireComponent (g, false);
    }
}

TEST_CASE ("WindowView editor close and reopen keeps fixed dimensions", "[ui][window][persistence][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;

    {
        std::unique_ptr<juce::AudioProcessorEditor> firstEditorBase (processor.createEditor());
        auto* firstEditor = dynamic_cast<UmbralDuckEditor*> (firstEditorBase.get());
        REQUIRE (firstEditor != nullptr);

        CHECK (firstEditor->getWidth() == t::windowFullWidth);
        CHECK (firstEditor->getHeight() == t::windowFullHeight);
    } // First editor closed

    {
        std::unique_ptr<juce::AudioProcessorEditor> secondEditorBase (processor.createEditor());
        auto* secondEditor = dynamic_cast<UmbralDuckEditor*> (secondEditorBase.get());
        REQUIRE (secondEditor != nullptr);

        CHECK (secondEditor->getWidth() == t::windowFullWidth);
        CHECK (secondEditor->getHeight() == t::windowFullHeight);
        CHECK (secondEditor->getLevelMeters().isVisible());
        CHECK (secondEditor->getCurveBankGrid().isVisible());
    }
}

TEST_CASE ("WindowView XML state serialization roundtrip preserves clean editor state", "[ui][window][state][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    SECTION ("State serialization survives getStateInformation and setStateInformation")
    {
        UmbralDuckProcessor sourceProcessor;
        juce::MemoryBlock stateBlock;
        sourceProcessor.getStateInformation (stateBlock);
        REQUIRE (stateBlock.getSize() > 0);

        UmbralDuckProcessor restoredProcessor;
        restoredProcessor.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));

        std::unique_ptr<juce::AudioProcessorEditor> editorBase (restoredProcessor.createEditor());
        auto* editor = dynamic_cast<UmbralDuckEditor*> (editorBase.get());
        REQUIRE (editor != nullptr);

        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (editor->getLevelMeters().isVisible());
        CHECK (editor->getCurveBankGrid().isVisible());
    }

    SECTION ("setStateInformation updates an already open editor without altering fixed geometry")
    {
        UmbralDuckProcessor sourceProcessor;
        juce::MemoryBlock block;
        sourceProcessor.getStateInformation (block);

        UmbralDuckProcessor targetProcessor;
        std::unique_ptr<juce::AudioProcessorEditor> editorBase (targetProcessor.createEditor());
        auto* editor = dynamic_cast<UmbralDuckEditor*> (editorBase.get());
        REQUIRE (editor != nullptr);

        targetProcessor.setStateInformation (block.getData(), static_cast<int> (block.getSize()));

        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (editor->getLevelMeters().isVisible());
        CHECK (editor->getCurveBankGrid().isVisible());
    }
}

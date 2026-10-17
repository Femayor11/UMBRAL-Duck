#include <catch2/catch_test_macros.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/DesignTokens.h"

#include <memory>

TEST_CASE ("WindowView default Full dimensions and fixed resize constraints", "[ui][window][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    SECTION ("Default state starts in Full view 780x540 and is not arbitrarily resizable")
    {
        CHECK_FALSE (processor.isCompactView());
        CHECK_FALSE (editor->isCompactView());
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK_FALSE (editor->isResizable());

        auto* constrainer = editor->getConstrainer();
        REQUIRE (constrainer != nullptr);
        CHECK (constrainer->getMinimumWidth() == t::windowFullWidth);
        CHECK (constrainer->getMaximumWidth() == t::windowFullWidth);
        CHECK (constrainer->getMinimumHeight() == t::windowFullHeight);
        CHECK (constrainer->getMaximumHeight() == t::windowFullHeight);

        // Un intento de redimensionar a un tamano arbitrario se restringe a 780x540
        juce::Rectangle<int> arbitraryBounds (0, 0, 900, 650);
        constrainer->checkBounds (arbitraryBounds,
                                  editor->getBounds(),
                                  juce::Rectangle<int> (0, 0, 3840, 2160),
                                  false, false, true, true);
        CHECK (arbitraryBounds.getWidth() == t::windowFullWidth);
        CHECK (arbitraryBounds.getHeight() == t::windowFullHeight);
    }

    SECTION ("Compact view locks resize limits strictly to 560x360")
    {
        editor->setCompactView (true);

        CHECK (editor->isCompactView());
        CHECK (editor->getWidth() == t::windowCompactWidth);
        CHECK (editor->getHeight() == t::windowCompactHeight);
        CHECK_FALSE (editor->isResizable());

        auto* constrainer = editor->getConstrainer();
        REQUIRE (constrainer != nullptr);
        CHECK (constrainer->getMinimumWidth() == t::windowCompactWidth);
        CHECK (constrainer->getMaximumWidth() == t::windowCompactWidth);
        CHECK (constrainer->getMinimumHeight() == t::windowCompactHeight);
        CHECK (constrainer->getMaximumHeight() == t::windowCompactHeight);

        juce::Rectangle<int> arbitraryBounds (0, 0, 1200, 800);
        constrainer->checkBounds (arbitraryBounds,
                                  editor->getBounds(),
                                  juce::Rectangle<int> (0, 0, 3840, 2160),
                                  false, false, true, true);
        CHECK (arbitraryBounds.getWidth() == t::windowCompactWidth);
        CHECK (arbitraryBounds.getHeight() == t::windowCompactHeight);
    }
}

TEST_CASE ("WindowView toggle updates dimensions layout and subcomponent visibility", "[ui][window][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& gainView   = editor->getGainResponseView();
    auto& meters     = editor->getLevelMeters();
    auto& modeButton = editor->getViewModeButton();

    SECTION ("Full view shows LevelMeters and playhead bar pill with Full layout bounds")
    {
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (gainView.isVisible());
        CHECK (meters.isVisible());
        CHECK (gainView.isBarPillVisible());

        const int expectedCenterX = t::windowPadding + t::fullLeftPanelWidth + t::panelGap;
        const int expectedRightX  = expectedCenterX + t::fullCenterPanelWidth + t::panelGap;

        CHECK (gainView.getBounds() == juce::Rectangle<int> (expectedCenterX, t::fullContentTop,
                                                             t::fullCenterPanelWidth, t::fullPanelRowHeight));
        CHECK (meters.getBounds() == juce::Rectangle<int> (expectedRightX, t::fullContentTop,
                                                           t::fullRightPanelWidth, t::fullPanelRowHeight));
    }

    SECTION ("Toggling to Compact hides LevelMeters and bar pill and applies Compact layout")
    {
        REQUIRE (modeButton.onClick != nullptr);
        modeButton.onClick();

        CHECK (editor->isCompactView());
        CHECK (processor.isCompactView());
        CHECK (editor->getWidth() == t::windowCompactWidth);
        CHECK (editor->getHeight() == t::windowCompactHeight);

        CHECK (gainView.isVisible());
        CHECK_FALSE (meters.isVisible());
        CHECK_FALSE (gainView.isBarPillVisible());

        CHECK (gainView.getBounds() == juce::Rectangle<int> (t::windowPaddingCompact,
                                                             t::compactContentTop,
                                                             t::compactLeftPanelWidth,
                                                             t::compactPanelRowHeight));

        // Render offscreen en modo Compact sin fallos
        juce::Image compactImg (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true,
                                juce::SoftwareImageType {});
        juce::Graphics g (compactImg);
        editor->paintEntireComponent (g, false);

        // Segundo clic en el boton de cabecera devuelve a Full (960x600)
        modeButton.onClick();

        CHECK_FALSE (editor->isCompactView());
        CHECK_FALSE (processor.isCompactView());
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (meters.isVisible());
        CHECK (gainView.isBarPillVisible());
    }
}

TEST_CASE ("WindowView mode persists across editor close and reopen", "[ui][window][persistence][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;

    {
        std::unique_ptr<juce::AudioProcessorEditor> firstEditorBase (processor.createEditor());
        auto* firstEditor = dynamic_cast<UmbralDuckEditor*> (firstEditorBase.get());
        REQUIRE (firstEditor != nullptr);

        CHECK_FALSE (firstEditor->isCompactView());
        firstEditor->setCompactView (true);
        CHECK (firstEditor->isCompactView());
        CHECK (processor.isCompactView());
    } // Se cierra el primer editor

    {
        std::unique_ptr<juce::AudioProcessorEditor> secondEditorBase (processor.createEditor());
        auto* secondEditor = dynamic_cast<UmbralDuckEditor*> (secondEditorBase.get());
        REQUIRE (secondEditor != nullptr);

        CHECK (secondEditor->isCompactView());
        CHECK (secondEditor->getWidth() == t::windowCompactWidth);
        CHECK (secondEditor->getHeight() == t::windowCompactHeight);
        CHECK_FALSE (secondEditor->getLevelMeters().isVisible());
        CHECK_FALSE (secondEditor->getGainResponseView().isBarPillVisible());
    }
}

TEST_CASE ("WindowView XML state serialization roundtrip restores Compact and Full modes", "[ui][window][state][t11]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    SECTION ("Compact view state survives getStateInformation and setStateInformation")
    {
        UmbralDuckProcessor sourceProcessor;
        sourceProcessor.setCompactView (true);
        REQUIRE (sourceProcessor.isCompactView());

        juce::MemoryBlock stateBlock;
        sourceProcessor.getStateInformation (stateBlock);
        REQUIRE (stateBlock.getSize() > 0);

        // Verifica que el XML serializado contiene el atributo isCompactView
        std::unique_ptr<juce::XmlElement> xml (
            juce::AudioProcessor::getXmlFromBinary (stateBlock.getData(), static_cast<int> (stateBlock.getSize())));
        REQUIRE (xml != nullptr);
        CHECK (xml->hasAttribute (UmbralDuckProcessor::kIsCompactViewProperty.toString()));
        CHECK (xml->getBoolAttribute (UmbralDuckProcessor::kIsCompactViewProperty.toString(), false) == true);

        UmbralDuckProcessor restoredProcessor;
        CHECK_FALSE (restoredProcessor.isCompactView());

        restoredProcessor.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));
        CHECK (restoredProcessor.isCompactView());

        std::unique_ptr<juce::AudioProcessorEditor> editorBase (restoredProcessor.createEditor());
        auto* editor = dynamic_cast<UmbralDuckEditor*> (editorBase.get());
        REQUIRE (editor != nullptr);

        CHECK (editor->isCompactView());
        CHECK (editor->getWidth() == t::windowCompactWidth);
        CHECK (editor->getHeight() == t::windowCompactHeight);
        CHECK_FALSE (editor->getLevelMeters().isVisible());
        CHECK_FALSE (editor->getGainResponseView().isBarPillVisible());
    }

    SECTION ("setStateInformation updates an already open editor immediately")
    {
        UmbralDuckProcessor compactSource;
        compactSource.setCompactView (true);
        juce::MemoryBlock compactBlock;
        compactSource.getStateInformation (compactBlock);

        UmbralDuckProcessor fullSource;
        fullSource.setCompactView (false);
        juce::MemoryBlock fullBlock;
        fullSource.getStateInformation (fullBlock);

        UmbralDuckProcessor targetProcessor;
        std::unique_ptr<juce::AudioProcessorEditor> editorBase (targetProcessor.createEditor());
        auto* editor = dynamic_cast<UmbralDuckEditor*> (editorBase.get());
        REQUIRE (editor != nullptr);
        CHECK_FALSE (editor->isCompactView());

        // El DAW carga un preset/sesion en modo Compact mientras la ventana del editor esta abierta
        targetProcessor.setStateInformation (compactBlock.getData(), static_cast<int> (compactBlock.getSize()));
        CHECK (targetProcessor.isCompactView());
        CHECK (editor->isCompactView());
        CHECK (editor->getWidth() == t::windowCompactWidth);
        CHECK (editor->getHeight() == t::windowCompactHeight);
        CHECK_FALSE (editor->getLevelMeters().isVisible());
        CHECK_FALSE (editor->getGainResponseView().isBarPillVisible());

        // El DAW restaura estado Full con el editor aun abierto
        targetProcessor.setStateInformation (fullBlock.getData(), static_cast<int> (fullBlock.getSize()));
        CHECK_FALSE (targetProcessor.isCompactView());
        CHECK_FALSE (editor->isCompactView());
        CHECK (editor->getWidth() == t::windowFullWidth);
        CHECK (editor->getHeight() == t::windowFullHeight);
        CHECK (editor->getLevelMeters().isVisible());
        CHECK (editor->getGainResponseView().isBarPillVisible());
    }
}

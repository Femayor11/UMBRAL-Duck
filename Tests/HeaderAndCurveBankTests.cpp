#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters/ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UI/CurveBankGrid.h"
#include "UI/DesignTokens.h"
#include "UI/HeaderBar.h"

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

TEST_CASE ("HeaderBar and CurveBankGrid default state matches APVTS defaults", "[ui][header][curvebank][t13]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;

    UmbralDuckProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& header    = editor->getHeaderBar();
    auto& curveBank = editor->getCurveBankGrid();

    SECTION ("HeaderBar defaults to RATE 1/4 SYNC active BPM 120.0 and BYPASS off")
    {
        CHECK (header.getSelectedRateIndex() == 1);
        CHECK_FALSE (header.getRateButton (0).getToggleState());
        CHECK (header.getRateButton (1).getToggleState());
        CHECK_FALSE (header.getRateButton (2).getToggleState());
        CHECK_FALSE (header.getRateButton (3).getToggleState());

        CHECK (header.getRateButton (0).getButtonText() == "1/8");
        CHECK (header.getRateButton (1).getButtonText() == "1/4");
        CHECK (header.getRateButton (2).getButtonText() == "1/2");
        CHECK (header.getRateButton (3).getButtonText() == "1/1");

        CHECK (header.isSyncEnabled());
        CHECK (header.getSyncButton().getToggleState());
        CHECK_FALSE (header.getBpmSlider().isEnabled());
        CHECK (header.getBpmReadoutText() == "120.0");

        CHECK_FALSE (header.isBypassEnabled());
        CHECK_FALSE (header.getBypassButton().getToggleState());
        CHECK (header.getBypassButton().isVisible());
        CHECK (header.areSecondaryLabelsVisible());
    }

    SECTION ("CurveBankGrid defaults to 04 Classic with 16 valid cells and slot 16 dashed")
    {
        CHECK (curveBank.isVisible());
        CHECK (curveBank.getSelectedCurveIndex() == 3);
        CHECK (curveBank.getHoveredCellIndex() == -1);
        CHECK (curveBank.getHeaderTitleText() == "CURVE BANK");
        CHECK (curveBank.getCustomNoteText().contains ("Custom: editor coming in a later version"));

        for (int i = 0; i < umbral::ui::CurveBankGrid::kNumCells; ++i)
        {
            const auto cellBounds = curveBank.getCellBounds (i);
            CHECK (cellBounds.getWidth() > 45.0f);
            CHECK_THAT (cellBounds.getHeight(), WithinAbs (static_cast<float> (t::curveBankCellHeight), 1.0f));
            CHECK_FALSE (curveBank.getCellPreviewPath (i).isEmpty());
            CHECK (curveBank.isCellDashed (i) == (i == 15));
        }

        CHECK (umbral::ui::CurveBankGrid::getCellNumberText (0)  == "01");
        CHECK (umbral::ui::CurveBankGrid::getCellNameText (0)    == "Click Duck");
        CHECK (umbral::ui::CurveBankGrid::getCellNumberText (3)  == "04");
        CHECK (umbral::ui::CurveBankGrid::getCellNameText (3)    == "Classic");
        CHECK (umbral::ui::CurveBankGrid::getCellNumberText (10) == "11");
        CHECK (umbral::ui::CurveBankGrid::getCellNameText (10)   == "Trance Gate");
        CHECK (umbral::ui::CurveBankGrid::getCellNumberText (15) == "16");
        CHECK (umbral::ui::CurveBankGrid::getCellNameText (15)   == "Custom");
    }
}

TEST_CASE ("HeaderBar RATE SYNC BPM and BYPASS interactions update APVTS", "[ui][header][t13]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;
    auto& apvts = processor.getAPVTS();

    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& header = editor->getHeaderBar();

    SECTION ("Clicking each RATE button updates APVTS and keeps mutually exclusive toggle state")
    {
        for (int target = 0; target < umbral::ui::HeaderBar::kNumRates; ++target)
        {
            REQUIRE (header.getRateButton (target).onClick != nullptr);
            header.getRateButton (target).onClick();

            CHECK (header.getSelectedRateIndex() == target);
            CHECK (juce::roundToInt (apvts.getRawParameterValue (ids::rate)->load()) == target);

            for (int i = 0; i < umbral::ui::HeaderBar::kNumRates; ++i)
                CHECK (header.getRateButton (i).getToggleState() == (i == target));
        }

        // Clic repetido sobre el mismo boton mantiene su estado activo
        header.getRateButton (3).onClick();
        CHECK (header.getRateButton (3).getToggleState());
    }

    SECTION ("Toggling SYNC enables manual BPM editing and propagates BPM to APVTS")
    {
        REQUIRE (header.getSyncButton().onClick != nullptr);
        header.getSyncButton().onClick();

        CHECK_FALSE (header.isSyncEnabled());
        CHECK_FALSE (header.getSyncButton().getToggleState());
        CHECK (apvts.getRawParameterValue (ids::sync)->load() < 0.5f);
        CHECK (header.getBpmSlider().isEnabled());

        header.getBpmSlider().setValue (140.5, juce::sendNotificationSync);
        CHECK_THAT (apvts.getRawParameterValue (ids::bpm)->load(), WithinAbs (140.5f, 0.15f));
        CHECK (header.getBpmReadoutText() == "140.5");

        // Reactivar SYNC vuelve a bloquear el control manual de BPM
        header.getSyncButton().onClick();
        CHECK (header.isSyncEnabled());
        CHECK (header.getSyncButton().getToggleState());
        CHECK (apvts.getRawParameterValue (ids::sync)->load() > 0.5f);
        CHECK_FALSE (header.getBpmSlider().isEnabled());
    }

    SECTION ("Toggling BYPASS button updates APVTS bypass parameter")
    {
        REQUIRE (header.getBypassButton().onClick != nullptr);
        header.getBypassButton().onClick();

        CHECK (header.isBypassEnabled());
        CHECK (header.getBypassButton().getToggleState());
        CHECK (apvts.getRawParameterValue (ids::bypass)->load() > 0.5f);

        header.getBypassButton().onClick();
        CHECK_FALSE (header.isBypassEnabled());
        CHECK_FALSE (header.getBypassButton().getToggleState());
        CHECK (apvts.getRawParameterValue (ids::bypass)->load() < 0.5f);
    }

    SECTION ("Programmatic APVTS changes update HeaderBar controls and BPM formatting")
    {
        setNormalizedParam (apvts, ids::rate,   0.0f); // 1/8
        setNormalizedParam (apvts, ids::sync,   0.0f); // Free
        setNormalizedParam (apvts, ids::bpm,    174.2f);
        setNormalizedParam (apvts, ids::bypass, 1.0f);

        CHECK (header.getSelectedRateIndex() == 0);
        CHECK (header.getRateButton (0).getToggleState());
        CHECK_FALSE (header.isSyncEnabled());
        CHECK (header.getBpmSlider().isEnabled());
        CHECK (header.getBpmReadoutText() == "174.2");
        CHECK (header.isBypassEnabled());
        CHECK (header.getBypassButton().getToggleState());

        CHECK (umbral::ui::HeaderBar::formatBpm (10.0) == "40.0");
        CHECK (umbral::ui::HeaderBar::formatBpm (300.0) == "240.0");
        CHECK (umbral::ui::HeaderBar::formatBpm (std::numeric_limits<double>::quiet_NaN()) == "120.0");
    }
}

TEST_CASE ("CurveBankGrid cell selection and bidirectional sync with ControlStrip and APVTS", "[ui][curvebank][t13]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;
    auto& apvts = processor.getAPVTS();

    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
    auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    auto& curveBank = editor->getCurveBankGrid();
    auto& strip     = editor->getControlStrip();

    SECTION ("Selecting each cell in CurveBankGrid updates APVTS and ControlStrip ComboBox")
    {
        for (int i = 0; i < umbral::ui::CurveBankGrid::kNumCells; ++i)
        {
            const auto cellCentre = curveBank.getCellBounds (i).getCentre();
            CHECK (curveBank.getCellIndexAtPosition (cellCentre) == i);

            curveBank.selectCurveIndex (i);

            CHECK (curveBank.getSelectedCurveIndex() == i);
            CHECK (juce::roundToInt (apvts.getRawParameterValue (ids::curve)->load()) == i);
            CHECK (strip.getCurveCombo().getSelectedItemIndex() == i);
        }
    }

    SECTION ("Changing curve in ControlStrip ComboBox or APVTS updates CurveBankGrid selection")
    {
        strip.getCurveCombo().setSelectedItemIndex (6, juce::sendNotificationSync); // 07 Logarithmic
        CHECK (curveBank.getSelectedCurveIndex() == 6);
        CHECK (juce::roundToInt (apvts.getRawParameterValue (ids::curve)->load()) == 6);

        setNormalizedParam (apvts, ids::curve, 15.0f); // 16 Custom
        CHECK (curveBank.getSelectedCurveIndex() == 15);
        CHECK (strip.getCurveCombo().getSelectedItemIndex() == 15);
        CHECK (curveBank.isCellDashed (15));
    }
}

TEST_CASE ("HeaderBar and CurveBankGrid Full vs Compact adaptability and offscreen render", "[ui][header][curvebank][window][t13]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace t = umbral::tokens;
    namespace ids = umbral::params::rawIDs;

    UmbralDuckProcessor processor;

    {
        std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditor());
        auto* editor = dynamic_cast<UmbralDuckEditor*> (baseEditor.get());
        REQUIRE (editor != nullptr);

        auto& header    = editor->getHeaderBar();
        auto& curveBank = editor->getCurveBankGrid();

        // 1. Vista Full (960 x 600)
        CHECK_FALSE (editor->isCompactView());
        CHECK_FALSE (header.isCompactMode());
        CHECK (header.getBounds() == juce::Rectangle<int> (0, 0, t::windowFullWidth, t::fullHeaderDividerY));
        CHECK (header.getBypassButton().isVisible());
        CHECK (header.areSecondaryLabelsVisible());

        CHECK (curveBank.isVisible());
        CHECK (curveBank.getBounds() == juce::Rectangle<int> (t::windowPadding,
                                                              t::fullBottomRowTop,
                                                              t::fullBottomRowWidth,
                                                              t::fullBottomRowHeight));

        // Selecciona celda 16 Custom y renderiza en Full
        curveBank.selectCurveIndex (15);
        header.selectRateIndex (2); // 1/2
        header.setSyncEnabled (false);
        header.getBpmSlider().setValue (128.4, juce::sendNotificationSync);

        juce::Image fullCanvas (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true,
                                juce::SoftwareImageType {});
        {
            juce::Graphics g (fullCanvas);
            editor->paintEntireComponent (g, false);
        }

        // 2. Alternar a vista Compact (560 x 360) desde el boton de cabecera
        header.getViewModeButton().onClick();

        CHECK (editor->isCompactView());
        CHECK (header.isCompactMode());
        CHECK (header.getBounds() == juce::Rectangle<int> (0, 0, t::windowCompactWidth, t::compactHeaderDividerY));
        CHECK_FALSE (header.getBypassButton().isVisible());
        CHECK_FALSE (header.areSecondaryLabelsVisible());
        CHECK_FALSE (curveBank.isVisible());

        juce::Image compactCanvas (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true,
                                   juce::SoftwareImageType {});
        {
            juce::Graphics g (compactCanvas);
            editor->paintEntireComponent (g, false);
        }
    }

    // 3. Reapertura del editor tras destruccion: verifica sincronizacion con APVTS sin leaks
    {
        std::unique_ptr<juce::AudioProcessorEditor> reopenedBase (processor.createEditor());
        auto* reopened = dynamic_cast<UmbralDuckEditor*> (reopenedBase.get());
        REQUIRE (reopened != nullptr);

        CHECK (reopened->isCompactView());
        CHECK (reopened->getHeaderBar().getSelectedRateIndex() == 2);
        CHECK_FALSE (reopened->getHeaderBar().isSyncEnabled());
        CHECK (reopened->getHeaderBar().getBpmReadoutText() == "128.4");
        CHECK (reopened->getCurveBankGrid().getSelectedCurveIndex() == 15);
        CHECK (juce::roundToInt (processor.getAPVTS().getRawParameterValue (ids::curve)->load()) == 15);
    }
}

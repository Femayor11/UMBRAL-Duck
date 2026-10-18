#pragma once

#include "DSP/CurveBank.h"
#include "DSP/CustomCurve.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace umbral::ui
{
    // Banco inferior de 16 celdas interactivas de curvas en matriz 2x8 (docs/DESIGN.md §4, §5 y §6).
    // Fila 0: ranuras 0..7 (sidechain frontal). Fila 1: ranuras 8..15 (gating, ritmicas y 15 Custom).
    // En vista Full ocupa 748 x 149 px (x = 16, y = 375); en vista Compact permanece oculto.
    // Enlazado bidireccionalmente al parametro "curve" de APVTS.
    class CurveBankGrid final : public juce::Component
    {
    public:
        static constexpr int kNumCells       = dsp::numCurves;
        static constexpr int kNumRows        = 2;
        static constexpr int kNumCols        = 8;
        static constexpr int kPreviewSamples = 96;

        struct CellLayout
        {
            juce::Rectangle<float> bounds;
            juce::Rectangle<float> previewArea;
            juce::Path             previewPath;
            bool                   isDashed = false;
        };

        explicit CurveBankGrid (juce::AudioProcessorValueTreeState& apvts);
        ~CurveBankGrid() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        void mouseMove (const juce::MouseEvent& event) override;
        void mouseDown (const juce::MouseEvent& event) override;
        void mouseExit (const juce::MouseEvent& event) override;

        void selectCurveIndex (int index);
        [[nodiscard]] int getSelectedCurveIndex() const noexcept { return selectedCurveIndex; }
        [[nodiscard]] int getHoveredCellIndex() const noexcept { return hoveredCellIndex; }

        void setCustomCurveData (const dsp::CustomCurveData& curve);
        [[nodiscard]] const dsp::CustomCurveData& getCustomCurveData() const noexcept { return customCurve; }

        [[nodiscard]] int getCellIndexAtPosition (juce::Point<float> position) const noexcept;
        [[nodiscard]] juce::Rectangle<float> getCellBounds (int index) const noexcept;
        [[nodiscard]] const juce::Path& getCellPreviewPath (int index) const noexcept;
        [[nodiscard]] bool isCellDashed (int index) const noexcept;

        [[nodiscard]] const juce::String& getHeaderTitleText() const noexcept { return headerTitleText; }

    private:
        void updateCurveFromParameter (float rawIndexValue);
        void rebuildCellPaths();

        dsp::CustomCurveData customCurve = dsp::CustomCurveData::createDefault();
        int selectedCurveIndex = dsp::defaultCurveIndex; // Default: 04 Classic
        int hoveredCellIndex   = -1;

        juce::Font sectionHeaderFont;
        juce::String headerTitleText;
        juce::Rectangle<float> headerTitleArea;

        std::array<CellLayout, kNumCells> cells {};

        // Declarado despues del estado UI para destruirse antes que el componente.
        std::unique_ptr<juce::ParameterAttachment> curveAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveBankGrid)
    };
} // namespace umbral::ui

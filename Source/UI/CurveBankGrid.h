#pragma once

#include "DSP/CurveBank.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace umbral::ui
{
    // Banco inferior de 16 celdas interactivas de curvas (docs/DESIGN.md §4, §5 y §6).
    // En vista Full ocupa 920 x 114 px (x = 20, y = 466); en vista Compact permanece oculto.
    // Enlazado bidireccionalmente al parametro "curve" de APVTS.
    class CurveBankGrid final : public juce::Component
    {
    public:
        static constexpr int kNumCells        = dsp::numCurves;
        static constexpr int kPreviewSamples  = 96;

        struct CellLayout
        {
            juce::Rectangle<float> bounds;
            juce::Rectangle<float> previewArea;
            juce::Rectangle<float> numberArea;
            juce::Rectangle<float> nameArea;
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

        [[nodiscard]] int getCellIndexAtPosition (juce::Point<float> position) const noexcept;
        [[nodiscard]] juce::Rectangle<float> getCellBounds (int index) const noexcept;
        [[nodiscard]] const juce::Path& getCellPreviewPath (int index) const noexcept;
        [[nodiscard]] bool isCellDashed (int index) const noexcept;

        [[nodiscard]] static juce::String getCellNumberText (int index);
        [[nodiscard]] static juce::String getCellNameText (int index);
        [[nodiscard]] const juce::String& getHeaderTitleText() const noexcept { return headerTitleText; }
        [[nodiscard]] const juce::String& getCustomNoteText() const noexcept { return customNoteText; }

    private:
        void updateCurveFromParameter (float rawIndexValue);
        void rebuildCellPaths();

        int selectedCurveIndex = dsp::defaultCurveIndex; // Default: 04 Classic
        int hoveredCellIndex   = -1;

        juce::Font sectionHeaderFont;
        juce::Font customNoteFont;
        juce::Font slotNumberFont;
        juce::Font curveNameFont;

        juce::String headerTitleText;
        juce::String customNoteText;

        juce::Rectangle<float> headerTitleArea;
        juce::Rectangle<float> customNoteArea;

        std::array<CellLayout, kNumCells> cells {};

        // Declarado despues del estado UI para destruirse antes que el componente.
        std::unique_ptr<juce::ParameterAttachment> curveAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveBankGrid)
    };
} // namespace umbral::ui

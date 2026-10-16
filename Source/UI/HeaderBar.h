#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

namespace umbral::ui
{
    // Cabecera superior de UMBRAL Duck (docs/DESIGN.md §2, §4, §5 y §6).
    // Contiene:
    //  - Logotipo vectorial y tipografico "UMBRAL duck"
    //  - Selector segmentado RATE (1/8, 1/4, 1/2, 1/1) enlazado a APVTS
    //  - Readout/control de BPM (40.0..240.0) y boton SYNC (Locked to DAW / Free)
    //  - Boton BYPASS (solo en vista Full) y boton de alternancia Full/Compact
    class HeaderBar final : public juce::Component
    {
    public:
        static constexpr int kNumRates = 4;

        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

        explicit HeaderBar (juce::AudioProcessorValueTreeState& apvts,
                            std::function<void()> onToggleViewMode = {});
        ~HeaderBar() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        void setCompactMode (bool compact);
        [[nodiscard]] bool isCompactMode() const noexcept { return compactMode; }
        [[nodiscard]] bool areSecondaryLabelsVisible() const noexcept { return ! compactMode; }

        void selectRateIndex (int index);
        [[nodiscard]] int getSelectedRateIndex() const noexcept { return selectedRateIndex; }

        void setSyncEnabled (bool enabled);
        [[nodiscard]] bool isSyncEnabled() const noexcept { return syncEnabled; }

        void setBypassEnabled (bool enabled);
        [[nodiscard]] bool isBypassEnabled() const noexcept { return bypassEnabled; }

        [[nodiscard]] static juce::String formatBpm (double bpmValue);
        [[nodiscard]] const juce::String& getBpmReadoutText() const noexcept { return bpmReadoutText; }

        [[nodiscard]] juce::TextButton& getRateButton (int index) noexcept;
        [[nodiscard]] const juce::TextButton& getRateButton (int index) const noexcept;
        [[nodiscard]] const std::array<juce::TextButton, kNumRates>& getRateButtons() const noexcept { return rateButtons; }

        [[nodiscard]] juce::Slider& getBpmSlider() noexcept { return bpmSlider; }
        [[nodiscard]] const juce::Slider& getBpmSlider() const noexcept { return bpmSlider; }

        [[nodiscard]] juce::TextButton& getSyncButton() noexcept { return syncButton; }
        [[nodiscard]] const juce::TextButton& getSyncButton() const noexcept { return syncButton; }

        [[nodiscard]] juce::TextButton& getBypassButton() noexcept { return bypassButton; }
        [[nodiscard]] const juce::TextButton& getBypassButton() const noexcept { return bypassButton; }

        [[nodiscard]] juce::TextButton& getViewModeButton() noexcept { return viewModeButton; }
        [[nodiscard]] const juce::TextButton& getViewModeButton() const noexcept { return viewModeButton; }

    private:
        void updateRateFromParameter (float rawIndexValue);
        void updateSyncFromParameter (float rawBoolValue);
        void updateBypassFromParameter (float rawBoolValue);
        void updateBpmReadout();
        void rebuildLogoPaths();

        bool compactMode   = false;
        int  selectedRateIndex = 1;
        bool syncEnabled   = true;
        bool bypassEnabled = false;

        std::function<void()> toggleViewCallback;

        juce::Font brandBoldFont;
        juce::Font brandLightFont;
        juce::Font sectionHeaderFont;
        juce::Font bpmValueFont;

        juce::Rectangle<float> logoIconArea;
        juce::Rectangle<float> logoBoldTextArea;
        juce::Rectangle<float> logoLightTextArea;
        juce::Rectangle<float> rateLabelArea;
        juce::Rectangle<float> bpmLabelArea;
        juce::Rectangle<float> bpmValueArea;

        juce::Path logoArchPath;
        juce::Path logoDuckPath;

        juce::String bpmReadoutText { "120.0" };

        std::array<juce::TextButton, kNumRates> rateButtons {
            juce::TextButton { "1/8" },
            juce::TextButton { "1/4" },
            juce::TextButton { "1/2" },
            juce::TextButton { "1/1" }
        };

        juce::Slider     bpmSlider;
        juce::TextButton syncButton     { "SYNC" };
        juce::TextButton bypassButton   { "BYPASS" };
        juce::TextButton viewModeButton { "COMPACT" };

        // Declarados despues de los controles UI para garantizar orden de destruccion seguro sin leaks.
        std::unique_ptr<juce::ParameterAttachment> rateAttachment;
        std::unique_ptr<SliderAttachment>          bpmAttachment;
        std::unique_ptr<juce::ParameterAttachment> syncAttachment;
        std::unique_ptr<juce::ParameterAttachment> bypassAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
    };
} // namespace umbral::ui

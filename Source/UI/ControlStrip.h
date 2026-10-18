#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace umbral::ui
{
    // Selector segmentado CHANNELS (Stereo / Mono) situado en la cabecera del panel derecho
    // sobre LevelMetersComponent (T-12, docs/DESIGN.md §5 y §6).
    class ChannelsSelector final : public juce::Component
    {
    public:
        explicit ChannelsSelector (juce::AudioProcessorValueTreeState& apvts);
        ~ChannelsSelector() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        [[nodiscard]] juce::TextButton& getStereoButton() noexcept { return stereoButton; }
        [[nodiscard]] const juce::TextButton& getStereoButton() const noexcept { return stereoButton; }
        [[nodiscard]] juce::TextButton& getMonoButton() noexcept { return monoButton; }
        [[nodiscard]] const juce::TextButton& getMonoButton() const noexcept { return monoButton; }
        [[nodiscard]] int getSelectedChannelIndex() const noexcept { return selectedChannelIndex; }

    private:
        void selectChannelIndex (int index);
        void updateButtonsFromParameter (float rawIndexValue);

        juce::Font headerFont;
        juce::String headerLabel;
        juce::Rectangle<float> headerArea;

        juce::TextButton stereoButton { "Stereo" };
        juce::TextButton monoButton   { "Mono" };
        int selectedChannelIndex = 0;

        // Declarado despues de los botones para destruirse antes que ellos.
        std::unique_ptr<juce::ParameterAttachment> channelsAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelsSelector)
    };

    // Control giratorio personalizado que soporta Host Context Menu (VST3) al hacer
    // clic derecho para exponer 'Create automation clip', enlace a controladores y
    // fallback 'Reset to default'.
    class ParameterKnobSlider : public juce::Slider
    {
    public:
        ParameterKnobSlider (juce::RangedAudioParameter* param = nullptr,
                             juce::AudioProcessorEditor* editorToUse = nullptr);
        ~ParameterKnobSlider() override = default;

        void setAssociatedParameter (juce::RangedAudioParameter* param) noexcept { parameter = param; }
        [[nodiscard]] juce::RangedAudioParameter* getAssociatedParameter() const noexcept { return parameter; }

        void setEditor (juce::AudioProcessorEditor* editorToUse) noexcept { editor = editorToUse; }
        [[nodiscard]] juce::AudioProcessorEditor* getEditor() const noexcept;

        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp (const juce::MouseEvent& e) override;

        [[nodiscard]] juce::PopupMenu buildContextMenu() const;
        void showContextMenu();
        void resetToDefault();

    private:
        juce::RangedAudioParameter* parameter = nullptr;
        juce::AudioProcessorEditor* editor = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterKnobSlider)
    };

    // Panel de controles MIX, SHIFT y SLOPE (T-12, docs/DESIGN.md §5 y §6).
    // En vista Full (193 x 368 px) se ubica a la izquierda mostrando MIX arriba y SHIFT/SLOPE debajo.
    // En vista Compact (160 x 279 px) se ubica a la derecha mostrando MIX.
    class ControlStrip final : public juce::Component
    {
    public:
        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

        explicit ControlStrip (juce::AudioProcessorValueTreeState& apvts,
                               juce::AudioProcessorEditor* editor = nullptr);
        ~ControlStrip() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        void setCompactMode (bool compact);
        [[nodiscard]] bool isCompactMode() const noexcept { return compactMode; }

        [[nodiscard]] static juce::String formatPercentage (double value);

        [[nodiscard]] ParameterKnobSlider& getMixSlider() noexcept { return mixSlider; }
        [[nodiscard]] const ParameterKnobSlider& getMixSlider() const noexcept { return mixSlider; }
        [[nodiscard]] ParameterKnobSlider& getShiftSlider() noexcept { return shiftSlider; }
        [[nodiscard]] const ParameterKnobSlider& getShiftSlider() const noexcept { return shiftSlider; }
        [[nodiscard]] ParameterKnobSlider& getSlopeSlider() noexcept { return slopeSlider; }
        [[nodiscard]] const ParameterKnobSlider& getSlopeSlider() const noexcept { return slopeSlider; }

        [[nodiscard]] const juce::String& getMixReadoutText() const noexcept { return mixReadoutText; }
        [[nodiscard]] const juce::String& getShiftReadoutText() const noexcept { return shiftReadoutText; }
        [[nodiscard]] const juce::String& getSlopeReadoutText() const noexcept { return slopeReadoutText; }

    private:
        static void configureRotaryKnob (juce::Slider& slider);
        void updateReadouts();

        bool compactMode = false;

        juce::Font sectionHeaderFont;
        juce::Font mixReadoutFont;
        juce::Font secondaryReadoutFont;

        juce::Rectangle<float> panelBounds;
        juce::Rectangle<float> mixHeaderArea;
        juce::Rectangle<float> mixReadoutArea;
        juce::Rectangle<float> shiftHeaderArea;
        juce::Rectangle<float> shiftReadoutArea;
        juce::Rectangle<float> slopeHeaderArea;
        juce::Rectangle<float> slopeReadoutArea;
        float mixDividerY = 0.0f;

        juce::String mixReadoutText;
        juce::String shiftReadoutText;
        juce::String slopeReadoutText;

        ParameterKnobSlider mixSlider;
        ParameterKnobSlider shiftSlider;
        ParameterKnobSlider slopeSlider;

        // Declarados despues de los controles UI para garantizar orden de destruccion seguro sin leaks.
        std::unique_ptr<SliderAttachment> mixAttachment;
        std::unique_ptr<SliderAttachment> shiftAttachment;
        std::unique_ptr<SliderAttachment> slopeAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlStrip)
    };
} // namespace umbral::ui

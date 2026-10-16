#pragma once

#include "Common/UiStateSnapshot.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace umbral::ui
{
    // Medidores verticales IN (Bone) y OUT (Signal) con escala dB central (T-10, DESIGN.md §3 y §5).
    // Lee el UiStateSnapshot (lock-free) a 60 Hz; paint() no reserva memoria.
    class LevelMetersComponent final : public juce::Component,
                                       private juce::Timer
    {
    public:
        static constexpr int refreshRateHz = 60;

        explicit LevelMetersComponent (const UiStateSnapshot& snapshotToRead);
        ~LevelMetersComponent() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        // Cuerpo del timer: lee el snapshot y solicita repaint si el nivel cambio.
        void refresh();

        // Nivel en dB -> fraccion de altura [0, 1] sobre la escala [meterMinDb, 0].
        [[nodiscard]] static float levelDbToFraction (float db) noexcept;

        [[nodiscard]] float getInputLevelDb() const noexcept { return inputDb; }
        [[nodiscard]] float getOutputLevelDb() const noexcept { return outputDb; }
        [[nodiscard]] juce::Rectangle<float> getBarArea (bool isInput) const noexcept
        {
            return isInput ? inputBar : outputBar;
        }

        // Reserva la franja superior del panel derecho para el bloque CHANNELS y su separador Hairline (DESIGN.md §5).
        void setChannelsSectionOffset (int offsetPx);
        [[nodiscard]] int getChannelsSectionOffset() const noexcept { return channelsSectionOffset; }

    private:
        void timerCallback() override { refresh(); }

        void drawBar (juce::Graphics&, const juce::Rectangle<float>& bar, float levelDb,
                      juce::Colour barColour) const;

        const UiStateSnapshot& snapshot;

        float inputDb  = UiStateSnapshot::kMinDb;
        float outputDb = UiStateSnapshot::kMinDb;
        int channelsSectionOffset = 0;

        juce::Rectangle<float> panelBounds;
        juce::Rectangle<float> headerArea;
        juce::Rectangle<float> inputBar;
        juce::Rectangle<float> outputBar;
        juce::Rectangle<float> scaleColumn;
        juce::Rectangle<float> inputLabelArea;
        juce::Rectangle<float> outputLabelArea;

        juce::Font headerFont;
        juce::Font channelFont;
        juce::Font scaleFont;
        juce::String headerLabel;
        juce::String inputLabel;
        juce::String outputLabel;

        static constexpr std::array<float, 5> scaleDb { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f };
        std::array<juce::String, 5> scaleLabels;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMetersComponent)
    };
}

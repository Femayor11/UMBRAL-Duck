#pragma once

#include "Common/UiStateSnapshot.h"
#include "UI/GainResponseGeometry.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <atomic>

namespace umbral::ui
{
    // Panel GAIN RESPONSE (T-10): reticula, curva efectiva con relleno, playhead y pastilla de compas.
    // Lee el UiStateSnapshot (lock-free) y los parametros del APVTS (atomics) desde un Timer a 60 Hz.
    // Los Path y los textos se reconstruyen en refresh()/resized(); paint() no reserva memoria.
    class GainResponseView final : public juce::Component,
                                   private juce::Timer
    {
    public:
        // Punteros a los valores crudos del APVTS (curve = indice, mix/shift/slope = 0..100).
        // Un puntero nulo usa el valor por defecto del parametro.
        struct ParameterSource
        {
            const std::atomic<float>* curve = nullptr;
            const std::atomic<float>* mix   = nullptr;
            const std::atomic<float>* shift = nullptr;
            const std::atomic<float>* slope = nullptr;
        };

        static constexpr int refreshRateHz = 60;

        GainResponseView (const UiStateSnapshot& snapshotToRead, ParameterSource parameters);
        ~GainResponseView() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        // La vista Compact oculta la pastilla "1.2.3".
        void setBarPillVisible (bool shouldBeVisible);
        [[nodiscard]] bool isBarPillVisible() const noexcept { return barPillVisible; }

        // Cuerpo del timer: lee el snapshot y los parametros, y solicita repaint si algo cambio.
        void refresh();

        [[nodiscard]] juce::Rectangle<float> getPlotArea() const noexcept { return plotArea; }
        [[nodiscard]] const juce::Path& getCurvePath() const noexcept { return curvePath; }
        [[nodiscard]] const juce::Path& getFillPath() const noexcept { return fillPath; }
        [[nodiscard]] const juce::String& getGainText() const noexcept { return gainText; }
        [[nodiscard]] const juce::String& getBarBeatText() const noexcept { return barBeatText; }
        [[nodiscard]] geometry::CurveParams getCurveParams() const noexcept { return curveParams; }
        [[nodiscard]] float getPlayheadPhase() const noexcept { return playheadPhase; }

    private:
        void timerCallback() override { refresh(); }

        void updateLayout();
        void rebuildPaths();
        [[nodiscard]] geometry::CurveParams readCurveParams() const noexcept;

        const UiStateSnapshot& snapshot;
        ParameterSource parameters;

        // Geometria
        juce::Rectangle<float> panelBounds;
        juce::Rectangle<float> headerArea;
        juce::Rectangle<float> plotArea;
        bool barPillVisible = true;

        // Estado cacheado (solo se modifica en refresh()/resized())
        geometry::CurveParams curveParams;
        float playheadPhase = 0.0f;
        int lastGainTenths  = 0;
        dsp::BarBeatTick lastTick { 1, 1, 1 };
        juce::String gainText;
        juce::String barBeatText;
        juce::Path curvePath;
        juce::Path fillPath;

        // Recursos preparados una sola vez
        juce::Font headerFont;
        juce::Font readoutFont;
        juce::Font axisFont;
        juce::Font pillFont;
        juce::String headerLabel;
        std::array<juce::String, 5> dbLabels;
        std::array<juce::String, 5> timeLabels;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainResponseView)
    };
}

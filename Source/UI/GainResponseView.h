#pragma once

#include "Common/UiStateSnapshot.h"
#include "DSP/CustomCurve.h"
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
            const std::atomic<float>* curve  = nullptr;
            const std::atomic<float>* mix    = nullptr;
            const std::atomic<float>* shift  = nullptr;
            const std::atomic<float>* slope  = nullptr;
            const std::atomic<float>* bypass = nullptr;
        };

        static constexpr int refreshRateHz = 60;

        GainResponseView (const UiStateSnapshot& snapshotToRead, ParameterSource parameters);
        ~GainResponseView() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        void mouseMove (const juce::MouseEvent& event) override;
        void mouseDown (const juce::MouseEvent& event) override;
        void mouseDrag (const juce::MouseEvent& event) override;
        void mouseUp (const juce::MouseEvent& event) override;
        void mouseDoubleClick (const juce::MouseEvent& event) override;
        void mouseExit (const juce::MouseEvent& event) override;

        // Establece el callback para cuando el usuario edita la curva custom
        std::function<void (const dsp::CustomCurveData&)> onCustomCurveChanged;

        // Actualiza los datos de la curva personalizada mostrada
        void setCustomCurveData (const dsp::CustomCurveData& curve);
        [[nodiscard]] const dsp::CustomCurveData& getCustomCurveData() const noexcept { return customCurve; }

        // La vista Compact oculta la pastilla "1.2.3".
        void setBarPillVisible (bool shouldBeVisible);
        [[nodiscard]] bool isBarPillVisible() const noexcept { return barPillVisible; }

        // Cuerpo del timer: lee el snapshot y los parametros, y solicita repaint si algo cambio.
        void refresh();

        [[nodiscard]] juce::Rectangle<float> getPlotArea() const noexcept { return plotArea; }
        [[nodiscard]] juce::Rectangle<float> getActivePillArea() const noexcept { return activePillArea; }
        [[nodiscard]] bool isActiveStatus() const noexcept { return isDuckingActive; }
        [[nodiscard]] const juce::Path& getCurvePath() const noexcept { return curvePath; }
        [[nodiscard]] const juce::Path& getFillPath() const noexcept { return fillPath; }
        [[nodiscard]] const juce::String& getBarBeatText() const noexcept { return barBeatText; }
        [[nodiscard]] geometry::CurveParams getCurveParams() const noexcept { return curveParams; }
        [[nodiscard]] float getPlayheadPhase() const noexcept { return playheadPhase; }

        [[nodiscard]] int getHoveredNodeIndex() const noexcept { return hoveredNodeIndex; }
        [[nodiscard]] int getDraggingNodeIndex() const noexcept { return draggingNodeIndex; }

    private:
        void timerCallback() override { refresh(); }

        void updateLayout();
        void rebuildPaths();
        [[nodiscard]] geometry::CurveParams readCurveParams() const noexcept;
        [[nodiscard]] int getNodeIndexAtPosition (juce::Point<float> pos) const noexcept;
        [[nodiscard]] juce::Point<float> getNodePosition (int nodeIndex) const noexcept;

        const UiStateSnapshot& snapshot;
        ParameterSource parameters;

        dsp::CustomCurveData customCurve = dsp::CustomCurveData::createDefault();
        int hoveredNodeIndex   = -1;
        int draggingNodeIndex  = -1;
        static constexpr float kNodeHitRadius = 9.0f;

        // Geometria
        juce::Rectangle<float> panelBounds;
        juce::Rectangle<float> headerArea;
        juce::Rectangle<float> activePillArea;
        juce::Rectangle<float> plotArea;
        bool barPillVisible = true;

        // Estado cacheado (solo se modifica en refresh()/resized())
        geometry::CurveParams curveParams;
        float playheadPhase = 0.0f;
        bool isDuckingActive = false;
        dsp::BarBeatTick lastTick { 1, 1, 1 };
        juce::String barBeatText;
        juce::Path curvePath;
        juce::Path fillPath;

        // Recursos preparados una sola vez
        juce::Font headerFont;
        juce::Font axisFont;
        juce::String headerLabel;
        std::array<juce::String, 5> dbLabels;
        std::array<juce::String, 5> timeLabels;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainResponseView)
    };
}

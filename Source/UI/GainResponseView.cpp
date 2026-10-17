#include "UI/GainResponseView.h"

#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    namespace
    {
        constexpr float defaultMixPercent   = 70.0f;
        constexpr float defaultSlopePercent = 50.0f;
        constexpr float percentToUnit       = 0.01f;

        [[nodiscard]] float loadOr (const std::atomic<float>* value, float fallback) noexcept
        {
            return value != nullptr ? value->load (std::memory_order_relaxed) : fallback;
        }
    }

    GainResponseView::GainResponseView (const UiStateSnapshot& snapshotToRead, ParameterSource parameterSource)
        : snapshot (snapshotToRead),
          parameters (parameterSource),
          headerFont (UmbralLookAndFeel::getSectionHeaderFont()),
          readoutFont (UmbralLookAndFeel::getGainReadoutFont()),
          axisFont (UmbralLookAndFeel::getAxisScaleFont()),
          pillFont (UmbralLookAndFeel::getBarPositionFont()),
          headerLabel ("GAIN RESPONSE")
    {
        setOpaque (false);

        dbLabels = { juce::String ("0"), juce::String ("-6"), juce::String ("-12"),
                     juce::String ("-18"), juce::String ("-24") };

        // 1/4, 1/2 y 3/4 como glifos Unicode (escapes: el fichero se mantiene en ASCII).
        timeLabels = { juce::String ("0"),
                       juce::String (juce::CharPointer_UTF8 ("\xC2\xBC")),
                       juce::String (juce::CharPointer_UTF8 ("\xC2\xBD")),
                       juce::String (juce::CharPointer_UTF8 ("\xC2\xBE")),
                       juce::String ("1") };

        curvePath.preallocateSpace (3 * (geometry::curvePointCount + 4));
        fillPath.preallocateSpace (3 * (geometry::curvePointCount + 6));

        curveParams = readCurveParams();
        updateLayout();
        rebuildPaths();
        refresh();

        startTimerHz (refreshRateHz);
    }

    GainResponseView::~GainResponseView()
    {
        stopTimer();
    }

    void GainResponseView::setBarPillVisible (bool shouldBeVisible)
    {
        if (barPillVisible == shouldBeVisible)
            return;

        barPillVisible = shouldBeVisible;
        updateLayout();
        rebuildPaths();
        repaint();
    }

    geometry::CurveParams GainResponseView::readCurveParams() const noexcept
    {
        geometry::CurveParams p;

        const float curveRaw = loadOr (parameters.curve, static_cast<float> (p.curveIndex));
        p.curveIndex = std::clamp (
            static_cast<int> (std::lround (math::finiteOr (curveRaw, static_cast<float> (dsp::defaultCurveIndex)))),
            0,
            dsp::numCurves - 1);

        p.mix   = math::clamp01 (math::finiteOr (loadOr (parameters.mix, defaultMixPercent), defaultMixPercent)
                                 * percentToUnit);
        p.shift = math::clamp01 (math::finiteOr (loadOr (parameters.shift, 0.0f), 0.0f) * percentToUnit);
        p.slope = math::clamp01 (math::finiteOr (loadOr (parameters.slope, defaultSlopePercent),
                                                 defaultSlopePercent) * percentToUnit);
        return p;
    }

    void GainResponseView::updateLayout()
    {
        panelBounds = getLocalBounds().toFloat();

        auto inner = panelBounds.reduced (static_cast<float> (tokens::panelPadding));
        headerArea = inner.removeFromTop (static_cast<float> (tokens::graphHeaderHeight));

        if (barPillVisible)
            inner.removeFromTop (tokens::graphPillHeight);

        inner.removeFromLeft (static_cast<float> (tokens::graphAxisLabelWidth + tokens::graphAxisGap));
        inner.removeFromBottom (static_cast<float> (tokens::graphAxisLabelHeight + tokens::graphAxisGap));
        plotArea = inner;
    }

    void GainResponseView::rebuildPaths()
    {
        geometry::buildPaths (plotArea, curveParams, curvePath, fillPath);
    }

    void GainResponseView::resized()
    {
        updateLayout();
        rebuildPaths();
    }

    void GainResponseView::refresh()
    {
        const UiStateData state = snapshot.read();
        bool changed = false;

        const auto newParams = readCurveParams();
        if (! (newParams == curveParams))
        {
            curveParams = newParams;
            rebuildPaths();
            changed = true;
        }

        const float newPhase = math::frac (state.currentPhase);
        if (newPhase != playheadPhase)
        {
            playheadPhase = newPhase;
            changed = true;
        }

        const int tenths = geometry::gainDbToTenths (state.currentGainDb);
        if (tenths != lastGainTenths || gainText.isEmpty())
        {
            lastGainTenths = tenths;

            char buffer[24] {};
            geometry::formatGainDb (state.currentGainDb, buffer, sizeof (buffer));
            gainText = juce::String (buffer);
            changed = true;
        }

        if (! (state.barBeatTick == lastTick) || barBeatText.isEmpty())
        {
            lastTick = state.barBeatTick;

            char buffer[32] {};
            geometry::formatBarBeat (state.barBeatTick, buffer, sizeof (buffer));
            barBeatText = juce::String (buffer);
            changed = true;
        }

        if (changed)
            repaint();
    }

    void GainResponseView::paint (juce::Graphics& g)
    {
        // Panel
        g.setColour (tokens::panel);
        g.fillRoundedRectangle (panelBounds, tokens::radius);
        g.setColour (tokens::hairline);
        g.drawRoundedRectangle (panelBounds.reduced (tokens::borderWidth * 0.5f), tokens::radius,
                                tokens::borderWidth);

        // Cabecera: etiqueta a la izquierda, lectura instantanea a la derecha
        g.setFont (headerFont);
        g.setColour (tokens::muted);
        g.drawText (headerLabel, headerArea, juce::Justification::centredLeft, false);

        g.setFont (readoutFont);
        g.setColour (tokens::signal);
        g.drawText (gainText, headerArea, juce::Justification::centredRight, false);

        if (plotArea.isEmpty())
            return;

        // Reticula
        g.setColour (tokens::hairline);
        for (const float db : geometry::gridDb)
            g.fillRect (plotArea.getX(), geometry::dbToY (plotArea, db) - tokens::borderWidth * 0.5f,
                        plotArea.getWidth(), tokens::borderWidth);

        for (const float phase : geometry::gridPhase)
            g.fillRect (geometry::phaseToX (plotArea, phase) - tokens::borderWidth * 0.5f, plotArea.getY(),
                        tokens::borderWidth, plotArea.getHeight());

        // Etiquetas de ejes
        g.setFont (axisFont);
        g.setColour (tokens::muted);

        const float labelH = static_cast<float> (tokens::graphAxisLabelHeight);
        for (std::size_t i = 0; i < geometry::gridDb.size(); ++i)
        {
            const float y = geometry::dbToY (plotArea, geometry::gridDb[i]);
            g.drawText (dbLabels[i],
                        juce::Rectangle<float> (plotArea.getX() - static_cast<float> (tokens::graphAxisGap + tokens::graphAxisLabelWidth),
                                                y - labelH * 0.5f,
                                                static_cast<float> (tokens::graphAxisLabelWidth), labelH),
                        juce::Justification::centredRight, false);
        }

        const float timeW = static_cast<float> (tokens::graphTimeLabelWidth);
        for (std::size_t i = 0; i < geometry::gridPhase.size(); ++i)
        {
            const float x = geometry::phaseToX (plotArea, geometry::gridPhase[i]);
            g.drawText (timeLabels[i],
                        juce::Rectangle<float> (x - timeW * 0.5f,
                                                plotArea.getBottom() + static_cast<float> (tokens::graphAxisGap),
                                                timeW, labelH),
                        juce::Justification::centred, false);
        }

        // Curva efectiva: relleno Signal 15% y trazo Signal 2.5 px
        g.setColour (tokens::curveFill());
        g.fillPath (fillPath);

        g.setColour (tokens::signal);
        g.strokePath (curvePath, juce::PathStrokeType (tokens::curveStroke, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Playhead: linea vertical de 1 px en Bone
        const float playheadX = geometry::phaseToX (plotArea, playheadPhase);
        g.setColour (tokens::bone);
        g.fillRect (playheadX - tokens::playheadWidth * 0.5f, plotArea.getY(),
                    tokens::playheadWidth, plotArea.getHeight());

        // Pastilla "1.2.3" anclada sobre el playhead
        if (barPillVisible)
        {
            const float pillX = std::clamp (playheadX - tokens::graphPillWidth * 0.5f, plotArea.getX(),
                                            std::max (plotArea.getX(), plotArea.getRight() - tokens::graphPillWidth));
            const juce::Rectangle<float> pill (pillX, plotArea.getY() - tokens::graphPillHeight,
                                               tokens::graphPillWidth, tokens::graphPillHeight);

            g.setColour (tokens::raised);
            g.fillRoundedRectangle (pill, tokens::radius);
            g.setColour (tokens::hairline);
            g.drawRoundedRectangle (pill.reduced (tokens::borderWidth * 0.5f), tokens::radius,
                                    tokens::borderWidth);

            g.setFont (pillFont);
            g.setColour (tokens::bone);
            g.drawText (barBeatText, pill, juce::Justification::centred, false);
        }
    }
}

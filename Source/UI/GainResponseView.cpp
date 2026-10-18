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
          axisFont (UmbralLookAndFeel::getAxisScaleFont()),
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

        // Separacion vertical superior para que el titulo GAIN RESPONSE respire sobre el grafico
        inner.removeFromTop (8.0f);

        inner.removeFromLeft (static_cast<float> (tokens::graphAxisLabelWidth + tokens::graphAxisGap));
        inner.removeFromBottom (static_cast<float> (tokens::graphAxisLabelHeight + tokens::graphAxisGap));
        plotArea = inner;
    }

    void GainResponseView::setCustomCurveData (const dsp::CustomCurveData& curve)
    {
        if (customCurve == curve)
            return;

        customCurve = curve;
        if (curveParams.curveIndex == dsp::customCurveIndex)
        {
            rebuildPaths();
            repaint();
        }
    }

    void GainResponseView::rebuildPaths()
    {
        std::span<const dsp::Node> customSpan {};
        if (curveParams.curveIndex == dsp::customCurveIndex)
            customSpan = customCurve.getSpan();

        geometry::buildPaths (plotArea, curveParams, curvePath, fillPath, customSpan);
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

        // Cabecera: titulo GAIN RESPONSE a la izquierda
        g.setFont (headerFont);
        g.setColour (tokens::muted);
        g.drawText (headerLabel, headerArea, juce::Justification::centredLeft, false);

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

        // Nodos interactivos: solo visibles si la curva seleccionada es 16 Custom
        if (curveParams.curveIndex == dsp::customCurveIndex)
        {
            for (int i = 0; i < customCurve.numNodes; ++i)
            {
                const auto pos = getNodePosition (i);
                const bool isDragging = (i == draggingNodeIndex);
                const bool isHovered  = (i == hoveredNodeIndex);

                float radius = 3.5f;
                juce::Colour fillColour   = tokens::raised;
                juce::Colour strokeColour = tokens::signal;
                float strokeWidth         = 1.5f;

                if (isDragging)
                {
                    radius       = 5.0f;
                    fillColour   = tokens::signal;
                    strokeColour = tokens::umbra;
                    strokeWidth  = 2.0f;
                }
                else if (isHovered)
                {
                    radius       = 4.5f;
                    fillColour   = tokens::panel;
                    strokeColour = tokens::bone;
                    strokeWidth  = 2.0f;
                }

                const juce::Rectangle<float> circle (pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f);
                g.setColour (fillColour);
                g.fillEllipse (circle);
                g.setColour (strokeColour);
                g.drawEllipse (circle, strokeWidth);
            }
        }
    }

    juce::Point<float> GainResponseView::getNodePosition (int nodeIndex) const noexcept
    {
        if (nodeIndex < 0 || nodeIndex >= customCurve.numNodes)
            return {};

        const auto& node = customCurve.nodes[static_cast<size_t> (nodeIndex)];
        const float x = geometry::phaseToX (plotArea, node.phi);
        const float gain = math::gainFromCurve (node.value, curveParams.mix);
        const float y = geometry::dbToY (plotArea, math::gainToDb (gain));
        return { x, y };
    }

    int GainResponseView::getNodeIndexAtPosition (juce::Point<float> pos) const noexcept
    {
        if (curveParams.curveIndex != dsp::customCurveIndex)
            return -1;

        int closestIdx = -1;
        float minDistanceSq = kNodeHitRadius * kNodeHitRadius;

        for (int i = 0; i < customCurve.numNodes; ++i)
        {
            const auto nodePos = getNodePosition (i);
            const float dx = pos.x - nodePos.x;
            const float dy = pos.y - nodePos.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq <= minDistanceSq)
            {
                minDistanceSq = distSq;
                closestIdx = i;
            }
        }

        return closestIdx;
    }

    void GainResponseView::mouseMove (const juce::MouseEvent& event)
    {
        if (curveParams.curveIndex != dsp::customCurveIndex)
            return;

        const int newHover = getNodeIndexAtPosition (event.position);
        if (newHover != hoveredNodeIndex)
        {
            hoveredNodeIndex = newHover;
            repaint();
        }
    }

    void GainResponseView::mouseDown (const juce::MouseEvent& event)
    {
        if (curveParams.curveIndex != dsp::customCurveIndex)
            return;

        const int hitIndex = getNodeIndexAtPosition (event.position);

        // Eliminacion de nodo intermedio con Clic Derecho o Alt+Clic
        if (event.mods.isPopupMenu() || event.mods.isAltDown())
        {
            if (hitIndex > 0 && hitIndex < customCurve.numNodes - 1)
            {
                if (customCurve.removeNode (hitIndex))
                {
                    hoveredNodeIndex  = -1;
                    draggingNodeIndex = -1;
                    rebuildPaths();
                    repaint();
                    if (onCustomCurveChanged)
                        onCustomCurveChanged (customCurve);
                }
            }
            return;
        }

        if (hitIndex >= 0)
        {
            draggingNodeIndex = hitIndex;
            repaint();
        }
    }

    void GainResponseView::mouseDrag (const juce::MouseEvent& event)
    {
        if (curveParams.curveIndex != dsp::customCurveIndex || draggingNodeIndex < 0)
            return;

        const float newPhi = geometry::xToPhase (plotArea, event.position.x);

        // Conversion inversa: Y -> dB -> Ganancia lineal -> Valor normalizado del nodo
        const float db = geometry::yToDb (plotArea, event.position.y);
        const float targetGain = math::dbToGain (db);
        const float mix = curveParams.mix;
        float newValue = targetGain;
        if (mix > 1e-4f)
            newValue = math::clamp01 ((targetGain - (1.0f - mix)) / mix);
        else
            newValue = math::clamp01 (targetGain);

        if (customCurve.moveNode (draggingNodeIndex, newPhi, newValue))
        {
            rebuildPaths();
            repaint();
            if (onCustomCurveChanged)
                onCustomCurveChanged (customCurve);
        }
    }

    void GainResponseView::mouseUp (const juce::MouseEvent& /*event*/)
    {
        if (draggingNodeIndex >= 0)
        {
            draggingNodeIndex = -1;
            repaint();
        }
    }

    void GainResponseView::mouseDoubleClick (const juce::MouseEvent& event)
    {
        if (curveParams.curveIndex != dsp::customCurveIndex)
            return;

        if (! plotArea.contains (event.position))
            return;

        // No insertar si se hace doble clic sobre un nodo existente
        if (getNodeIndexAtPosition (event.position) >= 0)
            return;

        const float phi = geometry::xToPhase (plotArea, event.position.x);
        const float db = geometry::yToDb (plotArea, event.position.y);
        const float targetGain = math::dbToGain (db);
        const float mix = curveParams.mix;
        float value = targetGain;
        if (mix > 1e-4f)
            value = math::clamp01 ((targetGain - (1.0f - mix)) / mix);
        else
            value = math::clamp01 (targetGain);

        const int insertedIdx = customCurve.insertNode (phi, value);
        if (insertedIdx >= 0)
        {
            hoveredNodeIndex  = insertedIdx;
            draggingNodeIndex = insertedIdx;
            rebuildPaths();
            repaint();
            if (onCustomCurveChanged)
                onCustomCurveChanged (customCurve);
        }
    }

    void GainResponseView::mouseExit (const juce::MouseEvent& /*event*/)
    {
        if (hoveredNodeIndex >= 0 && draggingNodeIndex < 0)
        {
            hoveredNodeIndex = -1;
            repaint();
        }
    }
}

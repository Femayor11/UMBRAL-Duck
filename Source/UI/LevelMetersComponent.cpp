#include "UI/LevelMetersComponent.h"

#include "Common/MathUtils.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>

namespace umbral::ui
{
    LevelMetersComponent::LevelMetersComponent (const UiStateSnapshot& snapshotToRead)
        : snapshot (snapshotToRead),
          headerFont (UmbralLookAndFeel::getSectionHeaderFont()),
          channelFont (UmbralLookAndFeel::getMeterLabelFont()),
          scaleFont (UmbralLookAndFeel::getAxisScaleFont()),
          headerLabel ("LEVEL"),
          inputLabel ("IN"),
          outputLabel ("OUT")
    {
        setOpaque (false);

        for (std::size_t i = 0; i < scaleDb.size(); ++i)
            scaleLabels[i] = juce::String (static_cast<int> (scaleDb[i]));

        resized();
        startTimerHz (refreshRateHz);
    }

    LevelMetersComponent::~LevelMetersComponent()
    {
        stopTimer();
    }

    float LevelMetersComponent::levelDbToFraction (float db) noexcept
    {
        const float clamped = std::clamp (math::finiteOr (db, tokens::meterMinDb), tokens::meterMinDb, 0.0f);
        return (clamped - tokens::meterMinDb) / (0.0f - tokens::meterMinDb);
    }

    void LevelMetersComponent::resized()
    {
        panelBounds = getLocalBounds().toFloat();

        auto inner = panelBounds.reduced (static_cast<float> (tokens::panelPadding));
        headerArea = inner.removeFromTop (static_cast<float> (tokens::meterHeaderHeight));

        auto labelRow = inner.removeFromBottom (static_cast<float> (tokens::meterLabelHeight));
        const float barW   = static_cast<float> (tokens::meterBarWidth);
        const float scaleW = static_cast<float> (tokens::meterScaleWidth);

        // Conjunto [IN | escala | OUT] centrado horizontalmente
        const float totalW = barW * 2.0f + scaleW;
        const float left   = inner.getCentreX() - totalW * 0.5f;

        inputBar    = { left, inner.getY(), barW, inner.getHeight() };
        scaleColumn = { left + barW, inner.getY(), scaleW, inner.getHeight() };
        outputBar   = { left + barW + scaleW, inner.getY(), barW, inner.getHeight() };

        inputLabelArea  = { left - barW, labelRow.getY(), barW * 3.0f, labelRow.getHeight() };
        outputLabelArea = { outputBar.getX() - barW, labelRow.getY(), barW * 3.0f, labelRow.getHeight() };
    }

    void LevelMetersComponent::refresh()
    {
        const float newIn  = snapshot.getInputLevelDb();
        const float newOut = snapshot.getOutputLevelDb();

        if (newIn != inputDb || newOut != outputDb)
        {
            inputDb  = newIn;
            outputDb = newOut;
            repaint();
        }
    }

    void LevelMetersComponent::drawBar (juce::Graphics& g, const juce::Rectangle<float>& bar,
                                        float levelDb, juce::Colour barColour) const
    {
        g.setColour (tokens::raised);
        g.fillRect (bar);

        const float h = bar.getHeight() * levelDbToFraction (levelDb);
        if (h > 0.0f)
        {
            g.setColour (barColour);
            g.fillRect (bar.getX(), bar.getBottom() - h, bar.getWidth(), h);
        }
    }

    void LevelMetersComponent::paint (juce::Graphics& g)
    {
        g.setColour (tokens::panel);
        g.fillRoundedRectangle (panelBounds, tokens::radius);
        g.setColour (tokens::hairline);
        g.drawRoundedRectangle (panelBounds.reduced (tokens::borderWidth * 0.5f), tokens::radius,
                                tokens::borderWidth);

        g.setFont (headerFont);
        g.setColour (tokens::muted);
        g.drawText (headerLabel, headerArea, juce::Justification::centredLeft, false);

        if (inputBar.isEmpty())
            return;

        drawBar (g, inputBar, inputDb, tokens::bone);
        drawBar (g, outputBar, outputDb, tokens::signal);

        // Escala central dB (Muted)
        g.setFont (scaleFont);
        g.setColour (tokens::muted);
        const float labelH = static_cast<float> (tokens::graphAxisLabelHeight);
        for (std::size_t i = 0; i < scaleDb.size(); ++i)
        {
            const float y = scaleColumn.getBottom() - scaleColumn.getHeight() * levelDbToFraction (scaleDb[i]);
            const float clampedY = std::clamp (y, scaleColumn.getY() + labelH * 0.5f,
                                               std::max (scaleColumn.getY() + labelH * 0.5f,
                                                         scaleColumn.getBottom() - labelH * 0.5f));
            g.drawText (scaleLabels[i],
                        juce::Rectangle<float> (scaleColumn.getX(), clampedY - labelH * 0.5f,
                                                scaleColumn.getWidth(), labelH),
                        juce::Justification::centred, false);
        }

        // Etiquetas de canal (Bone)
        g.setFont (channelFont);
        g.setColour (tokens::bone);
        g.drawText (inputLabel, inputLabelArea, juce::Justification::centred, false);
        g.drawText (outputLabel, outputLabelArea, juce::Justification::centred, false);
    }
}

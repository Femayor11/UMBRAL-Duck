#include "UI/ControlStrip.h"

#include "Common/MathUtils.h"
#include "Parameters/ParameterIDs.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    // =========================================================================
    // ChannelsSelector
    // =========================================================================

    ChannelsSelector::ChannelsSelector (juce::AudioProcessorValueTreeState& apvts)
        : headerFont (UmbralLookAndFeel::getSectionHeaderFont()),
          headerLabel ("CHANNELS")
    {
        setOpaque (false);

        stereoButton.setClickingTogglesState (false);
        monoButton.setClickingTogglesState (false);

        stereoButton.onClick = [this]() { selectChannelIndex (0); };
        monoButton.onClick   = [this]() { selectChannelIndex (1); };

        addAndMakeVisible (stereoButton);
        addAndMakeVisible (monoButton);

        if (auto* param = apvts.getParameter (params::rawIDs::channels))
        {
            channelsAttachment = std::make_unique<juce::ParameterAttachment> (
                *param,
                [this] (float newValue) { updateButtonsFromParameter (newValue); },
                apvts.undoManager);
            channelsAttachment->sendInitialUpdate();
        }
        else
        {
            updateButtonsFromParameter (0.0f);
        }
    }

    ChannelsSelector::~ChannelsSelector()
    {
        channelsAttachment.reset();
    }

    void ChannelsSelector::selectChannelIndex (int index)
    {
        const int clamped = std::clamp (index, 0, 1);
        updateButtonsFromParameter (static_cast<float> (clamped));

        if (channelsAttachment != nullptr)
            channelsAttachment->setValueAsCompleteGesture (static_cast<float> (clamped));
    }

    void ChannelsSelector::updateButtonsFromParameter (float rawIndexValue)
    {
        const int index = std::clamp (juce::roundToInt (math::finiteOr (rawIndexValue, 0.0f)), 0, 1);
        selectedChannelIndex = index;

        stereoButton.setToggleState (index == 0, juce::dontSendNotification);
        monoButton.setToggleState   (index == 1, juce::dontSendNotification);
        repaint();
    }

    void ChannelsSelector::resized()
    {
        auto inner = getLocalBounds().toFloat().reduced (static_cast<float> (tokens::panelPadding));
        headerArea = inner.removeFromTop (static_cast<float> (tokens::meterHeaderHeight));

        const float btnH   = static_cast<float> (tokens::controlHeightSmall);
        const float gap    = static_cast<float> (tokens::segmentedButtonGap);
        const float availW = std::max (0.0f, inner.getWidth() - gap);
        const float btnW   = availW * 0.5f;
        const float btnY   = inner.getY() + std::max (0.0f, (inner.getHeight() - btnH) * 0.5f);

        stereoButton.setBounds (juce::Rectangle<float> (inner.getX(), btnY, btnW, btnH).toNearestInt());
        monoButton.setBounds   (juce::Rectangle<float> (inner.getX() + btnW + gap, btnY, btnW, btnH).toNearestInt());
    }

    void ChannelsSelector::paint (juce::Graphics& g)
    {
        g.setFont (headerFont);
        g.setColour (tokens::muted);
        g.drawText (headerLabel, headerArea, juce::Justification::centredLeft, false);
    }

    // =========================================================================
    // ControlStrip
    // =========================================================================

    ControlStrip::ControlStrip (juce::AudioProcessorValueTreeState& apvts)
        : sectionHeaderFont (UmbralLookAndFeel::getSectionHeaderFont()),
          mixReadoutFont (UmbralLookAndFeel::getMixReadoutFont()),
          secondaryReadoutFont (UmbralLookAndFeel::getMidValueFont())
    {
        setOpaque (false);

        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (params::rawIDs::curve)))
        {
            const auto& choices = choiceParam->choices;
            for (int i = 0; i < choices.size(); ++i)
                curveCombo.addItem (choices[i], i + 1);
        }
        else
        {
            for (int i = 0; i < params::curves::count; ++i)
                curveCombo.addItem (params::curves::choices[i], i + 1);
        }

        configureRotaryKnob (mixSlider);
        configureRotaryKnob (shiftSlider);
        configureRotaryKnob (slopeSlider);

        mixSlider.onValueChange   = [this]() { updateReadouts(); };
        shiftSlider.onValueChange = [this]() { updateReadouts(); };
        slopeSlider.onValueChange = [this]() { updateReadouts(); };

        addAndMakeVisible (curveCombo);
        addAndMakeVisible (mixSlider);
        addAndMakeVisible (shiftSlider);
        addAndMakeVisible (slopeSlider);

        curveAttachment = std::make_unique<ComboBoxAttachment> (apvts, params::rawIDs::curve, curveCombo);
        mixAttachment   = std::make_unique<SliderAttachment>   (apvts, params::rawIDs::mix,   mixSlider);
        shiftAttachment = std::make_unique<SliderAttachment>   (apvts, params::rawIDs::shift, shiftSlider);
        slopeAttachment = std::make_unique<SliderAttachment>   (apvts, params::rawIDs::slope, slopeSlider);

        updateReadouts();
        resized();
    }

    ControlStrip::~ControlStrip()
    {
        slopeAttachment.reset();
        shiftAttachment.reset();
        mixAttachment.reset();
        curveAttachment.reset();
    }

    void ControlStrip::configureRotaryKnob (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

        const float halfSweepRad = juce::degreesToRadians (tokens::knobSweepDegrees * 0.5f);
        const float twoPi        = juce::MathConstants<float>::twoPi;
        slider.setRotaryParameters (twoPi - halfSweepRad, twoPi + halfSweepRad, true);
    }

    juce::String ControlStrip::formatPercentage (double value)
    {
        const double safe = std::isfinite (value) ? std::clamp (value, 0.0, 100.0) : 0.0;
        return juce::String (juce::roundToInt (safe)) + "%";
    }

    void ControlStrip::updateReadouts()
    {
        const auto newMix   = formatPercentage (mixSlider.getValue());
        const auto newShift = formatPercentage (shiftSlider.getValue());
        const auto newSlope = formatPercentage (slopeSlider.getValue());

        if (newMix != mixReadoutText || newShift != shiftReadoutText || newSlope != slopeReadoutText)
        {
            mixReadoutText   = newMix;
            shiftReadoutText = newShift;
            slopeReadoutText = newSlope;
            repaint();
        }
    }

    void ControlStrip::setCompactMode (bool compact)
    {
        compactMode = compact;
        shiftSlider.setVisible (! compact);
        slopeSlider.setVisible (! compact);
        resized();
        repaint();
    }

    void ControlStrip::resized()
    {
        panelBounds = getLocalBounds().toFloat();
        if (panelBounds.isEmpty())
            return;

        const float pad = static_cast<float> (tokens::panelPadding);
        const float headerH = static_cast<float> (tokens::knobHeaderHeight);

        // 1. Nivel superior: CURVE (Full y Compact)
        const float curveSectionH = std::min (static_cast<float> (tokens::curveSectionHeight), panelBounds.getHeight());
        auto curveSection = juce::Rectangle<float> (0.0f, 0.0f, panelBounds.getWidth(), curveSectionH).reduced (pad);
        curveHeaderArea = curveSection.removeFromTop (headerH);

        const float comboH = static_cast<float> (tokens::controlHeightLarge);
        const float comboY = curveSection.getY() + std::max (0.0f, (curveSection.getHeight() - comboH) * 0.5f);
        curveCombo.setBounds (juce::Rectangle<float> (curveSection.getX(), comboY, curveSection.getWidth(), comboH).toNearestInt());

        curveDividerY = curveSectionH;

        // 2. Nivel central: MIX (en Compact ocupa todo el resto del panel; en Full tiene altura mixSectionHeightFull)
        const float mixTop = curveDividerY + tokens::borderWidth;
        const float mixSectionH = compactMode
                                      ? std::max (0.0f, panelBounds.getHeight() - mixTop)
                                      : std::min (static_cast<float> (tokens::mixSectionHeightFull),
                                                  std::max (0.0f, panelBounds.getHeight() - mixTop));

        auto mixSection = juce::Rectangle<float> (0.0f, mixTop, panelBounds.getWidth(), mixSectionH).reduced (pad);
        mixHeaderArea  = mixSection.removeFromTop (headerH);
        mixReadoutArea = mixSection.removeFromBottom (static_cast<float> (tokens::mixReadoutHeight));

        const float mixKnobSize = std::min ({ static_cast<float> (tokens::knobMixDiameter),
                                              mixSection.getWidth(),
                                              std::max (0.0f, mixSection.getHeight()) });
        const auto mixKnobRect = juce::Rectangle<float> (mixKnobSize, mixKnobSize).withCentre (mixSection.getCentre());
        mixSlider.setBounds (mixKnobRect.toNearestInt());

        // 3. Nivel inferior: SHIFT y SLOPE en 2 columnas simetricas (solo en vista Full)
        if (! compactMode)
        {
            mixDividerY = mixTop + mixSectionH;
            const float bottomTop = mixDividerY + tokens::borderWidth;
            const float bottomH   = std::max (0.0f, panelBounds.getHeight() - bottomTop);

            auto bottomSection = juce::Rectangle<float> (0.0f, bottomTop, panelBounds.getWidth(), bottomH)
                                     .reduced (pad, pad * 0.65f);
            const float colW = bottomSection.getWidth() * 0.5f;

            auto leftCol  = juce::Rectangle<float> (bottomSection.getX(),        bottomSection.getY(), colW, bottomSection.getHeight());
            auto rightCol = juce::Rectangle<float> (bottomSection.getX() + colW, bottomSection.getY(), colW, bottomSection.getHeight());

            shiftHeaderArea  = leftCol.removeFromTop (headerH);
            shiftReadoutArea = leftCol.removeFromBottom (static_cast<float> (tokens::secondaryReadoutHeight));
            const float secKnobSize = std::min ({ static_cast<float> (tokens::knobSecondaryDiameter),
                                                  leftCol.getWidth(),
                                                  std::max (0.0f, leftCol.getHeight()) });
            shiftSlider.setBounds (juce::Rectangle<float> (secKnobSize, secKnobSize).withCentre (leftCol.getCentre()).toNearestInt());

            slopeHeaderArea  = rightCol.removeFromTop (headerH);
            slopeReadoutArea = rightCol.removeFromBottom (static_cast<float> (tokens::secondaryReadoutHeight));
            slopeSlider.setBounds (juce::Rectangle<float> (secKnobSize, secKnobSize).withCentre (rightCol.getCentre()).toNearestInt());
        }
        else
        {
            mixDividerY      = 0.0f;
            shiftHeaderArea  = {};
            shiftReadoutArea = {};
            slopeHeaderArea  = {};
            slopeReadoutArea = {};
        }
    }

    void ControlStrip::paint (juce::Graphics& g)
    {
        if (panelBounds.isEmpty())
            return;

        // Contenedor principal plano (Panel + borde Hairline 1 px)
        g.setColour (tokens::panel);
        g.fillRoundedRectangle (panelBounds, tokens::radius);

        g.setColour (tokens::hairline);
        g.drawRoundedRectangle (panelBounds.reduced (tokens::borderWidth * 0.5f),
                                tokens::radius,
                                tokens::borderWidth);

        // Lineas divisorias horizontales 1 px Hairline
        const float lineX = panelBounds.getX() + tokens::borderWidth;
        const float lineW = std::max (0.0f, panelBounds.getWidth() - tokens::borderWidth * 2.0f);

        if (curveDividerY > 0.0f && curveDividerY < panelBounds.getHeight())
            g.fillRect (lineX, curveDividerY, lineW, tokens::borderWidth);

        if (! compactMode && mixDividerY > 0.0f && mixDividerY < panelBounds.getHeight())
            g.fillRect (lineX, mixDividerY, lineW, tokens::borderWidth);

        // Encabezados de seccion en Archivo SemiBold 10 px ALL CAPS (Muted)
        g.setFont (sectionHeaderFont);
        g.setColour (tokens::muted);
        g.drawText ("CURVE", curveHeaderArea, juce::Justification::centredLeft, false);
        g.drawText ("MIX",   mixHeaderArea,   juce::Justification::centredLeft, false);

        if (! compactMode)
        {
            g.drawText ("SHIFT", shiftHeaderArea, juce::Justification::centred, false);
            g.drawText ("SLOPE", slopeHeaderArea, juce::Justification::centred, false);
        }

        // Lectura principal de MIX en IBM Plex Mono 21 px (Bone)
        g.setFont (mixReadoutFont);
        g.setColour (tokens::bone);
        g.drawText (mixReadoutText, mixReadoutArea, juce::Justification::centred, false);

        // Lecturas secundarias de SHIFT y SLOPE en IBM Plex Mono 13 px (Bone)
        if (! compactMode)
        {
            g.setFont (secondaryReadoutFont);
            g.drawText (shiftReadoutText, shiftReadoutArea, juce::Justification::centred, false);
            g.drawText (slopeReadoutText, slopeReadoutArea, juce::Justification::centred, false);
        }
    }
} // namespace umbral::ui

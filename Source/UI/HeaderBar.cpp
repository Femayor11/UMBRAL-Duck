#include "UI/HeaderBar.h"

#include "Common/MathUtils.h"
#include "Parameters/ParameterIDs.h"
#include "UI/DesignTokens.h"
#include "UI/UmbralLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace umbral::ui
{
    HeaderBar::HeaderBar (juce::AudioProcessorValueTreeState& apvts)
        : brandBoldFont (UmbralLookAndFeel::getBrandFont (true)),
          brandLightFont (UmbralLookAndFeel::getBrandFont (false)),
          sectionHeaderFont (UmbralLookAndFeel::getSectionHeaderFont())
    {
        setOpaque (false);

        for (int i = 0; i < kNumRates; ++i)
        {
            rateButtons[static_cast<size_t> (i)].setClickingTogglesState (false);
            rateButtons[static_cast<size_t> (i)].onClick = [this, i]() { selectRateIndex (i); };
            addAndMakeVisible (rateButtons[static_cast<size_t> (i)]);
        }

        syncButton.setClickingTogglesState (false);
        syncButton.getProperties().set (UmbralLookAndFeel::kOutlineActiveStyleProperty, true);
        syncButton.onClick = [this]() { setSyncEnabled (! syncEnabled); };
        addAndMakeVisible (syncButton);

        bypassButton.setClickingTogglesState (false);
        bypassButton.getProperties().set (UmbralLookAndFeel::kOutlineActiveStyleProperty, true);
        bypassButton.onClick = [this]() { setBypassEnabled (! bypassEnabled); };
        addAndMakeVisible (bypassButton);

        if (auto* rateParam = apvts.getParameter (params::rawIDs::rate))
        {
            rateAttachment = std::make_unique<juce::ParameterAttachment> (
                *rateParam,
                [this] (float newValue) { updateRateFromParameter (newValue); },
                apvts.undoManager);
            rateAttachment->sendInitialUpdate();
        }
        else
        {
            updateRateFromParameter (1.0f);
        }

        if (auto* syncParam = apvts.getParameter (params::rawIDs::sync))
        {
            syncAttachment = std::make_unique<juce::ParameterAttachment> (
                *syncParam,
                [this] (float newValue) { updateSyncFromParameter (newValue); },
                apvts.undoManager);
            syncAttachment->sendInitialUpdate();
        }
        else
        {
            updateSyncFromParameter (1.0f);
        }

        if (auto* bypassParam = apvts.getParameter (params::rawIDs::bypass))
        {
            bypassAttachment = std::make_unique<juce::ParameterAttachment> (
                *bypassParam,
                [this] (float newValue) { updateBypassFromParameter (newValue); },
                apvts.undoManager);
            bypassAttachment->sendInitialUpdate();
        }
        else
        {
            updateBypassFromParameter (0.0f);
        }

        resized();
    }

    HeaderBar::~HeaderBar()
    {
        bypassAttachment.reset();
        syncAttachment.reset();
        rateAttachment.reset();
    }

    juce::TextButton& HeaderBar::getRateButton (int index) noexcept
    {
        const int clamped = std::clamp (index, 0, kNumRates - 1);
        return rateButtons[static_cast<size_t> (clamped)];
    }

    const juce::TextButton& HeaderBar::getRateButton (int index) const noexcept
    {
        const int clamped = std::clamp (index, 0, kNumRates - 1);
        return rateButtons[static_cast<size_t> (clamped)];
    }

    void HeaderBar::selectRateIndex (int index)
    {
        const int clamped = std::clamp (index, 0, kNumRates - 1);
        updateRateFromParameter (static_cast<float> (clamped));

        if (rateAttachment != nullptr)
            rateAttachment->setValueAsCompleteGesture (static_cast<float> (clamped));
    }

    void HeaderBar::updateRateFromParameter (float rawIndexValue)
    {
        const int index = std::clamp (juce::roundToInt (math::finiteOr (rawIndexValue, 1.0f)), 0, kNumRates - 1);
        selectedRateIndex = index;

        for (int i = 0; i < kNumRates; ++i)
            rateButtons[static_cast<size_t> (i)].setToggleState (i == index, juce::dontSendNotification);

        repaint();
    }

    void HeaderBar::setSyncEnabled (bool enabled)
    {
        updateSyncFromParameter (enabled ? 1.0f : 0.0f);

        if (syncAttachment != nullptr)
            syncAttachment->setValueAsCompleteGesture (enabled ? 1.0f : 0.0f);
    }

    void HeaderBar::updateSyncFromParameter (float rawBoolValue)
    {
        const bool enabled = math::finiteOr (rawBoolValue, 1.0f) >= 0.5f;
        syncEnabled = enabled;

        syncButton.setToggleState (enabled, juce::dontSendNotification);
        repaint();
    }

    void HeaderBar::setBypassEnabled (bool enabled)
    {
        updateBypassFromParameter (enabled ? 1.0f : 0.0f);

        if (bypassAttachment != nullptr)
            bypassAttachment->setValueAsCompleteGesture (enabled ? 1.0f : 0.0f);
    }

    void HeaderBar::updateBypassFromParameter (float rawBoolValue)
    {
        const bool enabled = math::finiteOr (rawBoolValue, 0.0f) >= 0.5f;
        bypassEnabled = enabled;

        bypassButton.setToggleState (enabled, juce::dontSendNotification);
        repaint();
    }

    void HeaderBar::rebuildLogoPaths()
    {
        logoArchPath.clear();
        logoDuckPath.clear();

        if (logoIconArea.isEmpty())
            return;

        const auto box = logoIconArea.reduced (tokens::borderWidth);
        const float x0 = box.getX();
        const float y0 = box.getY();
        const float w  = box.getWidth();
        const float h  = box.getHeight();

        // Arco exterior en color Bone (simbolo de umbral/portal)
        logoArchPath.startNewSubPath (x0, y0 + h);
        logoArchPath.lineTo (x0, y0 + h * 0.45f);
        logoArchPath.cubicTo (x0, y0, x0 + w, y0, x0 + w, y0 + h * 0.45f);
        logoArchPath.lineTo (x0 + w, y0 + h);

        // Curva interior de ducking en color Signal
        logoDuckPath.startNewSubPath (x0 + w * 0.08f, y0 + h * 0.52f);
        logoDuckPath.cubicTo (x0 + w * 0.32f, y0 + h * 0.52f,
                              x0 + w * 0.36f, y0 + h * 0.88f,
                              x0 + w * 0.50f, y0 + h * 0.88f);
        logoDuckPath.cubicTo (x0 + w * 0.64f, y0 + h * 0.88f,
                              x0 + w * 0.68f, y0 + h * 0.52f,
                              x0 + w * 0.92f, y0 + h * 0.52f);
    }

    void HeaderBar::resized()
    {
        namespace t = umbral::tokens;

        const auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty())
            return;

        const float padX    = static_cast<float> (t::windowPadding);
        const float centreY = bounds.getCentreY();
        const float btnH    = static_cast<float> (t::controlHeightSmall);
        const float btnY    = centreY - btnH * 0.5f;

        // 1. Logotipo izquierdo: icono vectorial + "UMBRAL" (ExtraBold) + "duck" (Light)
        const float iconSize = static_cast<float> (t::headerLogoIconSize);
        logoIconArea = { padX, centreY - iconSize * 0.5f, iconSize, iconSize };
        rebuildLogoPaths();

        const float textLeft  = logoIconArea.getRight() + static_cast<float> (t::headerLogoTextGap);
        const float boldW     = 88.0f;
        const float lightW    = 54.0f;
        const float logoTextH = static_cast<float> (t::headerLogoIconSize);
        logoBoldTextArea  = { textLeft, centreY - logoTextH * 0.5f, boldW, logoTextH };
        logoLightTextArea = { logoBoldTextArea.getRight(), centreY - logoTextH * 0.5f, lightW, logoTextH };

        // 2. Extremo derecho: boton BYPASS
        const float rightEdge = bounds.getWidth() - padX;
        const float bypassW   = static_cast<float> (t::headerBypassButtonWidth);
        const float bypassX   = rightEdge - bypassW;
        bypassButton.setBounds (juce::Rectangle<float> (bypassX, btnY, bypassW, btnH).toNearestInt());

        // 3. Selector segmentado RATE (1/8, 1/4, 1/2, 1/1)
        const float rateBtnW = static_cast<float> (t::headerRateButtonWidth);
        const float rateGap  = static_cast<float> (t::segmentedButtonGap);

        const float rateLabelW = static_cast<float> (t::headerRateLabelWidth);
        const float rateLeft   = static_cast<float> (t::windowPadding + t::fullLeftPanelWidth + t::panelGap);
        rateLabelArea     = { rateLeft, btnY, rateLabelW, btnH };
        const float rateButtonsStartX = rateLabelArea.getRight() + static_cast<float> (t::headerControlGap);

        for (int i = 0; i < kNumRates; ++i)
        {
            const float bx = rateButtonsStartX + static_cast<float> (i) * (rateBtnW + rateGap);
            rateButtons[static_cast<size_t> (i)].setBounds (juce::Rectangle<float> (bx, btnY, rateBtnW, btnH).toNearestInt());
        }

        // 4. Boton SYNC ubicado junto al selector RATE
        const float syncW         = static_cast<float> (t::headerSyncButtonWidth);
        const float lastRateRight = rateButtons[static_cast<size_t> (kNumRates - 1)].getBounds().toFloat().getRight();
        const float syncX         = lastRateRight + static_cast<float> (t::headerControlGap);
        syncButton.setBounds (juce::Rectangle<float> (syncX, btnY, syncW, btnH).toNearestInt());
    }

    void HeaderBar::paint (juce::Graphics& g)
    {
        namespace t = umbral::tokens;

        // 1. Icono vectorial del logotipo
        g.setColour (t::bone);
        g.strokePath (logoArchPath, juce::PathStrokeType (t::knobNeedleStroke,
                                                          juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        g.setColour (t::signal);
        g.strokePath (logoDuckPath, juce::PathStrokeType (t::curveStroke * 0.8f,
                                                          juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        // 2. Tipografia de marca "UMBRAL duck"
        g.setColour (t::bone);
        g.setFont (brandBoldFont);
        g.drawText ("UMBRAL", logoBoldTextArea, juce::Justification::centredLeft, false);

        g.setFont (brandLightFont);
        g.drawText ("duck", logoLightTextArea, juce::Justification::centredLeft, false);

        // 3. Etiqueta secundaria RATE
        g.setFont (sectionHeaderFont);
        g.setColour (t::muted);
        g.drawText ("RATE", rateLabelArea, juce::Justification::centredLeft, false);
    }
} // namespace umbral::ui

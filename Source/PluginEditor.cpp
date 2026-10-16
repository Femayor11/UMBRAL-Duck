#include "PluginEditor.h"
#include "Common/DesignTokens.h"
#include "Parameters/ParameterIDs.h"

namespace
{
    umbral::ui::GainResponseView::ParameterSource makeParameterSource (UmbralDuckProcessor& processor)
    {
        auto& apvts = processor.getAPVTS();

        umbral::ui::GainResponseView::ParameterSource source;
        source.curve = apvts.getRawParameterValue (umbral::params::rawIDs::curve);
        source.mix   = apvts.getRawParameterValue (umbral::params::rawIDs::mix);
        source.shift = apvts.getRawParameterValue (umbral::params::rawIDs::shift);
        source.slope = apvts.getRawParameterValue (umbral::params::rawIDs::slope);
        return source;
    }
}

UmbralDuckEditor::UmbralDuckEditor (UmbralDuckProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      controlStrip (p.getAPVTS()),
      gainResponseView (p.getUiSnapshot(), makeParameterSource (p)),
      levelMeters (p.getUiSnapshot()),
      channelsSelector (p.getAPVTS()),
      viewModeButton ("COMPACT")
{
    setLookAndFeel (&lookAndFeel);

    levelMeters.setChannelsSectionOffset (umbral::tokens::channelsSectionHeight);

    viewModeButton.setClickingTogglesState (false);
    viewModeButton.getProperties().set (umbral::ui::UmbralLookAndFeel::kOutlineActiveStyleProperty, true);
    viewModeButton.onClick = [this]() { toggleViewMode(); };

    addAndMakeVisible (controlStrip);
    addAndMakeVisible (gainResponseView);
    addAndMakeVisible (levelMeters);
    addAndMakeVisible (channelsSelector);
    addAndMakeVisible (viewModeButton);

    setResizable (false, false);
    applyViewMode (processorRef.isCompactView(), false);
    processorRef.getAPVTS().state.addListener (this);
}

UmbralDuckEditor::~UmbralDuckEditor()
{
    processorRef.getAPVTS().state.removeListener (this);
    setLookAndFeel (nullptr);
}

void UmbralDuckEditor::setCompactView (bool compact)
{
    applyViewMode (compact, true);
}

void UmbralDuckEditor::toggleViewMode()
{
    setCompactView (! compactView);
}

void UmbralDuckEditor::applyViewMode (bool compact, bool persistToProcessor)
{
    namespace t = umbral::tokens;

    compactView = compact;
    if (persistToProcessor)
        processorRef.setCompactView (compact);

    controlStrip.setCompactMode (compact);
    levelMeters.setVisible (! compact);
    channelsSelector.setVisible (! compact);
    gainResponseView.setBarPillVisible (! compact);

    viewModeButton.setToggleState (compact, juce::dontSendNotification);
    viewModeButton.setButtonText (compact ? "FULL" : "COMPACT");

    const int targetWidth  = compact ? t::windowCompactWidth  : t::windowFullWidth;
    const int targetHeight = compact ? t::windowCompactHeight : t::windowFullHeight;

    setResizable (false, false);
    setResizeLimits (targetWidth, targetHeight, targetWidth, targetHeight);
    setSize (targetWidth, targetHeight);
    resized();
    repaint();
}

void UmbralDuckEditor::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                 const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged == processorRef.getAPVTS().state
        && property == UmbralDuckProcessor::kIsCompactViewProperty)
    {
        const bool targetCompact = processorRef.isCompactView();
        if (compactView != targetCompact)
            applyViewMode (targetCompact, false);
    }
}

void UmbralDuckEditor::valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged)
{
    if (treeWhichHasBeenChanged == processorRef.getAPVTS().state)
    {
        const bool targetCompact = processorRef.isCompactView();
        if (compactView != targetCompact)
            applyViewMode (targetCompact, false);
    }
}

void UmbralDuckEditor::paint (juce::Graphics& g)
{
    namespace t = umbral::tokens;

    g.fillAll (t::umbra);

    const float dividerY = static_cast<float> (compactView ? t::compactHeaderDividerY : t::fullHeaderDividerY);
    g.setColour (t::hairline);
    g.fillRect (0.0f, dividerY, static_cast<float> (getWidth()), t::borderWidth);
}

void UmbralDuckEditor::resized()
{
    namespace t = umbral::tokens;

    if (compactView)
    {
        const int headerRight = getWidth() - t::windowPaddingCompact;
        const int buttonY     = (t::compactHeaderDividerY - t::viewToggleHeight) / 2;
        viewModeButton.setBounds (headerRight - t::viewToggleWidth, buttonY, t::viewToggleWidth, t::viewToggleHeight);

        // Cuadricula Compact (DESIGN.md §5):
        // Panel izquierdo GAIN RESPONSE (350 x 279 en x = 16, y = 65)
        // Panel derecho CURVE & MIX (160 x 279 en x = 382, y = 65)
        const int rightX = t::windowPaddingCompact + t::compactLeftPanelWidth + t::panelGapCompact;

        gainResponseView.setBounds (t::windowPaddingCompact,
                                    t::compactContentTop,
                                    t::compactLeftPanelWidth,
                                    t::compactPanelRowHeight);

        controlStrip.setBounds (rightX,
                                t::compactContentTop,
                                t::compactRightPanelWidth,
                                t::compactPanelRowHeight);
    }
    else
    {
        const int headerRight = getWidth() - t::windowPadding;
        const int buttonY     = (t::fullHeaderDividerY - t::viewToggleHeight) / 2;
        viewModeButton.setBounds (headerRight - t::viewToggleWidth, buttonY, t::viewToggleWidth, t::viewToggleHeight);

        // Fila media Full (DESIGN.md §5): [20 | 193 | 20 | 534 | 20 | 153 | 20]
        const int centerX = t::windowPadding + t::fullLeftPanelWidth + t::panelGap;
        const int rightX  = centerX + t::fullCenterPanelWidth + t::panelGap;

        controlStrip.setBounds (t::windowPadding, t::fullContentTop, t::fullLeftPanelWidth, t::fullPanelRowHeight);
        gainResponseView.setBounds (centerX, t::fullContentTop, t::fullCenterPanelWidth, t::fullPanelRowHeight);
        levelMeters.setBounds (rightX, t::fullContentTop, t::fullRightPanelWidth, t::fullPanelRowHeight);
        channelsSelector.setBounds (rightX, t::fullContentTop, t::fullRightPanelWidth, t::channelsSectionHeight);
    }
}

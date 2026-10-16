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
      gainResponseView (p.getUiSnapshot(), makeParameterSource (p)),
      levelMeters (p.getUiSnapshot())
{
    addAndMakeVisible (gainResponseView);
    addAndMakeVisible (levelMeters);

    setSize (umbral::tokens::windowFullWidth, umbral::tokens::windowFullHeight);
}

void UmbralDuckEditor::paint (juce::Graphics& g)
{
    g.fillAll (umbral::tokens::umbra);
}

void UmbralDuckEditor::resized()
{
    namespace t = umbral::tokens;

    // Fila media (DESIGN.md §5): [20 | 193 | 20 | 534 | 20 | 153 | 20]
    const int centerX = t::windowPadding + t::fullLeftPanelWidth + t::panelGap;
    const int rightX  = centerX + t::fullCenterPanelWidth + t::panelGap;

    gainResponseView.setBounds (centerX, t::fullContentTop, t::fullCenterPanelWidth, t::fullPanelRowHeight);
    levelMeters.setBounds (rightX, t::fullContentTop, t::fullRightPanelWidth, t::fullPanelRowHeight);
}

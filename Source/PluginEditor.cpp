#include "PluginEditor.h"
#include "Common/DesignTokens.h"
#include "Parameters/ParameterIDs.h"

namespace
{
    umbral::ui::GainResponseView::ParameterSource makeParameterSource (UmbralDuckProcessor& processor)
    {
        auto& apvts = processor.getAPVTS();

        umbral::ui::GainResponseView::ParameterSource source;
        source.curve  = apvts.getRawParameterValue (umbral::params::rawIDs::curve);
        source.mix    = apvts.getRawParameterValue (umbral::params::rawIDs::mix);
        source.shift  = apvts.getRawParameterValue (umbral::params::rawIDs::shift);
        source.slope  = apvts.getRawParameterValue (umbral::params::rawIDs::slope);
        source.bypass = apvts.getRawParameterValue (umbral::params::rawIDs::bypass);
        return source;
    }
}

UmbralDuckEditor::UmbralDuckEditor (UmbralDuckProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      headerBar (p.getAPVTS()),
      controlStrip (p.getAPVTS(), this),
      gainResponseView (p.getUiSnapshot(), makeParameterSource (p)),
      levelMeters (p.getUiSnapshot()),
      channelsSelector (p.getAPVTS()),
      curveBankGrid (p.getAPVTS())
{
    setLookAndFeel (&lookAndFeel);

    levelMeters.setChannelsSectionOffset (umbral::tokens::channelsSectionHeight);

    // Sincronizar estado inicial de la curva personalizada
    const auto initialCustomCurve = processorRef.getCustomCurve();
    gainResponseView.setCustomCurveData (initialCustomCurve);
    curveBankGrid.setCustomCurveData (initialCustomCurve);

    // Enlazar cambios del editor interactivo al procesador y al grid
    gainResponseView.onCustomCurveChanged = [this] (const umbral::dsp::CustomCurveData& newCurve)
    {
        processorRef.setCustomCurve (newCurve);
        curveBankGrid.setCustomCurveData (newCurve);
    };

    addAndMakeVisible (headerBar);
    addAndMakeVisible (controlStrip);
    addAndMakeVisible (gainResponseView);
    addAndMakeVisible (levelMeters);
    addAndMakeVisible (channelsSelector);
    addAndMakeVisible (curveBankGrid);

    setResizable (false, false);
    setResizeLimits (umbral::tokens::windowFullWidth, umbral::tokens::windowFullHeight,
                     umbral::tokens::windowFullWidth, umbral::tokens::windowFullHeight);
    setSize (umbral::tokens::windowFullWidth, umbral::tokens::windowFullHeight);

    processorRef.getAPVTS().state.addListener (this);
}

UmbralDuckEditor::~UmbralDuckEditor()
{
    processorRef.getAPVTS().state.removeListener (this);
    setLookAndFeel (nullptr);
}

void UmbralDuckEditor::valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged)
{
    if (treeWhichHasBeenChanged == processorRef.getAPVTS().state)
    {
        const auto restoredCurve = processorRef.getCustomCurve();
        gainResponseView.setCustomCurveData (restoredCurve);
        curveBankGrid.setCustomCurveData (restoredCurve);
    }
}

void UmbralDuckEditor::paint (juce::Graphics& g)
{
    namespace t = umbral::tokens;

    g.fillAll (t::umbra);

    const float dividerY = static_cast<float> (t::fullHeaderDividerY);
    g.setColour (t::hairline);
    g.fillRect (0.0f, dividerY, static_cast<float> (getWidth()), t::borderWidth);
}

void UmbralDuckEditor::resized()
{
    namespace t = umbral::tokens;

    headerBar.setBounds (0, 0, getWidth(), t::fullHeaderDividerY);

    // Fila media (DESIGN.md): [16 | 176 | 14 | 408 | 14 | 136 | 16]
    const int centerX = t::windowPadding + t::fullLeftPanelWidth + t::panelGap;
    const int rightX  = centerX + t::fullCenterPanelWidth + t::panelGap;

    controlStrip.setBounds (t::windowPadding, t::fullContentTop, t::fullLeftPanelWidth, t::fullPanelRowHeight);
    gainResponseView.setBounds (centerX, t::fullContentTop, t::fullCenterPanelWidth, t::fullPanelRowHeight);
    levelMeters.setBounds (rightX, t::fullContentTop, t::fullRightPanelWidth, t::fullPanelRowHeight);
    channelsSelector.setBounds (rightX, t::fullContentTop, t::fullRightPanelWidth, t::channelsSectionHeight);

    // Fila inferior: CURVE BANK (748 x 149 en x = 16, y = 375)
    curveBankGrid.setBounds (t::windowPadding,
                             t::fullBottomRowTop,
                             t::fullBottomRowWidth,
                             t::fullBottomRowHeight);
}

#include "PluginEditor.h"
#include "Common/DesignTokens.h"

UmbralDuckEditor::UmbralDuckEditor (UmbralDuckProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize (umbral::tokens::windowFullWidth, umbral::tokens::windowFullHeight);
}

void UmbralDuckEditor::paint (juce::Graphics& g)
{
    g.fillAll (umbral::tokens::umbra);
}

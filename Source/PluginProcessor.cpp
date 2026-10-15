#include "PluginProcessor.h"
#include "PluginEditor.h"

UmbralDuckProcessor::UmbralDuckProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void UmbralDuckProcessor::prepareToPlay (double, int) {}
void UmbralDuckProcessor::releaseResources() {}

bool UmbralDuckProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void UmbralDuckProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // Pass-through: el buffer se deja intacto.
}

juce::AudioProcessorEditor* UmbralDuckProcessor::createEditor()
{
    return new UmbralDuckEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UmbralDuckProcessor();
}

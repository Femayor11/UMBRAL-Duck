#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters/ParameterIDs.h"
#include "Parameters/ParameterLayout.h"

UmbralDuckProcessor::UmbralDuckProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", umbral::params::createParameterLayout())
{
    rateParam     = apvts.getRawParameterValue (umbral::params::rawIDs::rate);
    syncParam     = apvts.getRawParameterValue (umbral::params::rawIDs::sync);
    bpmParam      = apvts.getRawParameterValue (umbral::params::rawIDs::bpm);
    curveParam    = apvts.getRawParameterValue (umbral::params::rawIDs::curve);
    mixParam      = apvts.getRawParameterValue (umbral::params::rawIDs::mix);
    shiftParam    = apvts.getRawParameterValue (umbral::params::rawIDs::shift);
    slopeParam    = apvts.getRawParameterValue (umbral::params::rawIDs::slope);
    channelsParam = apvts.getRawParameterValue (umbral::params::rawIDs::channels);
    bypassParam   = apvts.getRawParameterValue (umbral::params::rawIDs::bypass);
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

void UmbralDuckProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    midiMessages.clear();

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0 || totalNumInputChannels == 0)
        return;

    umbral::dsp::DuckingParameters params;
    params.curveIndex = static_cast<int> (curveParam->load (std::memory_order_relaxed));
    params.mix        = mixParam->load (std::memory_order_relaxed) * 0.01f;
    params.shift      = shiftParam->load (std::memory_order_relaxed) * 0.01f;
    params.slope      = slopeParam->load (std::memory_order_relaxed) * 0.01f;
    params.channels   = static_cast<umbral::dsp::ChannelsMode> (
        static_cast<int> (channelsParam->load (std::memory_order_relaxed)));
    params.bypass     = bypassParam->load (std::memory_order_relaxed) > 0.5f;

    // T-04: Mientras no exista PhaseClock (T-05), evalua en phase = 0.0f por defecto.
    // Con la curva por defecto 03 Smooth, C(0.0f) == 1.0f, preservando pass-through exacto.
    const float phase = 0.0f;

    duckingEngine.process (buffer.getArrayOfWritePointers(),
                           totalNumInputChannels,
                           numSamples,
                           params,
                           phase);
}

void UmbralDuckProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UmbralDuckProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* UmbralDuckProcessor::createEditor()
{
    return new UmbralDuckEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UmbralDuckProcessor();
}

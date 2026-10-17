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

    if (! apvts.state.hasProperty (kIsCompactViewProperty))
        apvts.state.setProperty (kIsCompactViewProperty, false, nullptr);
}

void UmbralDuckProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    phaseBuffer.assign (static_cast<size_t> (juce::jmax (samplesPerBlock, 1)), 0.0f);
    transportClock.prepare (sampleRate);
    duckingEngine.prepare (sampleRate);
    levelMeterSource.prepare (sampleRate);
    uiSnapshot.reset();
}

void UmbralDuckProcessor::releaseResources()
{
    duckingEngine.reset();
    levelMeterSource.reset();
    uiSnapshot.reset();
}

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

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0 || totalNumInputChannels == 0)
    {
        midiMessages.clear();
        return;
    }

    umbral::dsp::DuckingParameters params;
    params.curveIndex = static_cast<int> (curveParam->load (std::memory_order_relaxed));
    params.mix        = mixParam->load (std::memory_order_relaxed) * 0.01f;
    params.shift      = shiftParam->load (std::memory_order_relaxed) * 0.01f;
    params.slope      = slopeParam->load (std::memory_order_relaxed) * 0.01f;
    params.channels   = static_cast<umbral::dsp::ChannelsMode> (
        static_cast<int> (channelsParam->load (std::memory_order_relaxed)));
    params.bypass     = bypassParam->load (std::memory_order_relaxed) > 0.5f;

    // T-06: transporte. Lectura del PlayHead (JUCE 8: Optional<PositionInfo>), sin allocations.
    umbral::dsp::HostTransport host;
    if (auto* hostPlayHead = getPlayHead())
    {
        if (const auto position = hostPlayHead->getPosition())
        {
            host.hasInfo   = true;
            host.isPlaying = position->getIsPlaying();

            if (const auto ppq = position->getPpqPosition())
            {
                host.hasPpq = true;
                host.ppq    = *ppq;
            }
            if (const auto bpm = position->getBpm())
            {
                host.hasBpm = true;
                host.bpm    = *bpm;
            }
            if (const auto ts = position->getTimeSignature())
            {
                host.hasTimeSig = true;
                host.timeSig    = { ts->numerator, ts->denominator };
            }
            if (const auto barStart = position->getPpqPositionOfLastBarStart())
            {
                host.hasBarStartPpq = true;
                host.barStartPpq    = *barStart;
            }
        }
    }

    const bool sync        = syncParam->load (std::memory_order_relaxed) > 0.5f;
    const double paramBpm  = static_cast<double> (bpmParam->load (std::memory_order_relaxed));
    const auto rate        = umbral::dsp::rateFromIndex (static_cast<int> (rateParam->load (std::memory_order_relaxed)));
    const float baseMix    = params.mix;

    // Procesa en tramos del tamano preasignado en prepareToPlay (nunca redimensiona aqui).
    const auto capacity = static_cast<int> (phaseBuffer.size());
    if (capacity <= 0)
    {
        midiMessages.clear();
        return;
    }

    const int numChannels = juce::jmin (totalNumInputChannels, 2);

    // T-08: Medicion Peak de entrada (IN) antes del procesamiento de ducking.
    const float inLevelDb = levelMeterSource.processInput (buffer.getArrayOfReadPointers(), numChannels, numSamples);

    float* const* channelPointers = buffer.getArrayOfWritePointers();
    float* chunkPointers[2] = { nullptr, nullptr };
    umbral::dsp::TransportResult lastResult;

    auto midiIt        = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    int pos = 0;
    while (pos < numSamples)
    {
        bool triggeredAtPos = false;
        while (midiIt != midiEnd && (*midiIt).samplePosition <= pos)
        {
            if ((*midiIt).getMessage().isNoteOn())
                triggeredAtPos = true;
            ++midiIt;
        }

        if (triggeredAtPos)
        {
            transportClock.triggerNoteOn (sync, host, paramBpm, rate, pos);
            duckingEngine.triggerAntiPopRamp();
        }

        while (midiIt != midiEnd && ! (*midiIt).getMessage().isNoteOn())
            ++midiIt;

        const int limitSample = (midiIt != midiEnd)
                                    ? juce::jlimit (pos + 1, numSamples, (*midiIt).samplePosition)
                                    : numSamples;
        const int chunk = juce::jmin (capacity, limitSample - pos);
        const std::span<float> phases (phaseBuffer.data(), static_cast<size_t> (chunk));

        lastResult = transportClock.process (sync, host, paramBpm, rate, phases, pos);
        params.mix = (lastResult.mode == umbral::dsp::TransportMode::Bypass) ? 0.0f : baseMix;

        for (int ch = 0; ch < numChannels; ++ch)
            chunkPointers[ch] = channelPointers[ch] + pos;

        duckingEngine.process (chunkPointers,
                               numChannels,
                               chunk,
                               params,
                               std::span<const float> (phaseBuffer.data(), static_cast<size_t> (chunk)));

        pos += chunk;
    }

    midiMessages.clear();

    // T-08: Medicion Peak de salida (OUT) y publicacion atomica lock-free para la UI.
    const float outLevelDb        = levelMeterSource.processOutput (buffer.getArrayOfReadPointers(), numChannels, numSamples);
    const float currentGainLinear = duckingEngine.getGainSmoother().getCurrentValue();
    uiSnapshot.publishFromLinearGain (lastResult.endPhase, currentGainLinear, lastResult.tick, inLevelDb, outLevelDb);
}

bool UmbralDuckProcessor::isCompactView() const noexcept
{
    return static_cast<bool> (apvts.state.getProperty (kIsCompactViewProperty, false));
}

void UmbralDuckProcessor::setCompactView (bool compact)
{
    apvts.state.setProperty (kIsCompactViewProperty, compact, nullptr);
}

void UmbralDuckProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (! state.hasProperty (kIsCompactViewProperty))
        state.setProperty (kIsCompactViewProperty, false, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UmbralDuckProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto restored = juce::ValueTree::fromXml (*xmlState);
        const bool compact = static_cast<bool> (restored.getProperty (kIsCompactViewProperty, false));
        restored.setProperty (kIsCompactViewProperty, compact, nullptr);
        apvts.replaceState (restored);

        if (auto* editor = dynamic_cast<UmbralDuckEditor*> (getActiveEditor()))
            editor->setCompactView (compact);
    }
}

juce::AudioProcessorEditor* UmbralDuckProcessor::createEditor()
{
    return new UmbralDuckEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UmbralDuckProcessor();
}

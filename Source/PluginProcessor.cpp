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

    // Inicializar ranuras de curva custom con 04 Classic
    const auto defaultCurve = umbral::dsp::CustomCurveData::createDefault();
    for (auto& slot : customCurveSlots)
        slot = defaultCurve;
    activeCustomCurveSlot.store (0, std::memory_order_relaxed);

    // Inicializar sub-arbol ValueTree de CustomCurve si no existe
    if (! apvts.state.getChildWithName (kCustomCurveType).isValid())
    {
        juce::ValueTree customTree (kCustomCurveType);
        for (int i = 0; i < defaultCurve.numNodes; ++i)
        {
            const auto& n = defaultCurve.nodes[static_cast<size_t> (i)];
            juce::ValueTree nodeTree (kNodeTag);
            nodeTree.setProperty ("phi", n.phi, nullptr);
            nodeTree.setProperty ("value", n.value, nullptr);
            nodeTree.setProperty ("shape", static_cast<int> (n.shape), nullptr);
            nodeTree.setProperty ("exponent", n.exponent, nullptr);
            customTree.addChild (nodeTree, -1, nullptr);
        }
        apvts.state.addChild (customTree, -1, nullptr);
    }
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

    const int activeSlot = activeCustomCurveSlot.load (std::memory_order_acquire);
    params.customCurveNodes = customCurveSlots[static_cast<size_t> (activeSlot)].getSpan();

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
    const bool isDuckingActive    = (lastResult.mode != umbral::dsp::TransportMode::Bypass) && ! params.bypass;
    uiSnapshot.publishFromLinearGain (lastResult.endPhase, currentGainLinear, lastResult.tick, inLevelDb, outLevelDb, isDuckingActive);
}

bool UmbralDuckProcessor::isCompactView() const noexcept
{
    return static_cast<bool> (apvts.state.getProperty (kIsCompactViewProperty, false));
}

void UmbralDuckProcessor::setCompactView (bool compact)
{
    apvts.state.setProperty (kIsCompactViewProperty, compact, nullptr);
}

umbral::dsp::CustomCurveData UmbralDuckProcessor::getCustomCurve() const noexcept
{
    const int activeSlot = activeCustomCurveSlot.load (std::memory_order_acquire);
    return customCurveSlots[static_cast<size_t> (activeSlot)];
}

void UmbralDuckProcessor::setCustomCurve (const umbral::dsp::CustomCurveData& newCurve)
{
    if (! newCurve.isValid())
        return;

    // Escribir en un slot inactivo del triple buffer
    const int currentActive = activeCustomCurveSlot.load (std::memory_order_relaxed);
    int nextSlot = (currentActive + 1) % 3;
    customCurveSlots[static_cast<size_t> (nextSlot)] = newCurve;
    activeCustomCurveSlot.store (nextSlot, std::memory_order_release);

    // Actualizar ValueTree
    auto customTree = apvts.state.getChildWithName (kCustomCurveType);
    if (! customTree.isValid())
    {
        customTree = juce::ValueTree (kCustomCurveType);
        apvts.state.addChild (customTree, -1, nullptr);
    }
    else
    {
        customTree.removeAllChildren (nullptr);
    }

    for (int i = 0; i < newCurve.numNodes; ++i)
    {
        const auto& n = newCurve.nodes[static_cast<size_t> (i)];
        juce::ValueTree nodeTree (kNodeTag);
        nodeTree.setProperty ("phi", n.phi, nullptr);
        nodeTree.setProperty ("value", n.value, nullptr);
        nodeTree.setProperty ("shape", static_cast<int> (n.shape), nullptr);
        nodeTree.setProperty ("exponent", n.exponent, nullptr);
        customTree.addChild (nodeTree, -1, nullptr);
    }
}

void UmbralDuckProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (! state.hasProperty (kIsCompactViewProperty))
        state.setProperty (kIsCompactViewProperty, false, nullptr);

    // Asegurar que el sub-arbol CustomCurve este sincronizado
    const auto currentCurve = getCustomCurve();
    auto customTree = state.getChildWithName (kCustomCurveType);
    if (! customTree.isValid())
    {
        customTree = juce::ValueTree (kCustomCurveType);
        state.addChild (customTree, -1, nullptr);
    }
    else
    {
        customTree.removeAllChildren (nullptr);
    }

    for (int i = 0; i < currentCurve.numNodes; ++i)
    {
        const auto& n = currentCurve.nodes[static_cast<size_t> (i)];
        juce::ValueTree nodeTree (kNodeTag);
        nodeTree.setProperty ("phi", n.phi, nullptr);
        nodeTree.setProperty ("value", n.value, nullptr);
        nodeTree.setProperty ("shape", static_cast<int> (n.shape), nullptr);
        nodeTree.setProperty ("exponent", n.exponent, nullptr);
        customTree.addChild (nodeTree, -1, nullptr);
    }

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

        // Restaurar curva personalizada si esta presente en el estado
        auto customTree = restored.getChildWithName (kCustomCurveType);
        if (customTree.isValid() && customTree.getNumChildren() >= umbral::dsp::CustomCurveData::kMinNodes)
        {
            umbral::dsp::CustomCurveData curve;
            const int count = std::min (customTree.getNumChildren(), umbral::dsp::CustomCurveData::kMaxNodes);
            curve.numNodes = count;
            for (int i = 0; i < count; ++i)
            {
                auto nodeTree = customTree.getChild (i);
                curve.nodes[static_cast<size_t> (i)] = umbral::dsp::Node {
                    static_cast<float> (nodeTree.getProperty ("phi", 0.0)),
                    static_cast<float> (nodeTree.getProperty ("value", 0.0)),
                    static_cast<umbral::dsp::SegmentShape> (static_cast<int> (nodeTree.getProperty ("shape", 0))),
                    static_cast<float> (nodeTree.getProperty ("exponent", 1.0))
                };
            }

            if (curve.isValid())
            {
                const int currentActive = activeCustomCurveSlot.load (std::memory_order_relaxed);
                int nextSlot = (currentActive + 1) % 3;
                customCurveSlots[static_cast<size_t> (nextSlot)] = curve;
                activeCustomCurveSlot.store (nextSlot, std::memory_order_release);
            }
        }

        apvts.replaceState (restored);
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

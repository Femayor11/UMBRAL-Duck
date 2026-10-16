#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/DuckingEngine.h"
#include <atomic>

class UmbralDuckProcessor final : public juce::AudioProcessor
{
public:
    UmbralDuckProcessor();
    ~UmbralDuckProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;

    // Cache de punteros atomicos para lectura lock-free y RT-safe
    std::atomic<float>* rateParam     = nullptr;
    std::atomic<float>* syncParam     = nullptr;
    std::atomic<float>* bpmParam      = nullptr;
    std::atomic<float>* curveParam    = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* shiftParam    = nullptr;
    std::atomic<float>* slopeParam    = nullptr;
    std::atomic<float>* channelsParam = nullptr;
    std::atomic<float>* bypassParam   = nullptr;

    umbral::dsp::DuckingEngine duckingEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckProcessor)
};

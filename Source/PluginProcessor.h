#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Common/UiStateSnapshot.h"
#include "DSP/DuckingEngine.h"
#include "DSP/LevelMeterSource.h"
#include "DSP/TransportClock.h"
#include <atomic>
#include <vector>

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
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    static inline const juce::Identifier kIsCompactViewProperty { "isCompactView" };

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    [[nodiscard]] bool isCompactView() const noexcept;
    void setCompactView (bool compact);

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }

    [[nodiscard]] umbral::UiStateData getUiStateSnapshot() const noexcept { return uiSnapshot.read(); }
    [[nodiscard]] const umbral::UiStateSnapshot& getUiSnapshot() const noexcept { return uiSnapshot; }

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
    umbral::dsp::TransportClock transportClock;
    umbral::dsp::LevelMeterSource levelMeterSource;
    umbral::UiStateSnapshot uiSnapshot;

    // Fases por muestra preasignadas (constructor/prepareToPlay); processBlock nunca redimensiona.
    std::vector<float> phaseBuffer = std::vector<float> (4096, 0.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckProcessor)
};

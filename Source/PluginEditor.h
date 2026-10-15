#pragma once

#include "PluginProcessor.h"

// T-01: editor vacío con la ventana Full oficial. La UI real llega en T-10.
class UmbralDuckEditor final : public juce::AudioProcessorEditor
{
public:
    explicit UmbralDuckEditor (UmbralDuckProcessor&);
    ~UmbralDuckEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckEditor)
};

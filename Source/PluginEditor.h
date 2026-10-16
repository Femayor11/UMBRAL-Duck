#pragma once

#include "PluginProcessor.h"
#include "UI/GainResponseView.h"
#include "UI/LevelMetersComponent.h"

// T-10: vista Full (960x600) con GAIN RESPONSE (centro) y LEVEL (derecha). El resto de paneles llega en tareas posteriores.
class UmbralDuckEditor final : public juce::AudioProcessorEditor
{
public:
    explicit UmbralDuckEditor (UmbralDuckProcessor&);
    ~UmbralDuckEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    umbral::ui::GainResponseView gainResponseView;
    umbral::ui::LevelMetersComponent levelMeters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmbralDuckEditor)
};

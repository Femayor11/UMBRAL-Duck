#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace umbral::params
{
    // Crea el layout de 9 parametros APVTS segun docs/ARCHITECTURE.md y DSP_SPEC.md.
    [[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}

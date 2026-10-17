#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include <memory>

namespace umbral::params
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // 1. RATE: Choice 1/8, 1/4, 1/2, 1/1 (default 1/4)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            ids::rate,
            names::rate,
            juce::StringArray { "1/8", "1/4", "1/2", "1/1" },
            1));

        // 2. SYNC: Bool (default true)
        layout.add (std::make_unique<juce::AudioParameterBool> (
            ids::sync,
            names::sync,
            true));

        // 3. BPM: Float 40.0..240.0, paso 0.1 (default 120.0)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ids::bpm,
            names::bpm,
            juce::NormalisableRange<float> (40.0f, 240.0f, 0.1f),
            120.0f));

        // 4. CURVE BANK: Choice 01 Click Duck .. 16 Custom (default 04 Classic)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            ids::curve,
            names::curve,
            juce::StringArray {
                "01 Click Duck",
                "02 Tight",
                "03 Punch",
                "04 Classic",
                "05 Deep",
                "06 Linear",
                "07 Logarithmic",
                "08 Slow Sweep",
                "09 Reverse Pump",
                "10 Side-Drop",
                "11 Trance Gate",
                "12 Stutter",
                "13 Offbeat Dip",
                "14 Sine Smooth",
                "15 Ramp Down",
                "16 Custom"
            },
            curves::defaultIndex));

        // 5. MIX: Float 0.0%..100.0%, paso 0.1% (default 70.0%)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ids::mix,
            names::mix,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            70.0f));

        // 6. SHIFT: Float 0.0%..100.0%, paso 0.1% (default 0.0%)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ids::shift,
            names::shift,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            0.0f));

        // 7. SLOPE: Float 0.0%..100.0%, paso 0.1% (default 50.0%)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ids::slope,
            names::slope,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            50.0f));

        // 8. CHANNELS: Choice Stereo, Mono (default Stereo)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            ids::channels,
            names::channels,
            juce::StringArray { "Stereo", "Mono" },
            0));

        // 9. BYPASS: Bool (default false)
        layout.add (std::make_unique<juce::AudioParameterBool> (
            ids::bypass,
            names::bypass,
            false));

        return layout;
    }
}

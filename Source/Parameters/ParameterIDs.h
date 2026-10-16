#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace umbral::params
{
    namespace rawIDs
    {
        inline constexpr auto rate     = "rate";
        inline constexpr auto sync     = "sync";
        inline constexpr auto bpm      = "bpm";
        inline constexpr auto curve    = "curve";
        inline constexpr auto mix      = "mix";
        inline constexpr auto shift    = "shift";
        inline constexpr auto slope    = "slope";
        inline constexpr auto channels = "channels";
        inline constexpr auto bypass   = "bypass";
    }

    namespace ids
    {
        inline const juce::ParameterID rate     { rawIDs::rate,     1 };
        inline const juce::ParameterID sync     { rawIDs::sync,     1 };
        inline const juce::ParameterID bpm      { rawIDs::bpm,      1 };
        inline const juce::ParameterID curve    { rawIDs::curve,    1 };
        inline const juce::ParameterID mix      { rawIDs::mix,      1 };
        inline const juce::ParameterID shift    { rawIDs::shift,    1 };
        inline const juce::ParameterID slope    { rawIDs::slope,    1 };
        inline const juce::ParameterID channels { rawIDs::channels, 1 };
        inline const juce::ParameterID bypass   { rawIDs::bypass,   1 };
    }

    namespace names
    {
        inline constexpr auto rate     = "RATE";
        inline constexpr auto sync     = "SYNC";
        inline constexpr auto bpm      = "BPM";
        inline constexpr auto curve    = "CURVE BANK";
        inline constexpr auto mix      = "MIX";
        inline constexpr auto shift    = "SHIFT";
        inline constexpr auto slope    = "SLOPE";
        inline constexpr auto channels = "CHANNELS";
        inline constexpr auto bypass   = "BYPASS";
    }
}

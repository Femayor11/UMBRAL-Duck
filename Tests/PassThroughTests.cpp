#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"
#include <cmath>

TEST_CASE ("Bus layouts support mono and stereo symmetrically", "[buses]")
{
    UmbralDuckProcessor processor;

    SECTION ("Default configuration is stereo")
    {
        REQUIRE (processor.getTotalNumInputChannels() == 2);
        REQUIRE (processor.getTotalNumOutputChannels() == 2);
    }

    SECTION ("Supported layouts")
    {
        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add (juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
        REQUIRE (processor.isBusesLayoutSupported (stereoLayout));

        juce::AudioProcessor::BusesLayout monoLayout;
        monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
        monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
        REQUIRE (processor.isBusesLayoutSupported (monoLayout));
    }

    SECTION ("Unsupported asymmetric or multichannel layouts")
    {
        juce::AudioProcessor::BusesLayout monoToStereo;
        monoToStereo.inputBuses.add (juce::AudioChannelSet::mono());
        monoToStereo.outputBuses.add (juce::AudioChannelSet::stereo());
        REQUIRE_FALSE (processor.isBusesLayoutSupported (monoToStereo));

        juce::AudioProcessor::BusesLayout stereoToMono;
        stereoToMono.inputBuses.add (juce::AudioChannelSet::stereo());
        stereoToMono.outputBuses.add (juce::AudioChannelSet::mono());
        REQUIRE_FALSE (processor.isBusesLayoutSupported (stereoToMono));

        juce::AudioProcessor::BusesLayout surround51;
        surround51.inputBuses.add (juce::AudioChannelSet::create5point1());
        surround51.outputBuses.add (juce::AudioChannelSet::create5point1());
        REQUIRE_FALSE (processor.isBusesLayoutSupported (surround51));

        juce::AudioProcessor::BusesLayout emptyLayout;
        REQUIRE_FALSE (processor.isBusesLayoutSupported (emptyLayout));
    }

    SECTION ("Dynamically switching to mono and back to stereo")
    {
        juce::AudioProcessor::BusesLayout monoLayout;
        monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
        monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
        REQUIRE (processor.setBusesLayout (monoLayout));
        REQUIRE (processor.getTotalNumInputChannels() == 1);
        REQUIRE (processor.getTotalNumOutputChannels() == 1);

        juce::AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add (juce::AudioChannelSet::stereo());
        stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
        REQUIRE (processor.setBusesLayout (stereoLayout));
        REQUIRE (processor.getTotalNumInputChannels() == 2);
        REQUIRE (processor.getTotalNumOutputChannels() == 2);
    }
}

TEST_CASE ("Stereo pass-through produces bit-identical output across block sizes and sample rates", "[passthrough]")
{
    UmbralDuckProcessor processor;

    const double sampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const int blockSizes[]     = { 32, 64, 128, 256, 512, 1024 };

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            processor.prepareToPlay (sampleRate, blockSize);

            juce::AudioBuffer<float> buffer (2, blockSize);

            for (int i = 0; i < blockSize; ++i)
            {
                const auto t = static_cast<float> (i);
                // Canales con senales independientes
                buffer.setSample (0, i, std::sin (2.0f * 3.14159265f * 440.0f * t / static_cast<float> (sampleRate)));
                buffer.setSample (1, i, -0.5f + (t / static_cast<float> (blockSize)));
            }

            juce::AudioBuffer<float> expectedBuffer;
            expectedBuffer.makeCopyOf (buffer);

            juce::MidiBuffer midiMessages;
            processor.processBlock (buffer, midiMessages);

            for (int ch = 0; ch < 2; ++ch)
            {
                const float* outData = buffer.getReadPointer (ch);
                const float* expData = expectedBuffer.getReadPointer (ch);

                for (int i = 0; i < blockSize; ++i)
                {
                    REQUIRE (outData[i] == expData[i]);
                }
            }

            processor.releaseResources();
        }
    }
}

TEST_CASE ("Mono pass-through produces bit-identical output", "[passthrough]")
{
    UmbralDuckProcessor processor;

    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
    monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (processor.setBusesLayout (monoLayout));

    const int blockSize = 256;
    processor.prepareToPlay (48000.0, blockSize);

    juce::AudioBuffer<float> buffer (1, blockSize);
    for (int i = 0; i < blockSize; ++i)
    {
        buffer.setSample (0, i, static_cast<float> (i) / static_cast<float> (blockSize) - 0.5f);
    }

    juce::AudioBuffer<float> expectedBuffer;
    expectedBuffer.makeCopyOf (buffer);

    juce::MidiBuffer midiMessages;
    processor.processBlock (buffer, midiMessages);

    const float* outData = buffer.getReadPointer (0);
    const float* expData = expectedBuffer.getReadPointer (0);
    for (int i = 0; i < blockSize; ++i)
    {
        REQUIRE (outData[i] == expData[i]);
    }

    processor.releaseResources();
}

TEST_CASE ("Midi messages are cleared in processBlock", "[midi]")
{
    UmbralDuckProcessor processor;
    processor.prepareToPlay (44100.0, 64);

    juce::AudioBuffer<float> buffer (2, 64);
    buffer.clear();

    juce::MidiBuffer midiMessages;
    midiMessages.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    midiMessages.addEvent (juce::MidiMessage::noteOff (1, 60, static_cast<juce::uint8> (0)), 32);

    REQUIRE_FALSE (midiMessages.isEmpty());

    processor.processBlock (buffer, midiMessages);

    REQUIRE (midiMessages.isEmpty());

    processor.releaseResources();
}

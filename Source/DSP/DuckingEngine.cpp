#include "DuckingEngine.h"
#include "CurveBank.h"
#include "Common/MathUtils.h"

namespace umbral::dsp
{
    void DuckingEngine::prepare (double sampleRate, float smoothingTimeMs) noexcept
    {
        gainSmoother.prepare (sampleRate, smoothingTimeMs);
        gainSmoother.resetToNextSample();
    }

    void DuckingEngine::reset() noexcept
    {
        gainSmoother.resetToNextSample();
    }

    float DuckingEngine::computeGain (int curveIndex, float phi, float shift, float slope, float mix) noexcept
    {
        const float curveValue = evaluate (curveIndex, phi, shift, slope);
        return math::gainFromCurve (curveValue, mix);
    }

    void DuckingEngine::processBypassRamp (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (! gainSmoother.isPrimed())
        {
            gainSmoother.reset (1.0f);
            return;
        }

        if (gainSmoother.getCurrentValue() == 1.0f)
            return;

        if (numChannels == 1)
        {
            float* ch0 = channels[0];
            if (ch0 == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                const float gain = gainSmoother.processSample (1.0f);
                if (gain != 1.0f)
                    ch0[i] *= gain;
            }

            return;
        }

        float* left  = channels[0];
        float* right = channels[1];
        if (left == nullptr || right == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float gain = gainSmoother.processSample (1.0f);
            if (gain != 1.0f)
            {
                left[i]  *= gain;
                right[i] *= gain;
            }
        }
    }

    void DuckingEngine::process (float* const* channels, int numChannels, int numSamples,
                                 const DuckingParameters& params, float phase) noexcept
    {
        if (numSamples <= 0 || numChannels <= 0 || channels == nullptr)
            return;

        if (params.bypass)
        {
            processBypassRamp (channels, numChannels, numSamples);
            return;
        }

        const float rawGain = computeGain (params.curveIndex, phase, params.shift, params.slope, params.mix);

        if (numChannels == 1)
        {
            float* ch0 = channels[0];
            if (ch0 == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                const float gain = gainSmoother.processSample (rawGain);
                if (gain != 1.0f)
                    ch0[i] *= gain;
            }

            return;
        }

        float* left  = channels[0];
        float* right = channels[1];
        if (left == nullptr || right == nullptr)
            return;

        if (params.channels == ChannelsMode::Mono)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float gain = gainSmoother.processSample (rawGain);
                const float mono = 0.5f * (left[i] + right[i]) * gain;
                left[i]  = mono;
                right[i] = mono;
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float gain = gainSmoother.processSample (rawGain);
                if (gain != 1.0f)
                {
                    left[i]  *= gain;
                    right[i] *= gain;
                }
            }
        }
    }

    void DuckingEngine::process (float* const* channels, int numChannels, int numSamples,
                                 const DuckingParameters& params, std::span<const float> phases) noexcept
    {
        if (numSamples <= 0 || numChannels <= 0 || channels == nullptr)
            return;

        if (params.bypass)
        {
            processBypassRamp (channels, numChannels, numSamples);
            return;
        }

        const auto numPhases = static_cast<int> (phases.size());

        if (numChannels == 1)
        {
            float* ch0 = channels[0];
            if (ch0 == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                const float phi     = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float rawGain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
                const float gain    = gainSmoother.processSample (rawGain);
                if (gain != 1.0f)
                    ch0[i] *= gain;
            }

            return;
        }

        float* left  = channels[0];
        float* right = channels[1];
        if (left == nullptr || right == nullptr)
            return;

        if (params.channels == ChannelsMode::Mono)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float phi     = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float rawGain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
                const float gain    = gainSmoother.processSample (rawGain);
                const float mono    = 0.5f * (left[i] + right[i]) * gain;
                left[i]  = mono;
                right[i] = mono;
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float phi     = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float rawGain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
                const float gain    = gainSmoother.processSample (rawGain);
                if (gain != 1.0f)
                {
                    left[i]  *= gain;
                    right[i] *= gain;
                }
            }
        }
    }

    void DuckingEngine::process (float* left, float* right, int numSamples,
                                 const DuckingParameters& params, float phase) noexcept
    {
        float* channels[2] = { left, right };
        process (channels, 2, numSamples, params, phase);
    }
}

#include "DuckingEngine.h"
#include "CurveBank.h"
#include "Common/MathUtils.h"

namespace umbral::dsp
{
    float DuckingEngine::computeGain (int curveIndex, float phi, float shift, float slope, float mix) noexcept
    {
        const float curveValue = evaluate (curveIndex, phi, shift, slope);
        return math::gainFromCurve (curveValue, mix);
    }

    void DuckingEngine::process (float* const* channels, int numChannels, int numSamples,
                                 const DuckingParameters& params, float phase) noexcept
    {
        if (params.bypass || numSamples <= 0 || numChannels <= 0 || channels == nullptr)
            return;

        const float gain = computeGain (params.curveIndex, phase, params.shift, params.slope, params.mix);

        if (numChannels == 1)
        {
            float* ch0 = channels[0];
            if (ch0 == nullptr || gain == 1.0f)
                return;

            for (int i = 0; i < numSamples; ++i)
                ch0[i] *= gain;

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
                const float mono = 0.5f * (left[i] + right[i]) * gain;
                left[i]  = mono;
                right[i] = mono;
            }
        }
        else
        {
            if (gain == 1.0f)
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                left[i]  *= gain;
                right[i] *= gain;
            }
        }
    }

    void DuckingEngine::process (float* const* channels, int numChannels, int numSamples,
                                 const DuckingParameters& params, std::span<const float> phases) noexcept
    {
        if (params.bypass || numSamples <= 0 || numChannels <= 0 || channels == nullptr)
            return;

        const auto numPhases = static_cast<int> (phases.size());

        if (numChannels == 1)
        {
            float* ch0 = channels[0];
            if (ch0 == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                const float phi = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float gain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
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
                const float phi = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float gain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
                const float mono = 0.5f * (left[i] + right[i]) * gain;
                left[i]  = mono;
                right[i] = mono;
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float phi = (i < numPhases) ? phases[static_cast<size_t> (i)] : 0.0f;
                const float gain = computeGain (params.curveIndex, phi, params.shift, params.slope, params.mix);
                left[i]  *= gain;
                right[i] *= gain;
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

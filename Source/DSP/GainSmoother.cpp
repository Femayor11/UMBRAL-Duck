#include "GainSmoother.h"
#include "Common/MathUtils.h"

#include <cmath>

namespace umbral::dsp
{
    namespace
    {
        constexpr double kDefaultSampleRate = 44100.0;
        constexpr double kLn100             = 4.6051701859880913680359829093687; // ln(100) para 99% de asentamiento
    }

    void GainSmoother::prepare (double sampleRate, float smoothingTimeMs) noexcept
    {
        const double fs = (std::isfinite (sampleRate) && sampleRate > 0.0) ? sampleRate : kDefaultSampleRate;
        const float timeMs = (std::isfinite (smoothingTimeMs) && smoothingTimeMs > 0.0f)
                                 ? smoothingTimeMs
                                 : kDefaultSmoothingMs;

        const double settlingSamples = fs * (static_cast<double> (timeMs) * 1.0e-3);
        if (settlingSamples <= 1.0)
        {
            alpha = 1.0f;
        }
        else
        {
            const double a = 1.0 - std::exp (-kLn100 / settlingSamples);
            alpha = math::clamp01 (static_cast<float> (a));
        }

        const int computedRampSamples = static_cast<int> (std::lround (fs * (static_cast<double> (kAntiPopRampMs) * 1.0e-3)));
        rampTotalSamples     = std::max (computedRampSamples, 1);
        rampSamplesRemaining = 0;
        rampStartValue       = currentValue;
    }

    void GainSmoother::reset (float initialValue) noexcept
    {
        currentValue         = math::clamp01 (math::finiteOr (initialValue, 1.0f));
        rampStartValue       = currentValue;
        rampSamplesRemaining = 0;
        primed               = true;
    }

    void GainSmoother::resetToNextSample() noexcept
    {
        rampSamplesRemaining = 0;
        primed               = false;
    }

    void GainSmoother::triggerAntiPopRamp() noexcept
    {
        if (! primed || rampTotalSamples <= 1)
            return;

        rampStartValue       = currentValue;
        rampSamplesRemaining = rampTotalSamples;
    }

    float GainSmoother::processSample (float targetGain) noexcept
    {
        const float target = math::clamp01 (math::finiteOr (targetGain, 1.0f));

        if (! primed)
        {
            currentValue         = target;
            rampStartValue       = target;
            rampSamplesRemaining = 0;
            primed               = true;
            return currentValue;
        }

        if (rampSamplesRemaining > 0)
        {
            const int step = rampTotalSamples - rampSamplesRemaining + 1;
            --rampSamplesRemaining;

            const float t = static_cast<float> (step) / static_cast<float> (rampTotalSamples);
            const float s = math::smoothstep (t);
            currentValue  = math::clamp01 (math::lerp (rampStartValue, target, s));
            return currentValue;
        }

        const float diff = target - currentValue;
        if (std::abs (diff) <= kSnapEpsilon)
        {
            currentValue = target;
            return currentValue;
        }

        currentValue = math::clamp01 (currentValue + alpha * diff);
        if (std::abs (target - currentValue) <= kSnapEpsilon)
            currentValue = target;

        return currentValue;
    }

    void GainSmoother::processBlock (std::span<float> gains) noexcept
    {
        for (float& g : gains)
            g = processSample (g);
    }
}

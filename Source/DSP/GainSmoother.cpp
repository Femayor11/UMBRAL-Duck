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
    }

    void GainSmoother::reset (float initialValue) noexcept
    {
        currentValue = math::clamp01 (math::finiteOr (initialValue, 1.0f));
        primed       = true;
    }

    void GainSmoother::resetToNextSample() noexcept
    {
        primed = false;
    }

    float GainSmoother::processSample (float targetGain) noexcept
    {
        const float target = math::clamp01 (math::finiteOr (targetGain, 1.0f));

        if (! primed)
        {
            currentValue = target;
            primed       = true;
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

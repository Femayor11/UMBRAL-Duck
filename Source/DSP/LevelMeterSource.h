#pragma once

#include "Common/MathUtils.h"

#include <algorithm>
#include <cmath>

// Medidor de picos (Peak) por bloque con balistica de ataque instantaneo y caida exponencial suave
// (DSP_SPEC.md §5, ARCHITECTURE.md §2, §3). 100% libre de JUCE, noexcept y sin asignaciones en heap.
namespace umbral::dsp
{
    class LevelMeterSource
    {
    public:
        // Constante de tiempo de caida por defecto (~300 ms, aprox. 28.9 dB/s).
        static constexpr float kDefaultReleaseMs = 300.0f;
        static constexpr float kMinDb            = -60.0f;
        static constexpr float kMaxDb            = 0.0f;

        LevelMeterSource() noexcept = default;

        void prepare (double newSampleRate, float releaseTimeMs = kDefaultReleaseMs) noexcept
        {
            sampleRate = (std::isfinite (newSampleRate) && newSampleRate > 0.0) ? newSampleRate : 44100.0;
            const float validReleaseMs = (std::isfinite (releaseTimeMs) && releaseTimeMs > 0.0f)
                                             ? releaseTimeMs
                                             : kDefaultReleaseMs;
            releaseSeconds = static_cast<double> (validReleaseMs) * 1.0e-3;
            reset();
        }

        void reset() noexcept
        {
            inputEnvelope  = 0.0f;
            outputEnvelope = 0.0f;
        }

        // Calcula el pico lineal maximo |x[n]| de un conjunto de canales en el bloque.
        [[nodiscard]] static float computeBlockPeak (const float* const* channels,
                                                     int numChannels,
                                                     int numSamples) noexcept
        {
            if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
                return 0.0f;

            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* data = channels[ch];
                if (data == nullptr)
                    continue;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float sample = data[i];
                    if (std::isfinite (sample))
                    {
                        const float absSample = std::abs (sample);
                        if (absSample > peak)
                            peak = absSample;
                    }
                }
            }

            return std::min (peak, 1.0f);
        }

        // Convierte una amplitud lineal a dBFS acotada en [-60.0, 0.0] dB.
        [[nodiscard]] static float linearToMeterDb (float linear) noexcept
        {
            const float clamped = math::clamp01 (math::finiteOr (linear, 0.0f));
            const float db      = math::gainToDb (clamped);
            return std::clamp (db, kMinDb, kMaxDb);
        }

        // Actualiza el medidor de entrada (IN) con un bloque de muestras y devuelve el nivel en dBFS.
        float processInput (const float* const* channels, int numChannels, int numSamples) noexcept
        {
            const float blockPeak = computeBlockPeak (channels, numChannels, numSamples);
            inputEnvelope = updateEnvelope (inputEnvelope, blockPeak, numSamples);
            return linearToMeterDb (inputEnvelope);
        }

        // Actualiza el medidor de salida (OUT) con un bloque de muestras y devuelve el nivel en dBFS.
        float processOutput (const float* const* channels, int numChannels, int numSamples) noexcept
        {
            const float blockPeak = computeBlockPeak (channels, numChannels, numSamples);
            outputEnvelope = updateEnvelope (outputEnvelope, blockPeak, numSamples);
            return linearToMeterDb (outputEnvelope);
        }

        [[nodiscard]] float getInputEnvelopeLinear() const noexcept { return inputEnvelope; }
        [[nodiscard]] float getOutputEnvelopeLinear() const noexcept { return outputEnvelope; }
        [[nodiscard]] float getInputLevelDb() const noexcept { return linearToMeterDb (inputEnvelope); }
        [[nodiscard]] float getOutputLevelDb() const noexcept { return linearToMeterDb (outputEnvelope); }

    private:
        [[nodiscard]] float updateEnvelope (float currentEnv, float blockPeak, int numSamples) const noexcept
        {
            const float safePeak = math::clamp01 (math::finiteOr (blockPeak, 0.0f));
            if (safePeak >= currentEnv)
                return safePeak;

            if (numSamples <= 0 || releaseSeconds <= 0.0 || sampleRate <= 0.0)
                return safePeak;

            const double exponent = -static_cast<double> (numSamples) / (releaseSeconds * sampleRate);
            const float decay     = static_cast<float> (std::exp (exponent));
            const float decayed   = std::max (safePeak, currentEnv * decay);

            return (decayed < math::gainFloor && safePeak < math::gainFloor) ? 0.0f : decayed;
        }

        double sampleRate     = 44100.0;
        double releaseSeconds = static_cast<double> (kDefaultReleaseMs) * 1.0e-3;
        float inputEnvelope   = 0.0f;
        float outputEnvelope  = 0.0f;
    };
}

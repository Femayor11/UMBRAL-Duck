#pragma once

#include <span>

// Suavizador de ganancia en tiempo real (DSP_SPEC.md §1, §4, §5; T-07).
// Filtro IIR de un polo con snap al objetivo. C++20 puro, noexcept, sin heap ni dependencias de JUCE.
namespace umbral::dsp
{
    class GainSmoother
    {
    public:
        static constexpr float kDefaultSmoothingMs = 1.5f;
        static constexpr float kAntiPopRampMs      = 2.0f;
        static constexpr float kSnapEpsilon        = 1.0e-6f;

        GainSmoother() noexcept = default;

        // Configura el coeficiente del filtro para alcanzar el 99% del escalon en smoothingTimeMs.
        void prepare (double sampleRate, float smoothingTimeMs = kDefaultSmoothingMs) noexcept;

        // Fija inmediatamente el estado actual al valor indicado (acotado a [0, 1]).
        void reset (float initialValue = 1.0f) noexcept;

        // Marca el suavizador para que la siguiente muestra fije el estado inicial sin rampa de arranque.
        void resetToNextSample() noexcept;

        // Inicia una micro-rampa cubica Hermite (smoothstep de 2.0 ms, C1 continua) desde el valor actual.
        void triggerAntiPopRamp() noexcept;

        // Procesa una muestra hacia targetGain y devuelve la ganancia suavizada en [0, 1].
        [[nodiscard]] float processSample (float targetGain) noexcept;

        // Suaviza in-place un bloque de ganancias objetivo.
        void processBlock (std::span<float> gains) noexcept;

        [[nodiscard]] float getCurrentValue() const noexcept { return currentValue; }
        [[nodiscard]] float getAlpha() const noexcept { return alpha; }
        [[nodiscard]] bool isPrimed() const noexcept { return primed; }
        [[nodiscard]] bool isRampActive() const noexcept { return rampSamplesRemaining > 0; }

    private:
        float alpha              = 1.0f;
        float currentValue       = 1.0f;
        float rampStartValue     = 1.0f;
        int rampTotalSamples     = 0;
        int rampSamplesRemaining = 0;
        bool primed              = true;
    };
}

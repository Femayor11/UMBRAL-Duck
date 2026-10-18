#pragma once

#include "DSP/CurveBank.h"
#include "DSP/GainSmoother.h"

#include <span>
#include <cstddef>

// Motor de ganancia de ducking (DSP_SPEC.md §1, §5). Funciones puras/RT-safe, noexcept, sin heap ni locks.
// 100% libre de dependencias de JUCE o UI (usa punteros/spans estándar de C++).
namespace umbral::dsp
{
    enum class ChannelsMode : int
    {
        Stereo = 0,
        Mono   = 1
    };

    struct DuckingParameters
    {
        int curveIndex           = 3;                    // 04 Classic
        float mix                = 0.70f;                // Normalizado [0, 1]
        float shift              = 0.0f;                 // Normalizado [0, 1]
        float slope              = 0.5f;                 // Normalizado [0, 1]
        ChannelsMode channels    = ChannelsMode::Stereo;
        bool bypass              = false;
        std::span<const Node> customCurveNodes {};       // Nodos para curveIndex == customCurveIndex (15)
    };

    class DuckingEngine
    {
    public:
        DuckingEngine() noexcept = default;

        // Configura el suavizador de ganancia para la tasa de muestreo dada.
        void prepare (double sampleRate, float smoothingTimeMs = GainSmoother::kDefaultSmoothingMs) noexcept;

        // Reinicia el estado del suavizador para que la primera muestra del siguiente bloque fije el nivel inicial.
        void reset() noexcept;

        static constexpr float kPreReleaseStartPhi = 0.97f;
        static constexpr float kStepThreshold      = 0.35f;

        // Inicia una micro-rampa cubica Hermite (4.5 ms) anti-pop ante reinicios de fase (ej. MIDI NoteOn).
        void triggerAntiPopRamp() noexcept;

        // Calcula la ganancia efectiva instantanea (sin suavizado) para la representacion visual y matematica:
        // G(phi) = (1 - M) + M * C_slope(applyShift(phi, S))
        // Salida garantizada finita en [0, 1].
        [[nodiscard]] static float computeGain (int curveIndex, float phi, float shift, float slope, float mix,
                                                std::span<const Node> customNodes = {}) noexcept;

        // Calcula la ganancia acustica instantanea aplicando lookahead de fin de ciclo en [0.97, 1.0]:
        // Interpola suavemente con smoothstep C1 hacia G(0.0) para garantizar continuidad periodica exacta G(1.0-) == G(0.0+).
        [[nodiscard]] static float computeAcousticGain (int curveIndex, float phi, float shift, float slope, float mix,
                                                        std::span<const Node> customNodes = {}) noexcept;

        // Procesa un bloque de canales (1 mono o 2 estereo) aplicando una fase constante al bloque (T-04).
        void process (float* const* channels, int numChannels, int numSamples,
                      const DuckingParameters& params, float phase) noexcept;

        // Procesa un bloque de canales con un span de fases por muestra y suavizado anti-clic (T-05, T-07).
        void process (float* const* channels, int numChannels, int numSamples,
                      const DuckingParameters& params, std::span<const float> phases) noexcept;

        // Sobrecarga de conveniencia para pares L/R directos.
        void process (float* left, float* right, int numSamples,
                      const DuckingParameters& params, float phase) noexcept;

        [[nodiscard]] const GainSmoother& getGainSmoother() const noexcept { return gainSmoother; }
        [[nodiscard]] GainSmoother& getGainSmoother() noexcept { return gainSmoother; }

    private:
        [[nodiscard]] float evaluateAndSmoothSample (const DuckingParameters& params, float phi) noexcept;
        void processBypassRamp (float* const* channels, int numChannels, int numSamples) noexcept;

        GainSmoother gainSmoother;
        float lastShiftedPhase = -1.0f;
        float lastRawGain      = -1.0f;
    };
}

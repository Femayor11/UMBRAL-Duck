#pragma once

#include "Common/MathUtils.h"
#include "DSP/PhaseClock.h"

#include <algorithm>
#include <atomic>
#include <cstdint>

// Snapshot atomico unidireccional y lock-free (PluginProcessor::processBlock -> UI Timer 60 Hz)
// (ARCHITECTURE.md §3, DSP_SPEC.md §1, §5, T-08).
// Sin locks, sin asignaciones en heap, 100% libre de dependencias de JUCE o componentes UI.
namespace umbral
{
    struct UiStateData
    {
        float currentPhase  = 0.0f;               // [0.0f, 1.0f)
        float currentGainDb = 0.0f;               // [-60.0f, 0.0f] dB
        dsp::BarBeatTick barBeatTick { 1, 1, 1 }; // Compas.Tiempo.Subdivision (1-indexado)
        float inputLevelDb  = -60.0f;             // [-60.0f, 0.0f] dBFS
        float outputLevelDb = -60.0f;             // [-60.0f, 0.0f] dBFS
    };

    class UiStateSnapshot
    {
    public:
        static_assert (std::atomic<float>::is_always_lock_free,
                       "std::atomic<float> must be lock-free for real-time audio use");
        static_assert (std::atomic<std::uint64_t>::is_always_lock_free,
                       "std::atomic<uint64_t> must be lock-free for real-time audio use");

        static constexpr float kMinDb = -60.0f;
        static constexpr float kMaxDb = 0.0f;

        UiStateSnapshot() noexcept = default;

        UiStateSnapshot (const UiStateSnapshot&) = delete;
        UiStateSnapshot& operator= (const UiStateSnapshot&) = delete;

        void reset() noexcept
        {
            currentPhase.store (0.0f, std::memory_order_relaxed);
            currentGainDb.store (0.0f, std::memory_order_relaxed);
            packedBarBeatTick.store (packBarBeatTick ({ 1, 1, 1 }), std::memory_order_relaxed);
            inputLevelDb.store (kMinDb, std::memory_order_relaxed);
            outputLevelDb.store (kMinDb, std::memory_order_release);
        }

        // Publica una instantanea completa desde el hilo de audio (unico escritor) con dB ya calculados.
        void publish (float phase,
                      float gainDb,
                      dsp::BarBeatTick tick,
                      float inLevelDb,
                      float outLevelDb) noexcept
        {
            currentPhase.store (sanitizePhase (phase), std::memory_order_relaxed);
            currentGainDb.store (sanitizeDb (gainDb, 0.0f), std::memory_order_relaxed);
            packedBarBeatTick.store (packBarBeatTick (tick), std::memory_order_relaxed);
            inputLevelDb.store (sanitizeDb (inLevelDb, kMinDb), std::memory_order_relaxed);
            outputLevelDb.store (sanitizeDb (outLevelDb, kMinDb), std::memory_order_release);
        }

        // Sobrecarga que recibe la ganancia instantanea lineal G en [0, 1] y la convierte a dB en [-60, 0].
        void publishFromLinearGain (float phase,
                                    float gainLinear,
                                    dsp::BarBeatTick tick,
                                    float inLevelDb,
                                    float outLevelDb) noexcept
        {
            const float clampedLinear = math::clamp01 (math::finiteOr (gainLinear, 1.0f));
            const float gainDb        = math::gainToDb (clampedLinear);
            publish (phase, gainDb, tick, inLevelDb, outLevelDb);
        }

        // Lee una instantanea coherente desde el hilo de UI.
        [[nodiscard]] UiStateData read() const noexcept
        {
            UiStateData data;
            data.outputLevelDb = outputLevelDb.load (std::memory_order_acquire);
            data.inputLevelDb  = inputLevelDb.load (std::memory_order_relaxed);
            data.barBeatTick   = unpackBarBeatTick (packedBarBeatTick.load (std::memory_order_relaxed));
            data.currentGainDb = currentGainDb.load (std::memory_order_relaxed);
            data.currentPhase  = currentPhase.load (std::memory_order_relaxed);
            return data;
        }

        [[nodiscard]] float getPhase() const noexcept
        {
            return currentPhase.load (std::memory_order_relaxed);
        }

        [[nodiscard]] float getGainDb() const noexcept
        {
            return currentGainDb.load (std::memory_order_relaxed);
        }

        [[nodiscard]] dsp::BarBeatTick getBarBeatTick() const noexcept
        {
            return unpackBarBeatTick (packedBarBeatTick.load (std::memory_order_relaxed));
        }

        [[nodiscard]] float getInputLevelDb() const noexcept
        {
            return inputLevelDb.load (std::memory_order_relaxed);
        }

        [[nodiscard]] float getOutputLevelDb() const noexcept
        {
            return outputLevelDb.load (std::memory_order_relaxed);
        }

        [[nodiscard]] static constexpr std::uint64_t packBarBeatTick (dsp::BarBeatTick tick) noexcept
        {
            const int safeBar  = tick.bar < 1 ? 1 : tick.bar;
            const int safeBeat = tick.beat < 1 ? 1 : (tick.beat > 65535 ? 65535 : tick.beat);
            const int safeSub  = tick.subdivision < 1 ? 1 : (tick.subdivision > 65535 ? 65535 : tick.subdivision);

            const auto b  = static_cast<std::uint64_t> (static_cast<std::uint32_t> (safeBar));
            const auto bt = static_cast<std::uint64_t> (static_cast<std::uint16_t> (safeBeat));
            const auto sb = static_cast<std::uint64_t> (static_cast<std::uint16_t> (safeSub));

            return (b << 32u) | (bt << 16u) | sb;
        }

        [[nodiscard]] static constexpr dsp::BarBeatTick unpackBarBeatTick (std::uint64_t packed) noexcept
        {
            const auto rawBar  = static_cast<int> (static_cast<std::uint32_t> (packed >> 32u));
            const auto rawBeat = static_cast<int> (static_cast<std::uint16_t> ((packed >> 16u) & 0xFFFFu));
            const auto rawSub  = static_cast<int> (static_cast<std::uint16_t> (packed & 0xFFFFu));

            return {
                rawBar < 1 ? 1 : rawBar,
                rawBeat < 1 ? 1 : rawBeat,
                rawSub < 1 ? 1 : rawSub
            };
        }

        [[nodiscard]] static float sanitizePhase (float phase) noexcept
        {
            return math::frac (phase);
        }

        [[nodiscard]] static float sanitizeDb (float db, float fallback) noexcept
        {
            const float finite = math::finiteOr (db, fallback);
            return std::clamp (finite, kMinDb, kMaxDb);
        }

    private:
        std::atomic<float> currentPhase { 0.0f };
        std::atomic<float> currentGainDb { 0.0f };
        std::atomic<std::uint64_t> packedBarBeatTick { packBarBeatTick ({ 1, 1, 1 }) };
        std::atomic<float> inputLevelDb { kMinDb };
        std::atomic<float> outputLevelDb { kMinDb };
    };
}

#pragma once

#include "DSP/PhaseClock.h"

#include <span>

// Maquina de estados de transporte (DSP_SPEC.md §3, T-06).
// Modulo puro, noexcept, sin heap ni dependencias de JUCE o UI: el Processor convierte
// juce::AudioPlayHead::PositionInfo a HostTransport antes de llamar.
namespace umbral::dsp
{
    // Instantanea de los datos de transporte del host para un bloque.
    struct HostTransport
    {
        bool hasInfo         = false; // false si no hay PlayHead / posicion disponible
        bool isPlaying       = false;

        bool hasPpq          = false;
        double ppq           = 0.0;

        bool hasBpm          = false;
        double bpm           = 120.0;

        bool hasTimeSig      = false;
        TimeSignature timeSig { 4, 4 };

        bool hasBarStartPpq  = false;
        double barStartPpq   = 0.0;
    };

    enum class TransportMode : int
    {
        Locked = 0, // SYNC on, host reproduciendo con ppq valido: fase del DAW
        Free   = 1, // SYNC off: acumulador interno gobernado por el parametro bpm
        Bypass = 2  // SYNC on sin reproduccion o sin datos: fase congelada, ganancia 1.0
    };

    struct TransportResult
    {
        TransportMode mode = TransportMode::Bypass;
        BarBeatTick tick {};
        float endPhase = 0.0f; // fase proyectada tras el ultimo sample del tramo
    };

    class TransportClock
    {
    public:
        TransportClock() noexcept = default;

        void prepare (double sampleRate) noexcept;
        void reset() noexcept;

        // Llena outPhases con la fase por muestra del tramo.
        // sampleOffset: muestras ya consumidas del bloque del host (para procesar un bloque en
        // varios tramos); solo afecta al modo Locked, donde el ppq del host se avanza.
        TransportResult process (bool sync, const HostTransport& host, double paramBpm,
                                 RateDivision rate, std::span<float> outPhases,
                                 int sampleOffset = 0) noexcept;

        [[nodiscard]] float getLastPhase() const noexcept { return lastPhase; }

    private:
        double sampleRate = 44100.0;
        double freePhase  = 0.0;  // acumulador libre en [0, 1)
        float lastPhase   = 0.0f; // ultima fase valida emitida
        BarBeatTick lastTick {};
    };
}

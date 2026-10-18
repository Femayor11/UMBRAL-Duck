#pragma once

#include "Common/MathUtils.h"
#include "DSP/CurveBank.h"
#include "DSP/DuckingEngine.h"
#include "DSP/PhaseClock.h"

#include <juce_graphics/juce_graphics.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>

// Funciones puras de mapeo, paths y formato de texto para GainResponseView (T-10).
// Sin estado, sin acceso a Graphics y sin dependencia del hilo de audio: la curva se evalua con la
// misma funcion que usa el DSP (DuckingEngine::computeGain), asi la vista coincide 1:1 con el audio.
namespace umbral::ui::geometry
{
    // Rango visual del eje vertical. La lectura numerica puede reportar hasta -60 dB (recorte visual).
    inline constexpr float minDb = -24.0f;
    inline constexpr float maxDb = 0.0f;

    inline constexpr float snapshotMinDb = -60.0f;

    inline constexpr std::array<float, 5> gridDb    { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f };
    inline constexpr std::array<float, 5> gridPhase { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    // 257 puntos = 256 segmentos entre phi = 0 y phi = 1 para maxima nitidez en transitorios y escalones.
    inline constexpr int curvePointCount = 257;

    struct CurveParams
    {
        int curveIndex = dsp::defaultCurveIndex; // 04 Classic
        float mix      = 0.70f;                  // [0, 1]
        float shift    = 0.0f;                   // [0, 1]
        float slope    = 0.5f;                   // [0, 1]

        bool operator== (const CurveParams&) const = default;
    };

    [[nodiscard]] inline float phaseToX (const juce::Rectangle<float>& area, float phase) noexcept
    {
        const float p = math::clamp01 (math::finiteOr (phase, 0.0f));
        return area.getX() + p * area.getWidth();
    }

    [[nodiscard]] inline float dbToY (const juce::Rectangle<float>& area, float db) noexcept
    {
        const float clamped = std::clamp (math::finiteOr (db, maxDb), minDb, maxDb);
        const float t       = (maxDb - clamped) / (maxDb - minDb);
        return area.getY() + t * area.getHeight();
    }

    [[nodiscard]] inline float xToPhase (const juce::Rectangle<float>& area, float x) noexcept
    {
        if (area.getWidth() <= 0.0f)
            return 0.0f;

        return math::clamp01 ((math::finiteOr (x, area.getX()) - area.getX()) / area.getWidth());
    }

    [[nodiscard]] inline float yToDb (const juce::Rectangle<float>& area, float y) noexcept
    {
        if (area.getHeight() <= 0.0f)
            return maxDb;

        const float t = math::clamp01 ((math::finiteOr (y, area.getY()) - area.getY()) / area.getHeight());
        return maxDb + t * (minDb - maxDb);
    }

    // G(phi) = (1 - M) + M * C_slope(applyShift(phi, S)), convertido con gainToDb (DSP_SPEC).
    // En phi = 1.0f con shift = 0.0f preserva el valor final del ciclo sin envolver a 0.0f.
    [[nodiscard]] inline float effectiveGainDb (const CurveParams& p, float phi,
                                               std::span<const dsp::Node> customNodes = {}) noexcept
    {
        return math::gainToDb (dsp::DuckingEngine::computeGain (p.curveIndex, phi, p.shift, p.slope, p.mix, customNodes));
    }

    // Reconstruye la curva (trazo) y el relleno entre la curva y la linea de 0 dB.
    // Usa los Path recibidos (con espacio reservado): no debe llamarse desde paint().
    inline void buildPaths (const juce::Rectangle<float>& area,
                            const CurveParams& params,
                            juce::Path& curvePath,
                            juce::Path& fillPath,
                            std::span<const dsp::Node> customNodes = {})
    {
        curvePath.clear();
        fillPath.clear();

        if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
            return;

        const float zeroDbY = dbToY (area, 0.0f);

        float firstX = 0.0f;
        float lastX  = 0.0f;

        for (int i = 0; i < curvePointCount; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (curvePointCount - 1);
            const float x   = phaseToX (area, phi);
            const float y   = dbToY (area, effectiveGainDb (params, phi, customNodes));

            if (i == 0)
            {
                firstX = x;
                curvePath.startNewSubPath (x, y);
                fillPath.startNewSubPath (x, zeroDbY);
                fillPath.lineTo (x, y);
            }
            else
            {
                curvePath.lineTo (x, y);
                fillPath.lineTo (x, y);
            }

            lastX = x;
        }

        fillPath.lineTo (lastX, zeroDbY);
        fillPath.lineTo (firstX, zeroDbY);
        fillPath.closeSubPath();
    }

    // Lectura en decimas de dB (acotada a [-60, 0]). Sirve para detectar cambios visibles.
    [[nodiscard]] inline int gainDbToTenths (float db) noexcept
    {
        const float clamped = std::clamp (math::finiteOr (db, 0.0f), snapshotMinDb, 0.0f);
        return static_cast<int> (std::lround (clamped * 10.0f));
    }

    // "-14.2 dB". Devuelve el numero de caracteres escritos (sin heap).
    inline int formatGainDb (float db, char* buffer, std::size_t size) noexcept
    {
        const float value = static_cast<float> (gainDbToTenths (db)) * 0.1f;
        return std::snprintf (buffer, size, "%.1f dB", static_cast<double> (value));
    }

    // "1.2.3" (compas.tiempo.subdivision).
    inline int formatBarBeat (dsp::BarBeatTick tick, char* buffer, std::size_t size) noexcept
    {
        return std::snprintf (buffer, size, "%d.%d.%d", tick.bar, tick.beat, tick.subdivision);
    }
}

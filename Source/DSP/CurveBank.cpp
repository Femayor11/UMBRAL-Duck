#include "DSP/CurveBank.h"

#include "Common/MathUtils.h"

#include <array>

namespace umbral::dsp
{
    namespace
    {
        // =====================================================================
        // Grupo A (01-08): Sidechain / Ducking Frontal
        // Cada rampa ascendente es un unico tramo continuo sin nodos intermedios
        // con derivada cero, eliminando micro-ondulaciones parasitas.
        // =====================================================================

        // 01 Click Duck: recuperacion ultrarrapida para transitorios
        constexpr std::array<Node, 4> clickDuck { {
            { 0.00f, 0.00f },
            { 0.02f, 0.00f, SegmentShape::Smoothstep },
            { 0.12f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 02 Tight: exponencial rapida convexa
        constexpr std::array<Node, 4> tight { {
            { 0.00f, 0.00f },
            { 0.04f, 0.00f, SegmentShape::PowerOut, 2.2f },
            { 0.28f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 03 Punch: exponencial media con codo marcado
        constexpr std::array<Node, 4> punch { {
            { 0.00f, 0.00f },
            { 0.08f, 0.00f, SegmentShape::PowerIn, 1.6f },
            { 0.55f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 04 Classic: curva suave estandar (default)
        constexpr std::array<Node, 4> classic { {
            { 0.00f, 0.00f },
            { 0.10f, 0.00f, SegmentShape::Smoothstep },
            { 0.78f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 05 Deep: recuperacion tardia/larga
        constexpr std::array<Node, 4> deep { {
            { 0.00f, 0.00f },
            { 0.42f, 0.00f, SegmentShape::Smoothstep },
            { 0.95f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 06 Linear: rampa recta pura 0 a 1
        constexpr std::array<Node, 2> linear { {
            { 0.00f, 0.00f, SegmentShape::Linear },
            { 1.00f, 1.00f }
        } };

        // 07 Logarithmic: subida rapida que desacelera arriba
        constexpr std::array<Node, 2> logarithmic { {
            { 0.00f, 0.00f, SegmentShape::PowerOut, 2.5f },
            { 1.00f, 1.00f }
        } };

        // 08 Slow Sweep: recuperacion progresiva que llega arriba al final
        constexpr std::array<Node, 2> slowSweep { {
            { 0.00f, 0.00f, SegmentShape::PowerIn, 2.5f },
            { 1.00f, 1.00f }
        } };

        // =====================================================================
        // Grupo B (09-16): Gating / Ritmicas / Pumping inverso
        // =====================================================================

        // 09 Reverse Pump: 0 dB al inicio, caida rapida al final
        constexpr std::array<Node, 4> reversePump { {
            { 0.00f, 1.00f },
            { 0.55f, 1.00f, SegmentShape::Smoothstep },
            { 0.95f, 0.00f },
            { 1.00f, 0.00f }
        } };

        // 10 Side-Drop: arriba la 1ra mitad, caida en rampa recta en la 2da
        constexpr std::array<Node, 3> sideDrop { {
            { 0.00f, 1.00f },
            { 0.50f, 1.00f, SegmentShape::Linear },
            { 1.00f, 0.00f }
        } };

        // 11 Trance Gate: onda cuadrada 50% ON (1.0), 50% OFF (0.0)
        constexpr std::array<Node, 4> tranceGate { {
            { 0.00f, 1.00f },
            { 0.50f, 1.00f },
            { 0.50f, 0.00f },
            { 1.00f, 0.00f }
        } };

        // 12 Stutter: doble pulso / gating rapido simetrico
        constexpr std::array<Node, 5> stutter { {
            { 0.00f, 0.00f, SegmentShape::Smoothstep },
            { 0.25f, 1.00f, SegmentShape::Smoothstep },
            { 0.50f, 0.00f, SegmentShape::Smoothstep },
            { 0.75f, 1.00f, SegmentShape::Smoothstep },
            { 1.00f, 0.00f }
        } };

        // 13 Offbeat Dip: caida en V suave en el contratiempo (phi = 0.50)
        constexpr std::array<Node, 5> offbeatDip { {
            { 0.00f, 1.00f },
            { 0.25f, 1.00f, SegmentShape::Smoothstep },
            { 0.50f, 0.00f, SegmentShape::Smoothstep },
            { 0.75f, 1.00f },
            { 1.00f, 1.00f }
        } };

        // 14 Sine Smooth: onda sigmoide suave continua
        constexpr std::array<Node, 4> sineSmooth { {
            { 0.00f, 0.50f, SegmentShape::Smoothstep },
            { 0.25f, 0.00f, SegmentShape::Smoothstep },
            { 0.75f, 1.00f, SegmentShape::Smoothstep },
            { 1.00f, 0.50f }
        } };

        // 15 Ramp Down: rampa recta descendente pura 1 a 0
        constexpr std::array<Node, 2> rampDown { {
            { 0.00f, 1.00f, SegmentShape::Linear },
            { 1.00f, 0.00f }
        } };

        [[nodiscard]] int sanitizeIndex (int curveIndex) noexcept
        {
            if (curveIndex < 0)
                return 0;
            if (curveIndex >= numCurves)
                return numCurves - 1;
            return curveIndex;
        }

        [[nodiscard]] float evaluateSegmentShape (SegmentShape shape, float tPrime, float exponent) noexcept
        {
            const float c = math::clamp01 (tPrime);
            switch (shape)
            {
                case SegmentShape::Linear:
                    return c;
                case SegmentShape::PowerIn:
                {
                    const float exp = std::max (0.05f, math::finiteOr (exponent, 1.0f));
                    return std::pow (c, exp);
                }
                case SegmentShape::PowerOut:
                {
                    const float exp = std::max (0.05f, math::finiteOr (exponent, 1.0f));
                    return 1.0f - std::pow (1.0f - c, exp);
                }
                case SegmentShape::Smoothstep:
                default:
                    return math::smoothstep (c);
            }
        }
    }

    std::span<const Node> getNodes (int curveIndex) noexcept
    {
        switch (sanitizeIndex (curveIndex))
        {
            case 0:  return clickDuck;
            case 1:  return tight;
            case 2:  return punch;
            case 3:  return classic;
            case 4:  return deep;
            case 5:  return linear;
            case 6:  return logarithmic;
            case 7:  return slowSweep;
            case 8:  return reversePump;
            case 9:  return sideDrop;
            case 10: return tranceGate;
            case 11: return stutter;
            case 12: return offbeatDip;
            case 13: return sineSmooth;
            case 14: return rampDown;
            default: return classic; // 15: 16 Custom -> 04 Classic (v1)
        }
    }

    float evaluateNodes (std::span<const Node> nodes, float phiIn, float slope) noexcept
    {
        if (nodes.empty())
            return 1.0f;

        const float phi = math::clamp01 (math::finiteOr (phiIn, math::fallbackPhase));

        float result = nodes.front().value;

        for (std::size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const Node& a = nodes[i];
            const Node& b = nodes[i + 1];
            const float dx = b.phi - a.phi;

            if (dx <= math::zeroWidthEpsilon)
            {
                // Escalon: sin division; salta al valor del segundo nodo.
                if (phi >= b.phi)
                {
                    result = b.value;
                    continue;
                }
                break;
            }

            if (phi >= b.phi)
            {
                result = b.value;
                continue;
            }

            const float t      = math::clamp01 ((phi - a.phi) / dx);
            const float tPrime = math::applySlope (t, slope);
            const float s      = evaluateSegmentShape (a.shape, tPrime, a.exponent);
            result             = math::lerp (a.value, b.value, s);
            break;
        }

        return math::clamp01 (result);
    }

    float evaluate (int curveIndex, float phi, float shift, float slope) noexcept
    {
        return evaluateNodes (getNodes (curveIndex), math::applyShift (phi, shift), slope);
    }
}

#include "DSP/CurveBank.h"

#include "Common/MathUtils.h"

#include <array>

namespace umbral::dsp
{
    namespace
    {
        constexpr std::array<Node, 5> classic   { { { 0.00f, 0.00f }, { 0.12f, 0.00f }, { 0.35f, 0.30f }, { 0.65f, 0.85f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> sharp     { { { 0.00f, 0.00f }, { 0.05f, 0.00f }, { 0.25f, 0.90f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 6> smooth    { { { 0.00f, 1.00f }, { 0.15f, 0.80f }, { 0.42f, 0.00f }, { 0.58f, 0.00f }, { 0.85f, 0.80f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> tight     { { { 0.00f, 0.00f }, { 0.04f, 0.00f }, { 0.18f, 1.00f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> reverse   { { { 0.00f, 1.00f }, { 0.65f, 0.85f }, { 0.92f, 0.00f }, { 1.00f, 0.00f } } };
        constexpr std::array<Node, 5> rhythmic  { { { 0.00f, 0.00f }, { 0.25f, 1.00f }, { 0.50f, 0.35f }, { 0.75f, 1.00f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> deep      { { { 0.00f, 0.00f }, { 0.45f, 0.00f }, { 0.80f, 0.75f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> light     { { { 0.00f, 0.35f }, { 0.15f, 0.35f }, { 0.45f, 1.00f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 5> pump      { { { 0.00f, 0.00f }, { 0.25f, 0.05f }, { 0.65f, 0.40f }, { 0.92f, 1.00f }, { 1.00f, 1.00f } } };
        constexpr std::array<Node, 4> flow      { { { 0.00f, 0.50f }, { 0.25f, 0.00f }, { 0.75f, 1.00f }, { 1.00f, 0.50f } } };
        constexpr std::array<Node, 4> step      { { { 0.00f, 0.00f }, { 0.50f, 0.00f }, { 0.50f, 1.00f }, { 1.00f, 1.00f } } };

        [[nodiscard]] int sanitizeIndex (int curveIndex) noexcept
        {
            if (curveIndex < 0)
                return 0;
            if (curveIndex >= numCurves)
                return numCurves - 1;
            return curveIndex;
        }
    }

    std::span<const Node> getNodes (int curveIndex) noexcept
    {
        switch (sanitizeIndex (curveIndex))
        {
            case 0:  return classic;
            case 1:  return sharp;
            case 2:  return smooth;
            case 3:  return tight;
            case 4:  return reverse;
            case 5:  return rhythmic;
            case 6:  return deep;
            case 7:  return light;
            case 8:  return pump;
            case 9:  return flow;
            case 10: return step;
            default: return classic; // 11: Custom -> 01 Classic (v1)
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
                // Escalón: sin división; salta al valor del segundo nodo.
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

            const float t = math::clamp01 ((phi - a.phi) / dx);
            const float s = math::smoothstep (math::applySlope (t, slope));
            result = math::lerp (a.value, b.value, s);
            break;
        }

        return math::clamp01 (result);
    }

    float evaluate (int curveIndex, float phi, float shift, float slope) noexcept
    {
        return evaluateNodes (getNodes (curveIndex), math::applyShift (phi, shift), slope);
    }
}

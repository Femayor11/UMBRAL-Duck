#include "DSP/CustomCurve.h"
#include "Common/MathUtils.h"

#include <algorithm>
#include <cmath>

namespace umbral::dsp
{
    CustomCurveData CustomCurveData::createDefault() noexcept
    {
        CustomCurveData data;
        const auto classicNodes = getNodes (defaultCurveIndex); // 04 Classic
        data.numNodes = std::min (static_cast<int> (classicNodes.size()), kMaxNodes);
        for (int i = 0; i < data.numNodes; ++i)
            data.nodes[static_cast<std::size_t> (i)] = classicNodes[static_cast<std::size_t> (i)];
        return data;
    }

    bool CustomCurveData::isValid() const noexcept
    {
        if (numNodes < kMinNodes || numNodes > kMaxNodes)
            return false;

        // Comprobacion de limites extremos
        if (std::abs (nodes[0].phi - 0.0f) > 1e-5f)
            return false;

        const auto lastIdx = static_cast<std::size_t> (numNodes - 1);
        if (std::abs (nodes[lastIdx].phi - 1.0f) > 1e-5f)
            return false;

        // Comprobacion de monotonicidad estricta y valores finitos en [0, 1]
        for (int i = 0; i < numNodes; ++i)
        {
            const auto& n = nodes[static_cast<std::size_t> (i)];
            if (! std::isfinite (n.phi) || ! std::isfinite (n.value) || ! std::isfinite (n.exponent))
                return false;

            if (n.value < -1e-4f || n.value > 1.0f + 1e-4f)
                return false;

            if (i > 0)
            {
                const auto& prev = nodes[static_cast<std::size_t> (i - 1)];
                if (n.phi < prev.phi + kMinSpacing - 1e-5f)
                    return false;
            }
        }

        return true;
    }

    int CustomCurveData::insertNode (float phi, float value, SegmentShape shape, float exponent) noexcept
    {
        if (numNodes >= kMaxNodes)
            return -1;

        const float safePhi = math::clamp01 (math::finiteOr (phi, 0.0f));
        const float safeVal = math::clamp01 (math::finiteOr (value, 0.0f));

        // Debe respetar kMinSpacing respecto al inicio y fin
        if (safePhi < kMinSpacing || safePhi > 1.0f - kMinSpacing)
            return -1;

        // Encontrar posicion de insercion
        int insertIdx = 1;
        while (insertIdx < numNodes && nodes[static_cast<std::size_t> (insertIdx)].phi < safePhi)
            ++insertIdx;

        // Comprobar distancia con vecino anterior y siguiente
        const float prevPhi = nodes[static_cast<std::size_t> (insertIdx - 1)].phi;
        if (safePhi < prevPhi + kMinSpacing)
            return -1;

        if (insertIdx < numNodes)
        {
            const float nextPhi = nodes[static_cast<std::size_t> (insertIdx)].phi;
            if (nextPhi < safePhi + kMinSpacing)
                return -1;
        }

        // Desplazar elementos hacia la derecha
        for (int i = numNodes; i > insertIdx; --i)
            nodes[static_cast<std::size_t> (i)] = nodes[static_cast<std::size_t> (i - 1)];

        nodes[static_cast<std::size_t> (insertIdx)] = Node {
            safePhi,
            safeVal,
            shape,
            std::max (0.05f, math::finiteOr (exponent, 1.0f))
        };

        ++numNodes;
        return insertIdx;
    }

    bool CustomCurveData::removeNode (int index) noexcept
    {
        if (numNodes <= kMinNodes)
            return false;

        // Nodos 0 y numNodes - 1 no se pueden eliminar
        if (index <= 0 || index >= numNodes - 1)
            return false;

        for (int i = index; i < numNodes - 1; ++i)
            nodes[static_cast<std::size_t> (i)] = nodes[static_cast<std::size_t> (i + 1)];

        --numNodes;
        return true;
    }

    bool CustomCurveData::moveNode (int index, float newPhi, float newValue) noexcept
    {
        if (index < 0 || index >= numNodes)
            return false;

        const float safeVal = math::clamp01 (math::finiteOr (newValue, 0.0f));

        if (index == 0)
        {
            nodes[0].phi   = 0.0f;
            nodes[0].value = safeVal;
            return true;
        }

        if (index == numNodes - 1)
        {
            nodes[static_cast<std::size_t> (index)].phi   = 1.0f;
            nodes[static_cast<std::size_t> (index)].value = safeVal;
            return true;
        }

        const float minAllowedPhi = nodes[static_cast<std::size_t> (index - 1)].phi + kMinSpacing;
        const float maxAllowedPhi = nodes[static_cast<std::size_t> (index + 1)].phi - kMinSpacing;

        if (minAllowedPhi > maxAllowedPhi)
            return false;

        const float clampedPhi = std::clamp (math::finiteOr (newPhi, nodes[static_cast<std::size_t> (index)].phi),
                                            minAllowedPhi, maxAllowedPhi);

        nodes[static_cast<std::size_t> (index)].phi   = clampedPhi;
        nodes[static_cast<std::size_t> (index)].value = safeVal;
        return true;
    }

    void CustomCurveData::setNodeShape (int index, SegmentShape shape, float exponent) noexcept
    {
        if (index >= 0 && index < numNodes)
        {
            nodes[static_cast<std::size_t> (index)].shape    = shape;
            nodes[static_cast<std::size_t> (index)].exponent = std::max (0.05f, math::finiteOr (exponent, 1.0f));
        }
    }

    bool CustomCurveData::operator== (const CustomCurveData& other) const noexcept
    {
        if (numNodes != other.numNodes)
            return false;

        for (int i = 0; i < numNodes; ++i)
        {
            const auto& a = nodes[static_cast<std::size_t> (i)];
            const auto& b = other.nodes[static_cast<std::size_t> (i)];

            if (std::abs (a.phi - b.phi) > 1e-5f)
                return false;
            if (std::abs (a.value - b.value) > 1e-5f)
                return false;
            if (a.shape != b.shape)
                return false;
            if (std::abs (a.exponent - b.exponent) > 1e-5f)
                return false;
        }

        return true;
    }
}

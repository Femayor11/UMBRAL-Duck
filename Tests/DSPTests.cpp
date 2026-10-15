#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Common/MathUtils.h"
#include "DSP/CurveBank.h"

#include <array>
#include <cmath>
#include <limits>

using Catch::Matchers::WithinAbs;
namespace math = umbral::math;
namespace dsp = umbral::dsp;

static_assert (noexcept (math::frac (0.0f)));
static_assert (noexcept (math::applyShift (0.0f, 0.0f)));
static_assert (noexcept (math::applySlope (0.0f, 0.5f)));
static_assert (noexcept (math::gainToDb (1.0f)));
static_assert (noexcept (dsp::getNodes (0)));
static_assert (noexcept (dsp::evaluateNodes ({}, 0.0f, 0.5f)));
static_assert (noexcept (dsp::evaluate (0, 0.0f, 0.0f, 0.5f)));

namespace
{
    constexpr float eps = 1.0e-4f;
    constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
    constexpr float kInf = std::numeric_limits<float>::infinity();
}

TEST_CASE ("MathUtils: frac soporta negativos y se mantiene en [0,1)", "[dsp][math]")
{
    CHECK_THAT (math::frac (0.25f), WithinAbs (0.25f, 1e-6));
    CHECK_THAT (math::frac (-0.25f), WithinAbs (0.75f, 1e-6));
    CHECK_THAT (math::frac (3.5f), WithinAbs (0.5f, 1e-6));
    CHECK_THAT (math::frac (-3.25f), WithinAbs (0.75f, 1e-6));
    CHECK (math::frac (1.0f) == 0.0f);
    CHECK (math::frac (-1.0f) == 0.0f);
    CHECK (math::frac (-1.0e-10f) < 1.0f);
    CHECK (math::frac (kNaN) == 0.0f);
    CHECK (math::frac (kInf) == 0.0f);
}

TEST_CASE ("MathUtils: clamp01, lerp y smoothstep", "[dsp][math]")
{
    CHECK (math::clamp01 (-1.0f) == 0.0f);
    CHECK (math::clamp01 (2.0f) == 1.0f);
    CHECK (math::clamp01 (0.3f) == 0.3f);

    CHECK (math::lerp (2.0f, 6.0f, 0.0f) == 2.0f);
    CHECK (math::lerp (2.0f, 6.0f, 1.0f) == 6.0f);
    CHECK_THAT (math::lerp (2.0f, 6.0f, 0.25f), WithinAbs (3.0f, 1e-6));

    CHECK (math::smoothstep (0.0f) == 0.0f);
    CHECK (math::smoothstep (1.0f) == 1.0f);
    CHECK_THAT (math::smoothstep (0.5f), WithinAbs (0.5f, 1e-6));
    CHECK (math::smoothstep (-1.0f) == 0.0f);
    CHECK (math::smoothstep (2.0f) == 1.0f);

    float prev = 0.0f;
    bool monotonic = true;
    for (int i = 1; i <= 100; ++i)
    {
        const float v = math::smoothstep (static_cast<float> (i) / 100.0f);
        monotonic = monotonic && v >= prev;
        prev = v;
    }
    CHECK (monotonic);
}

TEST_CASE ("MathUtils: SLOPE p = 2^((K-0.5)*2)", "[dsp][math]")
{
    CHECK_THAT (math::slopeExponent (0.5f), WithinAbs (1.0f, 1e-6));
    CHECK_THAT (math::slopeExponent (0.0f), WithinAbs (0.5f, 1e-6));
    CHECK_THAT (math::slopeExponent (1.0f), WithinAbs (2.0f, 1e-6));
    CHECK_THAT (math::slopeExponent (kNaN), WithinAbs (1.0f, 1e-6));
    CHECK_THAT (math::slopeExponent (kInf), WithinAbs (1.0f, 1e-6));
    CHECK_THAT (math::slopeExponent (5.0f), WithinAbs (2.0f, 1e-6));

    CHECK_THAT (math::applySlope (0.5f, 0.5f), WithinAbs (0.5f, 1e-6));
    CHECK_THAT (math::applySlope (0.25f, 1.0f), WithinAbs (0.0625f, 1e-6));
    CHECK_THAT (math::applySlope (0.25f, 0.0f), WithinAbs (0.5f, 1e-6));
    CHECK (math::applySlope (0.0f, 0.0f) == 0.0f);
    CHECK (math::applySlope (1.0f, 1.0f) == 1.0f);
}

TEST_CASE ("MathUtils: shift, ganancia y dB", "[dsp][math]")
{
    CHECK_THAT (math::applyShift (0.3f, 0.0f), WithinAbs (0.3f, 1e-6));
    CHECK_THAT (math::applyShift (0.3f, 0.5f), WithinAbs (0.8f, 1e-6));
    CHECK_THAT (math::applyShift (0.3f, 1.0f), WithinAbs (0.3f, 1e-6));
    CHECK_THAT (math::applyShift (kNaN, 0.25f), WithinAbs (0.75f, 1e-6));
    CHECK_THAT (math::applyShift (0.3f, kInf), WithinAbs (0.3f, 1e-6));

    CHECK (math::gainFromCurve (0.0f, 0.0f) == 1.0f);
    CHECK (math::gainFromCurve (0.0f, 1.0f) == 0.0f);
    CHECK_THAT (math::gainFromCurve (0.0f, 0.7f), WithinAbs (0.3f, 1e-6));
    CHECK_THAT (math::gainFromCurve (0.5f, 0.5f), WithinAbs (0.75f, 1e-6));

    CHECK_THAT (math::gainToDb (1.0f), WithinAbs (0.0f, 1e-5));
    CHECK_THAT (math::gainToDb (0.0f), WithinAbs (-60.0f, 1e-3));
    CHECK_THAT (math::gainToDb (0.5f), WithinAbs (-6.0206f, 1e-3));
    CHECK (std::isfinite (math::gainToDb (kNaN)));
}

TEST_CASE ("CurveBank: extremos de nodos de cada curva", "[dsp][curves]")
{
    for (int c = 0; c < dsp::numCurves; ++c)
    {
        const auto nodes = dsp::getNodes (c);
        REQUIRE (nodes.size() >= 2);
        CHECK (nodes.front().phi == 0.0f);
        CHECK (nodes.back().phi == 1.0f);
        // phi = 1 devuelve exactamente el último nodo; phi = 0 el primero.
        CHECK (dsp::evaluateNodes (nodes, 1.0f, 0.5f) == nodes.back().value);
        CHECK (dsp::evaluateNodes (nodes, 0.0f, 0.5f) == nodes.front().value);
        CHECK (dsp::evaluateNodes (nodes, 5.0f, 0.5f) == nodes.back().value);
        CHECK (dsp::evaluateNodes (nodes, -5.0f, 0.5f) == nodes.front().value);
    }
}

TEST_CASE ("CurveBank: nodos ordenados", "[dsp][curves]")
{
    for (int c = 0; c < dsp::numCurves; ++c)
    {
        const auto nodes = dsp::getNodes (c);
        bool sorted = true;
        for (std::size_t i = 1; i < nodes.size(); ++i)
            sorted = sorted && nodes[i].phi >= nodes[i - 1].phi;
        CHECK (sorted);
    }
}

TEST_CASE ("CurveBank: puntos esperados", "[dsp][curves]")
{
    // 01 Classic
    CHECK_THAT (dsp::evaluate (0, 0.12f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (0, 0.35f, 0.0f, 0.5f), WithinAbs (0.30f, eps));
    CHECK_THAT (dsp::evaluate (0, 0.65f, 0.0f, 0.5f), WithinAbs (0.85f, eps));
    CHECK_THAT (dsp::evaluate (0, 0.235f, 0.0f, 0.5f), WithinAbs (0.15f, eps)); // t=0.5 -> smoothstep 0.5

    // 02 Sharp
    CHECK_THAT (dsp::evaluate (1, 0.05f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (1, 0.25f, 0.0f, 0.5f), WithinAbs (0.90f, eps));

    // 03 Smooth: valle en 1/2
    CHECK_THAT (dsp::evaluate (2, 0.0f, 0.0f, 0.5f), WithinAbs (1.0f, eps));
    CHECK_THAT (dsp::evaluate (2, 0.15f, 0.0f, 0.5f), WithinAbs (0.80f, eps));
    CHECK_THAT (dsp::evaluate (2, 0.42f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (2, 0.50f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (2, 0.58f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (2, 0.85f, 0.0f, 0.5f), WithinAbs (0.80f, eps));

    // 04 Tight
    CHECK_THAT (dsp::evaluate (3, 0.04f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (3, 0.18f, 0.0f, 0.5f), WithinAbs (1.0f, eps));

    // 05 Reverse
    CHECK_THAT (dsp::evaluate (4, 0.0f, 0.0f, 0.5f), WithinAbs (1.0f, eps));
    CHECK_THAT (dsp::evaluate (4, 0.65f, 0.0f, 0.5f), WithinAbs (0.85f, eps));
    CHECK_THAT (dsp::evaluate (4, 0.92f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluateNodes (dsp::getNodes (4), 1.0f, 0.5f), WithinAbs (0.0f, eps));

    // 06 Rhythmic
    CHECK_THAT (dsp::evaluate (5, 0.25f, 0.0f, 0.5f), WithinAbs (1.0f, eps));
    CHECK_THAT (dsp::evaluate (5, 0.50f, 0.0f, 0.5f), WithinAbs (0.35f, eps));
    CHECK_THAT (dsp::evaluate (5, 0.75f, 0.0f, 0.5f), WithinAbs (1.0f, eps));

    // 07 Deep
    CHECK_THAT (dsp::evaluate (6, 0.45f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (6, 0.80f, 0.0f, 0.5f), WithinAbs (0.75f, eps));

    // 08 Light
    CHECK_THAT (dsp::evaluate (7, 0.0f, 0.0f, 0.5f), WithinAbs (0.35f, eps));
    CHECK_THAT (dsp::evaluate (7, 0.15f, 0.0f, 0.5f), WithinAbs (0.35f, eps));
    CHECK_THAT (dsp::evaluate (7, 0.45f, 0.0f, 0.5f), WithinAbs (1.0f, eps));

    // 09 Pump
    CHECK_THAT (dsp::evaluate (8, 0.25f, 0.0f, 0.5f), WithinAbs (0.05f, eps));
    CHECK_THAT (dsp::evaluate (8, 0.65f, 0.0f, 0.5f), WithinAbs (0.40f, eps));
    CHECK_THAT (dsp::evaluate (8, 0.92f, 0.0f, 0.5f), WithinAbs (1.0f, eps));

    // 10 Flow
    CHECK_THAT (dsp::evaluate (9, 0.0f, 0.0f, 0.5f), WithinAbs (0.5f, eps));
    CHECK_THAT (dsp::evaluate (9, 0.25f, 0.0f, 0.5f), WithinAbs (0.0f, eps));
    CHECK_THAT (dsp::evaluate (9, 0.50f, 0.0f, 0.5f), WithinAbs (0.5f, eps)); // punto medio simétrico
    CHECK_THAT (dsp::evaluate (9, 0.75f, 0.0f, 0.5f), WithinAbs (1.0f, eps));
}

TEST_CASE ("CurveBank: 11 Step sin division por cero", "[dsp][curves]")
{
    CHECK (dsp::evaluate (10, 0.0f, 0.0f, 0.5f) == 0.0f);
    CHECK (dsp::evaluate (10, 0.25f, 0.0f, 0.5f) == 0.0f);
    CHECK (dsp::evaluate (10, 0.4999f, 0.0f, 0.5f) == 0.0f);
    CHECK (dsp::evaluate (10, 0.5f, 0.0f, 0.5f) == 1.0f);
    CHECK (dsp::evaluate (10, 0.5001f, 0.0f, 0.5f) == 1.0f);
    CHECK (dsp::evaluateNodes (dsp::getNodes (10), 1.0f, 0.5f) == 1.0f);

    bool allFinite = true;
    for (int i = 0; i <= 1000; ++i)
    {
        const float phi = static_cast<float> (i) / 1000.0f;
        for (float k : { 0.0f, 0.5f, 1.0f })
            allFinite = allFinite && std::isfinite (dsp::evaluate (10, phi, 0.0f, k));
    }
    CHECK (allFinite);
}

TEST_CASE ("CurveBank: segmentos de ancho cero genericos", "[dsp][curves]")
{
    constexpr std::array<dsp::Node, 3> nodes { { { 0.0f, 0.2f }, { 0.0f, 0.8f }, { 1.0f, 0.8f } } };
    CHECK (dsp::evaluateNodes (nodes, 0.0f, 0.5f) == 0.8f);
    CHECK (std::isfinite (dsp::evaluateNodes (nodes, 0.5f, 0.0f)));

    constexpr std::array<dsp::Node, 3> tiny { { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f + 5.0e-7f, 1.0f } } };
    CHECK (dsp::evaluateNodes (tiny, 0.99f, 0.5f) == 1.0f);

    CHECK (dsp::evaluateNodes ({}, 0.5f, 0.5f) == 1.0f);
}

TEST_CASE ("CurveBank: 12 Custom delega a 01 Classic", "[dsp][curves]")
{
    const auto classic = dsp::getNodes (0);
    const auto custom = dsp::getNodes (dsp::customCurveIndex);
    CHECK (classic.data() == custom.data());
    CHECK (classic.size() == custom.size());

    bool equal = true;
    for (int i = 0; i <= 200; ++i)
    {
        const float phi = static_cast<float> (i) / 200.0f;
        for (float s : { 0.0f, 0.3f })
            for (float k : { 0.0f, 0.5f, 1.0f })
                equal = equal && dsp::evaluate (11, phi, s, k) == dsp::evaluate (0, phi, s, k);
    }
    CHECK (equal);
}

TEST_CASE ("CurveBank: indices fuera de rango se acotan", "[dsp][curves]")
{
    CHECK (dsp::evaluate (-5, 0.35f, 0.0f, 0.5f) == dsp::evaluate (0, 0.35f, 0.0f, 0.5f));
    CHECK (dsp::evaluate (99, 0.35f, 0.0f, 0.5f) == dsp::evaluate (11, 0.35f, 0.0f, 0.5f));
}

TEST_CASE ("CurveBank: SHIFT desplaza la lectura con wrap ciclico", "[dsp][curves]")
{
    // C(frac(phi - S)): con S=0.25, phi=0.6 lee Classic en 0.35.
    CHECK_THAT (dsp::evaluate (0, 0.6f, 0.25f, 0.5f), WithinAbs (0.30f, eps));
    // Wrap: phi=0.1, S=0.25 -> 0.85
    CHECK_THAT (dsp::evaluate (0, 0.1f, 0.25f, 0.5f), WithinAbs (dsp::evaluate (0, 0.85f, 0.0f, 0.5f), eps));
    // S=1 equivale a S=0
    for (int c = 0; c < dsp::numCurves; ++c)
        CHECK_THAT (dsp::evaluate (c, 0.37f, 1.0f, 0.5f), WithinAbs (dsp::evaluate (c, 0.37f, 0.0f, 0.5f), eps));
    // phi fuera de [0,1) se envuelve
    CHECK_THAT (dsp::evaluate (2, 1.5f, 0.0f, 0.5f), WithinAbs (dsp::evaluate (2, 0.5f, 0.0f, 0.5f), eps));
    CHECK_THAT (dsp::evaluate (2, -0.5f, 0.0f, 0.5f), WithinAbs (dsp::evaluate (2, 0.5f, 0.0f, 0.5f), eps));
}

TEST_CASE ("CurveBank: SLOPE deforma el tiempo del segmento", "[dsp][curves]")
{
    // Segmento Classic (0.12,0)->(0.35,0.30), punto medio t=0.5.
    CHECK_THAT (dsp::evaluate (0, 0.235f, 0.0f, 0.5f), WithinAbs (0.15f, eps));
    CHECK_THAT (dsp::evaluate (0, 0.235f, 0.0f, 1.0f), WithinAbs (0.046875f, eps));  // p=2
    CHECK_THAT (dsp::evaluate (0, 0.235f, 0.0f, 0.0f), WithinAbs (0.237868f, eps));  // p=0.5

    // Los nodos no cambian con SLOPE.
    for (int c = 0; c < dsp::numCurves; ++c)
        for (float k : { 0.0f, 1.0f })
            CHECK_THAT (dsp::evaluate (c, 0.0f, 0.0f, k), WithinAbs (dsp::getNodes (c).front().value, eps));

    // También aplica en tramos descendentes (Reverse: 0.65 -> 0.92, 0.85 -> 0).
    const float mid = 0.65f + 0.5f * (0.92f - 0.65f);
    CHECK (dsp::evaluate (4, mid, 0.0f, 1.0f) > dsp::evaluate (4, mid, 0.0f, 0.0f));
}

TEST_CASE ("CurveBank: salida siempre finita y en [0,1] ante entradas extremas", "[dsp][curves]")
{
    const float badValues[] = { kNaN, kInf, -kInf, -1.0e30f, 1.0e30f, -3.7f, 0.0f, 0.5f, 1.0f, 2.5f };

    bool ok = true;
    for (int c = -1; c <= dsp::numCurves; ++c)
        for (float phi : badValues)
            for (float shift : badValues)
                for (float slope : badValues)
                {
                    const float v = dsp::evaluate (c, phi, shift, slope);
                    ok = ok && std::isfinite (v) && v >= 0.0f && v <= 1.0f;
                }
    CHECK (ok);
}

TEST_CASE ("CurveBank: barrido denso finito y acotado", "[dsp][curves]")
{
    bool ok = true;
    for (int c = 0; c < dsp::numCurves; ++c)
        for (int i = 0; i <= 500; ++i)
        {
            const float phi = static_cast<float> (i) / 500.0f;
            for (float shift : { 0.0f, 0.13f, 0.5f, 0.99f })
                for (float slope : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    const float v = dsp::evaluate (c, phi, shift, slope);
                    ok = ok && std::isfinite (v) && v >= 0.0f && v <= 1.0f;
                }
        }
    CHECK (ok);
}

TEST_CASE ("CurveBank: kNaN/kInf usan fallbacks seguros", "[dsp][curves]")
{
    // phi/shift no finitos -> 0.0f; slope no finito -> 0.5f
    CHECK (dsp::evaluate (0, kNaN, 0.0f, 0.5f) == dsp::evaluate (0, 0.0f, 0.0f, 0.5f));
    CHECK (dsp::evaluate (0, 0.6f, kInf, 0.5f) == dsp::evaluate (0, 0.6f, 0.0f, 0.5f));
    CHECK (dsp::evaluate (0, 0.235f, 0.0f, kNaN) == dsp::evaluate (0, 0.235f, 0.0f, 0.5f));
    CHECK (dsp::evaluateNodes (dsp::getNodes (0), kNaN, kNaN) == dsp::evaluateNodes (dsp::getNodes (0), 0.0f, 0.5f));
}

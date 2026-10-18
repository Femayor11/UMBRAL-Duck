#pragma once

#include "DSP/CurveBank.h"

#include <array>
#include <cstddef>
#include <span>

namespace umbral::dsp
{
    // Modelo de datos para curvas personalizadas (16 Custom).
    // Capacidad: 2 a 16 nodos.
    // Restricciones:
    // - Nodo 0 anclado en phi = 0.0 (value editable en [0, 1]).
    // - Ultimo nodo anclado en phi = 1.0 (value editable en [0, 1]).
    // - Nodos intermedios con phi estrictamente creciente y separacion minima kMinSpacing.
    struct CustomCurveData
    {
        static constexpr int kMaxNodes     = 16;
        static constexpr int kMinNodes     = 2;
        static constexpr float kMinSpacing = 0.005f;

        std::array<Node, kMaxNodes> nodes {};
        int numNodes = 0;

        // Devuelve la curva por defecto (equivalente a 04 Classic)
        [[nodiscard]] static CustomCurveData createDefault() noexcept;

        // Comprueba si la estructura cumple con las restricciones matematicas
        [[nodiscard]] bool isValid() const noexcept;

        // Obtiene la vista en span de los nodos activos
        [[nodiscard]] std::span<const Node> getSpan() const noexcept
        {
            if (numNodes <= 0)
                return {};
            return std::span<const Node> (nodes.data(), static_cast<std::size_t> (numNodes));
        }

        // Inserta un nodo ordenadamente segun phi (si hay espacio y respeta kMinSpacing).
        // Devuelve el indice insertado, o -1 si falla.
        int insertNode (float phi, float value, SegmentShape shape = SegmentShape::Smoothstep, float exponent = 1.0f) noexcept;

        // Elimina un nodo intermedio (no permite eliminar nodo 0 ni el ultimo nodo si quedan kMinNodes).
        // Devuelve true si se elimino con exito.
        bool removeNode (int index) noexcept;

        // Mueve la posicion (phi, value) de un nodo respetando los limites de sus vecinos y kMinSpacing.
        // Nodos extremos (0 y numNodes - 1) tienen su phi congelado en 0.0 y 1.0 respectivamente.
        bool moveNode (int index, float newPhi, float newValue) noexcept;

        // Establece la forma o exponente de un nodo.
        void setNodeShape (int index, SegmentShape shape, float exponent = 1.0f) noexcept;

        bool operator== (const CustomCurveData& other) const noexcept;
    };
}

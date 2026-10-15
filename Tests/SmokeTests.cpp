#include <catch2/catch_test_macros.hpp>

// T-01: prueba de humo para que CTest tenga un ejecutable real.
// La lógica DSP se probará desde T-03.
TEST_CASE ("Test harness works", "[smoke]")
{
    REQUIRE (1 + 1 == 2);
}

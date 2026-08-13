// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <array>
#include <cstdint>

int main()
{
    const carrot25519_impl *impl =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    std::array<std::uint8_t, 32> scalar{};
    std::array<std::uint8_t, 32> point{};
    std::array<std::uint8_t, 32> output{};
    scalar[0] = 1;
    point[0] = 9;
    carrot25519_mul_base(impl, output.data(), scalar.data());
    if (output != point)
        return 1;
    carrot25519_mul(impl, output.data(), scalar.data(), point.data());
    return output == point ? 0 : 1;
}

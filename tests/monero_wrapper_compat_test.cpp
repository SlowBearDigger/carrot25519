// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <array>
#include <cstdint>

struct bytes32 {
    std::array<std::uint8_t, 32> data{};
};

static const carrot25519_impl *implementation()
{
    static const carrot25519_impl *const value =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    return value;
}

static bytes32 mul_base(const bytes32 &scalar)
{
    bytes32 output;
    carrot25519_mul_base(
        implementation(), output.data.data(), scalar.data.data());
    return output;
}

static bytes32 mul(const bytes32 &scalar, const bytes32 &point)
{
    bytes32 output;
    carrot25519_mul(
        implementation(), output.data.data(), scalar.data.data(),
        point.data.data());
    return output;
}

int main()
{
    bytes32 scalar;
    scalar.data[0] = 1;
    bytes32 basepoint;
    basepoint.data[0] = 9;
    return implementation() != nullptr && mul_base(scalar).data == basepoint.data &&
                   mul(scalar, basepoint).data == basepoint.data
               ? 0
               : 1;
}

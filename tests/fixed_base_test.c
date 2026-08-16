// SPDX-License-Identifier: MIT

#include "carrot25519.h"
#include "fixed_base/fixed_base.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int equal(const uint8_t left[32], const uint8_t right[32])
{
    return memcmp(left, right, 32) == 0;
}

static void scalar_2i_plus_j(uint8_t scalar[32], unsigned bit, unsigned addend)
{
    memset(scalar, 0, 32);
    scalar[0] = (uint8_t)addend;
    unsigned carry = 1U << (bit % 8);
    for (size_t index = bit / 8; carry != 0 && index < 32; ++index)
    {
        const unsigned sum = scalar[index] + carry;
        scalar[index] = (uint8_t)sum;
        carry = sum >> 8;
    }
}

static int check_scalar(
    const carrot25519_impl *portable, const uint8_t scalar[32])
{
    static const uint8_t basepoint[32] = {9};
    uint8_t expected[32];
    uint8_t actual[32];
    uint8_t alias[32];

    carrot25519_mul(portable, expected, scalar, basepoint);
    carrot25519_fixed_base(actual, scalar);
    if (!equal(actual, expected))
        return 0;

    memcpy(alias, scalar, sizeof(alias));
    carrot25519_fixed_base(alias, alias);
    return equal(alias, expected);
}

static void fill_scalar(uint8_t scalar[32], uint32_t seed)
{
    uint32_t value = seed + UINT32_C(0x9e3779b9);
    for (size_t index = 0; index < 32; ++index)
    {
        value = value * UINT32_C(1664525) + UINT32_C(1013904223);
        scalar[index] = (uint8_t)(value >> 24);
    }
}

int main(void)
{
    const carrot25519_impl *portable =
        carrot25519_select_impl(CARROT25519_IMPL_PORTABLE);
    static const uint8_t edges[][32] = {
        {0},
        {1},
        {2},
        {4},
        {7},
        {8},
    };
    size_t cases = 0;

    if (portable == NULL)
        return 1;
    for (size_t index = 0; index < sizeof(edges) / sizeof(edges[0]); ++index)
    {
        if (!check_scalar(portable, edges[index]))
            return 1;
        ++cases;
    }
    uint8_t maximum[32];
    memset(maximum, 0xff, sizeof(maximum));
    if (!check_scalar(portable, maximum))
        return 1;
    ++cases;
    for (unsigned bit = 0; bit < 255; ++bit)
    {
        for (unsigned addend = 0; addend < 8; ++addend)
        {
            uint8_t scalar[32];
            scalar_2i_plus_j(scalar, bit, addend);
            if (!check_scalar(portable, scalar))
                return 1;
            ++cases;
        }
    }

    uint8_t scalar[32] = {0x53, 0xd1, 0x8c, 0x26, 0xe4, 0x7a, 0xb9, 0x05};
    uint8_t toggled[32];
    uint8_t first[32];
    uint8_t second[32];
    memcpy(toggled, scalar, sizeof(toggled));
    toggled[31] ^= UINT8_C(0x80);
    carrot25519_fixed_base(first, scalar);
    carrot25519_fixed_base(second, toggled);
    if (!equal(first, second))
        return 1;
    ++cases;

    for (uint32_t seed = 0; seed < 4096; ++seed)
    {
        fill_scalar(scalar, seed);
        if (!check_scalar(portable, scalar))
            return 1;
        ++cases;
    }

    printf("fixed_base_cases=%zu\n", cases);
    return 0;
}

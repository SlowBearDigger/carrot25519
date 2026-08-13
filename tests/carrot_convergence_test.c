// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t small_order_points[][32] = {
    {0},
    {1},
    {
        0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae,
        0x16, 0x56, 0xe3, 0xfa, 0xf1, 0x9f, 0xc4, 0x6a,
        0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32, 0xb1, 0xfd,
        0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x00,
    },
    {
        0x5f, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24,
        0xb1, 0xd0, 0xb1, 0x55, 0x9c, 0x83, 0xef, 0x5b,
        0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c, 0x8e, 0x86,
        0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0x57,
    },
    {
        0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
    },
    {
        0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
    },
    {
        0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
    },
};

static int equal(const uint8_t left[32], const uint8_t right[32])
{
    return memcmp(left, right, 32) == 0;
}

static int is_zero(const uint8_t value[32])
{
    static const uint8_t zero[32];
    return equal(value, zero);
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

static size_t available_implementations(const carrot25519_impl *out[4])
{
    size_t count = 0;
    for (uint32_t id = CARROT25519_IMPL_PORTABLE;
         id <= CARROT25519_IMPL_X86_64_BMI2_ADX; ++id)
    {
        const carrot25519_impl *impl =
            carrot25519_select_impl((carrot25519_impl_id)id);
        if (impl != NULL)
            out[count++] = impl;
    }
    return count;
}

static int scalar_matrix(
    const carrot25519_impl *const implementations[4], size_t count,
    size_t *cases)
{
    static const uint8_t raw_point[32] = {
        0x4d, 0x82, 0x17, 0xa9, 0x63, 0x0f, 0xb4, 0x56,
        0x38, 0xe1, 0x72, 0xcc, 0x95, 0x2a, 0x0b, 0x6f,
        0x73, 0x41, 0xd8, 0x2e, 0xa5, 0x19, 0xc0, 0x87,
        0x2b, 0xfd, 0x64, 0x90, 0x16, 0x3c, 0xe7, 0x5a,
    };
    static const uint8_t basepoint[32] = {9};
    const carrot25519_impl *portable = implementations[0];
    uint8_t eleven[32] = {11};
    uint8_t generated_point[32];
    carrot25519_mul_base(portable, generated_point, eleven);
    const uint8_t *points[] = {basepoint, generated_point, raw_point};

    for (unsigned bit = 0; bit < 255; ++bit)
    {
        for (unsigned addend = 0; addend < 8; ++addend)
        {
            uint8_t scalar[32];
            uint8_t expected_base[32];
            uint8_t expected_point[3][32];
            scalar_2i_plus_j(scalar, bit, addend);
            carrot25519_mul_base(portable, expected_base, scalar);
            for (size_t point = 0; point < 3; ++point)
                carrot25519_mul(
                    portable, expected_point[point], scalar, points[point]);

            for (size_t impl = 0; impl < count; ++impl)
            {
                uint8_t actual[32];
                carrot25519_mul_base(implementations[impl], actual, scalar);
                if (!equal(actual, expected_base))
                    return 0;
                carrot25519_mul(
                    implementations[impl], actual, scalar, basepoint);
                if (!equal(actual, expected_base))
                    return 0;
                for (size_t point = 0; point < 3; ++point)
                {
                    carrot25519_mul(
                        implementations[impl], actual, scalar, points[point]);
                    if (!equal(actual, expected_point[point]))
                        return 0;
                }
                *cases += 5;
            }
        }
    }
    return 1;
}

static int edge_contract(
    const carrot25519_impl *const implementations[4], size_t count,
    size_t *cases)
{
    static const uint8_t seed[32] = {
        0x53, 0xd1, 0x8c, 0x26, 0xe4, 0x7a, 0xb9, 0x05,
        0x11, 0x68, 0xf2, 0x39, 0x9d, 0xc7, 0x44, 0xae,
        0x7b, 0x02, 0xd8, 0x91, 0x35, 0xfc, 0x60, 0x1e,
        0xa3, 0x4d, 0x87, 0x5a, 0xc0, 0x2f, 0x76, 0x19,
    };
    static const uint8_t point[32] = {9};
    uint8_t toggled[32];
    memcpy(toggled, seed, 32);
    toggled[31] ^= 0x80;

    for (size_t impl = 0; impl < count; ++impl)
    {
        uint8_t first[32];
        uint8_t second[32];
        carrot25519_mul_base(implementations[impl], first, seed);
        carrot25519_mul_base(implementations[impl], second, toggled);
        if (!equal(first, second))
            return 0;
        carrot25519_mul(implementations[impl], first, seed, point);
        carrot25519_mul(implementations[impl], second, toggled, point);
        if (!equal(first, second))
            return 0;

        uint8_t eight[32] = {8};
        for (size_t index = 0;
             index < sizeof(small_order_points) / sizeof(small_order_points[0]);
             ++index)
        {
            carrot25519_mul(
                implementations[impl], first, eight,
                small_order_points[index]);
            if (!is_zero(first))
                return 0;
        }

        uint8_t seven[32] = {7};
        for (size_t index = 2; index < 4; ++index)
        {
            carrot25519_mul(
                implementations[impl], first, seven,
                small_order_points[index]);
            if (is_zero(first))
                return 0;
        }
        *cases += 11;
    }
    return 1;
}

static int ecdh_pair(
    const carrot25519_impl *impl, const uint8_t left[32],
    const uint8_t right[32])
{
    uint8_t left_public[32];
    uint8_t right_public[32];
    uint8_t left_shared[32];
    uint8_t right_shared[32];
    carrot25519_mul_base(impl, left_public, left);
    carrot25519_mul_base(impl, right_public, right);
    carrot25519_mul(impl, left_shared, left, right_public);
    carrot25519_mul(impl, right_shared, right, left_public);
    return equal(left_shared, right_shared) && !is_zero(left_shared);
}

static int ecdh_convergence(
    const carrot25519_impl *const implementations[4], size_t count,
    size_t *cases)
{
    static const uint8_t legacy_left[32] = {0xf8, 0x41, 0x27, 0x95, [31] = 0x40};
    static const uint8_t legacy_right[32] = {0x38, 0xac, 0x72, 0x19, [31] = 0x40};
    static const uint8_t carrot_left[32] = {0x07, 0x91, 0x62, 0xd4, [31] = 0x21};
    static const uint8_t carrot_right[32] = {0x05, 0x33, 0xb8, 0x4a, [31] = 0x63};

    for (size_t impl = 0; impl < count; ++impl)
    {
        if (!ecdh_pair(implementations[impl], legacy_left, legacy_right) ||
            !ecdh_pair(implementations[impl], carrot_left, carrot_right))
            return 0;
        *cases += 2;
    }
    return 1;
}

int main(void)
{
    const carrot25519_impl *implementations[4];
    const size_t count = available_implementations(implementations);
    size_t cases = 0;
    if (count == 0 ||
        carrot25519_impl_id_of(implementations[0]) !=
            CARROT25519_IMPL_PORTABLE ||
        !scalar_matrix(implementations, count, &cases) ||
        !edge_contract(implementations, count, &cases) ||
        !ecdh_convergence(implementations, count, &cases))
        return 1;
    printf("implementations=%zu convergence_cases=%zu\n", count, cases);
    return 0;
}

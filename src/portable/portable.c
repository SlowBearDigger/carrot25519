// SPDX-License-Identifier: MIT

#include "internal.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fiat/curve25519_64.c"

typedef fiat_25519_tight_field_element fe51;

static void fe51_copy(fe51 out, const fe51 in)
{
    memcpy(out, in, sizeof(fe51));
}

static void fe51_add(fe51 out, const fe51 left, const fe51 right)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_add(loose, left, right);
    fiat_25519_carry(out, loose);
}

static void fe51_sub(fe51 out, const fe51 left, const fe51 right)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_sub(loose, left, right);
    fiat_25519_carry(out, loose);
}

static void fe51_mul(fe51 out, const fe51 left, const fe51 right)
{
    fiat_25519_loose_field_element loose_left;
    fiat_25519_loose_field_element loose_right;
    fiat_25519_relax(loose_left, left);
    fiat_25519_relax(loose_right, right);
    fiat_25519_carry_mul(out, loose_left, loose_right);
}

static void fe51_square(fe51 out, const fe51 value)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_relax(loose, value);
    fiat_25519_carry_square(out, loose);
}

static void fe51_mul_121666(fe51 out, const fe51 value)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_relax(loose, value);
    fiat_25519_carry_scmul_121666(out, loose);
}

static void fe51_cswap(fe51 left, fe51 right, uint64_t swap)
{
    const uint64_t mask = UINT64_C(0) - swap;
    for (size_t i = 0; i < 5; ++i)
    {
        const uint64_t delta = mask & (left[i] ^ right[i]);
        left[i] ^= delta;
        right[i] ^= delta;
    }
}

static void fe51_invert(fe51 out, const fe51 value)
{
    fe51 t0;
    fe51 t1;
    fe51 t2;
    fe51 t3;

    fe51_square(t0, value);
    fe51_square(t1, t0);
    fe51_square(t1, t1);
    fe51_mul(t1, value, t1);
    fe51_mul(t0, t0, t1);
    fe51_square(t2, t0);
    fe51_mul(t1, t1, t2);
    fe51_square(t2, t1);
    for (int i = 0; i < 4; ++i)
        fe51_square(t2, t2);
    fe51_mul(t1, t2, t1);
    fe51_square(t2, t1);
    for (int i = 0; i < 9; ++i)
        fe51_square(t2, t2);
    fe51_mul(t2, t2, t1);
    fe51_square(t3, t2);
    for (int i = 0; i < 19; ++i)
        fe51_square(t3, t3);
    fe51_mul(t2, t3, t2);
    fe51_square(t2, t2);
    for (int i = 0; i < 9; ++i)
        fe51_square(t2, t2);
    fe51_mul(t1, t2, t1);
    fe51_square(t2, t1);
    for (int i = 0; i < 49; ++i)
        fe51_square(t2, t2);
    fe51_mul(t2, t2, t1);
    fe51_square(t3, t2);
    for (int i = 0; i < 99; ++i)
        fe51_square(t3, t3);
    fe51_mul(t2, t3, t2);
    fe51_square(t2, t2);
    for (int i = 0; i < 49; ++i)
        fe51_square(t2, t2);
    fe51_mul(t1, t2, t1);
    fe51_square(t1, t1);
    for (int i = 0; i < 4; ++i)
        fe51_square(t1, t1);
    fe51_mul(out, t1, t0);

    carrot25519_secure_zero(t0, sizeof(t0));
    carrot25519_secure_zero(t1, sizeof(t1));
    carrot25519_secure_zero(t2, sizeof(t2));
    carrot25519_secure_zero(t3, sizeof(t3));
}

static int greater_than_or_equal_le(
    const uint8_t left[32], const uint8_t right[32])
{
    for (size_t i = 32; i-- != 0;)
    {
        if (left[i] != right[i])
            return left[i] > right[i];
    }
    return 1;
}

static void subtract_le(uint8_t left[32], const uint8_t right[32])
{
    uint16_t borrow = 0;
    for (size_t i = 0; i < 32; ++i)
    {
        const uint16_t subtrahend = (uint16_t)right[i] + borrow;
        const uint16_t value = left[i];
        left[i] = (uint8_t)(value - subtrahend);
        borrow = value < subtrahend;
    }
}

static void decode_u_coordinate(fe51 out, const uint8_t encoded[32])
{
    static const uint8_t modulus[32] = {
        0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
    uint8_t canonical[32];

    memcpy(canonical, encoded, sizeof(canonical));
    canonical[31] &= UINT8_C(0x7f);
    if (greater_than_or_equal_le(canonical, modulus))
        subtract_le(canonical, modulus);
    fiat_25519_from_bytes(out, canonical);
    carrot25519_secure_zero(canonical, sizeof(canonical));
}

static void portable_mul(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
    uint8_t scalar_bits[32];
    fe51 x_1;
    fe51 x_2 = {1};
    fe51 z_2 = {0};
    fe51 x_3;
    fe51 z_3 = {1};
    fe51 a;
    fe51 aa;
    fe51 b;
    fe51 bb;
    fe51 e;
    fe51 c;
    fe51 d;
    fe51 da;
    fe51 cb;
    fe51 temporary;
    fe51 inverse;
    uint64_t swap = 0;

    memcpy(scalar_bits, scalar, sizeof(scalar_bits));
    scalar_bits[31] &= UINT8_C(0x7f);
    decode_u_coordinate(x_1, point);
    fe51_copy(x_3, x_1);

    for (int bit_index = 254; bit_index >= 0; --bit_index)
    {
        const uint64_t bit =
            (scalar_bits[(unsigned)bit_index >> 3] >> (bit_index & 7)) & 1U;
        swap ^= bit;
        fe51_cswap(x_2, x_3, swap);
        fe51_cswap(z_2, z_3, swap);
        swap = bit;

        fe51_add(a, x_2, z_2);
        fe51_square(aa, a);
        fe51_sub(b, x_2, z_2);
        fe51_square(bb, b);
        fe51_sub(e, aa, bb);
        fe51_add(c, x_3, z_3);
        fe51_sub(d, x_3, z_3);
        fe51_mul(da, d, a);
        fe51_mul(cb, c, b);
        fe51_add(temporary, da, cb);
        fe51_square(x_3, temporary);
        fe51_sub(temporary, da, cb);
        fe51_square(temporary, temporary);
        fe51_mul(z_3, x_1, temporary);
        fe51_mul(x_2, aa, bb);
        fe51_mul_121666(temporary, e);
        fe51_add(temporary, bb, temporary);
        fe51_mul(z_2, e, temporary);
    }

    fe51_cswap(x_2, x_3, swap);
    fe51_cswap(z_2, z_3, swap);
    fe51_invert(inverse, z_2);
    fe51_mul(x_2, x_2, inverse);
    fiat_25519_to_bytes(out, x_2);

    carrot25519_secure_zero(scalar_bits, sizeof(scalar_bits));
    carrot25519_secure_zero(x_1, sizeof(x_1));
    carrot25519_secure_zero(x_2, sizeof(x_2));
    carrot25519_secure_zero(z_2, sizeof(z_2));
    carrot25519_secure_zero(x_3, sizeof(x_3));
    carrot25519_secure_zero(z_3, sizeof(z_3));
    carrot25519_secure_zero(a, sizeof(a));
    carrot25519_secure_zero(aa, sizeof(aa));
    carrot25519_secure_zero(b, sizeof(b));
    carrot25519_secure_zero(bb, sizeof(bb));
    carrot25519_secure_zero(e, sizeof(e));
    carrot25519_secure_zero(c, sizeof(c));
    carrot25519_secure_zero(d, sizeof(d));
    carrot25519_secure_zero(da, sizeof(da));
    carrot25519_secure_zero(cb, sizeof(cb));
    carrot25519_secure_zero(temporary, sizeof(temporary));
    carrot25519_secure_zero(inverse, sizeof(inverse));
}

static void portable_mul_base(uint8_t out[32], const uint8_t scalar[32])
{
    static const uint8_t basepoint[32] = {9};
    portable_mul(out, scalar, basepoint);
}

const carrot25519_impl carrot25519_portable_impl = {
    CARROT25519_IMPL_PORTABLE,
    "portable/fiat51",
    portable_mul_base,
    portable_mul};

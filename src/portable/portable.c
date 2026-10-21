// SPDX-License-Identifier: MIT

#include "internal.h"
#include "fixed_base/fixed_base.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Full-width 64-bit MUL is variable-time on Cortex-A53/A55. */
#if defined(__aarch64__) || defined(__arm64__)
#include "fiat/curve25519_32.c"
typedef uint32_t fe_word;
#define fe_value_barrier fiat_25519_value_barrier_u32
#define PORTABLE_NAME "portable/fiat25.5"
#else
#include "fiat/curve25519_64.c"
typedef uint64_t fe_word;
#define fe_value_barrier fiat_25519_value_barrier_u64
#define PORTABLE_NAME "portable/fiat51"
#endif

typedef fiat_25519_tight_field_element fe;

static void fe_copy(fe out, const fe in)
{
    memcpy(out, in, sizeof(fe));
}

static void fe_add(fe out, const fe left, const fe right)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_add(loose, left, right);
    fiat_25519_carry(out, loose);
}

static void fe_sub(fe out, const fe left, const fe right)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_sub(loose, left, right);
    fiat_25519_carry(out, loose);
}

static void fe_mul(fe out, const fe left, const fe right)
{
    fiat_25519_loose_field_element loose_left;
    fiat_25519_loose_field_element loose_right;
    fiat_25519_relax(loose_left, left);
    fiat_25519_relax(loose_right, right);
    fiat_25519_carry_mul(out, loose_left, loose_right);
}

static void fe_square(fe out, const fe value)
{
#if defined(__aarch64__) || defined(__arm64__)
    /* The generated square introduces full-width product operands. */
    fe_mul(out, value, value);
#else
    fiat_25519_loose_field_element loose;
    fiat_25519_relax(loose, value);
    fiat_25519_carry_square(out, loose);
#endif
}

static void fe_mul_121666(fe out, const fe value)
{
    fiat_25519_loose_field_element loose;
    fiat_25519_relax(loose, value);
    fiat_25519_carry_scmul_121666(out, loose);
}

static void fe_cswap(fe left, fe right, uint64_t swap)
{
    const fe_word mask = fe_value_barrier((fe_word)0 - (fe_word)swap);
    for (size_t i = 0; i < sizeof(fe) / sizeof(fe_word); ++i)
    {
        const fe_word delta = mask & (left[i] ^ right[i]);
        left[i] ^= delta;
        right[i] ^= delta;
    }
}

static void fe_invert(fe out, const fe value)
{
    fe t0;
    fe t1;
    fe t2;
    fe t3;

    fe_square(t0, value);
    fe_square(t1, t0);
    fe_square(t1, t1);
    fe_mul(t1, value, t1);
    fe_mul(t0, t0, t1);
    fe_square(t2, t0);
    fe_mul(t1, t1, t2);
    fe_square(t2, t1);
    for (int i = 0; i < 4; ++i)
        fe_square(t2, t2);
    fe_mul(t1, t2, t1);
    fe_square(t2, t1);
    for (int i = 0; i < 9; ++i)
        fe_square(t2, t2);
    fe_mul(t2, t2, t1);
    fe_square(t3, t2);
    for (int i = 0; i < 19; ++i)
        fe_square(t3, t3);
    fe_mul(t2, t3, t2);
    fe_square(t2, t2);
    for (int i = 0; i < 9; ++i)
        fe_square(t2, t2);
    fe_mul(t1, t2, t1);
    fe_square(t2, t1);
    for (int i = 0; i < 49; ++i)
        fe_square(t2, t2);
    fe_mul(t2, t2, t1);
    fe_square(t3, t2);
    for (int i = 0; i < 99; ++i)
        fe_square(t3, t3);
    fe_mul(t2, t3, t2);
    fe_square(t2, t2);
    for (int i = 0; i < 49; ++i)
        fe_square(t2, t2);
    fe_mul(t1, t2, t1);
    fe_square(t1, t1);
    for (int i = 0; i < 4; ++i)
        fe_square(t1, t1);
    fe_mul(out, t1, t0);

    carrot25519_secure_zero(t0, sizeof(t0));
    carrot25519_secure_zero(t1, sizeof(t1));
    carrot25519_secure_zero(t2, sizeof(t2));
    carrot25519_secure_zero(t3, sizeof(t3));
}

static void decode_u_coordinate(fe out, const uint8_t encoded[32])
{
    static const uint8_t modulus[32] = {
        0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
    uint8_t canonical[32];
    uint8_t reduced[32];
    uint16_t borrow = 0;

    memcpy(canonical, encoded, sizeof(canonical));
    canonical[31] &= UINT8_C(0x7f);
    /* Masking bounds the input by p+18, so one subtraction suffices. */
    for (size_t i = 0; i < 32; ++i)
    {
        const uint16_t difference =
            (uint16_t)((uint16_t)canonical[i] - modulus[i] - borrow);
        reduced[i] = (uint8_t)difference;
        borrow = difference >> 15;
    }
    const fe_word mask = fe_value_barrier((fe_word)0 - (fe_word)(1U - borrow));
    for (size_t i = 0; i < 32; ++i)
        canonical[i] ^= (uint8_t)(mask & (canonical[i] ^ reduced[i]));
    fiat_25519_from_bytes(out, canonical);
    carrot25519_secure_zero(canonical, sizeof(canonical));
    carrot25519_secure_zero(reduced, sizeof(reduced));
}

static void portable_mul(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
    uint8_t scalar_bits[32];
    fe x_1;
    fe x_2 = {1};
    fe z_2 = {0};
    fe x_3;
    fe z_3 = {1};
    fe a;
    fe aa;
    fe b;
    fe bb;
    fe e;
    fe c;
    fe d;
    fe da;
    fe cb;
    fe temporary;
    fe inverse;
    uint64_t swap = 0;

    memcpy(scalar_bits, scalar, sizeof(scalar_bits));
    scalar_bits[31] &= UINT8_C(0x7f);
    decode_u_coordinate(x_1, point);
    fe_copy(x_3, x_1);

    for (int bit_index = 254; bit_index >= 0; --bit_index)
    {
        const uint64_t bit =
            (scalar_bits[(unsigned)bit_index >> 3] >> (bit_index & 7)) & 1U;
        swap ^= bit;
        fe_cswap(x_2, x_3, swap);
        fe_cswap(z_2, z_3, swap);
        swap = bit;

        fe_add(a, x_2, z_2);
        fe_square(aa, a);
        fe_sub(b, x_2, z_2);
        fe_square(bb, b);
        fe_sub(e, aa, bb);
        fe_add(c, x_3, z_3);
        fe_sub(d, x_3, z_3);
        fe_mul(da, d, a);
        fe_mul(cb, c, b);
        fe_add(temporary, da, cb);
        fe_square(x_3, temporary);
        fe_sub(temporary, da, cb);
        fe_square(temporary, temporary);
        fe_mul(z_3, x_1, temporary);
        fe_mul(x_2, aa, bb);
        fe_mul_121666(temporary, e);
        fe_add(temporary, bb, temporary);
        fe_mul(z_2, e, temporary);
    }

    fe_cswap(x_2, x_3, swap);
    fe_cswap(z_2, z_3, swap);
    fe_invert(inverse, z_2);
    fe_mul(x_2, x_2, inverse);
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
    carrot25519_fixed_base(out, scalar);
}

const carrot25519_impl carrot25519_portable_impl = {
    CARROT25519_IMPL_PORTABLE,
    PORTABLE_NAME,
    portable_mul_base,
    portable_mul};

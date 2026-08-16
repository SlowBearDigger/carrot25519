// SPDX-License-Identifier: MIT

#include "fixed_base.h"

#include "internal.h"
#include "ref10/ge.h"

#include <string.h>

void carrot25519_fixed_base(
    uint8_t out[32], const uint8_t scalar[32])
{
    uint8_t scalar_bits[32];
    ge_p3 point;
    fe numerator;
    fe denominator;
    fe inverse;

    memcpy(scalar_bits, scalar, sizeof(scalar_bits));
    scalar_bits[31] &= UINT8_C(0x7f);
    ge_scalarmult_base(&point, scalar_bits);

    fe_add(numerator, point.Z, point.Y);
    fe_sub(denominator, point.Z, point.Y);
    fe_invert(inverse, denominator);
    fe_mul(numerator, numerator, inverse);
    fe_tobytes(out, numerator);

    carrot25519_secure_zero(scalar_bits, sizeof(scalar_bits));
    carrot25519_secure_zero(&point, sizeof(point));
    carrot25519_secure_zero(numerator, sizeof(numerator));
    carrot25519_secure_zero(denominator, sizeof(denominator));
    carrot25519_secure_zero(inverse, sizeof(inverse));
}

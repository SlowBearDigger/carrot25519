// SPDX-License-Identifier: MIT

#include "internal.h"
#include "fixed_base/fixed_base.h"
#include "x86_64/x86_64.h"

#include "s2n/s2n-bignum.h"

#include <cpuid.h>
#include <stdint.h>
#include <string.h>

typedef void (*projective_mul_fn)(
    uint64_t out[8], const uint64_t scalar[4], const uint64_t point[4]);
typedef void (*field_mul_fn)(
    uint64_t out[4], const uint64_t left[4], const uint64_t right[4]);

carrot25519_impl_id carrot25519_x86_64_classify_features(
    int has_bmi2, int has_adx)
{
    return has_bmi2 && has_adx ? CARROT25519_IMPL_X86_64_BMI2_ADX
                               : CARROT25519_IMPL_X86_64_BASELINE;
}

int carrot25519_x86_64_runtime_has_bmi2_adx(void)
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    if (__get_cpuid_max(0, NULL) < 7)
        return 0;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    (void)eax;
    (void)ecx;
    (void)edx;
    return (ebx & bit_BMI2) != 0 && (ebx & bit_ADX) != 0;
}

static void composed_mul(
    uint8_t out[32], const uint8_t scalar_bytes[32],
    const uint8_t point_bytes[32], projective_mul_fn projective_mul,
    field_mul_fn field_mul)
{
    uint64_t scalar[4];
    uint64_t point_raw[4];
    uint64_t point[4];
    uint64_t projective[8];
    uint64_t inverse[4];
    uint64_t affine[4];

    memcpy(scalar, scalar_bytes, sizeof(scalar));
    scalar[3] &= UINT64_C(0x7fffffffffffffff);
    memcpy(point_raw, point_bytes, sizeof(point_raw));
    point_raw[3] &= UINT64_C(0x7fffffffffffffff);
    bignum_mod_p25519_4(point, point_raw);
    projective_mul(projective, scalar, point);
    bignum_inv_p25519(inverse, projective + 4);
    field_mul(affine, projective, inverse);
    memcpy(out, affine, sizeof(affine));

    carrot25519_secure_zero(affine, sizeof(affine));
    carrot25519_secure_zero(inverse, sizeof(inverse));
    carrot25519_secure_zero(projective, sizeof(projective));
    carrot25519_secure_zero(point, sizeof(point));
    carrot25519_secure_zero(point_raw, sizeof(point_raw));
    carrot25519_secure_zero(scalar, sizeof(scalar));
}

static void baseline_mul(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
    composed_mul(
        out, scalar, point, curve25519_pxscalarmul_alt,
        bignum_mul_p25519_alt);
}

static void fast_mul(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
    composed_mul(
        out, scalar, point, curve25519_pxscalarmul,
        bignum_mul_p25519);
}

static void fast_mul_base(uint8_t out[32], const uint8_t scalar[32])
{
    static const uint8_t basepoint[32] = {9};
    fast_mul(out, scalar, basepoint);
}

const carrot25519_impl carrot25519_x86_64_baseline_impl = {
    CARROT25519_IMPL_X86_64_BASELINE,
    "x86_64/s2n-baseline",
    carrot25519_fixed_base,
    baseline_mul};

const carrot25519_impl carrot25519_x86_64_bmi2_adx_impl = {
    CARROT25519_IMPL_X86_64_BMI2_ADX,
    "x86_64/s2n-bmi2-adx",
    fast_mul_base,
    fast_mul};

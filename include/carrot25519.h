// SPDX-License-Identifier: MIT

#ifndef CARROT25519_H
#define CARROT25519_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define CARROT25519_API __attribute__((visibility("default")))
#else
#define CARROT25519_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct carrot25519_impl carrot25519_impl;

typedef enum carrot25519_impl_id {
    CARROT25519_IMPL_AUTO = 0,
    CARROT25519_IMPL_PORTABLE = 1,
    CARROT25519_IMPL_ARM64 = 2,
    CARROT25519_IMPL_X86_64_BASELINE = 3,
    CARROT25519_IMPL_X86_64_BMI2_ADX = 4
} carrot25519_impl_id;

/*
 * Scalar bits 0 through 254 are processed unchanged. Bit 255 is ignored.
 * Point encodings ignore bit 255 and are reduced modulo 2^255 - 19.
 * The output is a canonical 32-byte little-endian ladder result.
 * A non-invertible final projective state is encoded as all zero.
 * Output may exactly alias either input. Partial overlaps are invalid.
 * Inputs and output may have arbitrary byte alignment. Null is invalid.
 * No point validation or all-zero rejection is performed.
 */
CARROT25519_API const carrot25519_impl *carrot25519_select_impl(
    carrot25519_impl_id requested);
CARROT25519_API carrot25519_impl_id carrot25519_impl_id_of(
    const carrot25519_impl *impl);
CARROT25519_API const char *carrot25519_impl_name(
    const carrot25519_impl *impl);
CARROT25519_API void carrot25519_mul_base(
    const carrot25519_impl *impl, uint8_t out[32],
    const uint8_t scalar[32]);
CARROT25519_API void carrot25519_mul(
    const carrot25519_impl *impl, uint8_t out[32],
    const uint8_t scalar[32], const uint8_t point[32]);

#ifdef __cplusplus
}
#endif

#endif

// SPDX-License-Identifier: MIT

#ifndef CARROT25519_INTERNAL_H
#define CARROT25519_INTERNAL_H

#include "carrot25519.h"

#include <stddef.h>
#include <stdint.h>

typedef void (*carrot25519_mul_base_fn)(
    uint8_t out[32], const uint8_t scalar[32]);
typedef void (*carrot25519_mul_fn)(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]);

struct carrot25519_impl {
    carrot25519_impl_id id;
    const char *name;
    carrot25519_mul_base_fn mul_base;
    carrot25519_mul_fn mul;
};

extern const carrot25519_impl carrot25519_portable_impl;
#if defined(CARROT25519_HAVE_ARM64)
extern const carrot25519_impl carrot25519_arm64_impl;
#endif
#if defined(CARROT25519_HAVE_X86_64)
extern const carrot25519_impl carrot25519_x86_64_baseline_impl;
extern const carrot25519_impl carrot25519_x86_64_bmi2_adx_impl;
int carrot25519_x86_64_runtime_has_bmi2_adx(void);
#endif

void carrot25519_secure_zero(void *memory, size_t length);

#endif

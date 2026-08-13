// SPDX-License-Identifier: MIT

#include "internal.h"

void carrot25519_arm64_scalarmult(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]);

static void arm64_mul(
    uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
    carrot25519_arm64_scalarmult(out, scalar, point);
}

static void arm64_mul_base(uint8_t out[32], const uint8_t scalar[32])
{
    static const uint8_t basepoint[32] = {9};
    carrot25519_arm64_scalarmult(out, scalar, basepoint);
}

const carrot25519_impl carrot25519_arm64_impl = {
    CARROT25519_IMPL_ARM64,
    "arm64/cc0",
    arm64_mul_base,
    arm64_mul};

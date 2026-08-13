// SPDX-License-Identifier: MIT

#include "internal.h"

#include <stddef.h>

const carrot25519_impl *carrot25519_select_impl(
    carrot25519_impl_id requested)
{
    switch (requested)
    {
    case CARROT25519_IMPL_AUTO:
    case CARROT25519_IMPL_PORTABLE:
        return &carrot25519_portable_impl;
    case CARROT25519_IMPL_ARM64:
    case CARROT25519_IMPL_X86_64_BASELINE:
    case CARROT25519_IMPL_X86_64_BMI2_ADX:
    default:
        return NULL;
    }
}

carrot25519_impl_id carrot25519_impl_id_of(const carrot25519_impl *impl)
{
    return impl->id;
}

const char *carrot25519_impl_name(const carrot25519_impl *impl)
{
    return impl->name;
}

void carrot25519_mul_base(
    const carrot25519_impl *impl, uint8_t out[32],
    const uint8_t scalar[32])
{
    impl->mul_base(out, scalar);
}

void carrot25519_mul(
    const carrot25519_impl *impl, uint8_t out[32],
    const uint8_t scalar[32], const uint8_t point[32])
{
    impl->mul(out, scalar, point);
}

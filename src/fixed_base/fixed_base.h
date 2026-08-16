// SPDX-License-Identifier: MIT

#ifndef CARROT25519_FIXED_BASE_H
#define CARROT25519_FIXED_BASE_H

#include <stdint.h>

void carrot25519_fixed_base(
    uint8_t out[32], const uint8_t scalar[32]);

#endif

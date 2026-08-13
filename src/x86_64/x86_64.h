// SPDX-License-Identifier: MIT

#ifndef CARROT25519_X86_64_H
#define CARROT25519_X86_64_H

#include "carrot25519.h"

carrot25519_impl_id carrot25519_x86_64_classify_features(
    int has_bmi2, int has_adx);
int carrot25519_x86_64_runtime_has_bmi2_adx(void);

#endif

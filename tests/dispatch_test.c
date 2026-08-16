// SPDX-License-Identifier: MIT

#include "carrot25519.h"
#include "fixed_base/fixed_base.h"
#include "internal.h"
#include "x86_64/x86_64.h"

#include <stddef.h>

static int classifier_is_exact(void)
{
    return carrot25519_x86_64_classify_features(0, 0) ==
               CARROT25519_IMPL_X86_64_BASELINE &&
           carrot25519_x86_64_classify_features(1, 0) ==
               CARROT25519_IMPL_X86_64_BASELINE &&
           carrot25519_x86_64_classify_features(0, 1) ==
               CARROT25519_IMPL_X86_64_BASELINE &&
           carrot25519_x86_64_classify_features(1, 1) ==
               CARROT25519_IMPL_X86_64_BMI2_ADX;
}

int main(void)
{
    const carrot25519_impl *baseline = carrot25519_select_impl(
        CARROT25519_IMPL_X86_64_BASELINE);
    const carrot25519_impl *fast = carrot25519_select_impl(
        CARROT25519_IMPL_X86_64_BMI2_ADX);
    const carrot25519_impl *automatic =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    const int has_fast = carrot25519_x86_64_runtime_has_bmi2_adx();

    if (!classifier_is_exact() || baseline == NULL || automatic == NULL ||
        carrot25519_x86_64_baseline_impl.mul_base !=
            carrot25519_fixed_base ||
        carrot25519_x86_64_bmi2_adx_impl.mul_base ==
            carrot25519_fixed_base ||
        carrot25519_impl_id_of(baseline) !=
            CARROT25519_IMPL_X86_64_BASELINE)
        return 1;

    if (has_fast)
        return fast != NULL && automatic == fast ? 0 : 1;
    return fast == NULL && automatic == baseline ? 0 : 1;
}

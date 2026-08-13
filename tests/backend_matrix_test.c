// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stddef.h>

int main(void)
{
    const carrot25519_impl *automatic =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    const carrot25519_impl *portable =
        carrot25519_select_impl(CARROT25519_IMPL_PORTABLE);
    if (automatic == NULL || portable == NULL)
        return 1;

#if defined(CARROT25519_TEST_EXPECT_ARM64)
    const carrot25519_impl *arm64 =
        carrot25519_select_impl(CARROT25519_IMPL_ARM64);
    if (arm64 == NULL || automatic != arm64 ||
        carrot25519_impl_id_of(arm64) != CARROT25519_IMPL_ARM64)
        return 1;
#else
    if (carrot25519_select_impl(CARROT25519_IMPL_ARM64) != NULL)
        return 1;
#endif

    return 0;
}

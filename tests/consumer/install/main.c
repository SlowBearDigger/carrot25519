// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stdint.h>
#include <string.h>

int main(void)
{
    static const uint8_t expected[32] = {9};
    uint8_t scalar[32] = {1};
    uint8_t output[32];
    const carrot25519_impl *impl =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    if (impl == NULL)
        return 1;
    carrot25519_mul_base(impl, output, scalar);
    return memcmp(output, expected, 32) == 0 ? 0 : 1;
}

// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stdint.h>
#include <string.h>

int main(void)
{
    static const uint8_t scalar_one[32] = {1};
    static const uint8_t basepoint[32] = {9};
    uint8_t output[32];

    const carrot25519_impl *automatic =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    const carrot25519_impl *portable =
        carrot25519_select_impl(CARROT25519_IMPL_PORTABLE);

    if (automatic == NULL || portable == NULL)
        return 1;
    const carrot25519_impl_id automatic_id =
        carrot25519_impl_id_of(automatic);
    if (automatic_id == CARROT25519_IMPL_AUTO ||
        automatic != carrot25519_select_impl(automatic_id))
        return 1;
    if (portable != carrot25519_select_impl(CARROT25519_IMPL_PORTABLE))
        return 1;
    if (carrot25519_impl_id_of(portable) != CARROT25519_IMPL_PORTABLE)
        return 1;
    if (strcmp(carrot25519_impl_name(portable), "portable/fiat51") != 0)
        return 1;
    if (carrot25519_select_impl((carrot25519_impl_id)UINT32_MAX) != NULL)
        return 1;

    carrot25519_mul_base(portable, output, scalar_one);
    if (memcmp(output, basepoint, sizeof(output)) != 0)
        return 1;
    carrot25519_mul(portable, output, scalar_one, basepoint);
    return memcmp(output, basepoint, sizeof(output)) == 0 ? 0 : 1;
}

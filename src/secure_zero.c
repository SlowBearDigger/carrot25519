// SPDX-License-Identifier: MIT

#include "internal.h"

#include <stdatomic.h>

void carrot25519_secure_zero(void *memory, size_t length)
{
    volatile uint8_t *bytes = memory;
    while (length-- != 0)
        *bytes++ = 0;
    atomic_signal_fence(memory_order_seq_cst);
}

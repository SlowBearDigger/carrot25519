// SPDX-License-Identifier: MIT

/* GCC trace-pc checks executed basic blocks, not elapsed wall-clock time. */
#include "../src/portable/portable.c"

#include <stdio.h>

static int tracing;
static uintptr_t trace_hash;
static size_t trace_count;

__attribute__((no_sanitize_coverage, noinline))
void __sanitizer_cov_trace_pc(void)
{
    if (tracing)
    {
        trace_hash = trace_hash * (uintptr_t)33 ^
                     (uintptr_t)__builtin_return_address(0);
        ++trace_count;
    }
}

static void begin_trace(void)
{
    trace_hash = 0;
    trace_count = 0;
    tracing = 1;
}

int main(void)
{
    static const uint8_t modulus[32] = {
        0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
    uintptr_t decode_hash = 0;
    size_t decode_count = 0;
    /* 0, p-1, every p..p+18, and both encodings of bit 255. */
    for (unsigned high_bit = 0; high_bit < 2; ++high_bit)
    {
        for (unsigned value = 0; value < 21; ++value)
        {
            uint8_t point[32] = {0};
            uint8_t expected[32] = {0};
            uint8_t actual[32];
            fiat_25519_tight_field_element decoded;
            if (value != 0)
            {
                memcpy(point, modulus, sizeof(point));
                point[0] = (uint8_t)(0xec + value - 1);
                if (value == 1)
                    memcpy(expected, point, sizeof(expected));
                else
                    expected[0] = (uint8_t)(value - 2);
            }
            point[31] |= (uint8_t)(high_bit << 7);
            begin_trace();
            decode_u_coordinate(decoded, point);
            tracing = 0;
            if (value == 0 && high_bit == 0)
            {
                decode_hash = trace_hash;
                decode_count = trace_count;
            }
            else if (trace_hash != decode_hash || trace_count != decode_count)
            {
                fprintf(stderr, "point decoding has input-dependent control flow\n");
                return 1;
            }
            fiat_25519_to_bytes(actual, decoded);
            if (memcmp(actual, expected, sizeof(actual)) != 0)
                return 2;
        }
    }

    uintptr_t ladder_hash = 0;
    size_t ladder_count = 0;
    for (unsigned value = 0; value < 8; ++value)
    {
        uint8_t scalar[32];
        uint8_t point[32] = {9};
        uint8_t output[32];
        memset(scalar, (int)(value * 37), sizeof(scalar));
        begin_trace();
        portable_mul(output, scalar, point);
        tracing = 0;
        if (value == 0)
        {
            ladder_hash = trace_hash;
            ladder_count = trace_count;
        }
        else if (trace_hash != ladder_hash || trace_count != ladder_count)
        {
            fprintf(stderr, "portable ladder has scalar-dependent control flow\n");
            return 3;
        }
    }
    puts("portable_schedule=pass (compiler control flow only)");
    return 0;
}

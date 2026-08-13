// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef CARROT25519_TEST_IMPL_ID
#error "CARROT25519_TEST_IMPL_ID is required"
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef struct guarded_buffer {
    uint8_t *mapping;
    size_t mapping_size;
    size_t page_size;
} guarded_buffer;

static int allocate_buffer(guarded_buffer *buffer)
{
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
        return 0;
    buffer->page_size = (size_t)page_size;
    buffer->mapping_size = buffer->page_size * 3;
    buffer->mapping = mmap(
        NULL, buffer->mapping_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);
    return buffer->mapping != MAP_FAILED &&
           mprotect(
               buffer->mapping + buffer->page_size, buffer->page_size,
               PROT_READ | PROT_WRITE) == 0;
}

static uint8_t *edge(guarded_buffer *buffer, int high)
{
    uint8_t *page = buffer->mapping + buffer->page_size;
    return high ? page + buffer->page_size - 32 : page;
}

static int set_access(guarded_buffer *buffer, int protection)
{
    return mprotect(
               buffer->mapping + buffer->page_size, buffer->page_size,
               protection) == 0;
}

int main(void)
{
    static const uint8_t scalar[32] = {1};
    static const uint8_t point[32] = {9};
    static const uint8_t expected[32] = {9};
    guarded_buffer scalar_buffer;
    guarded_buffer point_buffer;
    guarded_buffer output_buffer;
    size_t cases = 0;

    const carrot25519_impl *impl = carrot25519_select_impl(
        (carrot25519_impl_id)CARROT25519_TEST_IMPL_ID);
    if (impl == NULL || !allocate_buffer(&scalar_buffer) ||
        !allocate_buffer(&point_buffer) || !allocate_buffer(&output_buffer))
        return 2;

    for (int scalar_high = 0; scalar_high < 2; ++scalar_high)
    {
        for (int output_high = 0; output_high < 2; ++output_high)
        {
            uint8_t *scalar_at = edge(&scalar_buffer, scalar_high);
            uint8_t *output_at = edge(&output_buffer, output_high);
            if (!set_access(&scalar_buffer, PROT_READ | PROT_WRITE) ||
                !set_access(&output_buffer, PROT_READ | PROT_WRITE))
                return 2;
            memcpy(scalar_at, scalar, 32);
            if (!set_access(&scalar_buffer, PROT_READ))
                return 2;
            carrot25519_mul_base(impl, output_at, scalar_at);
            if (memcmp(output_at, expected, 32) != 0)
                return 1;
            ++cases;
        }

        for (int point_high = 0; point_high < 2; ++point_high)
        {
            for (int output_high = 0; output_high < 2; ++output_high)
            {
                uint8_t *scalar_at = edge(&scalar_buffer, scalar_high);
                uint8_t *point_at = edge(&point_buffer, point_high);
                uint8_t *output_at = edge(&output_buffer, output_high);
                if (!set_access(&scalar_buffer, PROT_READ | PROT_WRITE) ||
                    !set_access(&point_buffer, PROT_READ | PROT_WRITE) ||
                    !set_access(&output_buffer, PROT_READ | PROT_WRITE))
                    return 2;
                memcpy(scalar_at, scalar, 32);
                memcpy(point_at, point, 32);
                if (!set_access(&scalar_buffer, PROT_READ) ||
                    !set_access(&point_buffer, PROT_READ))
                    return 2;
                carrot25519_mul(impl, output_at, scalar_at, point_at);
                if (memcmp(output_at, expected, 32) != 0)
                    return 1;
                ++cases;
            }
        }
    }

    for (int high = 0; high < 2; ++high)
    {
        uint8_t *scalar_at = edge(&scalar_buffer, high);
        uint8_t *point_at = edge(&point_buffer, high);

        if (!set_access(&scalar_buffer, PROT_READ | PROT_WRITE))
            return 2;
        memcpy(scalar_at, scalar, 32);
        carrot25519_mul_base(impl, scalar_at, scalar_at);
        if (memcmp(scalar_at, expected, 32) != 0)
            return 1;
        ++cases;

        if (!set_access(&scalar_buffer, PROT_READ | PROT_WRITE) ||
            !set_access(&point_buffer, PROT_READ | PROT_WRITE))
            return 2;
        memcpy(scalar_at, scalar, 32);
        memcpy(point_at, point, 32);
        if (!set_access(&point_buffer, PROT_READ))
            return 2;
        carrot25519_mul(impl, scalar_at, scalar_at, point_at);
        if (memcmp(scalar_at, expected, 32) != 0)
            return 1;
        ++cases;

        if (!set_access(&scalar_buffer, PROT_READ | PROT_WRITE) ||
            !set_access(&point_buffer, PROT_READ | PROT_WRITE))
            return 2;
        memcpy(scalar_at, scalar, 32);
        memcpy(point_at, point, 32);
        if (!set_access(&scalar_buffer, PROT_READ))
            return 2;
        carrot25519_mul(impl, point_at, scalar_at, point_at);
        if (memcmp(point_at, expected, 32) != 0)
            return 1;
        ++cases;
    }

    if (munmap(scalar_buffer.mapping, scalar_buffer.mapping_size) != 0 ||
        munmap(point_buffer.mapping, point_buffer.mapping_size) != 0 ||
        munmap(output_buffer.mapping, output_buffer.mapping_size) != 0)
        return 2;

    printf("guard_page_cases=%zu\n", cases);
    return cases == 18 ? 0 : 1;
}

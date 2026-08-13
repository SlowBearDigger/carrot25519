// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int decode_nibble(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    return -1;
}

static int decode_hex(uint8_t out[32], const char input[65])
{
    for (size_t i = 0; i < 32; ++i)
    {
        const int high = decode_nibble(input[2 * i]);
        const int low = decode_nibble(input[2 * i + 1]);
        if (high < 0 || low < 0)
            return 0;
        out[i] = (uint8_t)((high << 4) | low);
    }
    return input[64] == '\0';
}

static int check(
    const char *backend, const char *id, const char *operation,
    const uint8_t expected[32], const uint8_t actual[32])
{
    if (memcmp(expected, actual, 32) == 0)
        return 1;
    fprintf(
        stderr, "backend=%s vector=%s operation=%s mismatch\n", backend, id,
        operation);
    return 0;
}

static int run_vector(
    const carrot25519_impl *impl, const char *id, const uint8_t scalar[32],
    const uint8_t point[32], const uint8_t expected_base[32],
    const uint8_t expected_point[32])
{
    const char *backend = carrot25519_impl_name(impl);
    uint8_t output[32];
    uint8_t scalar_alias[32];
    uint8_t point_alias[32];

    carrot25519_mul_base(impl, output, scalar);
    if (!check(backend, id, "base", expected_base, output))
        return 0;
    carrot25519_mul(impl, output, scalar, point);
    if (!check(backend, id, "point", expected_point, output))
        return 0;

    memcpy(scalar_alias, scalar, 32);
    carrot25519_mul_base(impl, scalar_alias, scalar_alias);
    if (!check(backend, id, "base-scalar-alias", expected_base, scalar_alias))
        return 0;

    memcpy(scalar_alias, scalar, 32);
    carrot25519_mul(impl, scalar_alias, scalar_alias, point);
    if (!check(
            backend, id, "point-scalar-alias", expected_point,
            scalar_alias))
        return 0;

    memcpy(point_alias, point, 32);
    carrot25519_mul(impl, point_alias, scalar, point_alias);
    return check(
        backend, id, "point-point-alias", expected_point, point_alias);
}

static int run_alignment(
    const carrot25519_impl *impl, const char *id, const uint8_t scalar[32],
    const uint8_t point[32], const uint8_t expected_base[32],
    const uint8_t expected_point[32])
{
    const char *backend = carrot25519_impl_name(impl);
    for (size_t offset = 0; offset < 16; ++offset)
    {
        uint8_t scalar_storage[47];
        uint8_t point_storage[47];
        uint8_t output_storage[47];
        uint8_t *scalar_at = scalar_storage + offset;
        uint8_t *point_at = point_storage + offset;
        uint8_t *output_at = output_storage + offset;

        memcpy(scalar_at, scalar, 32);
        memcpy(point_at, point, 32);
        carrot25519_mul_base(impl, output_at, scalar_at);
        if (!check(backend, id, "offset-base", expected_base, output_at))
            return 0;
        carrot25519_mul(impl, output_at, scalar_at, point_at);
        if (!check(backend, id, "offset-point", expected_point, output_at))
            return 0;
    }
    return 1;
}

static int run_corpus_for_impl(
    const carrot25519_impl *impl, const char *path, size_t *vector_count)
{
    FILE *stream = fopen(path, "r");
    if (stream == NULL)
        return 0;

    char line[512];
    size_t vectors = 0;
    while (fgets(line, sizeof(line), stream) != NULL)
    {
        if (line[0] == '#')
            continue;

        char class_name[32];
        char id[64];
        char fields[4][65];
        char trailing;
        if (sscanf(
                line,
                "%31[^\t]\t%63[^\t]\t%64[^\t]\t%64[^\t]\t%64[^\t]\t"
                "%64[^\n]%c",
                class_name, id, fields[0], fields[1], fields[2], fields[3],
                &trailing) != 7 ||
            trailing != '\n')
        {
            fclose(stream);
            return 0;
        }
        (void)class_name;

        uint8_t values[4][32];
        for (size_t i = 0; i < 4; ++i)
        {
            if (!decode_hex(values[i], fields[i]))
            {
                fclose(stream);
                return 0;
            }
        }

        if (!run_vector(
                impl, id, values[0], values[1], values[2], values[3]))
        {
            fclose(stream);
            return 0;
        }
        if (strcmp(id, "rfc7748-1") == 0 &&
            !run_alignment(
                impl, id, values[0], values[1], values[2], values[3]))
        {
            fclose(stream);
            return 0;
        }
        ++vectors;
    }

    if (ferror(stream) || fclose(stream) != 0 || vectors != 28)
        return 0;
    *vector_count = vectors;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;

    size_t implementations = 0;
    for (uint32_t id = CARROT25519_IMPL_PORTABLE;
         id <= CARROT25519_IMPL_X86_64_BMI2_ADX; ++id)
    {
        const carrot25519_impl *impl =
            carrot25519_select_impl((carrot25519_impl_id)id);
        if (impl == NULL)
            continue;

        size_t vectors = 0;
        if (!run_corpus_for_impl(impl, argv[1], &vectors))
            return 1;
        printf(
            "implementation=%s vectors=%zu operations=%zu\n",
            carrot25519_impl_name(impl), vectors, vectors * 5 + 32);
        ++implementations;
    }

    return implementations == 0 ? 1 : 0;
}

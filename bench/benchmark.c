// SPDX-License-Identifier: MIT

#include "carrot25519.h"

#if defined(CARROT25519_HAVE_MX25519_BENCHMARK)
#include <mx25519.h>
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef CARROT25519_BENCHMARK_COMPILER
#define CARROT25519_BENCHMARK_COMPILER "unknown"
#endif

#if !defined(CARROT25519_BENCHMARK_SANITIZED)
enum { POOL_SIZE = 64, MAX_RECORDS = 24, MAX_TRIALS = 31 };

typedef void benchmark_operation(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32]);

typedef struct measurement {
    const void *implementation;
    const char *implementation_name;
    const char *operation;
    benchmark_operation *run;
    double samples[MAX_TRIALS];
    double median;
    uint8_t sink;
} measurement;

static uint64_t random_state = UINT64_C(0xa324f17c6d982e51);
static volatile uint8_t benchmark_sink;
#endif

static const char *build_arch(void)
{
#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#else
    return "unknown";
#endif
}

#if !defined(CARROT25519_BENCHMARK_SANITIZED)
static const char *build_os(void)
{
#if defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

static uint64_t next_random(void)
{
    uint64_t value = random_state;
    value ^= value << 13;
    value ^= value >> 7;
    value ^= value << 17;
    random_state = value;
    return value;
}

static void fill_random(uint8_t out[32])
{
    for (size_t offset = 0; offset < 32; offset += 8)
    {
        const uint64_t value = next_random();
        memcpy(out + offset, &value, sizeof(value));
    }
}

static uint64_t monotonic_ns(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
        return 0;
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
           (uint64_t)value.tv_nsec;
}

static int compare_double(const void *left, const void *right)
{
    const double a = *(const double *)left;
    const double b = *(const double *)right;
    return (a > b) - (a < b);
}

static double median(const double samples[MAX_TRIALS], size_t trials)
{
    double sorted[MAX_TRIALS];
    memcpy(sorted, samples, trials * sizeof(samples[0]));
    qsort(sorted, trials, sizeof(sorted[0]), compare_double);
    return sorted[trials / 2];
}

static void prepare_inputs(
    uint8_t scalars[POOL_SIZE][32], uint8_t points[POOL_SIZE][32])
{
    const carrot25519_impl *portable =
        carrot25519_select_impl(CARROT25519_IMPL_PORTABLE);
    for (size_t index = 0; index < POOL_SIZE; ++index)
    {
        fill_random(scalars[index]);
        carrot25519_mul_base(portable, points[index], scalars[index]);
    }
}

static void carrot_base(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32])
{
    (void)point;
    carrot25519_mul_base(
        (const carrot25519_impl *)implementation, out, scalar);
}

static void carrot_point(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32])
{
    carrot25519_mul(
        (const carrot25519_impl *)implementation, out, scalar, point);
}

static void carrot_base_ladder(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32])
{
    static const uint8_t basepoint[32] = {9};
    (void)point;
    carrot25519_mul(
        (const carrot25519_impl *)implementation, out, scalar, basepoint);
}

#if defined(CARROT25519_HAVE_MX25519_BENCHMARK)
static void mx25519_base(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32])
{
    mx25519_privkey key;
    mx25519_pubkey result;
    (void)point;
    memcpy(key.data, scalar, sizeof(key.data));
    mx25519_scmul_base(
        (const mx25519_impl *)implementation, &result, &key);
    memcpy(out, result.data, sizeof(result.data));
}

static void mx25519_point(
    const void *implementation, uint8_t out[32], const uint8_t scalar[32],
    const uint8_t point[32])
{
    mx25519_privkey key;
    mx25519_pubkey input;
    mx25519_pubkey result;
    memcpy(key.data, scalar, sizeof(key.data));
    memcpy(input.data, point, sizeof(input.data));
    mx25519_scmul_key(
        (const mx25519_impl *)implementation, &result, &key, &input);
    memcpy(out, result.data, sizeof(result.data));
}

static const char *mx25519_name(mx25519_type type)
{
    switch (type)
    {
    case MX25519_TYPE_PORTABLE:
        return "mx25519/portable";
    case MX25519_TYPE_ARM64:
        return "mx25519/arm64";
    case MX25519_TYPE_AMD64:
        return "mx25519/amd64";
    case MX25519_TYPE_AMD64X:
        return "mx25519/amd64x";
    case MX25519_TYPE_AUTO:
        break;
    }
    return "mx25519/unknown";
}
#endif

static int verify_measurement(
    const measurement *reference, const measurement *candidate,
    uint8_t scalars[POOL_SIZE][32], uint8_t points[POOL_SIZE][32])
{
    uint8_t expected[32];
    uint8_t actual[32];
    for (size_t index = 0; index < POOL_SIZE; ++index)
    {
        const uint8_t *point = points[(index + 11) % POOL_SIZE];
        reference->run(
            reference->implementation, expected, scalars[index], point);
        candidate->run(
            candidate->implementation, actual, scalars[index], point);
        if (memcmp(expected, actual, sizeof(expected)) != 0)
            return 0;
    }
    return 1;
}

static void run_operation(
    const measurement *result, uint64_t iterations, size_t trial,
    uint8_t scalars[POOL_SIZE][32],
    uint8_t points[POOL_SIZE][32], uint8_t *sink)
{
    uint8_t output[32];
    for (uint64_t iteration = 0; iteration < iterations; ++iteration)
    {
        const size_t index =
            (size_t)((iteration + trial * 17U) % POOL_SIZE);
        result->run(
            result->implementation, output, scalars[index],
            points[(index + 11) % POOL_SIZE]);
        *sink ^= output[(iteration + trial) % 32];
    }
}

static int measure_once(
    measurement *result, uint64_t iterations, size_t trial,
    uint8_t scalars[POOL_SIZE][32],
    uint8_t points[POOL_SIZE][32])
{
    const uint64_t start = monotonic_ns();
    run_operation(
        result, iterations, trial, scalars, points, &result->sink);
    const uint64_t end = monotonic_ns();
    if (start == 0 || end <= start)
        return 0;
    result->samples[trial] = (double)(end - start) / (double)iterations;
    return 1;
}

static int measure_interleaved(
    measurement records[MAX_RECORDS], size_t record_count,
    uint64_t iterations, size_t trials,
    uint8_t scalars[POOL_SIZE][32], uint8_t points[POOL_SIZE][32])
{
    for (size_t index = 0; index < record_count; ++index)
        run_operation(
            &records[index], iterations, 0, scalars, points,
            &records[index].sink);

    for (size_t trial = 0; trial < trials; ++trial)
    {
        for (size_t offset = 0; offset < record_count; ++offset)
        {
            const size_t index =
                (trial & 1U) == 0 ? offset : record_count - 1U - offset;
            if (!measure_once(
                    &records[index], iterations, trial, scalars, points))
                return 0;
        }
    }

    for (size_t index = 0; index < record_count; ++index)
    {
        records[index].median = median(records[index].samples, trials);
        benchmark_sink ^= records[index].sink;
    }
    return 1;
}

static int parse_positive(const char *text, uint64_t *value)
{
    char *end = NULL;
    const unsigned long long parsed = strtoull(text, &end, 10);
    if (text[0] == '\0' || end == NULL || *end != '\0' || parsed == 0)
        return 0;
    *value = (uint64_t)parsed;
    return 1;
}
#endif

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--identity") == 0)
    {
        printf("arch=%s\n", build_arch());
#if defined(CARROT25519_BENCHMARK_SANITIZED)
        printf("sanitized=true\n");
#else
        printf("sanitized=false\n");
#endif
        return 0;
    }

#if defined(CARROT25519_BENCHMARK_SANITIZED)
    (void)argc;
    (void)argv;
    fprintf(stderr, "sanitized benchmark builds are rejected\n");
    return 2;
#else
    if (argc != 5)
        return 2;
    uint64_t iterations;
    uint64_t trial_count;
    if (!parse_positive(argv[3], &iterations) ||
        !parse_positive(argv[4], &trial_count) || trial_count > MAX_TRIALS ||
        strlen(argv[1]) != 40 ||
        (strcmp(argv[2], "true") != 0 && strcmp(argv[2], "false") != 0))
        return 2;

    const carrot25519_impl *implementations[4];
    size_t implementation_count = 0;
    for (uint32_t id = CARROT25519_IMPL_PORTABLE;
         id <= CARROT25519_IMPL_X86_64_BMI2_ADX; ++id)
    {
        const carrot25519_impl *impl =
            carrot25519_select_impl((carrot25519_impl_id)id);
        if (impl != NULL)
            implementations[implementation_count++] = impl;
    }
    if (implementation_count == 0)
        return 2;

    uint8_t scalars[POOL_SIZE][32];
    uint8_t points[POOL_SIZE][32];
    prepare_inputs(scalars, points);
    measurement records[MAX_RECORDS];
    size_t record_count = 0;
    for (size_t index = 0; index < implementation_count; ++index)
    {
        records[record_count++] =
            (measurement){
                implementations[index],
                carrot25519_impl_name(implementations[index]),
                "base",
                carrot_base,
                {0},
                0,
                0};
        records[record_count++] =
            (measurement){
                implementations[index],
                carrot25519_impl_name(implementations[index]),
                "base-ladder",
                carrot_base_ladder,
                {0},
                0,
                0};
        records[record_count++] =
            (measurement){
                implementations[index],
                carrot25519_impl_name(implementations[index]),
                "point",
                carrot_point,
                {0},
                0,
                0};
    }
#if defined(CARROT25519_HAVE_MX25519_BENCHMARK)
    for (mx25519_type type = MX25519_TYPE_PORTABLE;
         type <= MX25519_TYPE_AMD64X; ++type)
    {
        const mx25519_impl *implementation = mx25519_select_impl(type);
        if (implementation == NULL)
            continue;
        records[record_count++] = (measurement){
            implementation,
            mx25519_name(type),
            "base",
            mx25519_base,
            {0},
            0,
            0};
        records[record_count++] = (measurement){
            implementation,
            mx25519_name(type),
            "point",
            mx25519_point,
            {0},
            0,
            0};
    }
#endif
    for (size_t index = 0; index < record_count; ++index)
    {
        const size_t portable_index =
            strcmp(records[index].operation, "base") == 0
                ? 0
                : strcmp(records[index].operation, "base-ladder") == 0
                    ? 1
                    : 2;
        if (!verify_measurement(
                &records[portable_index], &records[index], scalars, points))
            return 3;
    }
    if (!measure_interleaved(
            records, record_count, iterations, (size_t)trial_count,
            scalars, points))
        return 2;
    for (size_t index = 0; index < record_count; ++index)
    {
        const size_t portable_index =
            strcmp(records[index].operation, "base") == 0
                ? 0
                : strcmp(records[index].operation, "base-ladder") == 0
                    ? 1
                    : 2;
        if (index > 2 && records[index].sink != records[portable_index].sink)
            return 3;
    }

    printf("schema=carrot25519-benchmark-v1\n");
    printf("commit=%s\n", argv[1]);
    printf("dirty=%s\n", argv[2]);
    printf("os=%s\n", build_os());
    printf("arch=%s\n", build_arch());
    printf("compiler=%s\n", CARROT25519_BENCHMARK_COMPILER);
#if defined(CARROT25519_HAVE_MX25519_BENCHMARK)
    printf("mx25519_commit=%s\n", CARROT25519_MX25519_COMMIT);
    printf("mx25519_tree=%s\n", CARROT25519_MX25519_TREE);
#endif
    printf(
        "auto_implementation=%s\n",
        carrot25519_impl_name(carrot25519_select_impl(CARROT25519_IMPL_AUTO)));
    printf("iterations=%" PRIu64 "\n", iterations);
    printf("trials=%" PRIu64 "\n", trial_count);
    for (size_t index = 0; index < record_count; ++index)
    {
        const double portable_median =
            strcmp(records[index].operation, "base") == 0
                ? records[0].median
                : strcmp(records[index].operation, "base-ladder") == 0
                    ? records[1].median
                    : records[2].median;
        printf(
            "\nimplementation=%s\n",
            records[index].implementation_name);
        printf("operation=%s\n", records[index].operation);
        printf("samples_ns_per_op=");
        for (size_t trial = 0; trial < trial_count; ++trial)
            printf("%s%.2f", trial == 0 ? "" : ",", records[index].samples[trial]);
        printf("\nmedian_ns_per_op=%.2f\n", records[index].median);
        printf("portable_ratio=%.4f\n", portable_median / records[index].median);
        printf("sink=%02x\n", records[index].sink);
    }
    return 0;
#endif
}

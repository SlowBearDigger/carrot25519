// SPDX-License-Identifier: MIT

#include "carrot25519.h"

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
enum { POOL_SIZE = 64, MAX_RECORDS = 8, MAX_TRIALS = 31 };

typedef struct measurement {
    const carrot25519_impl *impl;
    const char *operation;
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

static void run_operation(
    const carrot25519_impl *impl, int use_point, uint64_t iterations,
    size_t trial, const uint8_t scalars[POOL_SIZE][32],
    const uint8_t points[POOL_SIZE][32], uint8_t *sink)
{
    uint8_t output[32];
    for (uint64_t iteration = 0; iteration < iterations; ++iteration)
    {
        const size_t index =
            (size_t)((iteration + trial * 17U) % POOL_SIZE);
        if (use_point)
            carrot25519_mul(
                impl, output, scalars[index], points[(index + 11) % POOL_SIZE]);
        else
            carrot25519_mul_base(impl, output, scalars[index]);
        *sink ^= output[(iteration + trial) % 32];
    }
}

static int measure(
    measurement *result, uint64_t iterations, size_t trials,
    const uint8_t scalars[POOL_SIZE][32],
    const uint8_t points[POOL_SIZE][32])
{
    uint8_t sink = 0;
    const int use_point = strcmp(result->operation, "point") == 0;
    run_operation(
        result->impl, use_point, iterations, 0, scalars, points, &sink);
    for (size_t trial = 0; trial < trials; ++trial)
    {
        const uint64_t start = monotonic_ns();
        run_operation(
            result->impl, use_point, iterations, trial, scalars, points,
            &sink);
        const uint64_t end = monotonic_ns();
        if (start == 0 || end <= start)
            return 0;
        result->samples[trial] = (double)(end - start) / (double)iterations;
    }
    result->median = median(result->samples, trials);
    result->sink = sink;
    benchmark_sink ^= sink;
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
            (measurement){implementations[index], "base", {0}, 0, 0};
        records[record_count++] =
            (measurement){implementations[index], "point", {0}, 0, 0};
    }
    for (size_t index = 0; index < record_count; ++index)
    {
        if (!measure(
                &records[index], iterations, (size_t)trial_count, scalars,
                points))
            return 2;
        const size_t portable_index =
            strcmp(records[index].operation, "base") == 0 ? 0 : 1;
        if (index > 1 && records[index].sink != records[portable_index].sink)
            return 3;
    }

    printf("schema=carrot25519-benchmark-v1\n");
    printf("commit=%s\n", argv[1]);
    printf("dirty=%s\n", argv[2]);
    printf("os=%s\n", build_os());
    printf("arch=%s\n", build_arch());
    printf("compiler=%s\n", CARROT25519_BENCHMARK_COMPILER);
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
                : records[1].median;
        printf(
            "\nimplementation=%s\n",
            carrot25519_impl_name(records[index].impl));
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

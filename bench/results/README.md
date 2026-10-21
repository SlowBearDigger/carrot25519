# Benchmark results

These files are informational native measurements, not correctness or release
gates. Each result records the exact commit, dirty state, compiler, selected
backend, raw trial samples, median, and ratio to the portable implementation.

Run a local release benchmark with:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release \
  -DCARROT25519_BUILD_BENCHMARKS=ON
cmake --build build-bench --parallel
./bench/run-local.sh --iterations 5000 --trials 7
```

Translated, emulated, cross-compiled, and sanitized executions are rejected.

`linux-x86_64.txt` records the pre-change baseline. The post-change run is in
`linux-x86_64-fixed-base.txt`. Shared GitHub runners vary between runs, so the
authoritative comparison is the alternating `base` and `base-ladder` pair in
the post-change process: 36.78 us versus 39.32 us for `s2n-baseline`, a 6.4%
latency reduction. The BMI2+ADX path remained unchanged.

## Current mx25519 comparison

`linux-x86_64-mx25519-20261021.txt` records a local native run on an AMD Ryzen
5 5500U, GCC 16.2.1, Release (`-O3 -DNDEBUG`), pinned to logical CPU 2. Each
operation has 5,000 iterations and seven alternating trials. The reference is
tevador/mx25519 at `b6adab39cc16cc5c65b27c98b27e429e8394ed44`, tree
`0520f6b642347741dd0cb7a6f52dabb68ab960f7` (current on 2026-10-06), which includes
the Intel/AMD optimizations linked in issue #1. The comparator uses its raw
unclamped API and requires matching output bytes before timing.

| Implementation | Base median (us) | Point median (us) |
|---|---:|---:|
| carrot25519 s2n baseline | 55.57 | 62.78 |
| mx25519 amd64 | 62.86 | 62.87 |
| carrot25519 s2n BMI2+ADX | 50.68 | 50.68 |
| mx25519 amd64x | 48.23 | 48.21 |

These are measurements of one AMD CPU. They do not establish performance on
Intel or other AMD generations. The samples record clean commit `3b8aca7`;
the following commit adds only these results and their documentation.

To reproduce with a clean external mx25519 checkout:

```sh
cmake -S . -B build-compare -DCMAKE_BUILD_TYPE=Release \
  -DCARROT25519_BUILD_TESTS=ON -DCARROT25519_BUILD_BENCHMARKS=ON \
  -DCARROT25519_BUILD_MX25519_BENCHMARK=ON \
  -DCARROT25519_MX25519_SOURCE=/path/to/mx25519 \
  -DCARROT25519_MX25519_COMMIT=b6adab39cc16cc5c65b27c98b27e429e8394ed44 \
  -DCARROT25519_MX25519_TREE=0520f6b642347741dd0cb7a6f52dabb68ab960f7
cmake --build build-compare --parallel 4
ctest --test-dir build-compare --output-on-failure
CARROT25519_BENCHMARK_BINARY=build-compare/carrot25519_benchmark \
  taskset -c 2 ./bench/run-local.sh --iterations 5000 --trials 7
```

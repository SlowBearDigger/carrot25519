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

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

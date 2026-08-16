# carrot25519

carrot25519 is an experimental, allocation-free C11 primitive for the raw
Montgomery scalar multiplication used by CARROT.
It is not audited and is not production-ready.

The current pre-1.0 byte API version is `0.1.0`. See [AUDIT.md](AUDIT.md) for
the security boundary, open qualification gates, and review questions.

## Byte contract

- Scalars and points are 32-byte little-endian values.
- Scalar bits 0 through 254 are processed unchanged. Scalar bit 255 is ignored.
- Point bit 255 is ignored and the remaining value is reduced modulo
  `2^255 - 19`.
- The result is a canonical 32-byte ladder encoding.
- A non-invertible final projective state is encoded as all zero.
- Output may exactly alias either input. Partial overlaps and null pointers are
  invalid. Arbitrary byte alignment is supported.
- The primitive does not validate points or reject an all-zero result.

This is not RFC 7748 X25519. In particular, it does not clear scalar bits 0, 1,
or 2, and it does not force scalar bit 254.

## Backends

| ID | Name | Availability |
|---|---|---|
| `PORTABLE` | `portable/fiat51` | Portable C fallback |
| `ARM64` | `arm64/cc0` | macOS ARM64 and Linux AArch64 |
| `X86_64_BASELINE` | `x86_64/s2n-baseline` | Baseline x86_64 |
| `X86_64_BMI2_ADX` | `x86_64/s2n-bmi2-adx` | x86_64 with both BMI2 and ADX |

`AUTO` selects ARM64 on AArch64. On x86_64 it selects BMI2+ADX only when both
features are reported by CPUID, otherwise it selects the baseline backend.
Unsupported explicit selections return `NULL`. Set
`CARROT25519_PORTABLE_ONLY=ON` to disable optimized backends.

Portable and ARM64 basepoint multiplication uses a constant-schedule ref10
fixed-base table. Arbitrary-point multiplication remains unchanged: portable
uses generated Fiat-Crypto field arithmetic, while ARM64 uses pinned AArch64
assembly adapted to CARROT scalar semantics. The x86_64 backends compose pinned
s2n-bignum projective, reduction, inversion, and field multiplication objects.

## CARROT mapping

CARROT's basepoint operation maps to `carrot25519_mul_base`. Multiplication by
an encoded Montgomery point maps to `carrot25519_mul`.

```c
#include <carrot25519.h>

#include <stdint.h>

int main(void)
{
    uint8_t scalar[32] = {1};
    uint8_t point[32] = {9};
    uint8_t output[32];
    const carrot25519_impl *impl =
        carrot25519_select_impl(CARROT25519_IMPL_AUTO);
    if (impl == NULL)
        return 1;
    carrot25519_mul_base(impl, output, scalar);
    carrot25519_mul(impl, output, scalar, point);
    return 0;
}
```

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCARROT25519_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The normal build is offline and uses only vendored, digest-bound sources. CMake
installation, `find_package`, `add_subdirectory`, C, and C++17 consumers are
covered by tests.

## Benchmark

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release \
  -DCARROT25519_BUILD_BENCHMARKS=ON
cmake --build build-bench --parallel
./bench/run-local.sh --iterations 5000 --trials 7
```

Each implementation reports `base`, `base-ladder`, and `point`. The first two
use the same scalars and are measured in alternating order, providing a direct
fixed-base versus Montgomery-ladder comparison on the same host.

The recorded macOS ARM64 result at commit `9b7a930` measured fixed-base and
ladder medians of 24.15 us and 25.69 us on ARM64, a 6.0% latency reduction.
The portable medians were 24.14 us and 39.46 us, a 38.8% reduction.
Arbitrary-point medians were 25.68 us and 39.49 us respectively. See
[the raw samples](bench/results/macos-arm64.txt). Benchmarks are informational
and never decide correctness.

### Help test x86_64

The [mx25519 comparison run](https://github.com/SlowBearDigger/carrot25519/actions/runs/31843055542)
checks byte-identical outputs before timing both libraries in the same process.
To reproduce it, fork this repository, enable Actions, open the
[Native benchmark workflow](https://github.com/SlowBearDigger/carrot25519/actions/workflows/benchmark.yml),
and select **Run workflow**. Results from different Intel and AMD generations
are welcome. Please share the run URL and raw artifact with the CPU model.

## Verification

- A frozen 28-row corpus covers 172 operations per backend, including low
  scalar bits, bit 254, bit 255 invariance, non-canonical points, small-order
  points, exact aliases, and offsets 0 through 15.
- The CARROT convergence suite covers every scalar `2^i + j` for
  `i = 0..254` and `j = 0..7`, three points, ECDH convergence, and current
  small-order cases.
- A separate fixed-base regression compares the table path against the
  independent Montgomery ladder for 6,144 scalar cases and exact aliases.
- macOS ARM64 passes release, ASan, UBSan, guard-page, disassembly, CFI, install,
  and consumer tests.
- A Local Linux AArch64 VM using Colima/VZ on Apple ARM64 and GCC 13.3 passes
  release, ASan, UBSan, guard-page, ELF disassembly, install, and consumer
  tests. Bare-metal Linux AArch64 qualification remains pending.
- GitHub-hosted Linux x86_64 passes GCC and Clang release tests plus Clang ASan
  and UBSan. Both x86_64 tiers pass corpus, convergence, dispatch, guard-page,
  and disassembly gates.
- GitHub-hosted Linux ARM64 passes release, ASan, and UBSan tests on the native
  ARM64 backend.
- [Native benchmark run 31805437243](https://github.com/SlowBearDigger/carrot25519/actions/runs/31805437243)
  covers GitHub-hosted Linux x86_64, Linux ARM64, and macOS ARM64 at commit
  `5885f83` with a clean worktree.
- Windows is unsupported. The pinned portable Fiat-Crypto backend requires
  128-bit integer support that MSVC does not provide.

## Advantages and limits

The project is MIT licensed. The API is small, explicit, allocation-free, and
offline-buildable. Backends are visible to callers, so tests and applications
do not need to guess which implementation is active.

This repository is a reviewable experiment, not a production cryptography
library. It has no independent security audit. The ARM64 code clears its local
scratch frame, but not registers or system-level copies. The x86_64 wrapper
clears its own buffers, but the unmodified s2n-bignum assembly frames are not
cleared. The fixed-base path clears top-level scratch state, but compiler
temporaries and spills may remain. Its table and code add about 40 KiB of text
to the static library. Hardware timing behavior is not guaranteed across every
processor.

## License

MIT licensed. See [LICENSE](LICENSE).

## Donate

Monero donations:

```text
85qhvVeJwqd7LUhivp4YchfTQRCqs51GHaF13kkSgNLnBNtrnNVvADGVTvSUYKMDbfSitivYkZC39DwLByKBAWq9Gb38ggo
```

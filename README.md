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

The portable backend uses generated Fiat-Crypto field arithmetic. The ARM64
backend adapts a CC0-1.0 implementation to CARROT scalar semantics and clears
its 192-byte local scratch frame. The x86_64 backends compose pinned MIT-0
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

The recorded macOS ARM64 result at commit `8db8fd2` measured medians of
25.86 us for ARM64 basepoint multiplication and 26.22 us for arbitrary-point
multiplication. The corresponding portable medians were 41.19 us and 44.72 us.
See [the raw samples](bench/results/macos-arm64.txt). Benchmarks are
informational and never decide correctness.

## Verification

- A frozen 28-row corpus covers 172 operations per backend, including low
  scalar bits, bit 254, bit 255 invariance, non-canonical points, small-order
  points, exact aliases, and offsets 0 through 15.
- The CARROT convergence suite covers every scalar `2^i + j` for
  `i = 0..254` and `j = 0..7`, three points, ECDH convergence, and current
  small-order cases.
- macOS ARM64 passes release, ASan, UBSan, guard-page, disassembly, CFI, install,
  and consumer tests.
- Both x86_64 tiers pass the complete corpus, convergence, dispatch, guard-page,
  disassembly, and UBSan gates under Linux x86_64 emulation. Native Linux
  qualification remains pending.

## Advantages and limits

The API is small, explicit, allocation-free, offline-buildable, and permissively
licensed. Backends are visible to callers, so tests and applications do not need
to guess which implementation is active.

This repository is a reviewable experiment, not a production cryptography
library. It has no independent security audit. The ARM64 code clears its local
scratch frame, but not registers or system-level copies. The x86_64 wrapper
clears its own buffers, but the unmodified s2n-bignum assembly frames are not
cleared. Hardware timing behavior is not guaranteed across every processor.

## License and provenance

Original code is MIT licensed. Vendored material retains its selected MIT,
CC0-1.0, or MIT-0 terms. Exact commits, trees, file paths, licenses, and hashes
are listed in [THIRD_PARTY.md](THIRD_PARTY.md). No LGPL code is included or
linked.

## Donate

Monero donations:

```text
85qhvVeJwqd7LUhivp4YchfTQRCqs51GHaF13kkSgNLnBNtrnNVvADGVTvSUYKMDbfSitivYkZC39DwLByKBAWq9Gb38ggo
```

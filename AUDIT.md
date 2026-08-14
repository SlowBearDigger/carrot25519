# Review guide

## Status

carrot25519 is an experimental cryptographic primitive. Version 0.1.0 is not
independently audited and must not be used to protect production funds.

An assessment applies only to the exact commit and tree recorded by the
reviewer. This API is pre-1.0 and may change after review findings.

## Purpose

The library implements the raw Montgomery scalar multiplication needed by
CARROT. It preserves scalar bits 0 through 254 and ignores bit 255. It is not
RFC 7748 X25519 because it does not clear scalar bits 0, 1, or 2 and does not
force bit 254.

The primitive does not validate points and does not reject an all-zero result.
CARROT integrations must perform every protocol-level subgroup, point, and
result validation required by their specification. A successful function call
is not evidence that either input is acceptable for CARROT.

## Public surface

The complete public API is declared in `include/carrot25519.h`:

- `carrot25519_select_impl`
- `carrot25519_impl_id_of`
- `carrot25519_impl_name`
- `carrot25519_mul_base`
- `carrot25519_mul`

Inputs and output are fixed 32-byte buffers. Exact `out == scalar` and
`out == point` aliases are supported. Partial overlaps and null pointers are
invalid. The implementation performs no allocation or I/O.

## Security invariants

A review should establish that every available backend:

1. processes scalar bits 0 through 254 unchanged and ignores only bit 255;
2. masks encoded point bit 255 and reduces the remaining integer modulo
   `2^255 - 19`;
3. returns the same canonical 32-byte ladder encoding as the frozen corpus;
4. maps a non-invertible final projective state to all zero without reporting
   success or failure at the protocol layer;
5. supports arbitrary byte alignment and both documented exact aliases;
6. does not read or write outside any 32-byte caller buffer;
7. selects the BMI2+ADX backend only when both CPU features are present; and
8. does not expose vendored internal symbols from the library boundary.

The implementation is intended to avoid secret-dependent control flow and
memory access. Existing source inspection, differential tests, and disassembly
checks are not a proof of constant-time behavior on any processor.

## Source scope

Original implementation and dispatch code:

- `include/carrot25519.h`
- `src/carrot25519.c`
- `src/internal.h`
- `src/secure_zero.c`
- `src/portable/portable.c`
- `src/arm64/arm64.c`
- `src/arm64/x25519_aarch64.S`
- `src/x86_64/x86_64.c`
- `src/x86_64/x86_64.h`

Build, provenance, and behavioral evidence:

- `CMakeLists.txt`
- `THIRD_PARTY.md`
- `tests/`
- `bench/`
- `.github/workflows/`

Vendored Fiat-Crypto and s2n-bignum files are digest-bound in
`tests/provenance_test.cmake`. Their upstream proofs and tests are useful
evidence, but they do not prove the complete carrot25519 wrappers, byte
decoding, backend dispatch, or CARROT integration.

## Backend review questions

### Portable Fiat 5x51

- Is the 255-step ladder correct for all scalar bits, including an odd scalar?
- Is the final conditional swap present and correctly polarized?
- Does point decoding perform mask then reduction in the required order?
- Does inversion of zero produce the documented all-zero encoding?
- Are field bounds valid for every generated Fiat-Crypto call?
- Are compiler-emitted branches and memory accesses independent of secrets?

### ARM64

- Does the adaptation remove RFC clamping without changing any other scalar
  bit?
- Is the added final conditional swap correct for scalar bit zero?
- Are the Mach-O and ELF ABIs, CFI directives, symbol visibility, and stack
  restoration correct?
- Do unaligned output stores and exact aliases remain safe at page edges?
- Does the epilogue clear exactly the 192-byte local scratch frame before it is
  released?

### x86_64 s2n-bignum composition

- Are scalar and point masking, point reduction, projective multiplication,
  inversion, affine conversion, and serialization composed correctly?
- Is `Z = 0` mapped to the all-zero result without undefined behavior?
- Are baseline and BMI2+ADX projective and multiplication functions paired
  consistently?
- Can CPUID classification ever execute BMI2 or ADX instructions without both
  features?
- Are the individual s2n assumptions preserved for arbitrary Montgomery
  inputs, including twist and small-order encodings?

## Threat model

The scalar is secret. The encoded point may be attacker-controlled. Relevant
failures include wrong output bytes, out-of-bounds access, use of an unsupported
instruction set, secret-dependent execution, incomplete cleanup, ABI
corruption, or protocol code treating a raw result as validated.

Callers that pass null pointers, partial overlaps, invalid implementation
handles, or buffers shorter than 32 bytes are outside the API contract.

The following are outside this primitive and require separate review:

- CARROT key hierarchy, address construction, enote scanning, Janus checks,
  subgroup validation, and all-zero rejection;
- randomness, scalar generation, key storage, process isolation, and caller
  buffer cleanup;
- complete compiler, CPU, operating-system, cache, speculative-execution, and
  physical side-channel guarantees; and
- correctness or security of a downstream protocol merely because its byte
  multiplication matches this library.

## Current evidence

- The frozen corpus has 28 rows and 172 operations per backend.
- The convergence suite covers `2^i + j` for `i = 0..254`, `j = 0..7`, three
  points, ECDH convergence, non-canonical encodings, and small-order cases.
- Guard-page tests exercise read-only page-edge inputs, page-edge outputs, and
  exact aliases.
- Disassembly gates inspect the ARM64 scratch wipe and x86_64 instruction and
  symbol boundaries.
- Provenance checks pin every vendored cryptographic source and the corpus.
- macOS ARM64 has local release, ASan, UBSan, CFI, install, consumer, and
  benchmark evidence.
- A Local Linux AArch64 VM using Colima/VZ on Apple ARM64 and GCC 13.3 has
  release, ASan, UBSan, guard-page, ELF disassembly, install, and consumer
  evidence.
- GitHub-hosted Linux x86_64 has GCC and Clang release plus Clang ASan and
  UBSan evidence. GitHub-hosted Linux ARM64 has release, ASan, and UBSan
  evidence on the native ARM64 backend.
- Native benchmark run `31805437243` covers GitHub-hosted Linux x86_64, Linux
  ARM64, and macOS ARM64 at clean commit `5885f83`.

Bare-metal Linux AArch64 execution remains pending. Windows is unsupported. The
pinned portable Fiat-Crypto backend requires 128-bit integer support that MSVC
does not provide. Supporting Windows requires a separately qualified backend or
toolchain.

## Known limitations

- No independent review has been completed.
- The ARM64 backend clears its local scratch frame, not registers or external
  system copies.
- The x86_64 wrapper clears its own temporary buffers. Unmodified s2n-bignum
  assembly frames are not cleared.
- Sanitizers instrument C and C++ boundaries, not the assembly instructions.
- Native timing evidence does not establish behavior on other processors.
- The raw API cannot prevent a downstream integration from omitting mandatory
  CARROT validation.

## Reproduction

From a clean checkout on macOS or Linux:

```sh
./tools/review.sh
```

The runner performs fresh Release, ASan, and UBSan builds in a temporary
directory, executes the applicable CTest suite, and checks workflow policy. It
uses no network access and leaves no build output in the checkout.

Benchmarks are intentionally separate from correctness:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release \
  -DCARROT25519_BUILD_BENCHMARKS=ON
cmake --build build-bench --parallel
./bench/run-local.sh --iterations 5000 --trials 7
```

CARROT specification and `carrot_core` audit reports provide protocol context,
but their conclusions do not transfer automatically to this implementation.

# Third-party material

The top-level MIT license applies to original carrot25519 code. Vendored files
retain the terms selected below.

## Fiat-Crypto

- Repository: https://github.com/mit-plv/fiat-crypto
- Commit: `046758072159ee093e837ab6840cde89b9795997`
- Tree: `edd9dd3676c8502852ef2bd4524fda40cb3acec0`
- Selected license: MIT
- License copy: `licenses/MIT-FIAT.txt`
- Vendored path: `src/portable/fiat/curve25519_64.c`

The generated field arithmetic is vendored without local edits.

## X25519-AArch64

- Repository: https://github.com/Emill/X25519-AArch64
- Commit: `59b13b2f7dc615eccc1f14d14ef05662d9ab0fdd`
- Tree: `329bf77d9ff270dd5aed659e0a300d0097ab3719`
- License: CC0-1.0
- Legal code: `licenses/CC0-1.0.txt`
- Vendored path: `src/arm64/x25519_aarch64.S`
- Adapted source SHA-256: `2d267ce802f44c839adce339d823bbbe491462b312cdc36e3fa1b3ebc00870e6`

The assembly is adapted for the raw CARROT scalar contract, Mach-O and ELF
integration, unaligned output stores, and local scratch-frame clearing. The
original CC0 notice remains in the source.

## s2n-bignum

- Repository: https://github.com/awslabs/s2n-bignum
- Commit: `51147aaa18a990588f391a491a43048659888631`
- Tree: `b1f808d27be59107ca655fd8c0a0ef425604e3e0`
- Selected license: MIT-0
- License copy: `licenses/MIT-0-S2N.txt`
- Notice copy: `licenses/NOTICE-S2N.txt`
- Vendored paths:
  - `src/x86_64/s2n/_internal_s2n_bignum.h`
  - `src/x86_64/s2n/_internal_s2n_bignum_x86.h`
  - `src/x86_64/s2n/s2n-bignum.h`
  - `src/x86_64/s2n/curve25519_pxscalarmul.S`
  - `src/x86_64/s2n/curve25519_pxscalarmul_alt.S`
  - `src/x86_64/s2n/bignum_inv_p25519.S`
  - `src/x86_64/s2n/bignum_mod_p25519_4.S`
  - `src/x86_64/s2n/bignum_mul_p25519.S`
  - `src/x86_64/s2n/bignum_mul_p25519_alt.S`

The selected files retain their Amazon copyright and license identifiers. The
normal build uses only vendored sources and does not download dependencies.
The wrapper composes the projective multiplication, reduction, inversion, and
field multiplication primitives. It clears its own temporary buffers, but the
unmodified assembly frames are not cleared.

## ref10 fixed-base subset

- Origin: Daniel J. Bernstein's SUPERCOP Ed25519 ref10 implementation
- Source snapshot: https://github.com/seraphis-migration/monero
- Commit: `3f159b13e000ca9f0906599c9c5fd9c13f55233c`
- Tree: `bc91b1b48c9332a155dd9ab39c864781b42846c8`
- Status: public domain
- Vendored path: `src/fixed_base/ref10/`

Only the fixed-base table and its required field and group operations are
included. Local adaptations namespace the symbols, clear top-level scratch
state, and replace signed left shifts with equivalent defined arithmetic. The
original source notice is available at
https://ed25519.cr.yp.to/software.html.

#!/usr/bin/env bash
set -euo pipefail

test $# -eq 1

readonly symbols='curve25519_pxscalarmul curve25519_pxscalarmul_alt bignum_inv_p25519 bignum_mod_p25519_4 bignum_mul_p25519 bignum_mul_p25519_alt'

for symbol in $symbols; do
  nm "$1" | grep -Eq "[[:space:]]_?${symbol}$"
done

if nm "$1" | grep -Eq '[[:space:]]_?curve25519_x25519$'; then
  printf 'x86_64_disassembly=fail\n' >&2
  exit 1
fi

printf 'x86_64_disassembly=pass\n'

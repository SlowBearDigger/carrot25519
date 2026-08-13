#!/usr/bin/env bash
set -euo pipefail

test $# -eq 1

readonly symbols='curve25519_pxscalarmul curve25519_pxscalarmul_alt bignum_inv_p25519 bignum_mod_p25519_4 bignum_mul_p25519 bignum_mul_p25519_alt'
readonly OBJDUMP=${OBJDUMP:-objdump}
readonly NM=${NM:-nm}
readonly READELF=${READELF:-readelf}
readonly objdump_help=$("$OBJDUMP" --help)

case "$(uname -s)" in
  Darwin) readonly symbol_prefix=_ ;;
  *) readonly symbol_prefix= ;;
esac

disassemble_symbol() {
  case "$objdump_help" in
    *--disassemble-symbols*)
      "$OBJDUMP" --disassemble-symbols="${symbol_prefix}$1" "$2" 2>/dev/null
      ;;
    *)
      "$OBJDUMP" -d --disassemble="$1" "$2" 2>/dev/null
      ;;
  esac
}

require_fast_instructions() {
  local body
  body=$(disassemble_symbol "$1" "$2")
  case "$body" in *mulx*) ;; *) return 1 ;; esac
  case "$body" in *adcx*) ;; *) return 1 ;; esac
  case "$body" in *adox*) ;; *) return 1 ;; esac
}

require_baseline_instructions() {
  local body
  body=$(disassemble_symbol "$1" "$2")
  case "$body" in *mulx*|*adcx*|*adox*) return 1 ;; esac
}

for symbol in $symbols; do
  case "$(uname -s)" in
    Darwin)
      test "$("$NM" -m "$1" | grep -Ec "private external _${symbol}$")" -eq 1
      ;;
    *)
      test "$("$READELF" -Ws "$1" | awk -v name="$symbol" \
        '$8 == name && $7 != "UND" && $6 == "HIDDEN" { count++ } END { print count + 0 }')" -eq 1
      ;;
  esac
done

readonly nm_output=$("$NM" "$1")
case "$nm_output" in
  *curve25519_x25519*)
    printf 'x86_64_disassembly=fail\n' >&2
    exit 1
    ;;
esac

require_fast_instructions curve25519_pxscalarmul "$1"
require_fast_instructions bignum_mul_p25519 "$1"
require_baseline_instructions curve25519_pxscalarmul_alt "$1"
require_baseline_instructions bignum_mul_p25519_alt "$1"

printf 'x86_64_disassembly=pass\n'

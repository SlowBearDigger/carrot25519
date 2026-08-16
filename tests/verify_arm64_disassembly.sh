#!/usr/bin/env bash
set -euo pipefail

test $# -eq 1

readonly expected_prefix='a9b67bfd 910003fd a90153f3 a9025bf5 a90363f7 a9046bf9 a90573fb 6d0627e8 6d072fea 6d0837ec 6d093fee d10303ff f9005be0'
readonly expected_suffix='f9405bf1 f900022a f900062b f9000a2c f9000e2d a9007fff a9017fff a9027fff a9037fff a9047fff a9057fff a9067fff a9077fff a9087fff a9097fff a90a7fff a90b7fff 910303ff a94153f3 a9425bf5 a94363f7 a9446bf9 a94573fb 6d4627e8 6d472fea 6d4837ec 6d493fee a8ca7bfd d65f03c0'

function_opcodes=$(objdump -d "$1" | awk '
  /<_?carrot25519_arm64_scalarmult>:/ { active = 1; next }
  active && /<invtable>:/ { exit }
  active && $2 ~ /^[[:xdigit:]]{8}$/ {
    printf "%s%s", separator, tolower($2)
    separator = " "
  }
')

case "$function_opcodes" in
  "$expected_prefix "*"$expected_suffix") ;;
  *) printf 'arm64_disassembly=fail\n' >&2; exit 1 ;;
esac

case "$(uname -s)" in
  Darwin)
    nm -m "$1" | grep -q 'private external _carrot25519_arm64_scalarmult'
    unwind=$(xcrun dwarfdump --eh-frame "$1")
    printf '%s\n' "$unwind" | grep -Fq 'CFA=W29+160'
    main_last_row=$(printf '%s\n' "$unwind" | awk '
      /^0+[[:space:]].*FDE / {
        if (main && last != "") print last
        main = 0
        last = ""
      }
      /CFA=W29\+160/ { main = 1 }
      /^  0x[[:xdigit:]]+:/ { last = $0 }
      END { if (main && last != "") print last }
    ')
    case "$main_last_row" in
      *': CFA=WSP') ;;
      *) printf 'arm64_unwind=fail\n' >&2; exit 1 ;;
    esac
    ;;
esac

printf 'arm64_disassembly=pass\n'

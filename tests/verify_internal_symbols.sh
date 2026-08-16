#!/usr/bin/env bash
set -euo pipefail

test "$#" -eq 1
archive=$1

case "$(uname -s)" in
  Darwin)
    symbols=$(nm -m "$archive")
    ! grep -q 'crypto_sign_ed25519_ref10' <<<"$symbols"
    defined=$(grep '(__TEXT,__text).*external _carrot25519_ref10_' <<<"$symbols")
    count=$(grep -c 'private external' <<<"$defined")
    test "$count" -ge 20
    if grep -qv 'private external' <<<"$defined"; then
      exit 1
    fi
    ;;
  Linux)
    symbols=$(readelf -Ws "$archive")
    ! grep -q 'crypto_sign_ed25519_ref10' <<<"$symbols"
    count=$(awk '$8 ~ /^carrot25519_ref10_/ && $7 != "UND" && $6 == "HIDDEN" { count++ }
                 END { print count + 0 }' <<<"$symbols")
    test "$count" -ge 20
    non_hidden=$(awk '$8 ~ /^carrot25519_ref10_/ && $7 != "UND" && $6 != "HIDDEN" {
                        count++
                      }
                      END { print count + 0 }' <<<"$symbols")
    test "$non_hidden" -eq 0
    ;;
  *) exit 1 ;;
esac

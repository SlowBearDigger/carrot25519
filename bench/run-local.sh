#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
binary=${CARROT25519_BENCHMARK_BINARY:-"$root/build-bench/carrot25519_benchmark"}
iterations=5000
trials=7
output=

while (($# != 0)); do
  case "$1" in
    --iterations) iterations=${2:?}; shift 2 ;;
    --trials) trials=${2:?}; shift 2 ;;
    --output) output=${2:?}; shift 2 ;;
    *) printf 'unknown argument: %s\n' "$1" >&2; exit 2 ;;
  esac
done

case "$iterations:$trials" in
  0:*|*:0|*[!0-9:]*) printf 'iterations and trials must be positive integers\n' >&2; exit 2 ;;
esac

test -x "$binary"
identity=$("$binary" --identity)
case "$identity" in *'sanitized=true'*) printf 'sanitized benchmark rejected\n' >&2; exit 2 ;; esac
binary_arch=$(printf '%s\n' "$identity" | awk -F= '$1 == "arch" { print $2 }')
host_arch=$(uname -m)
case "$host_arch" in arm64|aarch64) host_arch=arm64 ;; x86_64|amd64) host_arch=x86_64 ;; esac
test "$binary_arch" = "$host_arch"

if test "$(uname -s)" = Darwin &&
   test "$(sysctl -n sysctl.proc_translated 2>/dev/null || printf 0)" = 1; then
  printf 'translated execution rejected\n' >&2
  exit 2
fi
if test -n "${QEMU_CPU:-}" || test -n "${QEMU_LD_PREFIX:-}"; then
  printf 'emulated execution rejected\n' >&2
  exit 2
fi

commit=$(git -C "$root" rev-parse HEAD)
dirty=false
if test -n "$(git -C "$root" status --porcelain)"; then dirty=true; fi

temporary=$(mktemp "${TMPDIR:-/tmp}/carrot25519-benchmark.XXXXXX")
trap 'rm -f "$temporary"' EXIT
"$binary" "$commit" "$dirty" "$iterations" "$trials" >"$temporary"

grep -qx 'schema=carrot25519-benchmark-v1' "$temporary"
grep -Eq '^commit=[0-9a-f]{40}$' "$temporary"
grep -Eq '^implementation=' "$temporary"
grep -Eq '^samples_ns_per_op=' "$temporary"
grep -Eq '^median_ns_per_op=' "$temporary"
grep -Eq '^portable_ratio=' "$temporary"

if test -n "$output"; then
  install -m 0644 "$temporary" "$output"
else
  cat "$temporary"
fi

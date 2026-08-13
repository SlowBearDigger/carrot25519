#!/usr/bin/env bash

set -euo pipefail

allow_dirty=0
if [[ "${1:-}" == "--allow-dirty" ]]; then
  allow_dirty=1
  shift
fi
if [[ $# -ne 0 ]]; then
  echo "usage: $0 [--allow-dirty]" >&2
  exit 2
fi

project_root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
dirty_state=false
if [[ -n "$(git -C "${project_root}" status --porcelain --untracked-files=all)" ]]; then
  dirty_state=true
fi
if [[ ${allow_dirty} -eq 0 && "${dirty_state}" == true ]]; then
  echo "review runner requires a clean checkout" >&2
  exit 1
fi

run_root="$(mktemp -d "${TMPDIR:-/tmp}/carrot25519-review.XXXXXX")"
trap 'rm -rf -- "${run_root}"' EXIT

run_variant()
{
  local name="$1"
  local flags="$2"
  local build_dir="${run_root}/${name}"

  cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCARROT25519_BUILD_TESTS=ON \
    "-DCMAKE_C_FLAGS=${flags}" \
    "-DCMAKE_CXX_FLAGS=${flags}" \
    "-DCMAKE_EXE_LINKER_FLAGS=${flags}"
  cmake --build "${build_dir}" --parallel 4
  if [[ "${name}" == "release" ]]; then
    ctest --test-dir "${build_dir}" --output-on-failure
  else
    ctest --test-dir "${build_dir}" -E '^install_consumer$' --output-on-failure
  fi
}

echo "commit=$(git -C "${project_root}" rev-parse HEAD)"
echo "tree=$(git -C "${project_root}" rev-parse 'HEAD^{tree}')"
echo "dirty=${dirty_state}"
echo "system=$(uname -s)"
echo "machine=$(uname -m)"
echo "compiler=$(${CC:-cc} --version | sed -n '1p')"

run_variant release ""

export ASAN_OPTIONS="detect_leaks=0:halt_on_error=1"
run_variant asan "-O1 -g -fno-omit-frame-pointer -fsanitize=address"

export UBSAN_OPTIONS="halt_on_error=1"
run_variant ubsan "-O1 -g -fno-omit-frame-pointer -fsanitize=undefined"

python3 "${project_root}/tests/workflow_policy_test.py"
echo "review_status=pass"

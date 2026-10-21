#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MX_COMMIT = "b6adab39cc16cc5c65b27c98b27e429e8394ed44"
MX_TREE = "0520f6b642347741dd0cb7a6f52dabb68ab960f7"
MX_REPOSITORY = "https://github.com/tevador/mx25519.git"


def require(text: str, value: str, location: str) -> None:
    if value not in text:
        raise SystemExit(f"{location} is missing {value}")


def main() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    benchmark = (ROOT / "bench" / "benchmark.c").read_text(encoding="utf-8")
    workflow = (ROOT / ".github" / "workflows" / "benchmark.yml").read_text(
        encoding="utf-8"
    )

    for value in (
        "CARROT25519_BUILD_MX25519_BENCHMARK",
        "CARROT25519_MX25519_SOURCE",
        "CARROT25519_MX25519_COMMIT",
        "CARROT25519_MX25519_TREE",
        "mx25519_static",
    ):
        require(cmake, value, "CMakeLists.txt")

    for value in (
        "mx25519_scmul_base_unclamped",
        "mx25519_scmul_key_unclamped",
        "MX25519_TYPE_PORTABLE",
        "MX25519_TYPE_AMD64",
        "MX25519_TYPE_AMD64X",
        '"base-ladder"',
        "measure_interleaved",
        "mx25519_commit=",
        "mx25519_tree=",
    ):
        require(benchmark, value, "bench/benchmark.c")

    for value in (
        MX_REPOSITORY,
        MX_COMMIT,
        MX_TREE,
        "CARROT25519_BUILD_MX25519_BENCHMARK=ON",
        "benchmark-linux-x86_64-mx25519.txt",
    ):
        require(workflow, value, "benchmark workflow")

    if "push:" in workflow.split("permissions:", 1)[0]:
        raise SystemExit("reference benchmark must remain manual")
    print("reference_benchmark_policy=pass")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
SHA = re.compile(r"^[0-9a-f]{40}$")


def fail(message: str) -> None:
    raise SystemExit(message)


def job_blocks(text: str) -> list[tuple[str, str]]:
    try:
        jobs = text.split("\njobs:\n", 1)[1]
    except IndexError:
        fail("workflow has no jobs")
    matches = list(re.finditer(r"^  ([a-zA-Z0-9_-]+):\n", jobs, re.MULTILINE))
    blocks = []
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(jobs)
        blocks.append((match.group(1), jobs[match.start() : end]))
    return blocks


def check_workflow(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    if "pull_request_target" in text:
        fail(f"unsafe trigger: {path}")
    if re.search(r"permissions:\s*(write-all|write)", text):
        fail(f"write permission: {path}")
    if "secrets." in text or "curl " in text or "wget " in text:
        fail(f"external mutable input: {path}")
    if not re.search(r"permissions:\n  contents: read\n", text):
        fail(f"missing read-only permissions: {path}")

    for action in re.findall(r"uses:\s*[^@\s]+@([^\s]+)", text):
        if not SHA.fullmatch(action):
            fail(f"mutable action reference: {path}")

    for runner in re.findall(r"runs-on:\s*([^\n]+)", text):
        if re.search(r"(?:large|xlarge|[234]xl)", runner, re.IGNORECASE):
            fail(f"paid larger runner: {path}")

    for name, block in job_blocks(text):
        if "timeout-minutes:" not in block:
            fail(f"job without timeout: {path}:{name}")


def main() -> None:
    paths = sorted(WORKFLOWS.glob("*.yml"))
    if {path.name for path in paths} != {"benchmark.yml", "ci.yml"}:
        fail("unexpected workflow inventory")
    for path in paths:
        check_workflow(path)

    benchmark = (WORKFLOWS / "benchmark.yml").read_text(encoding="utf-8")
    triggers = benchmark.split("\npermissions:", 1)[0]
    if "push:" in triggers or "pull_request:" in triggers:
        fail("benchmark must be manual only")
    if "workflow_dispatch:" not in triggers:
        fail("benchmark lacks manual trigger")

    ci = (WORKFLOWS / "ci.yml").read_text(encoding="utf-8")
    for required in (
        "ubuntu-24.04",
        "ubuntu-24.04-arm",
        "macos-15",
        "windows-2025",
        "asan",
        "ubsan",
        "CARROT25519_PORTABLE_ONLY=ON",
    ):
        if required not in ci:
            fail(f"CI matrix is missing {required}")
    print("workflow_policy=pass")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

"""Reject unqualified full-width multiplies in portable and fixed-base C objects.

Small constant multipliers have fixed upper bits. This is not a timing proof.
"""

import re
import subprocess
import sys


def has_small_constant(history: list[str], register: str) -> bool:
    if register == "zr":
        return True
    for instruction in reversed(history):
        if re.search(rf"\b[wx]{register}\b", instruction):
            constant = re.search(
                rf"\bmov\s+[wx]{register},\s*#(0x[0-9a-f]+|[0-9]+)\s*$",
                instruction,
            )
            return constant is not None and int(constant[1], 0) <= 0xffffffff
    return False


def check(disassembly: str) -> None:
    required = {"portable.c.o", "ref10.c.o", "fixed_base.c.o"}
    members = {member: [] for member in required}
    active = None
    for line in disassembly.splitlines():
        if "file format" in line:
            active = None
            for member in required:
                if member in line:
                    if not re.search(r"aarch64|arm64", line):
                        raise ValueError("expected AArch64 disassembly")
                    active = member
                    break
        elif active:
            members[active].append(line)

    for member, lines in members.items():
        targets = set()
        for line in lines:
            branch = re.search(
                r"\b(?:b(?:\.[a-z]+)?|cbz|cbnz|tbz|tbnz)\s+([^/]+)", line
            )
            if branch:
                target = branch[1].split(",")[-1].split()[0]
                targets.add(int(target, 16))

        history = []
        count = 0
        for line in lines:
            address = re.match(r"\s*([0-9a-f]+):\s", line)
            if address is None:
                if re.match(r"\s*[0-9a-f]+ <[^>]+>:", line):
                    history = []
                continue
            count += 1
            if int(address[1], 16) in targets:
                history = []
            instruction = line.split("//", 1)[0].strip()
            multiply = re.search(
                r"\b(?:mul|madd|msub|mneg|smulh|umulh)\s+x(?:[0-9]+|zr),"
                r"\s*x([0-9]+|zr),\s*x([0-9]+|zr)\b",
                instruction,
            )
            if multiply and not any(
                has_small_constant(history, register) for register in multiply.groups()
            ):
                context = "\n".join(history[-48:])
                raise ValueError(
                    f"{member}: unqualified full-width multiply: {line.strip()}\n{context}"
                )
            if re.search(
                r"\b(?:b(?:\.[a-z]+)?|bl|blr|br|ret|cbz|cbnz|tbz|tbnz)(?:\s|$)",
                instruction,
            ):
                history = []
            history.append(instruction)
        if count == 0:
            raise ValueError("missing portable/fixed-base AArch64 disassembly")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: verify_portable_arm64.py LIBRARY OBJDUMP")
    output = subprocess.check_output([sys.argv[2], "-d", sys.argv[1]], text=True)
    try:
        check(output)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    print("portable_arm64_multiply=pass (instruction hazard only)")

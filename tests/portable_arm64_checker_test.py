#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import unittest

from verify_portable_arm64 import check


def archive(instruction: str, format_name: str = "elf64-littleaarch64") -> str:
    return "\n".join(
        f"{member}: file format {format_name}\n   0: 9ba27c20 {instruction}"
        for member in ("portable.c.o", "ref10.c.o", "fixed_base.c.o")
    )


class MultiplyCheck(unittest.TestCase):
    def test_small_constant_multiplier_is_allowed(self) -> None:
        check(archive("mov w1, #0x13\n   4: 9b027c20 mul x0, x1, x2"))

    def test_overwritten_constant_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check(archive(
                "mov w1, #0x13\n   4: aa0303e1 mov x1, x3\n"
                "   8: 9b027c20 mul x0, x1, x2"
            ))

    def test_partial_register_write_invalidates_constant(self) -> None:
        with self.assertRaises(ValueError):
            check(archive(
                "mov w1, #0x13\n   4: f2c00041 movk x1, #2, lsl #32\n"
                "   8: 9b027c20 mul x0, x1, x2"
            ))

    def test_control_flow_invalidates_constant(self) -> None:
        for instruction in ("bl 0x20", "b.eq 0x20", "cbz x3, 0x20", "br x3"):
            with self.subTest(instruction=instruction), self.assertRaises(ValueError):
                check(archive(
                    "mov w1, #0x13\n   4: 94000000 " + instruction +
                    "\n   8: 9b027c20 mul x0, x1, x2"
                ))

    def test_branch_cannot_skip_constant_assignment(self) -> None:
        with self.assertRaises(ValueError):
            check(archive(
                "cbz x3, 8 <function+0x8>\n"
                "   4: 52800261 mov w1, #0x13\n"
                "   8: 9b027c20 mul x0, x1, x2"
            ))

    def test_full_width_constant_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check(archive("mov x1, #0x100000000\n   4: 9b027c20 mul x0, x1, x2"))

    def test_signed_operands_are_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check(archive("ldpsw x1, x2, [x3]\n   4: 9b027c20 mul x0, x1, x2"))

    def test_widening_32_bit_products_are_allowed(self) -> None:
        for instruction in ("umull x0, w1, w2", "smaddl x0, w1, w2, x3", "mul w0, w1, w2"):
            check(archive(instruction))

    def test_full_width_products_are_rejected(self) -> None:
        for opcode in ("mul", "madd", "msub", "mneg", "smulh", "umulh"):
            with self.subTest(opcode=opcode), self.assertRaises(ValueError):
                check(archive(f"{opcode} x0, x1, x2"))

    def test_macho_archive_is_supported(self) -> None:
        check(archive("umull x0, w1, w2", "mach-o arm64"))

    def test_wrong_architecture_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check(archive("imul %rax, %rbx", "elf64-x86-64"))

    def test_missing_member_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check("portable.c.o: file format elf64-littleaarch64\n  0: d65f03c0 ret")

    def test_empty_disassembly_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            check("")

    def test_each_member_must_have_instructions(self) -> None:
        with self.assertRaises(ValueError):
            check(archive("ret").replace("   0: 9ba27c20 ret", "", 1))


if __name__ == "__main__":
    unittest.main()

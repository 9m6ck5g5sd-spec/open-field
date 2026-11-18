#!/usr/bin/env python3
"""Disassemble a stretch of a firmware file, one right line per instruction.

    python -I tools/bfin_sweep.py <records folder> 0x8f01d9f0 0x8f01dbc8 [more start end pairs]

The records folder is what tools/ldr_dump_records.py writes from your own firmware file; keep it outside the
repository. The toolchain's disassembler (`bfin-elf-objdump`) does not know the instruction forms that only the
newer Blackfin+ core has. It prints something else for them and then loses its place for the bytes that follow, so
its listing of a whole record shows ILLEGAL and stray 2-byte lines around them. This tool walks the bytes
instruction by instruction: at each boundary it looks at the first 16-bit word for one of those forms and decodes it
by hand, and otherwise lets objdump decode from that exact boundary and takes only the first instruction it prints.
The result is one line per instruction.

Forms decoded by hand (little-endian 16-bit words w0 w1 w2 w3):
  DA0r hi lo 0000   reg = imm32 (r 0-7 = R0-R7, 8-15 = P0-P5, SP, FP)                     8 bytes
  D800 hi lo flags  absolute load/store of a global: flags bits 0-2 reg, bit 6 P-reg,
                    bit 9 store, bits 10-11 width (00 32-bit, 01 16-bit, 1x byte)       8 bytes
  DE00 hi lo 0000   CALL.a imm32;   DC00 hi lo 0000   JUMP.a imm32                          8 bytes
  E0Cx/E0Dx w1      LSETUP with an immediate count: count = (w0 & 0xF) << 6 | w1 >> 10,
                    0 = software loop; bottom = PC + 2 * (w1 & 0x3FF); LC0 for E0Cx, LC1 for E0Dx   4 bytes

A few forms are known neither here nor to objdump: a loop's set-up with its count from a P register (first word
E0Ax) is one. Such a line reads ILLEGAL, and the one to three lines after it can be wrong. docs/methods.md says how
to go on from there.

What it prints is the firmware's code: read it, and keep it out of this repository. Read-only: it reads the records
and writes nothing but a temporary file.
"""

import glob
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDUMP = os.path.join(ROOT, "open-xy", "local", "toolchain", "bin", "bfin-elf-objdump.exe")
if not os.path.exists(OBJDUMP):
    OBJDUMP = OBJDUMP[:-4]
REGS = ["R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "P0", "P1", "P2", "P3", "P4", "P5", "SP", "FP"]
LINE = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)$")


def load_records(folder):
    records = []
    for path in glob.glob(os.path.join(folder, "*.bin")):
        m = re.search(r"_([0-9a-f]{8})\.bin$", os.path.basename(path))
        if m:
            records.append((int(m.group(1), 16), path))
    records.sort()
    return records


def record_for(records, address):
    for base, path in records:
        size = os.path.getsize(path)
        if base <= address < base + size:
            return base, path
    return None


def plus_form(words, pc):
    w0, w1, w2, w3 = words
    target = (w1 << 16) | w2
    if (w0 & 0xff00) == 0xda00 and w3 == 0:
        index = w0 & 0xff
        if index < 16:
            name = REGS[index]
        elif index < 32:                                   # the DAG register groups, in the usual I, M, B, L order
            name = "IMBL"[(index - 16) >> 2] + str(index & 3)
        else:
            name = "reg%02x" % index
        return 8, "%s = 0x%08x;" % (name, target)
    if w0 == 0xd800:
        reg = REGS[(w3 & 7) + (8 if w3 & 0x40 else 0)]
        width = {0: "", 1: "W", 2: "B", 3: "B"}[(w3 >> 10) & 3]
        if w3 & 0x200:
            return 8, "%s[0x%08x] = %s;" % (width, target, reg)
        return 8, "%s = %s[0x%08x];" % (reg, width, target)
    if w0 == 0xde00 and w3 == 0:
        return 8, "CALL.a 0x%08x;" % target
    if w0 == 0xdc00 and w3 == 0:
        return 8, "JUMP.a 0x%08x;" % target
    if (w0 & 0xffe0) == 0xe0c0:
        count = ((w0 & 0xf) << 6) | (w1 >> 10)
        bottom = pc + 2 * (w1 & 0x3ff)
        counter = "LC1" if w0 & 0x10 else "LC0"
        body = "software loop" if count == 0 else "%s = %d" % (counter, count)
        return 4, "LSETUP(0x%08x, 0x%08x) %s;" % (pc + 4, bottom, body)
    return 0, ""


def objdump_first(data, pc, tmp):
    """objdump the bytes at pc and return (length, text) of the first instruction."""
    with open(tmp, "wb") as f:
        f.write(data[:32].ljust(32, b"\x00"))
    out = subprocess.run([OBJDUMP, "-D", "-b", "binary", "-m", "bfin", "--adjust-vma=0x%x" % pc, tmp],
                         capture_output=True, text=True).stdout
    first = None
    for line in out.splitlines():
        m = LINE.match(line)
        if m and int(m.group(1), 16) == pc:
            first = m
            break
    if not first:
        return 2, "?? %02x%02x" % (data[1], data[0])
    length = len(first.group(2).split())
    text = first.group(3).strip()
    # a 64-bit multi-issue instruction: objdump prints the 32-bit op, the two 16-bit slots follow on the next line
    if length == 4 and "||" in text:
        length = 8
    return length, text


def sweep(records, start, end, out):
    found = record_for(records, start)
    if not found:
        out.write("%08x: not stored\n" % start)
        return
    base, path = found
    data = open(path, "rb").read()
    tmp = os.path.join(tempfile.mkdtemp(), "slice.bin")
    pc = start
    while pc < end:
        off = pc - base
        if off + 2 > len(data):
            break
        chunk = data[off:off + 8].ljust(8, b"\x00")
        words = [int.from_bytes(chunk[i:i + 2], "little") for i in range(0, 8, 2)]
        length, text = plus_form(words, pc)
        if not length:
            length, text = objdump_first(data[off:off + 32], pc, tmp)
        raw = " ".join("%02x" % b for b in data[off:off + length])
        out.write("%08x: %-26s %s\n" % (pc, raw, text))
        pc += length


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) < 3 or len(argv) % 2 == 0:
        print(__doc__)
        return 2
    records = load_records(argv[0])
    for i in range(1, len(argv), 2):
        start, end = int(argv[i], 16), int(argv[i + 1], 16)
        sys.stdout.write("== %08x..%08x\n" % (start, end))
        sweep(records, start, end, sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())

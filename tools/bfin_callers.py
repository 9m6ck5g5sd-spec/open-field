#!/usr/bin/env python3
"""Who reaches an address: direct calls, jumps, immediate loads and data pointers in the record dumps.

  python -I tools/bfin_callers.py <records folder> 0x8c0b11d0 [0x... ...]

Scans every stored record (from `ldr_dump_records.py`, `index.json` beside the `.bin` files) at each
2-byte position for the Blackfin+ forms that name a 32-bit address:

  CALL/JUMP.L   4 bytes  `E3xx xxxx` / `E2xx xxxx`, 24-bit halfword offset from the instruction itself
  CALL.a/JUMP.a 8 bytes  `DE00 hi lo 0000` / `DC00 hi lo 0000`
  Rn = imm32    8 bytes  `DA0r hi lo 0000` (function pointers taken by value)
  abs load/store 8 bytes `D800 hi lo flags`: a load (flags bit 9 clear) or store (set) of a global at hi:lo
  data word     4 bytes  the little-endian address at a 4-byte aligned position (method and callback tables)

Matches inside code records are reliable; a data record can produce a coincidental pattern, so the
record index is printed with every hit. An address that code builds from two half loads is not found here
(tools/ldr_xref.py finds those), and neither is one that code works out by arithmetic. Read-only.
"""
import json
import os
import struct
import sys


def load(folder):
    with open(os.path.join(folder, "index.json"), encoding="utf-8") as f:
        rows = json.load(f)["records"]
    records = []
    for r in rows:
        if "file" in r:
            with open(os.path.join(folder, r["file"]), "rb") as f:
                records.append((r["index"], r["target"], f.read()))
    return records


def scan(records, wanted):
    hits = {w: [] for w in wanted}
    for index, base, data in records:
        n = len(data)
        for i in range(0, n - 3, 2):
            w0 = data[i] | (data[i + 1] << 8)
            kind = w0 >> 8
            if kind in (0xe2, 0xe3):
                encoded = ((w0 & 0xff) << 16) | data[i + 2] | (data[i + 3] << 8)
                if encoded & 0x800000:
                    encoded -= 1 << 24
                target = (base + i + encoded * 2) & 0xffffffff
                if target in hits:
                    hits[target].append((base + i, index, "CALL" if kind == 0xe3 else "JUMP.L"))
            if i + 8 <= n and (w0 in (0xde00, 0xdc00, 0xd800) or (w0 & 0xfff0) == 0xda00):
                hi, lo, w3 = struct.unpack_from("<3H", data, i + 2)
                target = (hi << 16) | lo
                if target in hits:
                    if w0 == 0xd800:
                        width = {0: "32", 1: "16", 2: "8", 3: "8"}[(w3 >> 10) & 3]
                        reg = ("P%d" if w3 & 0x40 else "R%d") % (w3 & 7)
                        what = "abs %s %s-bit %s" % ("store" if w3 & 0x200 else "load", width, reg)
                        hits[target].append((base + i, index, what))
                    elif w3 == 0:
                        what = {0xde00: "CALL.a", 0xdc00: "JUMP.a"}.get(w0, "imm32 -> %s" % register(w0 & 15))
                        hits[target].append((base + i, index, what))
            if i % 4 == 0:
                word = struct.unpack_from("<I", data, i)[0]
                if word in hits:
                    hits[word].append((base + i, index, "data word"))
    return hits


def register(n):
    return (["R%d" % i for i in range(8)] + ["P0", "P1", "P2", "P3", "P4", "P5", "SP", "FP"])[n]


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) < 2:
        print(__doc__)
        return 2
    records = load(argv[0])
    wanted = [int(a, 16) for a in argv[1:]]
    hits = scan(records, wanted)
    for w in wanted:
        print("0x%08x: %d reference(s)" % (w, len(hits[w])))
        for address, index, what in sorted(hits[w]):
            print("   0x%08x  record %3d  %s" % (address, index, what))
    return 0


if __name__ == "__main__":
    sys.exit(main())

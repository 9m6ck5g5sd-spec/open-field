#!/usr/bin/env python3
"""A firmware file's records seen as memory: find 32-bit words, dump bytes, read strings.

  python -I tools/ldrmem.py <records folder> find 0xVALUE [0xVALUE ...]   little-endian word search in every record
  python -I tools/ldrmem.py <records folder> dump 0xADDR N                hex dump of N bytes at a load address
  python -I tools/ldrmem.py <records folder> str 0xADDR                   the NUL-terminated string at an address

The records folder is what tools/ldr_dump_records.py writes from your own firmware file. What this prints are the
firmware's own bytes: keep them out of this repository. Read-only.
"""
import sys, os, struct, glob, re
def load(recdir):
    recs = []
    for p in glob.glob(os.path.join(recdir, "*.bin")):
        m = re.match(r"(\d+)_([0-9a-f]+)\.bin$", os.path.basename(p))
        if not m: continue
        recs.append((int(m.group(2), 16), open(p, "rb").read(), int(m.group(1))))
    recs.sort()
    return recs
def read(recs, addr, n):
    for base, data, _ in recs:
        if base <= addr < base + len(data):
            return data[addr - base: addr - base + n]
    return None
def find_u32(recs, val):
    pat = struct.pack("<I", val)
    out = []
    for base, data, idx in recs:
        i = data.find(pat)
        while i != -1:
            out.append((base + i, idx)); i = data.find(pat, i + 1)
    return out
if __name__ == "__main__":
    recs = load(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "find":
        for v in sys.argv[3:]:
            v = int(v, 0)
            hits = find_u32(recs, v)
            print("0x%08x: %d hits" % (v, len(hits)))
            for a, idx in hits[:40]:
                print("   at 0x%08x (record %d)%s" % (a, idx, "  [unaligned]" if a % 4 else ""))
    elif cmd == "dump":
        addr = int(sys.argv[3], 0); n = int(sys.argv[4], 0)
        b = read(recs, addr, n)
        for off in range(0, len(b), 16):
            chunk = b[off:off+16]
            words = " ".join("%08x" % w for w in struct.unpack("<%dI" % (len(chunk)//4), chunk[:len(chunk)//4*4]))
            print("0x%08x: %s" % (addr + off, words))
    elif cmd == "str":
        addr = int(sys.argv[3], 0)
        b = read(recs, addr, 64) or b""
        print(repr(b.split(b"\0")[0]))

#!/usr/bin/env python3
"""Read-only cross-reference finder for a plain Blackfin+ boot stream (os.ldr).

    python -I tools/ldr_xref.py <os.ldr> [--value 0x01400000 ...] [--string "content/op1_factory.json" ...]
                                 [--addr-of-string ...] [--window 24]

For every value (a constant, or the address of a string found with --string) it reports where the
image references it:
  ldimm32   Blackfin+ 64-bit immediate load  <reg> = value     (words: 0xDA00|reg, hi16, lo16, 0)
  call.a    Blackfin+ absolute call/jump to value               (words: 0xDE00|x,   hi16, lo16, 0)
  half      classic pair  reg.H = hi16 / reg.L = lo16 (0xE1xx opcodes) within --window bytes
  data      a little-endian 32-bit word equal to the value in any stored record (tables, vtables)
Each hit gives the virtual address, the record index and the file offset, so the function around it
can be disassembled:  bfin-elf-objdump -D -b binary -m bfin --adjust-vma=<record target> <record dump>.
Writes nothing.
"""
import argparse
import re
import struct
import sys
from functools import reduce
from operator import xor


def parse(data):
    blocks, offset = [], 0
    while offset < len(data):
        raw = data[offset:offset + 16]
        if len(raw) < 16:
            raise ValueError('truncated header')
        code, target, count, argument = struct.unpack('<IIII', raw)
        if code >> 24 != 0xad or reduce(xor, raw) != 0:
            raise ValueError('not a plain LDR at 0x%x' % offset)
        fill = bool(code & 0x100)
        stored = 0 if fill else count
        blocks.append(dict(index=len(blocks), offset=offset, target=target, count=count, fill=fill,
                           payload=data[offset + 16:offset + 16 + stored]))
        offset += 16 + stored
        if code & 0x8000:
            return blocks
    raise ValueError('no FINAL record')


def stored(blocks):
    return [b for b in blocks if not b['fill'] and b['payload']]


def find_strings(blocks, text):
    needle = text.encode('ascii')
    hits = []
    for b in stored(blocks):
        start = 0
        while True:
            i = b['payload'].find(needle, start)
            if i < 0:
                break
            end = i + len(needle)
            terminated = end >= len(b['payload']) or b['payload'][end] == 0
            hits.append((b['target'] + i, b['index'], terminated))
            start = i + 1
    return hits


def xrefs(blocks, value, window):
    hi, lo = value >> 16, value & 0xffff
    results = []
    le = struct.pack('<I', value)
    tail = struct.pack('<HHH', hi, lo, 0)
    for b in stored(blocks):
        p, base = b['payload'], b['target']
        # data words (any alignment, flagged)
        start = 0
        while True:
            i = p.find(le, start)
            if i < 0:
                break
            results.append((base + i, b['index'], b['offset'] + 16 + i, 'data' + ('' if i % 4 == 0 else ' (unaligned)')))
            start = i + 1
        # 64-bit immediate forms: opcode word then hi, lo, 0
        start = 0
        while True:
            i = p.find(tail, start)
            if i < 0:
                break
            if i >= 2 and i % 2 == 0:
                op = struct.unpack_from('<H', p, i - 2)[0]
                if op & 0xff00 == 0xda00:
                    results.append((base + i - 2, b['index'], b['offset'] + 16 + i - 2, 'ldimm32 reg%d' % (op & 0xff)))
                elif op & 0xff00 == 0xde00:
                    results.append((base + i - 2, b['index'], b['offset'] + 16 + i - 2, 'call.a op%02x' % (op & 0xff)))
            start = i + 1
        # classic half loads: find every E1xx word with imm == hi, look for the matching lo nearby
        for i in range(0, len(p) - 3, 2):
            op = struct.unpack_from('<H', p, i)[0]
            if op & 0xff80 != 0xe140:  # H=1 loads only (reg.H = imm)
                continue
            if struct.unpack_from('<H', p, i + 2)[0] != hi:
                continue
            regbits = op & 0x1f
            for j in range(max(0, i - window), min(len(p) - 3, i + window) + 1, 2):
                if j == i:
                    continue
                op2 = struct.unpack_from('<H', p, j)[0]
                if op2 & 0xffe0 == 0xe100 and op2 & 0x1f == regbits and struct.unpack_from('<H', p, j + 2)[0] == lo:
                    results.append((base + min(i, j), b['index'], b['offset'] + 16 + min(i, j), 'half reg%d%s' % (regbits & 7, '' if regbits < 8 else ' (P)')))
                    break
    return sorted(set(results))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('file')
    parser.add_argument('--value', action='append', default=[], help='hex constant or address, repeatable')
    parser.add_argument('--string', action='append', default=[], help='ASCII string to locate, then cross-reference its address')
    parser.add_argument('--window', type=int, default=24, help='bytes between the two halves of a classic load')
    parser.add_argument('--max', type=int, default=40, help='hits printed per value')
    args = parser.parse_args(argv)
    with open(args.file, 'rb') as f:
        blocks = parse(f.read())
    values = []
    for v in args.value:
        values.append((int(v, 16), v))
    for text in args.string:
        hits = find_strings(blocks, text)
        if not hits:
            print('string %r: not found' % text)
        for va, idx, terminated in hits:
            print('string %r at 0x%08x (record %d%s)' % (text, va, idx, '' if terminated else ', not NUL-terminated here'))
            values.append((va, 'string %r' % text))
    for value, label in values:
        hits = xrefs(blocks, value, args.window)
        print()
        print('== 0x%08x  %s: %d reference%s' % (value, label, len(hits), '' if len(hits) == 1 else 's'))
        for va, idx, off, kind in hits[:args.max]:
            print('   0x%08x  record %3d  file 0x%07x  %s' % (va, idx, off, kind))
        if len(hits) > args.max:
            print('   ... %d more' % (len(hits) - args.max))
    return 0


if __name__ == '__main__':
    sys.exit(main())

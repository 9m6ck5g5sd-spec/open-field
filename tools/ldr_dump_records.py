#!/usr/bin/env python3
"""Write every stored record of a Blackfin boot stream into a file of its own, with an index: the form in which
the check before a commit (tools/publish_check.py) and the tools for reading the firmware take your own copy of it.

    python -I tools/ldr_dump_records.py <file.ldr> --out <new folder> [--min-bytes N]

Writes <out>/<index>_<target>.bin for each record that stores bytes (fill records are
listed but not written) and <out>/index.json describing all of them. Refuses an
existing output folder, so nothing is ever overwritten. The folder it writes is a copy of the firmware:
keep it outside the repository and never publish it. The reading tools take the folder (tools/bfin_sweep.py,
tools/bfin_callers.py, tools/ldrmem.py), and the toolchain's disassembler takes one record of it:

    bfin-elf-objdump -D -b binary -m bfin --adjust-vma=0x<target> <out>/<index>_<target>.bin

docs/methods.md says what that disassembler gets wrong and how the reading tools get round it.
"""
import argparse
import json
import os
import struct
import sys
from functools import reduce
from operator import xor


def parse(data):
    blocks, offset = [], 0
    while offset < len(data):
        raw = data[offset:offset + 16]
        if len(raw) < 16:
            raise ValueError('truncated header at 0x%x' % offset)
        code, target, count, argument = struct.unpack('<IIII', raw)
        if code >> 24 != 0xad or reduce(xor, raw) != 0:
            raise ValueError('bad record header at 0x%x' % offset)
        fill = bool(code & 0x100)
        stored = 0 if fill else count
        blocks.append(dict(index=len(blocks), offset=offset, flags=code & 0xffff, target=target, count=count,
                           argument=argument, fill=fill, stored=stored))
        offset += 16 + stored
        if code & 0x8000:
            return blocks
    raise ValueError('no FINAL record')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('file')
    parser.add_argument('--out', required=True, help='new folder for the dumps')
    parser.add_argument('--min-bytes', type=int, default=1, help='skip stored records smaller than this')
    args = parser.parse_args(argv)
    with open(args.file, 'rb') as f:
        data = f.read()
    blocks = parse(data)
    os.makedirs(args.out, exist_ok=False)
    written = 0
    for b in blocks:
        if b['stored'] >= args.min_bytes and not b['fill']:
            name = '%03d_%08x.bin' % (b['index'], b['target'])
            with open(os.path.join(args.out, name), 'xb') as f:
                f.write(data[b['offset'] + 16:b['offset'] + 16 + b['stored']])
            b['file'] = name
            written += 1
    with open(os.path.join(args.out, 'index.json'), 'x', encoding='utf-8') as f:
        json.dump(dict(source=os.path.abspath(args.file), bytes=len(data), records=blocks), f, indent=1)
    print('%d records, %d dumped to %s' % (len(blocks), written, args.out))
    return 0


if __name__ == '__main__':
    sys.exit(main())

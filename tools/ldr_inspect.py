#!/usr/bin/env python3
"""Read-only inspector for a Blackfin / Blackfin+ boot stream (os.ldr, bootloader.ldr).

    python -I tools/ldr_inspect.py <file.ldr> [--strings | --all-strings] [--dump N --out FILE]

Lists every loader record (16-byte header: 0xAD + flags, target address, byte count,
argument; the 16 header bytes XOR to zero), the load map, and whether each payload
looks like plain code (entropy roughly 5-7 bits/byte) or ciphertext (close to 8.0).
Writes nothing unless --dump is given, and then only to a new file named by --out.
Same record rules as open-xy/tooling/formats.py, kept standalone so it runs on any file.
"""
import argparse
import hashlib
import math
import re
import struct
import sys
from collections import Counter
from functools import reduce
from operator import xor

FLAGS = {0x8000: 'FINAL', 0x4000: 'FIRST', 0x2000: 'INDIRECT', 0x1000: 'IGNORE', 0x0800: 'INIT',
         0x0400: 'CALLBACK', 0x0200: 'QUICKBOOT', 0x0100: 'FILL', 0x0020: 'AUX', 0x0010: 'SAVE'}

LANDMARKS = re.compile(r'(\d+\.\d+\.\d+)|op-?1|op-?xy|teenage|\.ldr|\.svg|\.json|\.aif|\.wav|/content|/presets|'
                       r'/tape|upgrade|bootloader|firmware|factory|undo|sampler|tape|mixer|engine', re.I)


def entropy(data):
    if not data:
        return 0.0
    counts = Counter(data)
    n = len(data)
    return -sum(c / n * math.log2(c / n) for c in counts.values())


def flag_names(code):
    names = [name for bit, name in FLAGS.items() if code & bit]
    names.append('dma%d' % (code & 0xf))
    return '+'.join(names)


def parse(data):
    blocks, offset = [], 0
    while offset < len(data):
        if offset + 16 > len(data):
            raise ValueError('truncated header at 0x%x' % offset)
        raw = data[offset:offset + 16]
        code, target, count, argument = struct.unpack('<IIII', raw)
        if code >> 24 != 0xad:
            raise ValueError('bad magic 0x%02x at 0x%x (expected 0xad): not a plain LDR, or encrypted' % (code >> 24, offset))
        if reduce(xor, raw) != 0:
            raise ValueError('header checksum mismatch at 0x%x' % offset)
        fill = bool(code & 0x100)
        stored = 0 if fill else count
        if offset + 16 + stored > len(data):
            raise ValueError('payload of record at 0x%x runs past the end of the file' % offset)
        blocks.append(dict(index=len(blocks), offset=offset, code=code & 0xffff, target=target, count=count,
                           argument=argument, fill=fill, stored=stored,
                           payload=data[offset + 16:offset + 16 + stored]))
        offset += 16 + stored
        if code & 0x8000:
            if offset != len(data):
                raise ValueError('%d trailing bytes after FINAL' % (len(data) - offset))
            return blocks
    raise ValueError('no FINAL record')


def strings(data, minimum):
    pattern = re.compile(rb'[\x20-\x7e]{%d,}' % minimum)
    for match in pattern.finditer(data):
        yield match.start(), match.group().decode('ascii')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('file')
    parser.add_argument('--strings', action='store_true', help='print landmark strings (versions, paths, file types)')
    parser.add_argument('--all-strings', action='store_true', help='print every printable string of 8+ characters')
    parser.add_argument('--dump', type=int, metavar='N', help='write record N payload to --out (input for objdump)')
    parser.add_argument('--out', help='output file for --dump; must not exist yet')
    args = parser.parse_args(argv)

    with open(args.file, 'rb') as f:
        data = f.read()
    print('%s: %d bytes, sha256 %s' % (args.file, len(data), hashlib.sha256(data).hexdigest()))
    print('whole-file entropy %.3f bits/byte (plain firmware: roughly 5-7; ciphertext: about 8.0)' % entropy(data))
    try:
        blocks = parse(data)
    except ValueError as error:
        print('NOT A PLAIN LDR: %s' % error)
        return 1
    first = blocks[0]
    print('%d loader records; FIRST target 0x%08x, FIRST argument %d (file length - 16 = %d)'
          % (len(blocks), first['target'], first['argument'], len(data) - 16))
    print()
    print('%4s %9s %-28s %10s %9s %10s  note' % ('idx', 'offset', 'flags', 'target', 'count', 'argument'))
    for b in blocks:
        note = ''
        if b['fill']:
            note = 'fill with 0x%08x' % b['argument']
        elif b['stored'] >= 4096:
            note = 'entropy %.2f' % entropy(b['payload'])
        print('%4d 0x%07x %-28s 0x%08x %9d 0x%08x  %s'
              % (b['index'], b['offset'], flag_names(b['code']), b['target'], b['count'], b['argument'], note))
    print()
    loaded = [b for b in blocks if b['count']]
    if loaded:
        low = min(b['target'] for b in loaded)
        high = max(b['target'] + b['count'] for b in loaded)
        stored = sum(b['stored'] for b in loaded)
        filled = sum(b['count'] for b in loaded if b['fill'])
        print('load map: 0x%08x..0x%08x; %d bytes stored, %d bytes filled' % (low, high, stored, filled))
        regions = Counter(b['target'] >> 28 for b in loaded)
        print('address groups (Blackfin+: 0x0.. and 0x1.. on-chip memory, 0x8.. external DDR): '
              + ', '.join('0x%x_______ x%d' % (k, v) for k, v in sorted(regions.items())))
    if args.strings or args.all_strings:
        print()
        seen = 0
        for b in loaded:
            if b['fill']:
                continue
            for pos, text in strings(b['payload'], 8 if args.all_strings else 6):
                if args.all_strings or LANDMARKS.search(text):
                    print('0x%08x  %s' % (b['target'] + pos, text[:120]))
                    seen += 1
                    if seen >= 4000:
                        print('... (truncated at 4000 strings)')
                        return 0
    if args.dump is not None:
        if not args.out:
            parser.error('--dump needs --out')
        block = blocks[args.dump]
        with open(args.out, 'xb') as f:  # mode x: never overwrite an existing file
            f.write(block['payload'])
        print('wrote record %d (%d bytes, target 0x%08x) to %s' % (args.dump, len(block['payload']), block['target'], args.out))
        print('disassemble with: bfin-elf-objdump -D -b binary -m bfin --adjust-vma=0x%x %s' % (block['target'], args.out))
    return 0


if __name__ == '__main__':
    sys.exit(main())

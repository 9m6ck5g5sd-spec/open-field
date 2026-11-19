#!/usr/bin/env python3
"""What a place of the firmware needs in a profile, worked out from your own firmware file.

  python -I tools/place_sum.py --stock <your own os.ldr> 0xADDRESS [0xADDRESS ...] [--word]
  python -I tools/place_sum.py --stock <your own os.ldr> --profile profiles/op1f-1.7.3.json

A profile names each place where the running firmware is changed by its address, its length and a checksum of the
firmware's own bytes there (`sum`: their CRC-32). The loader takes the same checksum from the unit's memory before it
changes anything, so a unit with another firmware is left alone. The bytes themselves never go into a profile; this
tool reads them from your file and prints the checksum.

For each address it prints:

  - the entry for the profile's `patches`: the address, the length (8 for code; 4 with --word, for a word of data
    such as an entry of a table of functions) and the sum;
  - for a place in code, what a jump or a call there needs when the firmware's function is to go on afterwards: where
    the instructions under the eight bytes end (8, 10, 12 or 14 bytes in). That is what a trampoline copies
    (`copies`), and when it is more than eight the table entry gets a sum of its own over those bytes;
  - whether one of those instructions names a place by its distance from itself (a short jump, a call by distance,
    a loop's set-up). The loader cannot copy such an instruction to another address: no trampoline there. Choose
    another place, or put the whole function's work into your own;
  - whether the eight bytes are the load of an address into a register, which is what the form "load" needs.

With --profile it holds a whole profile against the file and writes nothing: every place's sum, every trampoline's
count of bytes and its own sum. Use it before you trust a profile with another unit's file, and after a port.

It prints checksums and lengths, never the firmware's bytes. The file is the one --stock names, or the one the
environment's OPEN_FIELD_STOCK names; keep it outside the repository. Reads only. Standard library only.
"""

import argparse
import hashlib
import importlib.util
import json
import os
import sys
import zlib

sys.dont_write_bytecode = True          # the reader of records is loaded from beside this file: no cache is left there

TOOLS = os.path.dirname(os.path.abspath(__file__))
L1_CODE = (0x11a00000, 0x11a10000)      # the two kinds of memory the firmware's code runs from
DDR = (0x80000000, 0x90000000)
HOOK = 8                                # a call, a jump or a load of ours takes eight bytes
MOST = 14                               # the instructions under those eight end at the eighth byte or two, four or six bytes on


def reader(path):
    """read(address, count) over a firmware file: the bytes its records put at an address, or None where the file
    stores none there (memory that is filled or left empty at the start)."""
    spec = importlib.util.spec_from_file_location("place_sum_records", os.path.join(TOOLS, "ldr_dump_records.py"))
    records = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(records)
    with open(path, "rb") as f:
        image = f.read()
    stored = [record for record in records.parse(image) if not record["fill"]]

    def read(address, count):
        for record in stored:
            if record["target"] <= address and address + count <= record["target"] + record["count"]:
                start = record["offset"] + 16 + address - record["target"]
                return image[start:start + count]
        return None
    return image, read


def instruction_bytes(first):
    """The length of the Blackfin instruction that starts with the 16-bit word `first`: 2, 4, or 8."""
    if first & 0xc000 != 0xc000:
        return 2
    if first & 0x0800 and first & 0xe800 != 0xe800:
        return 8
    return 4


def by_distance(first):
    """Whether the instruction that starts with `first` names a place by its distance from itself: a conditional or
    a short jump, a long jump or a call by distance, a loop's set-up, a jump or a call through PC + Pn."""
    return (first & 0xf000 in (0x1000, 0x2000) or first & 0xfe00 == 0xe200 or first & 0xff80 == 0xe080 or first & 0xfff8 in (0x0070, 0x0080))


def walk(read, address):
    """Where the instructions under an eight-byte change at `address` end: (bytes in, the offset of the first
    instruction that goes by distance or None). None when the file does not store the bytes."""
    at, relative = 0, None
    while at < HOOK:
        word = read(address + at, 2)
        if word is None:
            return None
        first = word[0] | (word[1] << 8)
        if by_distance(first) and relative is None:
            relative = at
        at += instruction_bytes(first)
    return at, relative


def in_code(address, count):
    return (L1_CODE[0] <= address and address + count <= L1_CODE[1]) or (DDR[0] <= address and address + count <= DDR[1])


def crc(data):
    return "0x%08x" % (zlib.crc32(data) & 0xffffffff)


def describe(read, address, word):
    """The lines for one address, and whether everything could be said."""
    length = 4 if word else HOOK
    lines = ["0x%08x" % address]
    held = read(address, length)
    if held is None:
        return lines + ["  the file stores no bytes there: it is no place a profile can name"], False
    if address % (4 if word else 2) or not (DDR[0] <= address < DDR[1] or (not word and in_code(address, HOOK))):
        return lines + ["  a word of data lies in DDR on a multiple of four, and code in L1 code or DDR on an even address: this is neither"], False
    lines.append('  for "patches":  {"address": "0x%08x", "length": %d, "sum": "%s", "group": "<your pack>", "name": "<what stands there, in your own words>"}'
                 % (address, length, crc(held)))
    if word:
        lines.append('  for a table:    {"address": "0x%08x", "form": "word", "target": "<your function>", "does": "<what yours does there>"}' % address)
        return lines, True
    if held[1] == 0xda and held[0] < 0x10 and held[6:8] == b"\0\0":
        lines.append('  the eight bytes are the load of an address into a register: the form "load" fits')
    found = walk(read, address)
    if found is None:
        return lines + ["  the instructions under the eight bytes run past what the file stores"], False
    count, relative = found
    if relative is not None:
        lines.append("  an instruction %d bytes in names a place by its distance from itself: no trampoline can copy it" % relative)
        lines.append('  for a table:    {"address": "0x%08x", "form": "<call or jump>", "target": "<your function>", "does": "<what yours does there>"}' % address)
        return lines, True
    if not in_code(address, count):
        return lines + ["  the instructions under the eight bytes end %d bytes in, past the end of the code's memory" % count], False
    entry = '{"address": "0x%08x", "form": "<call or jump>", "target": "<your function>", "room": "<your room>", "copies": %d' % (address, count)
    if count > HOOK:
        entry += ', "sum": "%s"' % crc(read(address, count))
    lines.append("  the instructions under the eight bytes end %d bytes in: a trampoline copies %d, and its room is %d bytes" % (count, count, count + 8))
    lines.append("  for a table:    " + entry + ', "does": "<what yours does there>"}')
    return lines, True


def check_profile(read, image, profile):
    """Every place and every trampoline of a profile against the file: the lines for what differs."""
    wrong = []
    if hashlib.sha256(image).hexdigest() != profile.get("stock_sha256"):
        wrong.append("the file is not the one the profile is for (its SHA-256 differs)")
    places = {}
    for place in profile["patches"]:
        address, length = int(place["address"], 16), int(place["length"])
        places[address] = place
        held = read(address, length)
        if held is None:
            wrong.append("%s: the file stores no bytes there" % place["address"])
        elif crc(held) != place["sum"]:
            wrong.append("%s: the sum of the file's bytes there is not the profile's" % place["address"])
    entries = 0
    for table in profile.get("tables", ()):
        for entry in table["entries"]:
            entries += 1
            address, copies = int(entry["address"], 16), int(entry.get("copies", 0))
            if not copies:
                continue
            found = walk(read, address)
            if found is None:
                wrong.append("%s: the file stores no bytes there" % entry["address"])
                continue
            count, relative = found
            if relative is not None:
                wrong.append("%s: an instruction under the change goes by distance, and the entry has a trampoline" % entry["address"])
            elif count != copies:
                wrong.append("%s: the instructions end %d bytes in, and the entry's trampoline copies %d" % (entry["address"], count, copies))
            wanted = entry.get("sum", places.get(address, {}).get("sum"))
            held = read(address, copies)
            if held is None or crc(held) != wanted:
                wrong.append("%s: the sum over the %d bytes its trampoline copies is not the entry's" % (entry["address"], copies))
    return wrong, len(places), entries


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("addresses", nargs="*", help="addresses in the firmware, in hex")
    ap.add_argument("--stock", default=os.environ.get("OPEN_FIELD_STOCK") or None, help="your own firmware file (os.ldr); without it, the one OPEN_FIELD_STOCK names")
    ap.add_argument("--word", action="store_true", help="the addresses are words of data (four bytes), not code")
    ap.add_argument("--profile", help="hold a whole profile against the file")
    args = ap.parse_args(argv)
    if not args.stock:
        print("error: give --stock, your own firmware file (or set OPEN_FIELD_STOCK)")
        return 2
    if bool(args.addresses) == bool(args.profile):
        print("error: give addresses, or --profile FILE")
        return 2
    try:
        image, read = reader(args.stock)
        print("the file: %d bytes, SHA-256 %s" % (len(image), hashlib.sha256(image).hexdigest()))
        if args.profile:
            with open(args.profile, encoding="utf-8") as f:
                profile = json.load(f)
            wrong, places, entries = check_profile(read, image, profile)
            for line in wrong:
                print("DIFFERS   " + line)
            print("%d place(s) and %d table entr%s: %s" % (places, entries, "y" if entries == 1 else "ies",
                                                         "all as the file has them" if not wrong else "%d difference(s)" % len(wrong)))
            return 1 if wrong else 0
        good = True
        for text in args.addresses:
            lines, said = describe(read, int(text, 16), args.word)
            good = good and said
            print("\n".join(lines))
        return 0 if good else 1
    except (OSError, ValueError, KeyError) as problem:
        print("error: %s" % problem)
        return 2


if __name__ == "__main__":
    sys.exit(main())

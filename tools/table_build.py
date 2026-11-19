#!/usr/bin/env python3
"""Write a pack's or the loader's table of firmware changes from the profile.

  python -I tools/table_build.py --profile profiles/op1f-1.7.3.json --all
  python -I tools/table_build.py --profile profiles/op1f-1.7.3.json --table module/build_m2_base_table.s [--out FILE]
  python -I tools/table_build.py --profile profiles/op1f-1.7.3.json --check

The profile lists every place where open-field changes the running firmware (`patches`: an address, a length, the
CRC-32 of the firmware's own bytes there, and what stands there in our words) and, for each pack and for the loader, a
table (`tables`): which function of ours goes to which place, and how. This tool writes each table as the assembly
source its pack is built from. `--check` writes nothing: it says whether the sources in the tree are what the profile
gives, and answers 1 when one is not.

One entry of a table is five 32-bit words:

  kind     0x10 and: 0 a data word, 1 a call in L1 code, 2 a call in DDR code, 3 a jump in L1 code, 4 a jump in DDR
           code, 5 the load of an address in DDR code; in its high half the bytes its trampoline copies
  place    the address
  target   the function of ours; the linker fills in its address
  sum      the CRC-32 of the firmware's bytes the change replaces (four for a data word, else eight), or of the bytes
           its trampoline copies when those are more
  room     the address of the pack's room for the trampoline, or 0

The loader (our small piece inside the firmware file) checks a place's bytes against the sum before it changes
anything, so a unit with other firmware is left alone. Where an entry has a room, the loader copies the instructions
the change displaces from the running firmware
into the room and puts a jump back behind them; the function of ours ends its way into the firmware in a jump to the
room. So neither this tool, nor the profile, nor a table holds a byte of the firmware. No firmware file is needed to
run it. Standard library only.
"""

import argparse
import json
import os
import sys

FORMS = {"word": 0, "call": (1, 2), "jump": (3, 4), "load": 5}
L1_CODE = (0x11a00000, 0x11a10000)
DDR = (0x80000000, 0x90000000)
KIND_SUM = 0x10                         # an entry holds a checksum where older tables held the firmware's bytes
COPIES = (0, 8, 10, 12, 14)             # where the instructions under an 8-byte change can end
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def kind_word(form, address, copies):
    """The first word of an entry."""
    if form not in FORMS:
        raise ValueError("An entry is a word, a call, a jump or a load, not %r (0x%08x)" % (form, address))
    in_l1 = L1_CODE[0] <= address and address + 8 <= L1_CODE[1]
    in_ddr = DDR[0] <= address and address + (4 if form == "word" else 8) <= DDR[1]
    if form in ("call", "jump"):
        if not (in_l1 or in_ddr) or address % 2:
            raise ValueError("A call or a jump lies in L1 code or in DDR, on an even address: 0x%08x" % address)
        number = FORMS[form][0 if in_l1 else 1]
    else:
        if not in_ddr or address % (4 if form == "word" else 2):
            raise ValueError("A %s lies in DDR, on its alignment: 0x%08x" % (form, address))
        number = FORMS[form]
    if copies not in COPIES or (copies and form not in ("call", "jump")):
        raise ValueError("A trampoline goes with a call or a jump and copies 8, 10, 12 or 14 bytes (0x%08x)" % address)
    return KIND_SUM | number | (copies << 16)


def entries(profile, table):
    """A table's entries as (kind word, address, target, sum, room or None, what ours does there)."""
    places = {int(place["address"], 16): place for place in profile["patches"]}
    out = []
    for entry in table["entries"]:
        address = int(entry["address"], 16)
        if address not in places:
            raise ValueError("%s names 0x%08x, which is no place of the profile" % (table["file"], address))
        place = places[address]
        copies = int(entry.get("copies", 0))
        room = entry.get("room")
        if bool(room) != bool(copies):
            raise ValueError("A room and the bytes its trampoline copies go together (0x%08x)" % address)
        length = 4 if entry["form"] == "word" else 8
        if int(place["length"]) != length:
            raise ValueError("The place 0x%08x is %d bytes long and the entry changes %d" % (address, int(place["length"]), length))
        if copies > length and "sum" not in entry:
            raise ValueError("The entry at 0x%08x copies %d bytes and needs a sum of its own over them" % (address, copies))
        total = int(entry["sum"], 16) if "sum" in entry else int(place["sum"], 16)
        out.append((kind_word(entry["form"], address, copies), address, entry["target"], total, room, entry.get("does", "")))
    if not out:
        raise ValueError("%s has no entry" % table["file"])
    return out


def render(profile, table, profile_name):
    """The assembly source of one table."""
    symbol = table.get("symbol", "op1f_boot_table")
    whose = "the loader makes for itself" if table["part"] == "loader" else "the pack '%s' asks the loader to make" % table["part"]
    lines = [
        "/* Generated by tools/table_build.py from %s; do not edit by hand." % profile_name,
        " * The changes of the running firmware that %s at the start. One entry is five words: the kind" % whose,
        " * (0x10 and: 0 a data word, 1 a call in L1 code, 2 a call in DDR code, 3 a jump in L1 code, 4 a jump in DDR code,",
        " * 5 the load of an address in DDR code; in its high half the bytes its trampoline copies), the place, the function",
        " * of ours, the CRC-32 of the firmware's bytes the change replaces or its trampoline copies, and the room for the",
        " * trampoline or 0. No byte of the firmware stands in this table.",
        " */",
        ".section .rodata",
        ".balign 4",
        ".global %s" % symbol,
        ".type %s, STT_OBJECT" % symbol,
        "%s:" % symbol,
    ]
    for kind, address, target, total, room, does in entries(profile, table):
        text = does.replace("*/", "* /").replace("\n", " ")
        lines.append("    .long 0x%08x, 0x%08x, %s, 0x%08x, %s    /* %s */" % (kind, address, target, total, room or "0", text))
    lines += [
        ".size %s, .-%s" % (symbol, symbol),
        ".global %s_end" % symbol,
        ".type %s_end, STT_OBJECT" % symbol,
        "%s_end:" % symbol,
        "    .long 0xffffffff    /* never read: the loader stops at this address */",
        ".size %s_end, .-%s_end" % (symbol, symbol),
        "",
    ]
    return "\n".join(lines)


def code_lines(text):
    """A table's source without its comments and blank lines: what the assembler makes bytes of."""
    out, inside = [], False
    for line in text.split("\n"):
        kept = ""
        while line:
            if inside:
                end = line.find("*/")
                if end < 0:
                    line = ""
                else:
                    line, inside = line[end + 2:], False
            else:
                start = line.find("/*")
                if start < 0:
                    kept, line = kept + line, ""
                else:
                    kept, line, inside = kept + line[:start], line[start + 2:], True
        kept = " ".join(kept.split())
        if kept:
            out.append(kept)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--profile", required=True)
    ap.add_argument("--table", help="one table, by the file the profile names for it")
    ap.add_argument("--out", help="with --table: write there and not to the file the profile names")
    ap.add_argument("--all", action="store_true", help="write every table to its file")
    ap.add_argument("--check", action="store_true", help="write nothing: are the sources in the tree what the profile gives?")
    args = ap.parse_args(argv)
    if sum((bool(args.table), args.all, args.check)) != 1 or (args.out and not args.table):
        print("error: one of --table FILE, --all and --check; --out goes with --table")
        return 2
    try:
        with open(args.profile, encoding="utf-8") as f:
            profile = json.load(f)
        tables = profile["tables"]
        if args.table:
            tables = [table for table in tables if table["file"] == args.table.replace("\\", "/")]
            if not tables:
                raise ValueError("The profile has no table for %s" % args.table)
        name = args.profile.replace("\\", "/")
        stale = 0
        for table in tables:
            text = render(profile, table, name)
            path = args.out or os.path.join(ROOT, table["file"])
            if args.check:
                held = None
                if os.path.isfile(path):
                    with open(path, encoding="utf-8", newline="") as f:
                        held = f.read()
                if held != text:
                    stale += 1
                    print("%s: %s" % (table["file"], "not there" if held is None else "not what the profile gives"))
                continue
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
            made = sum(1 for entry in table["entries"] if entry.get("room"))
            print("wrote %s: %d entries, %d with a trampoline" % (path if args.out else table["file"], len(table["entries"]), made))
        if args.check:
            print("%d table(s): %s" % (len(tables), "all as the profile gives them" if not stale else "%d to write anew (--all)" % stale))
            return 1 if stale else 0
    except (OSError, ValueError, KeyError) as problem:
        print("error: %s" % problem)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())

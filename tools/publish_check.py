#!/usr/bin/env python3
"""Do files that would be published hold anything of the stock firmware? The check of docs/14, section 10, row G.

  python -I tools/publish_check.py PATH [PATH ...] [--list FILE] [--stock FOLDER] [--display FOLDER]
         [--profile FILE] [--allow FILE] [--show N] [--passed] [--deep] [--strict] [--forms-only] [--json OUT]

A PATH is a file or a folder (every file under it; a `.git` folder is passed over). `--list FILE` names more files,
one path a line. The answer is the exit code: 0 when nothing of the stock firmware was found in any of the files, 1
when something was, 2 when the check could not be made. Every finding says what was found and where.

What the files are held against, all of it on this computer and none of it ever published: the stock OS image and the
bootloader as folders of memory records (`--stock`, repeatable; without it `analysis/os-records` and
`analysis/boot-records`), the stock pictures (`--display`; without it `op-1field backup/content/display`), and the
places the firmware is changed at (`--profile`, repeatable; without it the profiles of `profiles/` that name such
places). A profile is read for addresses and lengths only: the bytes of a place are taken from the image. A profile
that names its stock image's size (and its record count, where it names one) is read in the folder of records whose
index names the same, so that one run with the records of several firmwares holds each profile's places to its own
firmware's bytes.

The rules. Each has a name, which stands in front of its findings, and its numbers are named below the imports.

 by its name or its first bytes, with nothing searched (`--deep` searches such a file all the same):
  name      a file that never goes: a firmware image or an update package (by its ending, or by the first bytes of
            either form), anything in a folder that holds a copy of a unit's storage, the official archives, files or
            logs read from a unit, the stock image as records, or its disassembly, and a log file by its name.

 a pack file, a binary, and every other file that is not text (the searches of tools/stock_search.py):
  table     the stock bytes a table of stock changes keeps to check its places (an entry of the new kind holds a
            checksum there and is not counted);
  site      the stock bytes of a place the firmware is changed at, anywhere else in the file;
  bytes     RUN bytes or more in a row that stand in a stock image and are not text (small numbers among zeros and a
            row that counts by one step say nothing and are passed over, here and under `hex`);
  text      a stock image's texts in bulk: a row of ROW strings or more in the image's order and layout, or one file
            that quotes BULK_TEXTS different phrases of the image, or BULK_BYTES bytes of them (one phrase quoted is
            let through and counted: `quoted` below);
  picture   what a picture has of the stock pictures (tools/picture_provenance.py): a pack's pictures as the unit
            makes them, with only the bytes the pack itself carries counted; a picture carried whole, and every
            stretch from `<svg` to `</svg>` of any file, in every byte.

 a text file, besides `text` and `picture` as above:
  hex       HEX bytes or more of a stock image written as hex: a long string of digits, `\\x` escapes, or a row of
            numbers of one width (two, four or eight digits; words are tried in both byte orders). Fewer than RUN
            bytes of which HEX stand at MANY places of the image are a habit of the instruction set and not counted;
  listing   lines of a disassembly of stock code: an address, the bytes and an instruction, where the bytes are the
            image's own at that address; or an address at which the stock firmware has code, a colon and an
            instruction. A listing of code of ours is searched as the bytes it shows (`site`, `bytes`);
  quote     QUOTE instructions or more on one line that also names an address at which the stock firmware has code.
            In an assembly source only the comments are read: the code is ours;
  log       a file of which half the lines or more have the form of the unit's own log.

 found, counted and let through, each for a reason that is written beside its number below:
  names           one name the firmware itself uses (a file, a key, a picture's node), however long;
  quoted          a phrase of the image's, quoted: a message, a line the unit shows (PHRASE bytes or more with white
                  space in them that are a whole string of the image, or LONG bytes or more wherever they lie in one).
                  What a screen or a log says is no secret, and a guide that quotes it is clearer than one that talks
                  round it. A file is held to a bound all the same, since many quotes make a copy: `text` above;
  pieces          a piece of one of the image's strings, with white space in it, of PHRASE bytes or more and fewer than
                  LONG: words two English texts share, or a few words of a message quoted;
  addresses       a row of addresses of the stock image;
  licence         the wording of a public licence (MIT, BSD), which the bootloader carries for the libraries it is built
                  with: told by where the run lies in the image, from such a licence's first words to the end of the
                  string that holds it (te-boot keeps all its notices in one string);
  one instruction one instruction on a line that names a stock code address: a description, not a listing;
  log lines       lines in the form of the unit's log in a file that is mostly something else;
  words           long stretches a stock picture has that are only the wording of the picture format;
  allowed         a text that stands in the `--allow` file, one text a line: read by a person and accepted;
  no pack         a file that begins as a pack file and cannot be read as one: it is searched as a plain binary.

 not judged: a picture of pixels (.png, .gif, .jpg). No rule here reads pixels; a person looks at it. With `--strict`
 a file that was not judged counts as a finding.

What the check cannot see, so that a 0 is not read for more than it says:
  - a routine of ours that follows a stock routine with other registers or in another language: the bytes differ.
    In a built pack a close copy still shows as `bytes`; a function written in C++ from a reading of stock's shows
    nowhere (analysis/pack_clean_study.md, section 6);
  - instructions quoted with no address beside them, and stock instructions written as code of ours in an assembly
    source (a stub that replays what its hook displaced): the built pack shows those, as `site`;
  - numbers of a stock picture kept as numbers in a tool's source, and a page of ours laid out like a stock page;
  - anything of another OS version than the images it was given.

`--forms-only` makes the part of the check that needs no stock image (name, log, listing and quote by their form) and
says so; its 0 is not the whole check's. Reads only. `--json` writes one file and refuses to overwrite it. Standard
library only.
"""

import argparse
import bisect
import hashlib
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack_build  # noqa: E402
import picture_make  # noqa: E402
import picture_provenance as provenance  # noqa: E402
import stock_search as ss  # noqa: E402

# ---------------------------------------------------------------- the rules' numbers

RUN = 16                    # a shared run of this many bytes is looked at: below it a run proves nothing by its length
                            # (analysis/pack_clean_study.md, section 3: compiled code of ours shares runs of up to 14)
HEX = ss.GRAM               # 8: the smallest thing known to be stock's is a place of change, eight bytes; hex is written
                            # on purpose, so what chance brings into compiled code does not come into it
MANY = 16                   # a run of fewer than RUN bytes written as hex is a habit of the instruction set (a row of
                            # register saves), not a copy of one place, when HEX bytes of it stand at this many places of the
                            # image or more
SMALL = 4                   # a run in which no more than one byte in this many is other than 0x00 or 0xff is small numbers
                            # among zeros (a row of words that each fit a byte), whatever its length
QUOTE = 2                   # one instruction names an operation; two in a row begin to be a listing
LOG_LINES = 3               # this many lines in the form of the unit's log, and
LOG_SHARE = 2               # at least one line in this many, make a file a log read from the unit
PHRASE = 16                 # a shared text with white space inside it, this long without the white space at its ends, is a
                            # phrase of the image's (a message, a line the unit shows) when it is a whole string of the image
LONG = 24                   # from this length on a phrase counts wherever it lies in one of the image's strings; a shorter
                            # piece of a string is words two English texts share ("the second half of"), or a few words quoted
PAD = 8                     # white space that may stand between a string's own letters and the zero byte at its edge
ROW = 2                     # this many strings that stand whole in a shared run, one behind the other as the image has
                            # them, with RUN bytes of letters between them, are a row of the image's names
BULK_TEXTS = 64             # one file that quotes this many different phrases of the image, or
BULK_BYTES = 4096           # this many bytes of them, holds the image's texts in bulk: a list of its strings, not a quote.
                            # A guide quotes a dozen lines of a screen and a page on a method some dozens of messages
LICENCES = ((b"Permission is hereby granted", b"DEALINGS IN THE"),       # the MIT licence, from its first words to its last
            (b"Redistribution and use in source and binary forms", b"POSSIBILITY OF SUCH DAMAGE"))      # the BSD licences
LICENCE_SPAN = 2048         # a licence's last words follow its first within this many bytes
KEEP = 40                   # findings kept for one rule in one file; the count goes on
SHOW = 6                    # findings printed for one file unless --show says otherwise
SNIFF = 8192                # a file is text when its first bytes hold no zero byte

TFW_MAGIC = b"\xba\xbe\xca\xfe"
LDR_SIGN = 0xad             # the last byte of every record header of a boot stream; the sixteen bytes XOR to zero
CODE = ss.CODE + ((0x889bb628, 0x889f500c, "te-boot"),)      # where OS 1.7.0 and te-boot 1000.1.1 have their code

NEVER_FOLDERS = {
    "op-1field backup": "a copy of a unit's storage",
    "second copy of firmware folder": "a copy of a unit's firmware folder",
    "firmware": "the official update archives",
    "device-files": "files read from a unit",
    "device-logs": "logs read from a unit",
    "os-records": "the stock OS image as memory records",
    "boot-records": "the bootloader's image as memory records",
    "records": "the stock OS image as memory records",
    "records_bootloader": "the bootloader's image as memory records",
    "os-disasm": "a disassembly of the stock OS image",
    "os-disasm-full": "a disassembly of the stock OS image",
    "boot-disasm": "a disassembly of the bootloader",
    "disasm": "a disassembly of the stock OS image",
}
NEVER_ENDINGS = {".ldr": "a firmware image", ".tfw": "a firmware update package"}
NEVER_NAMES = ((re.compile(r"^log_\d{4}-\d{2}-\d{2}_\d{2}\.\d{2}\.\d{2}_\d+\.txt$"), "a log read from a unit"),)

ASSEMBLY_ENDINGS = {".s", ".asm"}
PIXEL_ENDINGS = {".png", ".gif", ".jpg", ".jpeg", ".webp", ".bmp", ".ico"}
PIXEL_MAGICS = (b"\x89PNG\r\n\x1a\n", b"GIF87a", b"GIF89a", b"\xff\xd8\xff")

REFUSING = ("name", "log", "table", "site", "bytes", "text", "picture", "hex", "listing", "quote")
NOTES = ("names", "quoted", "pieces", "addresses", "licence", "one instruction", "log lines", "words", "allowed", "no pack")

LOG_LINE = re.compile(rb"^\s*\d+\.\d{3}\.\d{3}: [IWEF] \S+\[\d+\]: ")
SVG_OPEN = re.compile(rb"<svg\b")
PATH_VALUE = re.compile(rb'\sd="([^"]*)"')
PATH_TOKEN = re.compile(rb"[A-Za-z]|-?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")

# an instruction of the processor as a note writes it: a register or a memory place takes a value, or a word of the
# instruction set with what it works on
REG = r"(?:R[0-7]|P[0-5]|SP|FP|I[0-3]|M[0-3]|B[0-3]|L[0-3]|A[01]|CC|RETS|RETI|RETX|LC[01]|LT[01]|LB[01]|ASTAT|CYCLES2?|USP)(?:\.[LHBW])?"
MEM = r"(?:[BW]\s*)?\[\s*(?:--\s*)?(?:%s|0x[0-9A-Fa-f]+)[^\]\n]{0,30}\]" % REG
ASSIGN = re.compile(r"(?<![A-Za-z0-9_.\]])(?:%s|%s)\s*(?:=|\+=|-=|\*=|>>>?=|<<=|\|=|&=|\^=)(?!=)\s*[^;,`\n]{1,60}" % (REG, MEM))
WORD = re.compile(r"(?<![A-Za-z0-9_.])(?:LINK\s+(?:0x)?[0-9A-Fa-f]+|UNLINK|RTS|RTI|RTX|CSYNC|SSYNC"
                  r"|CALL(?:\.a)?\s*\(?\s*(?:0x[0-9A-Fa-f]+|%s\s*\))|JUMP(?:\.[aSL])?\s*\(?\s*(?:0x[0-9A-Fa-f]+|%s\s*\))"
                  r"|IF\s+!?CC\s+(?:JUMP|R[0-7]|P[0-5])|LSETUP\s*\(|CLI\s+R[0-7]|STI\s+R[0-7]|I?FLUSH\s*\[|BIT(?:SET|CLR|TGL|TST)\s*\()" % (REG, REG))
ADDRESS = re.compile(r"(?<![0-9A-Za-z_])0x([0-9A-Fa-f]{8})(?![0-9A-Za-z_])")
WITH_BYTES = re.compile(r"^\s*(?:0x)?([0-9A-Fa-f]{6,8}):\s+((?:[0-9A-Fa-f]{2}[ \t])+)\s*(.*)$")
LED = re.compile(r"^[\s>*`|-]*(?:0x)?([0-9A-Fa-f]{8})`?:\s+(.*)$")

ESCAPED = re.compile(r"(?:\\x[0-9A-Fa-f]{2}){%d,}" % HEX)
DIGITS = re.compile(r"(?<![0-9A-Za-z_])(?:0[xX])?((?:[0-9A-Fa-f]{2}){%d,})(?![0-9A-Za-z_])" % HEX)
TOKEN = re.compile(r"(?<![0-9A-Za-z_\\])(0[xX])?([0-9A-Fa-f]{8}|[0-9A-Fa-f]{4}|[0-9A-Fa-f]{2})(?![0-9A-Za-z_])")
GAP = re.compile(r"(?:[\s,;]|\.(?:long|word|short|byte|int|hword|[24]byte|d[bwd])\b)*")
LETTER = re.compile(r"[A-Fa-f]")


# ---------------------------------------------------------------- what the files are held against


class Images:
    """The stock images: one index for the search of runs, and each image's memory by address."""

    def __init__(self, sources):
        regions, self.memories, self.identities = [], [], []
        for source in sources:
            if os.path.isdir(source):
                more, pieces = ss.load_records(source)
                regions += more
                pieces = sorted(pieces, key=lambda piece: piece[0])
                self.memories.append((pieces, [piece[0] for piece in pieces]))
                self.identities.append(identity_of(source))
            else:
                with open(source, "rb") as f:
                    regions.append((os.path.basename(source), 0, f.read()))
        self.sources = list(sources)
        self.stock = ss.Stock(regions)
        self.public = []                # [(start, end)]: where an image carries the wording of a public licence
        for _name, address, data in regions:
            for first, last in LICENCES:
                at = data.find(first)
                while at >= 0:
                    end = data.find(last, at, at + LICENCE_SPAN)
                    if end >= 0:
                        stop = data.find(b"\0", end)
                        self.public.append((address + at, address + (stop if stop >= 0 else len(data))))
                    at = data.find(first, at + 1)

    def of(self, identity):
        """The numbers of the images whose index names `identity`, (size of the image, count of its records); a count
        of None matches any count."""
        if not identity or identity[0] is None:
            return []
        return [number for number, held in enumerate(self.identities)
                if held is not None and held[0] == identity[0] and identity[1] in (None, held[1])]

    def read(self, address, count, stored=True, among=None):
        """What each image holds at `address`: [bytes], one for every image that holds all `count` bytes there (in
        stored records only, unless `stored` is false: then a fill record's bytes count too). `among`, when given, is
        the numbers of the images read; the others are passed over."""
        out = []
        for which, (pieces, starts) in enumerate(self.memories):
            if among is not None and which not in among:
                continue
            data, at, left = b"", address, count
            while left:
                number = bisect.bisect_right(starts, at) - 1
                if number < 0:
                    break
                start, held, size, word = pieces[number]
                if at >= start + size or (held is None and stored):
                    break
                take = min(left, start + size - at)
                data += held[at - start:at - start + take] if held is not None else bytes(word[(at - start + k) & 3] for k in range(take))
                at, left = at + take, left - take
            if not left:
                out.append(data)
        return out

    def holds(self, address):
        """Whether `address` lies in the memory an image fills (its stored records and its fill)."""
        for pieces, starts in self.memories:
            number = bisect.bisect_right(starts, address) - 1
            if number >= 0 and address < pieces[number][0] + pieces[number][2]:
                return True
        return False

    def places(self, run):
        return self.stock.places(run)

    def licence(self, run):
        """Whether a run is the wording of a public licence: every place of it lies in a stretch where an image
        carries such a licence for a part of its own (te-boot has those of the libraries it is built with)."""
        places = self.places(run)
        return bool(places) and all(any(start <= address and address + len(run) <= end for start, end in self.public) for address, _name in places)


def read_places(profiles, images):
    """The places the firmware is changed at, in the form tools/stock_search.py takes them: ([addresses], [(address,
    the image's own bytes there, group)]). A profile is read for the address and the length; its "before" bytes are
    used only where no image holds the place. A profile that names its stock image's size (and its record count,
    where it names one) is read only in the images whose index names the same, when one does: the same address holds
    other bytes in another firmware. A profile that names no size, or that no image here matches, is read in the first
    image that holds the place."""
    found = {}
    for path in profiles:
        with open(path, encoding="utf-8") as f:
            profile = json.load(f)
        identity = (profile.get("stock_bytes"), profile.get("stock_records")) if isinstance(profile, dict) else None
        among = images.of(identity)
        for patch in profile.get("patches", ()):
            if "address" not in patch:
                continue
            address = int(patch["address"], 16)
            length = int(patch.get("length") or len(patch.get("before", "")) // 2)
            held = images.read(address, length, among=among or None) if length else []
            data = held[0] if held else bytes.fromhex(patch.get("before", ""))
            if data:
                found.setdefault((address, data), (address, data, patch.get("group", "")))
    places = sorted(found.values())
    return sorted({place[0] for place in places}), places


def identity_of(source):
    """(size of the image, count of its records) as a folder of records names them in its index, or None."""
    index = os.path.join(source, "index.json")
    try:
        with open(index, encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return None
    if not isinstance(data, dict):
        return None
    records = data.get("records")
    return (data.get("bytes"), len(records) if isinstance(records, list) else records)


def of_these_images(profile_path, sources):
    """Whether a profile of the default list is of a stock image held here (since the port to 1.7.3, 2026-11-19, the
    profiles folder holds a profile of each firmware): its stock size and record count are those of one folder of
    records, as that folder's index names them. A profile that names neither, or a folder with no index, is kept. A
    place is read at its address in the images held, so another firmware's profile would read other bytes."""
    try:
        with open(profile_path, encoding="utf-8") as f:
            profile = json.load(f)
    except (OSError, ValueError):
        return True
    if not isinstance(profile, dict) or "stock_bytes" not in profile or "stock_records" not in profile:
        return True
    held = []
    for source in sources:
        index = os.path.join(source, "index.json")
        if not os.path.isfile(index):
            return True
        with open(index, encoding="utf-8") as f:
            data = json.load(f)
        records = data.get("records")
        held.append((data.get("bytes"), len(records) if isinstance(records, list) else records))
    return (profile["stock_bytes"], profile["stock_records"]) in held


def read_allowed(path):
    with open(path, encoding="utf-8") as f:
        return {line.rstrip("\r\n") for line in f if line.strip() and not line.startswith("#")}


def stock_code(address):
    return any(start <= address < end for start, end, _name in CODE)


# ---------------------------------------------------------------- one file's findings


class Findings:
    """What one file holds: findings by rule (the first KEEP of each kept, all counted), and what was let through."""

    def __init__(self):
        self.kept, self.counts, self.notes, self.unjudged = [], {}, {}, []
        self.quoted = {}                # {a phrase of the image's that the file quotes: its length}, for the bound on quotes

    def add(self, rule, what, line=None, offset=None, count=1):
        self.counts[rule] = self.counts.get(rule, 0) + count
        if sum(1 for item in self.kept if item["rule"] == rule) < KEEP:
            item = {"rule": rule, "what": what}
            if line is not None:
                item["line"] = line
            if offset is not None:
                item["offset"] = offset
            self.kept.append(item)

    def note(self, rule, what=None, count=1):
        held = self.notes.setdefault(rule, {"count": 0, "samples": []})
        held["count"] += count
        if what is not None and what not in held["samples"] and len(held["samples"]) < SHOW:
            held["samples"].append(what)


def shown(run):
    """A run for a person to read: its text when it is text, else its first bytes as hex."""
    if ss.printable(run):
        return repr(run.decode("ascii").replace("\0", "\\0"))
    return run[:24].hex() + ("..." if len(run) > 24 else "")


def addresses_only(run, images):
    """Whether a run is a row of addresses: at the alignment of its place in the image, every whole word of it (two at
    least) is an address inside an image's memory."""
    places = images.places(run)
    if not places:
        return False
    skip = -places[0][0] % 4
    words = [int.from_bytes(run[at:at + 4], "little") for at in range(skip, len(run) - 3, 4)]
    return len(words) >= 2 and all(images.holds(word) for word in words)


def sort_text(run, allowed, begins=True, ends=True):
    """What a shared run of text is: (the rule it is a finding of or None, the note it is let through under or None,
    the text to show). Zero bytes are what stands between two strings in a binary, so the run is read piece by piece.
    `begins` and `ends` say whether the run begins where a string of the image begins and ends where one ends."""
    pieces = run.split(b"\0")
    texts = [piece.strip(b" \t\r\n") for piece in pieces]
    if any(text.decode("ascii") in allowed for text in texts if text):
        return None, "allowed", max(texts, key=len)
    standing = [piece for piece in pieces[1:-1] if piece]  # the strings that stand whole in the run, between zero bytes
    if len(standing) >= ROW and sum(len(piece) for piece in standing) >= RUN:
        return "text", None, run.strip(b"\0")              # a row of the image's strings, in its order and layout: a
    short = None                                            # piece of its memory, whatever the strings say
    for number, text in enumerate(texts):
        if len(text) < PHRASE or not any(space in text for space in b" \t\r\n"):
            continue
        whole = (number > 0 or begins) and (number < len(pieces) - 1 or ends)
        if whole or len(text) >= LONG:
            return None, "quoted", text                     # a phrase: a message, a line the unit shows, quoted
        short = text
    for text in texts:
        if len(text) >= RUN and not any(space in text for space in b" \t\r\n"):
            return None, "names", text                      # one name the firmware itself uses
    if short:
        return None, "pieces", short                        # a few words from one of the image's strings
    return None, None, None                                 # short strings that meet by their padding


def string_edges(run, images):
    """(whether the run begins where a string of an image begins, whether it ends where one ends): at one of its
    places a zero byte, or the image's own edge, stands before it and behind it, with at most PAD bytes of white space
    between."""
    def edge(address, step):
        for _ in range(PAD):
            held = images.read(address, 1, stored=False)
            if not held or b"\0" in held:
                return True
            if not any(byte in b" \t\r\n" for byte in held):
                return False
            address += step
        return False

    begins = ends = False
    for address, _name in images.places(run)[:MANY]:
        begins = begins or edge(address - 1, -1)
        ends = ends or edge(address + len(run), 1)
    return begins, ends


def counting(run):
    """Whether a run is a row that counts by one step, in bytes, half words or words: each byte is the byte one word
    before it plus a step that is the same along the row (a block map's 3, 4, 5 ..; a table of indexes)."""
    for width in (1, 2, 4):
        if len(run) >= 3 * width and all(len({(run[at + width] - run[at]) & 0xff for at in range(phase, len(run) - width, width)}) == 1
                                         for phase in range(width)):
            return True
    return False


def sort_run(run, images, allowed):
    """What a shared run is: (the rule it is a finding of or None, the note it is let through under or None, what to
    show of it)."""
    telling = sum(1 for c in run if c not in (0, 0xff))
    if telling < ss.PLAIN or telling * SMALL <= len(run):
        return None, None, None                             # small numbers among zeros: counts, flags, a row of little words
    if counting(run):
        return None, None, None                             # a row that counts by one step (00 01 02 ..): any table of indexes
    if ss.printable(run):
        if images.licence(run.strip(b" \t\r\n")):             # the empty lines around it are not the licence's
            return None, "licence", run.strip()
        return sort_text(run, allowed, *string_edges(run, images))
    if addresses_only(run, images):
        return None, "addresses", run
    if len(run) < RUN and any(len(images.places(run[at:at + HEX])) >= MANY for at in range(len(run) - HEX + 1)):
        return None, None, None                             # a habit of the instruction set, in hex of our own code: the
    return "bytes", None, run                               # byte or two that follow it by chance make it no copy


def let_through(found, note, what):
    """A run that is let through and counted. A quoted phrase is kept by itself besides, for the bound on what one
    file quotes (`bulk`)."""
    found.note(note, shown(what))
    if note == "quoted":
        found.quoted[bytes(what)] = len(what)


def bulk(found):
    """The bound on what one file quotes of the image's texts. A phrase quoted is a quote. BULK_TEXTS different phrases
    in one file, or BULK_BYTES bytes of them, are a list of the image's strings: a copy of a part of it."""
    count, size = len(found.quoted), sum(found.quoted.values())
    if count >= BULK_TEXTS or size >= BULK_BYTES:
        first = "; ".join(shown(text) for text in sorted(found.quoted)[:3])
        found.add("text", "%d different phrases of the image, %d bytes of them: its texts in bulk, where %d phrases or %d bytes are the bound for one file. %s"
                  % (count, size, BULK_TEXTS, BULK_BYTES, first), count=count)


def stock_places(run, images):
    places = images.places(run)
    first = ", ".join("0x%08x" % address for address, _name in places[:3])
    return "at %s%s in the image (%d place%s)" % (first, ", ..." if len(places) > 3 else "", len(places), "" if len(places) == 1 else "s")


def same_bytes(places):
    """{the stock bytes of a place of change: the places that hold them, as text}, for places of GRAM bytes or more (a
    shorter one is one address). Several places may hold the same bytes: three loads of one address, two prologues."""
    out = {}
    for site, before, group in places[1] if places else ():
        if len(before) >= ss.GRAM:
            out.setdefault(before, []).append("0x%08x (%s)" % (site, group or "no group"))
    return {before: ", ".join(sites[:3]) + (" and %d more" % (len(sites) - 3) if len(sites) > 3 else "") for before, sites in out.items()}


def search_bytes(found, data, images, places, allowed, where, least=RUN, rule=None, counted=()):
    """The search of a stretch of bytes that is not part of a pack: the bytes of a place of change, and shared runs of
    `least` bytes or more. `where(offset)` says where a byte of the stretch stands, for the finding; `counted` are
    stretches [(start, end)] that another rule has counted already."""
    taken = list(counted)
    for before, sites in same_bytes(places).items():
        at = data.find(before)
        while at >= 0:
            if not any(start <= at and at + len(before) <= end for start, end in counted):
                line, offset = where(at)
                found.add("site", "the %d stock bytes of the place of change %s: %s" % (len(before), sites, before.hex()), line, offset)
                taken.append((at, at + len(before)))
            at = data.find(before, at + 1)
    for offset, length in ss.runs_in(images.stock, data, 0, len(data), least):
        if any(start <= offset and offset + length <= end for start, end in taken):
            continue
        run = data[offset:offset + length]
        kind, note, what = sort_run(run, images, allowed)
        if note:
            let_through(found, note, what)
        elif kind:
            line, at = where(offset)
            found.add(rule or kind, "%d bytes, %s: %s" % (length, stock_places(run, images), shown(what)), line, at)


def path_data(value):
    """Whether a `d` attribute is path data the measure of pictures can read: it begins with a command, its letters are
    commands, and no number follows a close. A tool's template or a test's broken path is not. The measure's own
    reader refuses such data since 2026-11-16 (until then it did not come back from a number behind a close, which is
    why a path is looked at here first); the look stays, and a picture with such a path is not judged."""
    tokens = PATH_TOKEN.findall(value)
    if not tokens or tokens[0] not in (b"M", b"m"):
        return False
    closed = False
    for token in tokens:
        if token.isalpha():
            if token not in b"MmLlHhVvCcSsQqTtAaZz":
                return False
            closed = token in b"Zz"
        elif closed:
            return False
    return True


def scaled_loose(display, text):
    """A stock shape at another size or place in a text that is no picture: each `d` and `points` by itself, since
    one that is no path data (a tool's template) must not end the look at the others."""
    out = []
    for found in provenance.SHAPE_VALUE.finditer(text):
        if found.group()[1:2] == b"d" and not path_data(found.group(1)):
            continue
        try:
            out.extend((found.start() + start, found.start() + end, source) for start, end, source in display.scaled_shapes(found.group()))
        except Exception:
            continue
    return out


def svg_regions(data):
    """[(start, end)] of every stretch from `<svg` to the end of the next `</svg>` tag."""
    out, at = [], 0
    while True:
        first = SVG_OPEN.search(data, at)
        last = data.find(b"</svg", first.end()) if first else -1
        close = data.find(b">", last) if last >= 0 else -1
        if close < 0:
            return out
        out.append((first.start(), close + 1))
        at = close + 1


def judge_pictures(found, pictures, display, label="the picture", templates=False):
    """`pictures` is [(name, the picture as the unit makes it, a mask of the bytes the file itself carries)]. The
    answer is the names of those that could not be read as a picture: with `templates` that is no fault (a stretch from
    `<svg` to `</svg>` in a tool's source need not be a picture), otherwise the picture is not judged and the file says so."""
    if not pictures:
        return []
    if display is None:
        found.unjudged.append("%d picture(s): no folder of stock pictures to hold them against" % len(pictures))
        return []
    readable, unread = [], {}
    for picture in pictures:
        if all(path_data(found.group(1)) for found in PATH_VALUE.finditer(picture[1])):
            readable.append(picture)
        else:
            unread[picture[0]] = "a path in it is no path data"
    try:
        results = provenance.judge(display, readable) if readable else []
    except Exception as problem:
        results = []
        unread.update((picture[0], problem) for picture in readable)
    if not templates:
        found.unjudged.extend("%s %s cannot be read as a picture: %s" % (label, name, problem) for name, problem in unread.items())
    for name, counts, findings in results:
        if counts[provenance.WORDS]:
            found.note("words", count=counts[provenance.WORDS])
        if not counts["all"]:
            continue
        parts = ", ".join("%d in %s" % (counts[kind], kind) for kind in provenance.KINDS if counts[kind])
        first = "; ".join("%s of %s at %d: %s" % (kind, source, start, text[:60].decode("latin-1").replace("\n", " "))
                          for kind, start, _end, source, text in findings[:3])
        found.add("picture", "%s %s carries %d bytes of a stock picture (%s). %s" % (label, name, counts["all"], parts, first), count=counts["all"])
    return unread


def judge_pack(found, path, data, images, places, allowed, display):
    try:
        target = ss.Target(path)
    except Exception as problem:                            # a pack that does not hold together (a damaged one, kept for a test)
        found.note("no pack", "it begins as a pack file and cannot be read as one (%s): searched as a plain binary" % problem)
        return judge_binary(found, path, data, images, places, allowed, display)
    stock_words = []
    for name, number, at, _kind, site, before in target.entries():
        if before and before in images.read(site, len(before), stored=False):
            stock_words.append((at + 12, at + 12 + len(before)))
            found.add("table", "%s entry %d holds the %d stock bytes of its place 0x%08x: %s" % (name, number, len(before), site, before.hex()),
                      offset=at + 12, count=len(before))
    copies = {}
    for at, site, size, group in ss.site_copies(target, places):
        copies.setdefault((at, size), []).append("0x%08x (%s)" % (site, group or "no group"))
    for (at, size), sites in sorted(copies.items()):
        found.add("site", "the %d stock bytes of the place of change %s, in the %s: %s"
                  % (size, ", ".join(sites[:3]), target.where(at, size)["part"], data[at:at + size].hex()), offset=at)
        stock_words.append((at, at + size))
    for start, end, part in target.parts:
        for offset, length in ss.runs_in(images.stock, data, start, end, RUN):
            if any(first <= offset and offset + length <= last for first, last in stock_words):
                continue
            if target.where(offset, length)["kind"] == "table, the stock words":
                continue                                    # counted with its entry
            run = data[offset:offset + length]
            kind, note, what = sort_run(run, images, allowed)
            if note:
                let_through(found, note, what)
            elif kind:
                found.add(kind, "%d bytes in the %s, %s: %s" % (length, part, stock_places(run, images), shown(what)), offset=offset)
    pictures = []
    if target.fields is not None and data[8:12] == pack_build.FORMAT_LINKED.to_bytes(4, "little"):
        for name, text in pack_build.pictures_in(data):
            if display is None:
                pictures.append((name, text, bytearray(b"\x01" * len(text))))
                continue
            try:
                pictures.append((name,) + provenance.walk(text, display.read))
            except Exception as problem:                    # a set the unit would refuse, or a stock picture not here
                found.unjudged.append("the picture %s cannot be made here: %s" % (name, problem))
    for start, end, part in target.parts:                   # and a picture that stands whole in its body, as a pack of
        if not part.startswith("picture "):                 # before the pictures had a part of their own carried them
            pictures.extend(("in the %s at byte %d" % (part, start + first), data[start + first:start + last], bytearray(b"\x01" * (last - first)))
                            for first, last in svg_regions(data[start:end]))
    judge_pictures(found, pictures, display)


def judge_binary(found, path, data, images, places, allowed, display):
    """A binary that is no pack: the bytes of a place of change, shared runs, and pictures that stand in it whole."""
    counted = []
    if os.path.isfile(os.path.splitext(path)[0] + ".elf") and not path.lower().endswith(".elf"):
        try:                                                # a base or a module beside its link: its tables are known
            entries = ss.Target(path, os.path.splitext(path)[0]).entries()
        except Exception:                                   # not a link of ours: the file is searched as it is
            entries = []
        for name, number, at, _kind, site, before in entries:
            if before and before in images.read(site, len(before), stored=False):
                counted.append((at + 12, at + 12 + len(before)))
                found.add("table", "%s entry %d holds the %d stock bytes of its place 0x%08x: %s" % (name, number, len(before), site, before.hex()),
                          offset=at + 12, count=len(before))
    search_bytes(found, data, images, places, allowed, lambda at: (None, at), counted=counted)
    judge_pictures(found, [("at byte %d" % start, data[start:end], bytearray(b"\x01" * (end - start))) for start, end in svg_regions(data)], display)


def comments_only(lines):
    """An assembly source's lines with only what stands in comments (`/* */` and `//`): the code is ours."""
    out, inside = [], False
    for line in lines:
        kept, at = [], 0
        while at < len(line):
            if inside:
                end = line.find("*/", at)
                kept.append(line[at:] if end < 0 else line[at:end])
                if end < 0:
                    break
                at, inside = end + 2, False
            else:
                block, rest = line.find("/*", at), line.find("//", at)
                if block < 0 and rest < 0:
                    break
                if rest >= 0 and (block < 0 or rest < block):
                    kept.append(line[rest + 2:])
                    break
                at, inside = block + 2, True
        out.append(" ".join(kept))
    return out


def statements(text):
    """How many instructions a stretch of text writes out."""
    spans = sorted([found.span() for found in ASSIGN.finditer(text)] + [found.span() for found in WORD.finditer(text)])
    count, edge = 0, -1
    for start, end in spans:
        if start >= edge:
            count, edge = count + 1, end
    return count


def hex_written(text):
    """Every stretch of bytes a text writes as hex: [(the bytes, the offset in the text of each byte's digits)]."""
    out = []
    for found in ESCAPED.finditer(text):
        digits = found.group()
        out.append((bytes.fromhex(digits.replace("\\x", "")), [found.start() + 4 * k for k in range(len(digits) // 4)]))
    for found in DIGITS.finditer(text):
        digits = found.group(1)
        out.append((bytes.fromhex(digits), [found.start(1) + 2 * k for k in range(len(digits) // 2)]))
    group, width, edge = [], 0, 0

    def close():
        if not group or len(group) * width // 2 < HEX or not any(prefix or LETTER.search(digits) for prefix, digits, _at in group):
            return
        if width == 2:
            out.append((bytes.fromhex("".join(digits for _p, digits, _at in group)), [at for _p, _d, at in group]))
            return
        size = width // 2
        starts = [at for _p, _d, at in group for _k in range(size)]
        out.append((b"".join(int(digits, 16).to_bytes(size, "little") for _p, digits, _at in group), starts))
        out.append((b"".join(int(digits, 16).to_bytes(size, "big") for _p, digits, _at in group), starts))

    for found in TOKEN.finditer(text):
        digits = found.group(2)
        if group and (len(digits) != width or not GAP.fullmatch(text, edge, found.start())):
            close()
            group = []
        group.append((found.group(1), digits, found.start(2)))
        width, edge = len(digits), found.end()
    close()
    return out


def judge_text(found, path, data, images, places, allowed, display):
    text = data.decode("utf-8", "replace")
    lines = text.split("\n")
    logs = sum(1 for line in data.split(b"\n") if LOG_LINE.match(line))
    if logs >= LOG_LINES and logs * LOG_SHARE >= len(lines):
        found.add("log", "%d of its %d lines have the form of the unit's own log" % (logs, len(lines)), count=logs)
        return
    if logs:
        found.note("log lines", count=logs)
    starts, at = [], 0
    for line in lines:
        starts.append(at)
        at += len(line) + 1

    def line_of(offset):
        return bisect.bisect_right(starts, offset), offset

    # listing lines with their bytes: the image's own at that address, or a listing of code of ours
    read = comments_only(lines) if os.path.splitext(path)[1].lower() in ASSEMBLY_ENDINGS else list(lines)
    listed, ours, quoted = 0, [], set()
    first = last = None
    for number, line in enumerate(lines):
        match = WITH_BYTES.match(line)
        if not match:
            continue
        address, held = int(match.group(1), 16), bytes.fromhex(match.group(2).replace(" ", "").replace("\t", ""))
        quoted.add(number)
        stock = held in images.read(address, len(held)) if images else stock_code(address)
        if stock:
            listed += 1
            first = number + 1 if first is None else first
            last = number + 1
            if listed <= 3:
                found.add("listing", "a line of a disassembly of stock code: %s" % line.strip()[:110], number + 1, count=0)
        else:
            if ours and ours[-1][1] + len(ours[-1][2]) == address:
                ours[-1][2] += held
            else:
                ours.append([number + 1, address, bytearray(held)])
    if listed:
        found.add("listing", "%d lines of a disassembly of stock code with their bytes, lines %d to %d" % (listed, first, last), first, count=listed)
    if images:
        for number, address, held in ours:
            search_bytes(found, bytes(held), images, places, allowed, lambda at, number=number: (number, None))
    # an address at which stock has code, then an instruction; or instructions beside such an address
    for number, line in enumerate(read):
        if number in quoted or not line:
            continue
        match = LED.match(line)
        if match and stock_code(int(match.group(1), 16)) and statements(match.group(2)):
            found.add("listing", "a line of a disassembly of stock code: %s" % line.strip()[:110], number + 1)
            continue
        if not any(stock_code(int(address.group(1), 16)) for address in ADDRESS.finditer(line)):
            continue
        count = statements(line)
        if count >= QUOTE:
            found.add("quote", "%d instructions beside a stock code address: %s" % (count, line.strip()[:110]), number + 1)
        elif count:
            found.note("one instruction", line.strip()[:110])
    if images:
        # stock bytes written as hex
        rest = "\n".join("" if number in quoted else line for number, line in enumerate(lines))
        for held, offsets in hex_written(rest):
            search_bytes(found, held, images, places, allowed, lambda at, offsets=offsets: line_of(offsets[at]), HEX, "hex")
        # the image's own text, as it stands in the file
        kept = [b"" if number in quoted else line for number, line in enumerate(data.split(b"\n"))]
        raw, begins, at = b"\n".join(kept), [], 0
        for line in kept:
            begins.append(at)
            at += len(line) + 1
        for offset, length in ss.runs_in(images.stock, raw, 0, len(raw), RUN):
            run = raw[offset:offset + length]
            kind, note, what = sort_run(run, images, allowed)
            if note:
                let_through(found, note, what)
            elif kind:
                found.add(kind, "%d bytes, %s: %s" % (length, stock_places(run, images), shown(what)), bisect.bisect_right(begins, offset), offset)
    # pictures: a set of instructions, a picture, and every picture that stands in the text
    if picture_make.is_instructions(data):
        if display is None:
            found.unjudged.append("a set of instructions for a picture: no folder of stock pictures to make it from")
        else:
            try:
                judge_pictures(found, [(os.path.basename(path),) + provenance.walk(data, display.read)], display)
            except Exception as problem:
                found.unjudged.append("the picture cannot be made here: %s" % problem)
        return
    regions = {"at byte %d" % start: (start, end) for start, end in svg_regions(data)}
    unread = judge_pictures(found, [(name, data[start:end], bytearray(b"\x01" * (end - start))) for name, (start, end) in regions.items()], display,
                            templates=True)
    if display is not None:
        loose = bytearray(data)                             # what is no picture: a tag, a shape, a line of a stock picture in it
        for name, (start, end) in regions.items():
            if name not in unread:
                loose[start:end] = b" " * (end - start)
        loose = bytes(loose)
        for kind, finder in (("tags", display.tags), ("shapes", display.shapes), ("scaled", lambda text: scaled_loose(display, text)),
                             ("primitives", display.primitive_elements)):
            for start, end, source in finder(loose):
                found.add("picture", "%s of the stock picture %s: %s" % (kind, source, loose[start:end][:80].decode("latin-1").replace("\n", " ")),
                          data.count(b"\n", 0, start) + 1, start, count=end - start)


def never(shown_path):
    """Why a file never goes, by its path as it was given; None for a file that may be looked at."""
    parts = [part for part in shown_path.replace("\\", "/").split("/") if part not in ("", ".")]
    for part in parts[:-1]:
        if part.lower() in NEVER_FOLDERS:
            return "in a folder `%s`: %s" % (part, NEVER_FOLDERS[part.lower()])
    name = parts[-1] if parts else ""
    ending = os.path.splitext(name)[1].lower()
    if ending in NEVER_ENDINGS:
        return "by its ending %s: %s" % (ending, NEVER_ENDINGS[ending])
    for pattern, reason in NEVER_NAMES:
        if pattern.match(name):
            return "by its name: %s" % reason
    return None


def is_boot_stream(data):
    if len(data) < 16 or data[3] != LDR_SIGN:
        return False
    check = 0
    for byte in data[:16]:
        check ^= byte
    return check == 0


def kind_of(path, data):
    ending = os.path.splitext(path)[1].lower()
    if not data:
        return "empty"
    if data[:8] == pack_build.MAGIC:
        return "pack"
    if data[:4] == TFW_MAGIC:
        return "update package"
    if is_boot_stream(data):
        return "firmware image"
    if ending in PIXEL_ENDINGS or any(data.startswith(magic) for magic in PIXEL_MAGICS):
        return "pixels"
    if data[:4] == b"\x7fELF":
        return "binary"
    if b"\0" in data[:SNIFF]:
        return "binary"
    return "text"


class Check:
    """The check with what it holds files against. `images` is None for the part that needs no stock image."""

    def __init__(self, images=None, places=None, display=None, allowed=(), deep=False):
        self.images, self.places, self.display, self.allowed, self.deep = images, places, display, set(allowed), deep
        self.seen = {}

    def file(self, path, shown_path=None):
        """One file's record: its path as given, its kind, its verdict (`refused`, `passed` or `not judged`), its
        findings, what was let through and what could not be judged."""
        shown_path = (shown_path or path).replace("\\", "/")
        record = {"path": shown_path}
        found = Findings()
        named = never(shown_path)
        if named:
            found.add("name", "never goes, %s" % named)
        if named and not self.deep:
            record["kind"] = "named"
        else:
            with open(path, "rb") as f:
                data = f.read()
            kind = record["kind"] = kind_of(path, data)
            if kind in ("update package", "firmware image"):
                found.add("name", "never goes, by its first bytes: a %s" % ("firmware update package" if kind == "update package" else "firmware image (a boot stream)"))
            key = (hashlib.sha256(data).digest(), os.path.splitext(path)[1].lower(), kind)
            if key in self.seen and kind not in ("binary",):
                same = self.seen[key]
                record["same_as"] = same["path"]
                for item in same["findings"]:
                    if item["rule"] != "name":
                        found.kept.append(dict(item))
                for rule, count in same["counts"].items():
                    if rule != "name":
                        found.counts[rule] = found.counts.get(rule, 0) + count
                found.notes = {rule: dict(held) for rule, held in same["notes"].items()}
                found.unjudged = list(same["not_judged"])
            elif kind == "pixels":
                found.unjudged.append("a picture of pixels: no rule here reads it")
            elif kind in ("empty", "update package", "firmware image") and not self.deep:
                pass
            elif self.images is None and kind in ("pack", "binary"):
                found.unjudged.append("a %s: its bytes were not searched (no stock image)" % kind)
            else:
                judge = judge_pack if kind == "pack" else judge_text if kind == "text" else judge_binary
                try:
                    judge(found, path, data, self.images, self.places, self.allowed, self.display)
                except Exception as problem:                # one file that cannot be read does not end the check of the others
                    found.unjudged.append("the check could not read this file to its end (%s: %s)" % (type(problem).__name__, problem))
                bulk(found)
            self.seen.setdefault(key, record)
        record["findings"], record["counts"], record["notes"], record["not_judged"] = found.kept, found.counts, found.notes, found.unjudged
        record["verdict"] = "refused" if found.counts else "not judged" if found.unjudged else "passed"
        return record


def files_of(paths, lists=()):
    """[(the path to open, the path as given)] for files, folders (walked in the order of the names) and lists."""
    out = []
    for path in paths:
        if os.path.isdir(path):
            for folder, folders, names in os.walk(path):
                folders[:] = sorted(name for name in folders if name != ".git")
                for name in sorted(names):
                    full = os.path.join(folder, name)
                    out.append((full, full))
        else:
            out.append((path, path))
    for listed in lists:
        with open(listed, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\r\n")
                if line:
                    out.append((line, line))
    return out


def summary(records):
    out = {"files": len(records), "refused": 0, "passed": 0, "not judged": 0, "by rule": {}, "let through": {}}
    for record in records:
        out[record["verdict"]] += 1
        for rule, count in record["counts"].items():
            held = out["by rule"].setdefault(rule, {"files": 0, "findings": 0})
            held["files"] += 1
            held["findings"] += count
        for rule, held in record["notes"].items():
            total = out["let through"].setdefault(rule, {"files": 0, "count": 0})
            total["files"] += 1
            total["count"] += held["count"]
    return out


def answer(records, strict=False):
    totals = summary(records)
    return 1 if totals["refused"] or (strict and totals["not judged"]) else 0


def print_record(record, show, out=print):
    out("%-10s %s%s" % (record["verdict"].upper() if record["verdict"] == "refused" else record["verdict"], record["path"],
                        "   (the same bytes as %s)" % record["same_as"] if "same_as" in record else ""))
    for rule in REFUSING:
        if rule not in record["counts"]:
            continue
        mine = [item for item in record["findings"] if item["rule"] == rule]
        for item in mine[:show]:
            where = " line %d" % item["line"] if item.get("line") else " byte 0x%x" % item["offset"] if item.get("offset") is not None else ""
            out("    %-8s%s: %s" % (rule, where, item["what"]))
        if len(mine) > show or record["counts"][rule] > len(mine):
            out("    %-8s %d in all" % (rule, record["counts"][rule]))
    for text in record["not_judged"]:
        out("    not judged: %s" % text)
    for rule in NOTES:
        if rule in record["notes"]:
            held = record["notes"][rule]
            out("    let through: %s, %d%s" % (rule, held["count"], ": " + "; ".join(held["samples"][:3]) if held["samples"] else ""))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="*", metavar="PATH", help="a file, or a folder whose every file is checked")
    ap.add_argument("--list", action="append", default=[], help="a file that names more files, one path a line (repeatable)")
    ap.add_argument("--stock", action="append", help="a folder of memory records of a stock image (repeatable)")
    ap.add_argument("--display", help="the folder of stock pictures")
    ap.add_argument("--profile", action="append", help="a profile whose places of change are searched for (repeatable)")
    ap.add_argument("--allow", help="a file of texts a person has read and accepted, one a line")
    ap.add_argument("--show", type=int, default=SHOW, help="findings printed for one rule in one file (default %d)" % SHOW)
    ap.add_argument("--passed", action="store_true", help="print the files that passed too")
    ap.add_argument("--deep", action="store_true", help="search a file that never goes by its name all the same")
    ap.add_argument("--strict", action="store_true", help="a file that could not be judged counts as a finding")
    ap.add_argument("--forms-only", action="store_true", help="only the rules that need no stock image")
    ap.add_argument("--json", help="write every file's record to this file")
    args = ap.parse_args(argv)
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="backslashreplace")       # a finding may quote a letter the console cannot print
    if args.json and os.path.exists(args.json):
        print("error: %s exists; this tool does not overwrite" % args.json)
        return 2
    try:
        files = files_of(args.paths, args.list)
    except OSError as problem:
        print("error: %s" % problem)
        return 2
    if not files:
        print("error: no file to check")
        return 2
    missing = [path for path, _shown in files if not os.path.isfile(path)]
    if missing:
        print("error: not a file: %s%s" % (missing[0], " (and %d more)" % (len(missing) - 1) if len(missing) > 1 else ""))
        return 2
    images = places = None
    if not args.forms_only:
        sources = args.stock or [folder for folder in (os.path.join(ROOT, "analysis", "os-records"), os.path.join(ROOT, "analysis", "boot-records"))
                                 if os.path.isfile(os.path.join(folder, "index.json"))]
        if not sources:
            print("error: no stock image to hold the files against (--stock), so the check cannot be made; --forms-only makes the part that needs none")
            return 2
        try:
            images = Images(sources)
            profiles = args.profile
            if profiles is None:
                folder = os.path.join(ROOT, "profiles")
                profiles = sorted(os.path.join(folder, name) for name in (os.listdir(folder) if os.path.isdir(folder) else ()) if name.endswith(".json"))
                profiles = [path for path in profiles if of_these_images(path, sources)]
            places = read_places(profiles, images)
        except (OSError, ValueError) as problem:
            print("error: %s" % problem)
            return 2
    folder = args.display or os.path.join(ROOT, "op-1field backup", "content", "display")
    display = provenance.Stock(folder) if os.path.isdir(folder) else None
    if args.display and display is None:
        print("error: %s is not a folder" % args.display)
        return 2
    check = Check(images, places, display, read_allowed(args.allow) if args.allow else (), args.deep)
    if images:
        print("held against: %d bytes of stock memory (%s), %d places of change, %s"
              % (len(images.stock.data), ", ".join(os.path.basename(os.path.normpath(source)) for source in images.sources), len(places[1]),
                 "%d stock pictures" % len(display.files) if display else "no stock pictures"))
    else:
        print("the part of the check that needs no stock image: names, the form of a log, of a listing and of a quoted instruction. A 0 from it is not the whole check's.")
    records = []
    for path, shown_path in files:
        record = check.file(path, shown_path)
        records.append(record)
        if record["verdict"] != "passed" or args.passed:
            print_record(record, args.show)
    totals = summary(records)
    print("")
    print("%d file(s): %d refused, %d passed, %d not judged" % (totals["files"], totals["refused"], totals["passed"], totals["not judged"]))
    for rule in REFUSING:
        if rule in totals["by rule"]:
            print("  %-8s %5d file(s), %7d %s" % (rule, totals["by rule"][rule]["files"], totals["by rule"][rule]["findings"],
                                                 "bytes" if rule in ("table", "picture") else "line(s)" if rule in ("listing", "log") else "finding(s)"))
    for rule in NOTES:
        if rule in totals["let through"]:
            print("  let through: %-16s %5d file(s), %7d" % (rule, totals["let through"][rule]["files"], totals["let through"][rule]["count"]))
    code = answer(records, args.strict)
    print("answer: %d (%s)" % (code, "something of the stock firmware was found" if totals["refused"] else
                               "a file could not be judged" if code else "nothing of the stock firmware was found" + (", by the rules that need no image" if images is None else "")))
    if args.json:
        with open(args.json, "w", encoding="utf-8", newline="\n") as f:
            json.dump({"rules": {"RUN": RUN, "HEX": HEX, "MANY": MANY, "SMALL": SMALL, "QUOTE": QUOTE, "LOG_LINES": LOG_LINES, "LOG_SHARE": LOG_SHARE, "PHRASE": PHRASE, "LONG": LONG, "PAD": PAD, "ROW": ROW, "BULK_TEXTS": BULK_TEXTS, "BULK_BYTES": BULK_BYTES, "LICENCE_SPAN": LICENCE_SPAN},
                       "stock": [source.replace("\\", "/") for source in images.sources] if images else [], "summary": totals, "answer": code,
                       "files": records}, f, indent=1)
            f.write("\n")
    return code


if __name__ == "__main__":
    sys.exit(main())

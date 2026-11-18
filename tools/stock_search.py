#!/usr/bin/env python3
"""Find what a file of ours holds that also stands in the stock firmware: the search of docs/14, section 10, rows C and D.

  python -I tools/stock_search.py runs "candidates/pack-j5i/kit.pack=build/build_j5i_kit_placed" [more FILE[=LINK]]
         [--stock analysis/os-records] [--min 8] [--histogram] [--profile profiles/op1f-1.7.0.json] [--json OUT] [--quiet]
  python -I tools/stock_search.py pictures "candidates/pack-j5i/8 tracks.pack" [more packs or .svg files]
         [--display "op-1field backup/content/display"] [--json OUT]

`runs` lists every run of at least `--min` bytes of a file that also stands, whole, somewhere in the stock OS image,
and says where the run lies in the file. A FILE is a pack file (any format of tools/pack_build.py), a base or a
module binary, or any other file. LINK is the kept link the file was made from, without its ending
(`build/<set>_<pack>_placed` for a pack as the unit places it, `build/build_j5c_base` for a base): its `.elf` names
the sections and the symbols, its `.map` the object file each piece came from, its `.dis.txt` the instructions.
Without a LINK a pack is still cut into its header, body, table, lists and pictures.

The stock image is the folder of memory records `tools/ldr_dump_records.py` writes (its `index.json` and record
files; `analysis/os-records` is the OS, `analysis/boot-records` te-boot). `--stock` may be given several times;
a plain file is taken as one stretch of memory from address 0.

What a run is, exactly:

  - A run is a stretch of the file that stands byte for byte in one stretch of the stock's memory and can be made
    longer on neither side. Runs may overlap. The list is exact: no run of at least `--min` bytes is left out.
  - Fill says nothing about copying, so it is set aside on both sides before the search: a stretch of at least
    8 bytes in which every byte is the byte four places before it (zeros, 0xff, one word again and again; the
    image's own fill records are of this kind). A run never holds 8 such bytes in a row; it may hold up to 7 of them
    at an edge. A shared stretch with fill in its middle comes out as the runs on both sides of the fill.
  - A run never crosses from one part of a file into the next (a pack's header, body, lists, each of its pictures).

For every run: its place in the file and what lies there (the part; with a LINK the section, the object file, the
symbol; inside a table of stock changes the entry and its word), its bytes, the instructions the listing has for
them, every address at which the run stands in the stock (the count is how often: a compiler's habit stands at many
places, a copied site at one), and with `--profile` the change site of the profile the stock place is.
A table of stock changes is also read by its structure, whatever `--min` is: each entry's two "before" words are
held against the stock image at the entry's site, and the count of stock bytes the table holds is printed. An entry
of the new kind (Build L: tools/boot_table.py --sums) has a checksum and a trampoline's room where those words were:
it holds no stock bytes, and is counted apart.

`--histogram` counts the runs for a row of lengths from 4 up, by kind of place: the measure behind the choice of
`--min` (analysis/pack_clean_study.md, section 3).

`pictures` measures a picture's text against the stock pictures of the device's `content/display` folder, tag by
tag (a tag is the text from a `<` to its `>`, runs of white space taken as one space): how many of its bytes are
tags that stand in a stock file as they are, how many are the path data (`d="..."`, `points="..."`) of a changed
tag that a stock tag has too, as it is or but for its first point, and how many are neither. A geometry that was
scaled or drawn anew from stock's numbers is not seen by any such comparison: the tool that made the picture says
that, not this one.

Reads only. `--json` writes one file and refuses to overwrite it.
"""

import argparse
import array
import bisect
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack_build  # noqa: E402

GRAM = 8                    # the bytes of a key: the search is by the eight bytes from every position
FILL = 8                    # this many bytes with the period four are fill
EDGE = FILL - 1             # what a cut leaves of a fill stretch on each side
LEAST = 4                   # the shortest run the engine counts at all (the histogram's first row)
LENGTHS = (4, 6, 8, 10, 12, 16, 20, 24, 32, 48, 64)
PLAIN = 4                   # a run with fewer bytes that are neither 0x00 nor 0xff is small numbers among zeros
FILL_RUN = re.compile(b"\\x00{%d,}" % (FILL - 4))
CODE = ((0x11a00000, 0x11a0c000, "L1 code"), (0x8c03d000, 0x8c0e36cc, "DDR code"), (0x8d4e36cc, 0x8d5174d8, "DDR code"),
        (0x8f001dec, 0x8f01f780, "DDR code"), (0x8fc1f780, 0x8fccbf18, "DDR code"))


# ---------------------------------------------------------------- the engine


def stretches(data):
    """The parts of `data` between its fill stretches, each with up to EDGE bytes of the fill at its edges:
    [(start, end)]. A fill stretch is at least FILL bytes in which every byte is the byte four places before it."""
    n = len(data)
    if n < FILL:
        return [(0, n)] if n else []
    same = (int.from_bytes(data[4:], "little") ^ int.from_bytes(data[:-4], "little")).to_bytes(n - 4, "little")
    out, at = [], 0
    for found in FILL_RUN.finditer(same):
        out.append((at, found.start() + EDGE))          # the stretch is data[found.start():found.end() + 4]
        at = found.end() + 4 - EDGE
    out.append((at, n))
    return out


def grams(data):
    """For every position of `data` the GRAM bytes from it as one big-endian number, zero bytes behind the end."""
    n = len(data)
    padded = data + bytes(2 * GRAM)
    out = [0] * n
    for phase in range(min(GRAM, n)):
        count = (n - phase + GRAM - 1) // GRAM
        words = array.array("Q")
        words.frombytes(padded[phase:phase + GRAM * count])
        if sys.byteorder == "little":
            words.byteswap()
        out[phase::GRAM] = words.tolist()
    return out


def common(a, i, b, j, limit):
    """How many of the `limit` bytes from a[i] and from b[j] are the same from the start."""
    done, step = 0, 32
    while done < limit:
        width = step if limit - done > step else limit - done
        x, y = a[i + done:i + done + width], b[j + done:j + done + width]
        if x != y:
            return done + ((width << 3) - (int.from_bytes(x, "big") ^ int.from_bytes(y, "big")).bit_length() >> 3)
        done += width
        if step < 4096:
            step <<= 1
    return limit


class Stock:
    """The stock's memory, indexed for the search. `regions` are [(name, address, bytes)], each one stretch of memory
    that is contiguous; `pieces` are what a read by address is answered from: [(address, bytes or None, count, word)]."""

    def __init__(self, regions, pieces=()):
        self.pieces = sorted(pieces, key=lambda piece: piece[0])
        self.piece_starts = [piece[0] for piece in self.pieces]
        chunks, self.starts, self.addresses, self.names = [], [], [], []
        at = 0
        for name, address, data in regions:
            for start, end in stretches(data):
                if end - start < LEAST:
                    continue
                self.starts.append(at)
                self.addresses.append(address + start)
                self.names.append(name)
                chunks.append(data[start:end])
                at += end - start
        self.data = b"".join(chunks)
        size = len(self.data)
        if size >= 1 << 32:
            raise ValueError("The stock is too large for this index")
        self.ends = array.array("I")
        for number, start in enumerate(self.starts):
            end = self.starts[number + 1] if number + 1 < len(self.starts) else size
            self.ends.extend(array.array("I", [end]) * (end - start))
        keys = grams(self.data)
        order = sorted(range(size), key=keys.__getitem__)
        self.keys = array.array("Q", map(keys.__getitem__, order))
        self.pos = array.array("I", order)

    def address_of(self, p):
        number = bisect.bisect_right(self.starts, p) - 1
        return self.addresses[number] + p - self.starts[number], self.names[number]

    def read(self, address, count):
        """`count` bytes of the stock's memory at `address`, or None where the image does not hold them all."""
        out = b""
        while count:
            number = bisect.bisect_right(self.piece_starts, address) - 1
            if number < 0:
                return None
            start, data, size, word = self.pieces[number]
            if address >= start + size:
                return None
            take = min(count, start + size - address)
            if data is None:
                out += bytes(word[(address - start + k) & 3] for k in range(take))
            else:
                out += data[address - start:address - start + take]
            address, count = address + take, count - take
        return out

    def lengths(self, text, least=GRAM):
        """For every position of `text` the length of the longest run from it that stands whole in one stretch of the
        stock; 0 where it is shorter than `least`. Exact."""
        size = len(text)
        out = [0] * size
        if size < least:
            return out
        keys, pos, ends, data = self.keys, self.pos, self.ends, self.data
        count = len(keys)
        short = least < GRAM
        wanted = grams(text)
        left, right = bisect.bisect_left, bisect.bisect_right
        for i in range(size - least + 1):
            ahead = size - i
            key = wanted[i]
            if ahead >= GRAM:
                lo = left(keys, key)
                if lo < count and keys[lo] == key:
                    best = 0
                    for p in pos[lo:right(keys, key, lo)]:
                        room = ends[p] - p
                        if room > ahead:
                            room = ahead
                        if room < GRAM or room <= best:
                            continue
                        length = GRAM + common(text, i + GRAM, data, p + GRAM, room - GRAM)
                        if length > best:
                            best = length
                    if best:
                        if best >= least:
                            out[i] = best
                        continue
                    most = GRAM - 1                     # the key stands only where a stretch ends inside it
                elif not short:
                    continue
                else:
                    most = 0
                    if lo < count:
                        most = 64 - (keys[lo] ^ key).bit_length() >> 3
                    if lo:
                        before = 64 - (keys[lo - 1] ^ key).bit_length() >> 3
                        if before > most:
                            most = before
            elif not short:
                break
            else:
                most = ahead
            while most >= least:                        # a run shorter than a key: some place must have room for it
                shift = (GRAM - most) * 8
                first = key >> shift << shift
                found = False
                for number in range(left(keys, first), left(keys, first + (1 << shift))):
                    p = pos[number]
                    if ends[p] - p >= most:
                        found = True
                        break
                if found:
                    out[i] = most
                    break
                most -= 1
        return out

    def places(self, run):
        """Every address at which `run` stands whole in one stretch of the stock, in order, each once."""
        size = len(run)
        key = int.from_bytes(run[:GRAM].ljust(GRAM, b"\0"), "big")
        if size >= GRAM:
            lo = bisect.bisect_left(self.keys, key)
            hi = bisect.bisect_right(self.keys, key, lo)
        else:
            shift = (GRAM - size) * 8
            lo = bisect.bisect_left(self.keys, key >> shift << shift)
            hi = bisect.bisect_left(self.keys, (key >> shift << shift) + (1 << shift))
        out = set()
        for p in self.pos[lo:hi]:
            if self.ends[p] - p >= size and self.data[p:p + size] == run:
                out.add(self.address_of(p))
        return sorted(out)


def runs_in(stock, data, start, end, least=GRAM):
    """The runs of data[start:end]: [(offset in data, length)], in order. A run is at least `least` bytes long,
    stands whole in the stock and can be made longer on neither side."""
    out = set()
    for first, last in stretches(data[start:end]):
        if last - first < least:
            continue
        before = 0
        for i, length in enumerate(stock.lengths(data[start + first:start + last], least)):
            if length and length != before - 1:
                out.add((start + first + i, length))
            before = length
    return sorted(out)


def fill_bytes(data, start, end):
    """How many bytes of data[start:end] lie in fill stretches (and are not searched but for the edges)."""
    kept = stretches(data[start:end])
    return sum(max(0, (kept[k + 1][0] + EDGE) - (kept[k][1] - EDGE)) for k in range(len(kept) - 1))


def covered(runs):
    """How many bytes the runs cover together."""
    total, edge = 0, -1
    for offset, length in sorted(runs):
        if offset + length > edge:
            total += offset + length - max(offset, edge)
            edge = offset + length
    return total


# ---------------------------------------------------------------- the stock image


def load_records(folder):
    """(regions, pieces) of a folder of memory records with its index.json: the stored records as they are, a fill
    record as its first and last FILL bytes (the search sets fill aside anyway), neighbours in memory joined."""
    with open(os.path.join(folder, "index.json"), encoding="utf-8") as f:
        index = json.load(f)
    name = os.path.basename(os.path.normpath(folder))
    pieces = []
    for record in index["records"]:
        if not record["count"]:
            continue
        if record.get("file"):
            with open(os.path.join(folder, record["file"]), "rb") as f:
                data = f.read()
            if len(data) != record["count"]:
                raise ValueError("The record %s is not as long as the index says" % record["file"])
            pieces.append((record["target"], data, len(data), None))
        elif record.get("fill"):
            pieces.append((record["target"], None, record["count"], struct.pack("<I", record["argument"])))
    pieces.sort(key=lambda piece: piece[0])
    regions, start, parts, at = [], None, [], None
    for address, data, count, word in pieces:
        if at != address:
            if parts:
                regions.append((name, start, b"".join(parts)))
            start, parts = address, []
        if data is not None:
            parts.append(data)
        elif count <= 2 * FILL:
            parts.append(bytes(word[k & 3] for k in range(count)))
        else:
            parts.append(bytes(word[k & 3] for k in range(FILL)))
            regions.append((name, start, b"".join(parts)))
            start, parts = address + count - FILL, [bytes(word[k & 3] for k in range(count - FILL, count))]
        at = address + count
    if parts:
        regions.append((name, start, b"".join(parts)))
    return regions, pieces


def load_stock(sources):
    regions, pieces = [], []
    for source in sources:
        if os.path.isdir(source):
            more, held = load_records(source)
            regions += more
            pieces += held
        else:
            with open(source, "rb") as f:
                data = f.read()
            regions.append((os.path.basename(source), 0, data))
    return Stock(regions, pieces)


def stock_kind(address):
    for start, end, name in CODE:
        if start <= address < end:
            return name
    return "data"


# ---------------------------------------------------------------- a file of ours, cut into what it holds


def read_elf(data):
    """(sections, symbols) of a 32-bit little-endian ELF file: sections as dicts, symbols as (value, size, type, section's
    number, name)."""
    section_offset, = struct.unpack_from("<I", data, 0x20)
    entry, count, names_at = struct.unpack_from("<HHH", data, 0x2e)
    raw = [struct.unpack_from("<10I", data, section_offset + i * entry) for i in range(count)]

    def text(at):
        return data[at:data.index(b"\0", at)].decode("ascii", "replace")

    sections = [{"name": text(raw[names_at][4] + r[0]), "type": r[1], "address": r[3], "offset": r[4], "size": r[5], "link": r[6]}
                for r in raw]
    symbols = []
    for section in sections:
        if section["type"] != 2:
            continue
        strings = sections[section["link"]]["offset"]
        for at in range(section["offset"], section["offset"] + section["size"], 16):
            name, value, size, info, _other, where = struct.unpack_from("<IIIBBH", data, at)
            symbols.append((value, size, info & 15, where, text(strings + name)))
    return sections, symbols


MAP_LINE = re.compile(r"^\s+(\.\S+)?\s*0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+\.o)\s*$")
LISTING_LINE = re.compile(r"^([0-9a-f]{8}):\t((?:[0-9a-f]{2} )+)\s*\t?(.*)$")


class Link:
    """A kept link: where its sections, object files, symbols and instructions lie. `stem` is its path without ending."""

    def __init__(self, stem):
        with open(stem + ".elf", "rb") as f:
            sections, symbols = read_elf(f.read())
        self.sections = [(s["address"], s["address"] + s["size"], s["name"]) for s in sections
                         if s["name"] in (".text", ".rodata", ".data") and s["size"]]
        self.place = next(s["address"] for s in sections if s["name"] == ".text")
        names = {}
        for value, size, kind, where, name in symbols:
            if kind > 2 or where in (0, 0xfff1, 0xfff2) or not name or name[0] in ".$" or name.startswith("L$"):
                continue
            if value not in names or kind > names[value][0]:
                names[value] = (kind, name)
        self.symbol_at = sorted(names)
        self.symbol_name = [names[address][1] for address in self.symbol_at]
        self.values = {name: value for value, _size, _kind, where, name in symbols if name and where != 0}
        self.objects = []
        if os.path.isfile(stem + ".map"):
            with open(stem + ".map", encoding="utf-8", errors="replace") as f:
                for line in f:
                    found = MAP_LINE.match(line)
                    if found and int(found.group(3), 16):
                        start = int(found.group(2), 16)
                        self.objects.append((start, start + int(found.group(3), 16), os.path.basename(found.group(4))))
        self.objects.sort()
        self.object_at = [o[0] for o in self.objects]
        self.lines = {}
        if os.path.isfile(stem + ".dis.txt"):
            with open(stem + ".dis.txt", encoding="utf-8", errors="replace") as f:
                for line in f:
                    found = LISTING_LINE.match(line)
                    if found:
                        self.lines[int(found.group(1), 16)] = (len(found.group(2).split()), found.group(3).split("/*")[0].strip())
        self.line_at = sorted(self.lines)

    def section(self, address):
        for start, end, name in self.sections:
            if start <= address < end:
                return name
        return None

    def symbol(self, address):
        number = bisect.bisect_right(self.symbol_at, address) - 1
        if number < 0:
            return None
        return self.symbol_name[number], address - self.symbol_at[number]

    def object_file(self, address):
        number = bisect.bisect_right(self.object_at, address) - 1
        if number >= 0 and self.objects[number][0] <= address < self.objects[number][1]:
            return self.objects[number][2]
        return None

    def instructions(self, address, length):
        """(the instructions the listing has wholly inside the stretch, whether the stretch begins and ends at one)."""
        first = bisect.bisect_left(self.line_at, address)
        texts, at, whole = [], address, first < len(self.line_at) and self.line_at[first] == address
        for start in self.line_at[first:]:
            size, text = self.lines[start]
            if start + size > address + length:
                break
            if start != at:
                whole = False
            texts.append(text)
            at = start + size
        return texts, whole and at == address + length


TABLE_WORDS = ("kind", "site", "target", "before", "before")
SUM_WORDS = ("kind", "site", "target", "checksum", "room")      # an entry of the new kind (Build L)
OLD_KINDS = 5                                                   # the kinds of an entry with stock bytes: 0 to 5; a larger kind word is the new kind


class Target:
    """A file of ours: its bytes, its parts (which no run crosses) and what stands at each place."""

    def __init__(self, path, link=None):
        self.path = path
        self.name = os.path.basename(path)
        with open(path, "rb") as f:
            self.data = f.read()
        self.link = Link(link) if link else None
        self.parts = []                 # (start, end, name)
        self.tables = []                # (start, end, name): offsets in the file
        self.body = None                # (start in the file, end) of what the link describes
        self.lists = []                 # (start, end, name) inside the lists
        data = self.data
        fields = None
        if data[:8] == pack_build.MAGIC:
            fmt = struct.unpack_from("<I", data, 8)[0]
            if fmt == pack_build.FORMAT_LINKED:
                fields, body, extra = pack_build.parse3(data)
                head = pack_build.HEADER3_BYTES
            else:
                fields, body = pack_build.parse(data)
                extra, head = b"", pack_build.HEADER_BYTES
            self.kind = "pack, format %d, '%s'" % (fmt, fields["name"])
            at = head + len(body)
            self.parts.append((0, head, "header"))
            self.parts.append((head, at, "body"))
            self.body = (head, at)
            if fields.get("table"):
                self.tables.append((head + fields["table"] - fields["load_address"], head + fields["table_end"] - fields["load_address"],
                                    "op1f_boot_table"))
            if fmt == pack_build.FORMAT_LINKED:
                lists_end = at + fields["lists_bytes"]
                self.parts.append((at, lists_end, "lists"))
                counts = struct.unpack_from("<6I", data, at)
                inner = at + 24
                for name, size in zip(("works", "exports", "imports", "places", "marks", "names"),
                                      (12 * counts[0], 8 * counts[1], 8 * counts[2], 12 * counts[3], 8 * counts[4], counts[5])):
                    self.lists.append((inner, inner + size, name))
                    inner += size
                if extra:
                    inner = lists_end
                    for name, picture in pack_build.pictures_in(data):
                        self.parts.append((inner, inner + len(picture), "picture " + name))
                        inner += len(picture)
                    if inner != len(data):
                        self.parts.append((inner, len(data), "behind the lists"))
        else:
            self.kind = "binary" if link else "file"
            self.parts.append((0, len(data), "body" if link else "file"))
            if link:
                self.body = (0, len(data))
        self.fields = fields
        if self.link and self.body:
            for name, value in sorted(self.link.values.items()):
                end = self.link.values.get(name + "_end")
                if name.endswith("table") and end and (end - value) % pack_build.ENTRY_BYTES == 0 and end > value:
                    start = self.body[0] + value - self.link.place
                    if not any(t[0] == start for t in self.tables) and start + end - value <= self.body[1]:
                        self.tables.append((start, start + end - value, name))
        self.tables.sort()

    def entries(self):
        """Every entry of every table of stock changes: (table's name, number, offset in the file, kind, site, the
        "before" bytes: four for a data word, eight otherwise; none for an entry of the new kind, which has a checksum
        and a room there)."""
        out = []
        for start, end, name in self.tables:
            for number, at in enumerate(range(start, end, pack_build.ENTRY_BYTES)):
                kind, site, _target = struct.unpack_from("<3I", self.data, at)
                out.append((name, number, at, kind, site, self.data[at + 12:at + (12 if kind > OLD_KINDS else 16 if kind == 0 else 20)]))
        return out

    def where(self, offset, length=1):
        """What stands at `offset`: a dict with the part and, as far as known, the section, object file, symbol and
        table entry, and `kind`, one word for the tables of the report."""
        part = next((name for start, end, name in self.parts if start <= offset < end), "?")
        out = {"part": part, "kind": part.split(" ")[0]}
        if out["kind"] == "picture":
            out["kind"] = "pictures"
        for start, end, name in self.tables:
            if start <= offset < end:
                # the entry whose stock bytes the stretch holds most of: a run may begin or end in a neighbouring word
                number, word = divmod(offset - start, pack_build.ENTRY_BYTES)
                held, last = 0, min((end - start - 1) // pack_build.ENTRY_BYTES, (offset + length - 1 - start) // pack_build.ENTRY_BYTES)
                for entry in range(number, last + 1):
                    at = start + entry * pack_build.ENTRY_BYTES
                    kind = struct.unpack_from("<I", self.data, at)[0]
                    size = 0 if kind > OLD_KINDS else 4 if kind == 0 else 8
                    shared = min(offset + length, at + 12 + size) - max(offset, at + 12)
                    if shared > held:
                        number, held = entry, shared
                kind, site = struct.unpack_from("<2I", self.data, start + number * pack_build.ENTRY_BYTES)
                out.update(table=name, entry=number, site=site, word=(SUM_WORDS if kind > OLD_KINDS else TABLE_WORDS)[word // 4])
                out["kind"] = "table, the stock words" if held >= 4 else "table, other words"
        if part == "lists":
            out["list"] = next((name for start, end, name in self.lists if start <= offset < end), "counts")
        if part == "body" and self.link:
            address = self.link.place + offset - self.body[0]
            out["address"] = address
            out["section"] = self.link.section(address)
            out["object"] = self.link.object_file(address)
            found = self.link.symbol(address)
            if found:
                out["symbol"], out["symbol_offset"] = found
            if "table" not in out:
                if out["section"] == ".text":
                    out["kind"] = "code, written by hand" if (out["object"] or "").endswith(".s.o") else "code, compiled"
                elif out["section"] == ".rodata":
                    out["kind"] = "constant data"
                elif out["section"] == ".data":
                    out["kind"] = "data"
        return out


def printable(run):
    """Whether a run is text: plain characters, at least four of them, with at most zero bytes and white space
    between them (strings stand in constant data with zero bytes between them)."""
    return all(0x20 <= c < 0x7f or c in (0, 9, 10, 13) for c in run) and sum(1 for c in run if 0x20 < c < 0x7f) >= 4


def site_copies(target, sites):
    """Where the stock bytes of a change site of the profile stand in the file, outside the "before" words of its
    tables: [(offset, site, bytes, group)]. Only sites of at least GRAM bytes: a data word is an address."""
    held = [(at + 12, at + 20) for _name, _number, at, _kind, _site, _before in target.entries()]
    out = []
    for site, before, group in sites[1] if sites else ():
        if len(before) < GRAM:
            continue
        at = target.data.find(before)
        while at >= 0:
            if not any(start <= at and at + len(before) <= end for start, end in held):
                out.append((at, site, len(before), group))
            at = target.data.find(before, at + 1)
    return sorted(out)


def measure(stock, target, least=GRAM, sites=None):
    """Every run of `target` of at least `least` bytes, as dicts, in the order of the file. With `sites` (read_sites)
    a run is given the change sites whose stock bytes it holds or touches, and those its stock places are."""
    out = []
    copies = site_copies(target, sites)
    for start, end, _name in target.parts:
        for offset, length in runs_in(stock, target.data, start, end, least):
            run = target.data[offset:offset + length]
            entry = target.where(offset, length)
            entry.update(offset=offset, length=length, bytes=run.hex(), plain=sum(1 for c in run if c not in (0, 0xff)) < PLAIN)
            if printable(run):
                entry["text"] = run.decode("ascii").replace("\0", "\\0")
            places = stock.places(run)
            entry["count"] = len(places)
            entry["stock"] = [address for address, _name in places[:8]]
            entry["stock_kind"] = sorted(set(stock_kind(address) for address, _name in places))
            if sites:
                found = [(site, group) for at, site, size, group in copies if at < offset + length and offset < at + size]
                for address, _name in places:
                    number = bisect.bisect_right(sites[0], address + length - 1) - 1
                    while number >= 0 and sites[1][number][0] + len(sites[1][number][1]) > address:
                        if sites[1][number][0] < address + length:
                            found.append((sites[1][number][0], sites[1][number][2]))
                        number -= 1
                if found:
                    entry["change_site"] = sorted(set("0x%08x %s" % item for item in found))
            if target.link and entry.get("section") == ".text":
                texts, whole = target.link.instructions(entry["address"], length)
                entry["instructions"], entry["whole"] = texts, whole
            out.append(entry)
    return out


def table_check(stock, target):
    """What the tables of stock changes hold of the stock, by their structure: (entries, entries whose "before" bytes
    are the stock's bytes at the site, the stock bytes held, [(table, number, site)] of entries that differ). An entry
    of the new kind has no "before" bytes: it is among the entries and in neither of the other counts."""
    entries = target.entries()
    same, held, other = 0, 0, []
    for name, number, _at, _kind, site, before in entries:
        if not before:
            continue
        held += len(before)
        if stock.read(site, len(before)) == before:
            same += 1
        else:
            other.append((name, number, site))
    return len(entries), same, held, other


def histogram(stock, targets, lengths=LENGTHS):
    """{kind: {length: (runs, bytes covered)}} over the targets, and the same under the kind "all"."""
    least = min(lengths)
    by_kind = {}
    for target in targets:
        for start, end, _name in target.parts:
            for offset, length in runs_in(stock, target.data, start, end, least):
                by_kind.setdefault(target.where(offset, length)["kind"], []).append((id(target), offset, length))
    out = {}
    for kind, runs in list(by_kind.items()) + [("all", [run for runs in by_kind.values() for run in runs])]:
        out[kind] = {}
        for limit in lengths:
            kept = [run for run in runs if run[2] >= limit]
            total = sum(covered([(offset, length) for owner, offset, length in kept if owner == one])
                        for one in set(owner for owner, _offset, _length in kept))
            out[kind][limit] = (len(kept), total)
    return out


def read_sites(profile_path):
    """([site addresses], [(site, the stock bytes there, group)]) of a profile's edits, in the order of the addresses."""
    with open(profile_path, encoding="utf-8") as f:
        profile = json.load(f)
    sites = sorted((int(patch["address"], 16), bytes.fromhex(patch["before"]), patch["group"])
                   for patch in profile["patches"] if "address" in patch and "before" in patch)
    return [site[0] for site in sites], sites


# ---------------------------------------------------------------- the pictures


TAG = re.compile(rb"<[^<>]*>")
SPACE = re.compile(rb"\s+")
GEOMETRY = re.compile(rb'\s(?:d|points)="([^"]*)"')
FIRST_POINT = re.compile(rb"^\s*[Mm]?\s*-?[0-9.]+[\s,]*-?[0-9.]+")
PLAIN_TAG = 16              # a shorter tag is structure every picture has (`</g>`, `<g id="bg">`): counted apart


def tags_of(text):
    """[(start, end, the tag with its white space as single spaces)] for every tag of a picture's text."""
    return [(found.start(), found.end(), SPACE.sub(b" ", found.group())) for found in TAG.finditer(text)]


def moved(geometry):
    """Path data without its first point, so that a shape drawn at another place is still told."""
    return FIRST_POINT.sub(b"", geometry, 1)


class Display:
    """The stock pictures of a `content/display` folder, as the tags and the path data they hold."""

    def __init__(self, folder):
        self.tags, self.shapes, self.moved = {}, {}, {}
        self.files = sorted(name for name in os.listdir(folder) if name.lower().endswith(".svg"))
        for name in self.files:
            with open(os.path.join(folder, name), "rb") as f:
                text = f.read()
            for _start, _end, tag in tags_of(text):
                self.tags.setdefault(tag, name)
                for found in GEOMETRY.finditer(tag):
                    self.shapes.setdefault(found.group(1), name)
                    self.moved.setdefault(moved(found.group(1)), name)

    def measure(self, text):
        """A picture's bytes by what they are: a dict of counts, and under "from" the stock files its stock tags and
        shapes stand in, each with its bytes."""
        out = {"bytes": len(text), "stock tags": 0, "stock shapes": 0, "stock shapes, moved": 0, "structure": 0, "tags": 0, "tags as stock": 0}
        source = {}
        for start, end, tag in tags_of(text):
            out["tags"] += 1
            if len(tag) < PLAIN_TAG:
                out["structure"] += end - start
            elif tag in self.tags:
                out["stock tags"] += end - start
                out["tags as stock"] += 1
                source[self.tags[tag]] = source.get(self.tags[tag], 0) + end - start
            else:
                for found in GEOMETRY.finditer(tag):
                    shape = found.group(1)
                    if len(shape) < PLAIN_TAG:
                        continue
                    if shape in self.shapes:
                        out["stock shapes"] += len(shape)
                        source[self.shapes[shape]] = source.get(self.shapes[shape], 0) + len(shape)
                    elif moved(shape) in self.moved and len(moved(shape)) >= PLAIN_TAG:
                        out["stock shapes, moved"] += len(moved(shape))
                        source[self.moved[moved(shape)]] = source.get(self.moved[moved(shape)], 0) + len(moved(shape))
        out["other"] = out["bytes"] - out["stock tags"] - out["stock shapes"] - out["stock shapes, moved"] - out["structure"]
        out["from"] = sorted(source.items(), key=lambda item: -item[1])
        return out


def pictures_of(path):
    """[(name, text)] of a pack file's pictures, or the one picture a plain file is."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] == pack_build.MAGIC:
        if struct.unpack_from("<I", data, 8)[0] != pack_build.FORMAT_LINKED:
            return []
        return pack_build.pictures_in(data)
    return [(os.path.basename(path), data)]


# ---------------------------------------------------------------- the commands


def split_target(text):
    path, sep, link = text.partition("=")
    return Target(path, link if sep else None)


def write_json(path, value):
    if os.path.exists(path):
        raise ValueError("%s exists; this tool does not overwrite" % path)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(value, f, indent=1)
        f.write("\n")


def show_run(entry, out):
    place = entry["part"]
    if "table" in entry:
        place += ", %s entry %d (site 0x%08x), the %s word" % (entry["table"], entry["entry"], entry["site"], entry["word"])
    elif "symbol" in entry:
        place += ", %s %s+0x%x (%s)" % (entry["section"], entry["symbol"], entry["symbol_offset"], entry.get("object") or "?")
    elif "list" in entry:
        place += ", " + entry["list"]
    stock = ", ".join("0x%08x" % address for address in entry["stock"][:3]) + (", ..." if entry["count"] > 3 else "")
    out("  0x%06x  %4d  %-24s  %s%s" % (entry["offset"], entry["length"], entry["kind"], place, "   [small numbers among zeros]" if entry["plain"] else ""))
    out("      %s%s" % (entry["bytes"][:96], "..." if len(entry["bytes"]) > 96 else ""))
    if "text" in entry:
        out("      text: %r" % entry["text"][:96])
    if entry.get("instructions"):
        out("      %s%s" % ("; ".join(entry["instructions"])[:160], "" if entry["whole"] else "   [not whole instructions]"))
    out("      in the stock %d time(s): %s (%s)%s" % (entry["count"], stock, ", ".join(entry["stock_kind"]),
                                                     "; change site " + "; ".join(entry["change_site"]) if "change_site" in entry else ""))


def cmd_runs(args):
    out = print
    stock = load_stock(args.stock or [os.path.join(ROOT, "analysis", "os-records")])
    out("the stock: %d bytes in %d stretches between its fill, %d stretches of memory" % (len(stock.data), len(stock.starts), len(stock.pieces)))
    targets = [split_target(text) for text in args.files]
    sites = read_sites(args.profile) if args.profile else None
    record = {"min": args.min, "files": []}
    for target in targets:
        runs = measure(stock, target, args.min, sites)
        entries, same, held, other = table_check(stock, target)
        out("")
        out("%s: %s, %d bytes" % (target.path, target.kind, len(target.data)))
        for start, end, name in target.parts:
            mine = [(r["offset"], r["length"]) for r in runs if start <= r["offset"] < end]
            out("  %-28s %8d bytes, %6d of them fill; %4d run(s) of %d bytes or more, covering %d bytes"
                % (name, end - start, fill_bytes(target.data, start, end), len(mine), args.min, covered(mine)))
        sums = sum(1 for entry in target.entries() if not entry[5])
        if entries:
            out("  tables of stock changes: %d entries, %d with the stock's own bytes at the site as their \"before\" words: %d stock bytes held%s%s"
                % (entries, same, held, "" if not other else "; not the stock's: " + ", ".join("%s[%d] 0x%08x" % o for o in other),
                   "" if not sums else "; %d of the new kind, with a checksum where the stock bytes were" % sums))
        copies = site_copies(target, sites)
        if sites:
            out("  the stock bytes of %d change site(s) of the profile stand in the file outside its tables' \"before\" words%s"
                % (len(set(c[1] for c in copies)), ":" if copies else ""))
            for at, site, size, group in copies:
                place = target.where(at, size)
                out("    0x%06x  site 0x%08x (%s): %s, %s" % (at, site, group, place["kind"], place.get("symbol") or place["part"]))
        kinds = {}
        for r in runs:
            kinds.setdefault(r["kind"], []).append((r["offset"], r["length"]))
        for kind in sorted(kinds):
            out("  by kind: %-26s %4d run(s), %6d bytes" % (kind, len(kinds[kind]), covered(kinds[kind])))
        out("  %d of the runs are small numbers among zeros (fewer than %d bytes that are neither 0x00 nor 0xff)" % (sum(1 for r in runs if r["plain"]), PLAIN))
        if not args.quiet:
            for r in runs:
                show_run(r, out)
        record["files"].append({"path": target.path.replace("\\", "/"), "kind": target.kind, "bytes": len(target.data),
                                "parts": [(name, start, end, fill_bytes(target.data, start, end)) for start, end, name in target.parts],
                                "table": {"entries": entries, "as_stock": same, "stock_bytes": held},
                                "site_copies": [(at, site, size, group, target.where(at, size).get("symbol")) for at, site, size, group in copies],
                                "runs": runs})
    if args.histogram:
        counts = histogram(stock, targets)
        record["histogram"] = {kind: {str(n): v for n, v in row.items()} for kind, row in counts.items()}
        out("")
        out("runs of at least N bytes, by kind of place (runs / bytes they cover):")
        out("  %-26s" % "N" + "".join("%14d" % n for n in LENGTHS))
        for kind in sorted(counts, key=lambda k: (k == "all", k)):
            out("  %-26s" % kind + "".join("%14s" % ("%d / %d" % counts[kind][n]) for n in LENGTHS))
    if args.json:
        write_json(args.json, record)
    return 0


def cmd_pictures(args):
    display = Display(args.display or os.path.join(ROOT, "op-1field backup", "content", "display"))
    print("the stock pictures: %d files, %d different tags, %d different shapes" % (len(display.files), len(display.tags), len(display.shapes)))
    record = []
    for path in args.files:
        for name, text in pictures_of(path):
            m = display.measure(text)
            print("%s: %s, %d bytes: %d in %d of %d tags that a stock file has as they are, %d in stock shapes of changed tags, "
                  "%d in stock shapes moved, %d structure, %d other; from %s"
                  % (path, name, m["bytes"], m["stock tags"], m["tags as stock"], m["tags"], m["stock shapes"], m["stock shapes, moved"],
                     m["structure"], m["other"], ", ".join("%s %d" % item for item in m["from"][:4]) or "no stock file"))
            m.update(file=path.replace("\\", "/"), name=name)
            record.append(m)
    if args.json:
        write_json(args.json, record)
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="command", required=True)
    runs = sub.add_parser("runs", help="every run of bytes a file shares with the stock image")
    runs.add_argument("files", nargs="+", metavar="FILE[=LINK]")
    runs.add_argument("--stock", action="append", help="a folder of memory records, or a file (repeatable; default analysis/os-records)")
    runs.add_argument("--min", type=int, default=GRAM, help="the shortest run listed, at least %d (default %d)" % (LEAST, GRAM))
    runs.add_argument("--profile", help="a profile whose change sites are named where a run's stock place is one")
    runs.add_argument("--histogram", action="store_true", help="count the runs for a row of lengths from %d up" % LEAST)
    runs.add_argument("--quiet", action="store_true", help="the summaries only, not every run")
    runs.add_argument("--json", help="write the measure to this file")
    runs.set_defaults(run=cmd_runs)
    pictures = sub.add_parser("pictures", help="how much of a picture's text is a stock picture's")
    pictures.add_argument("files", nargs="+")
    pictures.add_argument("--display", help="the folder of stock pictures (default op-1field backup/content/display)")
    pictures.add_argument("--json", help="write the measure to this file")
    pictures.set_defaults(run=cmd_pictures)
    args = ap.parse_args(argv)
    if args.command == "runs" and args.min < LEAST:
        print("error: --min is at least %d" % LEAST)
        return 2
    try:
        return args.run(args)
    except ValueError as e:
        print("error: %s" % e)
        return 2


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""What the pictures a pack file makes on the unit still have of the stock pictures, byte by byte (step P3b).

  python -I tools/picture_provenance.py measure --display "op-1field backup/content/display" candidates/pack-p3/*.pack
  python -I tools/picture_provenance.py measure --display "op-1field backup/content/display" --show 20 "candidates/pack-p3/fx train.pack"

The measure of step P3 (tools/stock_search.py's tags and shapes, tools/picture_take.py's scaled shapes) looked at what
a set of instructions carries of its own, one block behind the other, and reported nothing for a set that still
carried the key hint pills of tbuttons.svg and the preset browser's cursor and slider. It missed them for three
reasons (analysis/picture_maker_study.md, section 12): a stock tag that is split between the unit's bytes and ours
never equals a whole tag; the shapes it knew are path data and rows of points, so a line, a rectangle, a circle and an
ellipse were not looked at; and a long stretch was only looked for in the stock pictures a set already takes from.

This measure looks at the picture the unit makes, and at every byte of it knows where it comes from: the unit's own
stock pictures (a `c` or an `m` line, or the stock picture a set of the first kind starts from), or the pack. It then
finds what the made picture has of ANY of the stock pictures, and counts the bytes of that which are the pack's:

  tags        a whole tag that a stock picture has as it is (its white space as single spaces);
  shapes      path data or a row of points that a stock picture has, as it is or without its first point;
  scaled      path data or a row of points that is a stock shape at another size or place (tools/picture_take.py);
  primitives  a line, a rectangle, a circle, an ellipse or a row of points whose numbers are a stock picture's;
  stretches   RUN bytes or more in a row that a stock picture has as they are.

A byte is counted once however many of the five find it: `all` is the pack's bytes that any of them found. A picture
a pack carries whole is the pack's in every byte. White space is not counted: where a tool wrote a space for a stock
picture's line break, the tag around it is still the unit's.

A stretch counts when it holds three whole numbers of geometry or more (a coordinate, a size, a radius, path data, a
row of points, a matrix). With fewer it is words of the picture format, which any two pictures drawn in this
vocabulary share (an element's and its attributes' names, `fill="none"`, a colour of the palette, a line's width
and cap), at most with one coordinate or one size that happens to be the same: counted apart, as `words`, and not as
a stock picture's. Reads only; standard library only.
"""

import argparse
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack_build  # noqa: E402
import picture_delta  # noqa: E402
import picture_make  # noqa: E402
import picture_map  # noqa: E402
import picture_take  # noqa: E402
import stock_search  # noqa: E402

RUN = picture_take.RUN      # a stretch of this many bytes that a stock picture has as it is
SOURCES = picture_take.SOURCES
KINDS = ("tags", "shapes", "scaled", "primitives", "stretches")      # what counts as a stock picture's
WORDS = "words"            # counted apart: a long stretch with fewer than NUMBERS numbers of geometry in it
NUMBERS = 3                 # one number is a coordinate, two are a point or a size; three begin to say a shape
SPACE = b" \t\r\n"            # white space is nobody's: a made picture's own space inside a stock tag is not counted
GEOMETRY_NAMES = (b"x1", b"y1", b"x2", b"y2", b"x", b"y", b"width", b"height", b"cx", b"cy", b"r", b"rx", b"ry", b"points", b"d", b"transform", b"viewBox")
ANY_TAG = re.compile(rb"<[A-Za-z][^<>]*>")
PRIMITIVE = re.compile(rb"<(line|rect|circle|ellipse|polyline|polygon)\b[^<>]*>")
ATTRIBUTE = re.compile(rb'\s([A-Za-z_:][-A-Za-z0-9_:.]*)\s*=\s*"([^"]*)"')
NUMBER = re.compile(rb"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")
GEOMETRY_OF = {b"line": (b"x1", b"y1", b"x2", b"y2"), b"rect": (b"x", b"y", b"width", b"height", b"rx", b"ry"),
               b"circle": (b"cx", b"cy", b"r"), b"ellipse": (b"cx", b"cy", b"rx", b"ry"), b"polyline": (b"points",), b"polygon": (b"points",)}
SHAPE_VALUE = re.compile(rb'\s(?:d|points)="([^"]*)"')


def walk(data, read_stock, most=SOURCES):
    """(the picture the unit makes, a mask of its bytes: 1 where a byte is the pack's own, 0 where it is the unit's).
    `data` is a set of instructions of either kind, or a picture carried whole, which is the pack's in every byte."""
    if not picture_make.is_instructions(data):
        return data, bytearray(b"\x01" * len(data))
    made = picture_make.make(data, read_stock, None, most)                    # refused here as the unit refuses it
    if data[:len(picture_make.MARK2)] == picture_make.MARK2:
        lines = picture_make._Lines(data, picture_make.MARK2)
        lines.size()
        sources, mask = [], bytearray()
        while lines.more():
            words = lines.line()
            if words[0] == b"from":
                sources.append(read_stock(words[1].decode("ascii")))
            elif words[0] == b"c":
                mask += bytes(int(words[3]))
            elif words[0] == b"m":
                source, offset, length = sources[int(words[1])], int(words[2]), int(words[3])
                mask += bytes(len(picture_map.map_range(source[offset:offset + length], int(words[4]), int(words[5]), int(words[6]))))
            else:                                                             # a: the pack's own bytes
                count = int(words[1])
                mask += b"\x01" * count
                lines.at += count + 1
    else:
        lines = picture_make._Lines(data)
        lines.size()
        work, mask = None, None
        while lines.more():
            words = lines.line()
            if words[0] == b"from":
                work = bytearray(read_stock(words[1].decode("ascii")))
                mask = bytearray(len(work))
            else:                                                             # add end: before the closing tag
                count = int(words[2])
                at = bytes(work).rfind(picture_make.CLOSING)
                work[at:at] = data[lines.at:lines.at + count]
                mask[at:at] = b"\x01" * count
                lines.at += count + 1
    if len(mask) != len(made):
        raise ValueError("the walk does not follow the maker: %d bytes for a picture of %d" % (len(mask), len(made)))
    return made, mask


def numbers_of(value):
    return tuple(round(float(n), 6) for n in NUMBER.findall(value))


def primitives_of(text):
    """[(the key of a primitive element: its tag and its geometry's numbers, the spans of its geometry attributes)]."""
    out = []
    for found in PRIMITIVE.finditer(text):
        tag = found.group(1)
        values, spans = {}, []
        for attribute in ATTRIBUTE.finditer(found.group()):
            if attribute.group(1) in GEOMETRY_OF[tag]:
                values[attribute.group(1)] = attribute.group(2)
                spans.append((found.start() + attribute.start() + 1, found.start() + attribute.end()))
        try:
            if tag in (b"polyline", b"polygon"):
                key = (b"points",) + numbers_of(values.get(b"points", b""))
                if len(key) < 5:                                               # two points at least
                    continue
            else:
                key = (tag,) + tuple(numbers_of(values.get(name, b"0"))[:1] or (0.0,) for name in GEOMETRY_OF[tag])
        except ValueError:
            continue
        out.append((key, spans))
    return out


def geometry_numbers(text):
    """[(start, end)] of every number in a geometry attribute of any tag: a coordinate, a size, a radius, path data,
    a row of points, a matrix. A colour, a line's width and its cap are not geometry."""
    out = []
    for tag in ANY_TAG.finditer(text):
        for attribute in ATTRIBUTE.finditer(tag.group()):
            if attribute.group(1) in GEOMETRY_NAMES:
                base = tag.start() + attribute.start(2)
                out.extend((base + number.start(), base + number.end()) for number in NUMBER.finditer(attribute.group(2)))
    return out


def with_geometry(made, mask, stretches):
    """The stretches split: (those in which NUMBERS whole numbers of geometry or more are the pack's own bytes, each
    with what stands before and behind it, so that it is the same number in the stock picture; the others, which are
    words of the format, at most with a coordinate or a size that two pictures happen to share). A number the unit
    supplies is not counted: the pack does not carry it."""
    numbers = geometry_numbers(made)
    holding, words, k = [], [], 0
    for stretch in stretches:
        start, end = stretch[0], stretch[1]
        while k < len(numbers) and numbers[k][0] - 1 < start:
            k += 1
        j, whole = k, 0
        while j < len(numbers) and numbers[j][0] < end:
            if numbers[j][1] + 1 <= end and all(mask[numbers[j][0]:numbers[j][1]]):
                whole += 1
            j += 1
        (holding if whole >= NUMBERS else words).append(stretch)
    return holding, words


class Stock:
    """Every stock picture of a `content/display` folder, as what the five measures look for."""

    def __init__(self, folder):
        self.display = stock_search.Display(folder)
        self.files = list(self.display.files)
        self.data = {}
        for name in self.files:
            with open(os.path.join(folder, name), "rb") as f:
                self.data[name] = f.read()
        self.primitives, self.scaled = {}, {}
        for name in self.files:
            for key, _spans in primitives_of(self.data[name]):
                self.primitives.setdefault(key, name)
            for (letters, numbers), _first in picture_take._shapes(self.data[name].decode("latin-1")):
                self.scaled.setdefault((letters, len(numbers)), []).append((numbers, name))

    def read(self, name):
        return self.data.get(name)

    # each finder answers [(start, end, the stock picture)] in the made picture's text

    def tags(self, made):
        return [(start, end, self.display.tags[tag]) for start, end, tag in stock_search.tags_of(made)
                if len(tag) >= stock_search.PLAIN_TAG and tag in self.display.tags]

    def shapes(self, made):
        out = []
        for found in SHAPE_VALUE.finditer(made):
            shape = found.group(1)
            if len(shape) < stock_search.PLAIN_TAG:
                continue
            if shape in self.display.shapes:
                out.append((found.start(1), found.end(1), self.display.shapes[shape]))
            elif stock_search.moved(shape) in self.display.moved and len(stock_search.moved(shape)) >= stock_search.PLAIN_TAG:
                out.append((found.end(1) - len(stock_search.moved(shape)), found.end(1), self.display.moved[stock_search.moved(shape)]))
        return out

    def scaled_shapes(self, made):
        out = []
        for found in SHAPE_VALUE.finditer(made):
            value = found.group(1).decode("latin-1")
            try:
                if found.group()[1:2] == b"d":
                    shape = picture_take._shape(picture_delta.path_groups(value))
                else:
                    values = [float(v) for v in picture_delta.NUMBER.findall(value)]
                    shape = picture_take._shape([("M", values[:2]), ("L", values[2:])]) if len(values) >= 10 and len(values) % 2 == 0 else None
            except (KeyError, IndexError, ValueError, TypeError):  # the reader's ValueError: no path data, so no shape
                continue
            if not shape:
                continue
            letters, numbers = shape
            for other, name in self.scaled.get((letters, len(numbers)), ()):
                if all(abs(a - b) <= 0.01 for a, b in zip(numbers, other)):
                    out.append((found.start(1), found.end(1), name))
                    break
        return out

    def primitive_elements(self, made):
        out = []
        for key, spans in primitives_of(made):
            if key in self.primitives:
                out.extend((start, end, self.primitives[key]) for start, end in spans)
        return out

    def stretches(self, pictures, run=RUN):
        """For several made pictures at once, [(made, mask)]: for each, [(start, end, the stock picture)] of every
        stretch of `run` bytes or more that a stock picture has as it is and that holds a byte of the pack's."""
        index = {}
        for number, (made, mask) in enumerate(pictures):
            total, sums = 0, [0]
            for bit in mask:
                total += bit
                sums.append(total)
            for at in range(len(made) - run + 1):
                if sums[at + run] - sums[at]:
                    index.setdefault(made[at:at + run], []).append((number, at))
        covered = [dict() for _ in pictures]                                   # position of a stretch's first byte -> (end, name)
        for name in self.files:
            data = self.data[name]
            for k in range(len(data) - run + 1):
                hits = index.get(data[k:k + run])
                if hits:
                    for number, at in hits:
                        covered[number].setdefault(at, name)
        out = []
        for number, (made, _mask) in enumerate(pictures):
            found, start, end, name = [], None, None, None
            for at in sorted(covered[number]):
                if start is not None and at <= end:                            # the next gram overlaps or touches: one stretch
                    end = max(end, at + run)
                    continue
                if start is not None:
                    found.append((start, end, name))
                start, end, name = at, at + run, covered[number][at]
            if start is not None:
                found.append((start, end, name))
            out.append(found)
        return out


def coverage(stock, pictures, wanted, skip=(), run=RUN):
    """For several made pictures at once: for each, {stock picture: the count of `wanted` bytes that lie in a stretch of
    `run` bytes or more which that stock picture has as it is}. `pictures` is [made], `wanted` [a mask of the bytes
    asked about]; `skip` names stock pictures not to look at (per picture: a set)."""
    index = {}
    for number, (made, mask) in enumerate(zip(pictures, wanted)):
        total, sums = 0, [0]
        for bit in mask:
            total += bit
            sums.append(total)
        for at in range(len(made) - run + 1):
            if sums[at + run] - sums[at]:
                index.setdefault(made[at:at + run], []).append((number, at))
    out = [dict() for _ in pictures]
    for name in stock.files:
        data = stock.data[name]
        marks = {}
        for k in range(len(data) - run + 1):
            hits = index.get(data[k:k + run])
            if hits:
                for number, at in hits:
                    if name not in skip[number]:
                        marks.setdefault(number, set()).add(at)
        for number, starts in marks.items():
            mask, covered = wanted[number], set()
            for at in starts:
                covered.update(pos for pos in range(at, at + run) if mask[pos])
            if covered:
                out[number][name] = covered
    return out


def choose_sources(stock, pictures, most=SOURCES, largest=picture_make.LARGEST):
    """For [(name, made, mask, the stock pictures the set takes already)]: for each, the stock pictures to take
    besides, chosen one at a time by how many of the pack's bytes each lets the unit supply (the smaller file when two
    serve alike), until none serves any more, the set takes `most`, or what it holds at once would pass `largest`.
    Also the count of the pack's bytes that no chosen picture supplies though some stock picture has them."""
    chosen = [list(taken) for _name, _made, _mask, taken in pictures]
    wanted = [bytearray(mask) for _name, _made, mask, _taken in pictures]
    left = [0] * len(pictures)
    while True:
        cover = coverage(stock, [made for _name, made, _mask, _taken in pictures], wanted, [set() for _ in pictures])
        progress = False
        for number, by_stock in enumerate(cover):
            # what the pictures already taken supply is no longer asked about
            for name in chosen[number]:
                for pos in by_stock.pop(name, ()):
                    wanted[number][pos] = 0
            held = sum(len(stock.data[name]) for name in chosen[number])
            fits = {name: {pos for pos in covered if wanted[number][pos]} for name, covered in by_stock.items()
                    if held + len(stock.data[name]) <= largest}
            fits = {name: covered for name, covered in fits.items() if covered}
            if not fits or len(chosen[number]) >= most:
                left[number] = len({pos for covered in by_stock.values() for pos in covered if wanted[number][pos]})
                continue
            best = max(fits, key=lambda name: (len(fits[name]), -len(stock.data[name]), name))
            chosen[number].append(best)
            for pos in fits[best]:
                wanted[number][pos] = 0
            progress = True
        if not progress:
            return chosen, left


def judge(stock, pictures):
    """For [(name, made, mask)]: [(name, {kind: the pack's bytes it found, "all": ..., "bytes": ..., "own": ...},
    [(kind, start, end, the stock picture, the text)])], the findings in the order of the made picture."""
    stretches = stock.stretches([(made, mask) for _name, made, mask in pictures])
    out = []
    for (name, made, mask), stretch in zip(pictures, stretches):
        holding, words = with_geometry(made, mask, stretch)
        found = {"tags": stock.tags(made), "shapes": stock.shapes(made), "scaled": stock.scaled_shapes(made),
                 "primitives": stock.primitive_elements(made), "stretches": holding, WORDS: words}
        counts, flagged, findings = {"bytes": len(made), "own": sum(mask)}, bytearray(len(made)), []
        own = bytearray(1 if bit and made[at] not in SPACE else 0 for at, bit in enumerate(mask))      # the pack's bytes that are not white space
        counts[WORDS] = sum(sum(own[start:end]) for start, end, _source in words)
        for kind in KINDS:
            marked = bytearray(len(made))
            for start, end, source in found[kind]:
                if any(own[start:end]):
                    findings.append((kind, start, end, source, made[start:end]))
                    for at in range(start, end):
                        if own[at]:
                            marked[at] = flagged[at] = 1
            counts[kind] = sum(marked)
        counts["all"] = sum(flagged)
        out.append((name, counts, sorted(findings, key=lambda finding: (finding[1], finding[2]))))
    return out


def pictures_of_pack(path, read_stock, most=SOURCES):
    """[(the picture's name, made, mask)] for a pack file's pictures."""
    with open(path, "rb") as f:
        data = f.read()
    return [(name, ) + walk(text, read_stock, most) for name, text in pack_build.pictures_in(data)]


def measure_pack(stock, path, most=SOURCES):
    return judge(stock, pictures_of_pack(path, stock.read, most))


def total(results):
    out = {key: 0 for key in ("bytes", "own", "all", WORDS) + KINDS}
    for _name, counts, _findings in results:
        for key in out:
            out[key] += counts[key]
    return out


def line_of(counts):
    return ("%8d bytes made, %7d of them the pack's; of those a stock picture's: %5d in tags, %5d in shapes, %5d in shapes at another size, %5d in primitives, %5d in stretches of %d with geometry; %5d in all (and %6d in stretches that are words of the format)"
            % (counts["bytes"], counts["own"], counts["tags"], counts["shapes"], counts["scaled"], counts["primitives"], counts["stretches"], RUN, counts["all"], counts[WORDS]))


def cmd_measure(args):
    stock = Stock(args.display)
    paths = [path for pattern in args.pack for path in (sorted(glob.glob(pattern)) or [pattern])]
    everything = []
    for path in paths:
        results = measure_pack(stock, path, args.sources)
        if not results:
            continue
        print(os.path.basename(path))
        for name, counts, findings in results:
            print("  %-20s %s" % (name, line_of(counts)))
            for kind, start, end, source, text in findings[:args.show]:
                print("      %-10s at %6d, %4d bytes, %s: %s" % (kind, start, end - start, source, text[:90].decode("latin-1").replace("\n", " ")))
        everything.extend(results)
    print("%-22s %s" % ("together", line_of(total(everything))))
    return 0 if total(everything)["all"] == 0 else 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="command", required=True)
    one = sub.add_parser("measure", help="what the pictures of pack files have of the stock pictures, by where each byte comes from")
    one.add_argument("--display", required=True, help="the folder of stock pictures")
    one.add_argument("--sources", type=int, default=SOURCES, help="the stock pictures the kit holds for one set (8 from the kit of P3 on)")
    one.add_argument("--show", type=int, default=0, help="print this many findings a picture")
    one.add_argument("pack", nargs="+")
    args = ap.parse_args(argv)
    return cmd_measure(args)


if __name__ == "__main__":
    sys.exit(main())

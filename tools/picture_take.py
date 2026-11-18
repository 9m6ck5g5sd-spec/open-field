#!/usr/bin/env python3
"""The instructions for the pictures that are ours but for parts taken from stock pictures (pictures step P3).

  python -I tools/picture_take.py build --display "op-1field backup/content/display" --out-dir candidates/content-p3/display
  python -I tools/picture_take.py show --display "op-1field backup/content/display"

The sidechain's two pages, the train's page with the UFOs and the two preset screens are drawings of ours in which
some parts are Teenage Engineering's: every letter (a shape of the font sheet op_font_34.svg, scaled and placed),
the parameter card and its knob (param_popup.svg), the metronome (tempo.svg), the track digits 1 to 4 (tape.svg),
the meters and the whole flow drawing (signalflow.svg), the frame and clock of the save screen (save.svg). Until P3
the packs carried these pictures whole. Here each becomes a set of instructions of the second kind
(tools/picture_make.py): the parts of stock pictures are named by where they stand in the unit's own files, and
what the set carries is our drawing alone.

How a set is found. The artwork tool (tools/duck_svg.py, tools/fx_train_svg.py, tools/fx_presets_svg.py) is run as
it is, and what it writes must be the picture the pack carried, byte for byte. While it runs, every call of the
two shared steps that place a stock part (svg_geometry.transform_span and signalflow_svg_ext.shift_x) is written
down: what went in, the map, what came out. Then:

  1. what went into a step is laid beside the stock picture it was cut from (the longest stretch they share finds
     the place; an alignment finds what the tool took out or put in, an id, a colour). A step whose input shares
     no stretch of 24 bytes with any stock picture placed a drawing of ours and is left alone;
  2. wherever such a step's output stands in the finished picture, the picture is made there from the stock
     stretches mapped by the unit (an `m` line each) and the tool's own bytes between them (`a`);
  3. everywhere else, a stretch of 24 bytes or more that stands in a stock picture as it is, is taken from there
     (`c`), and the rest is text of ours (`a`).

What the unit then makes is held against the tool's picture part for part: the same parts, names and attributes,
every number within a hundredth of a pixel (the unit's mapper takes a scale in thousandths and writes three
decimals; a name shrunk to fit its box has a scale between two thousandths). One construction differs and is
normalised before the comparison: the track digits 1, 2 and 4 stand in stock's tape.svg inside a group with a
matrix of two different scales, which the tool worked into the numbers and the unit's mapper cannot (it has one
scale); they are taken as stock has them, inside a group of ours with that matrix moved to the digit's place,
which is how stock's own tape screen draws them.

Reads the stock pictures and the pack's pictures; standard library only.
"""

import argparse
import difflib
import os
import re
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import picture_delta  # noqa: E402
import picture_make  # noqa: E402
import picture_map  # noqa: E402

SOURCES = 8                 # the stock pictures a set of P3 takes stretches from, at most: the kit of P3 holds eight
RUN = 24                    # a stretch of this many bytes that a stock picture has as it is, is taken from the unit
BLOCK = 4                   # inside a placed part: a shared stretch shorter than this is written as ours
TOLERANCE = 0.01            # a number of the unit's picture against the tool's, in pixels
STROKE_SET, STROKE_STOCK = 'stroke-width="2"', 'stroke-width="1.4805"'     # tools/duck_svg.py big_digit
TAG = re.compile(rb"<[^<>]*>")
SPACES = re.compile(rb"\s+")
WORDS = re.compile(rb"\s+|\S+")
TAG_LEAST = 16              # a shorter tag is structure every picture has (tools/stock_search.py's PLAIN_TAG)
NESTED_LEAST = 40           # P3b: a placed part looked for inside a later step's input is this long at least


class Index:
    """The stock pictures, and where every stretch of RUN bytes of them stands."""

    def __init__(self, sources):
        self.sources = list(sources)                         # [(name, bytes)]
        self._tags = None
        self.where = {}
        for number, (_name, data) in enumerate(self.sources):
            for at in range(len(data) - RUN + 1):
                self.where.setdefault(data[at:at + RUN], []).append((number, at))

    def tags(self):
        """{a whole tag of a stock picture, its white space as single spaces: (picture, start, end)}, the first place."""
        if self._tags is None:
            self._tags = {}
            for number, (_name, data) in enumerate(self.sources):
                for found in TAG.finditer(data):
                    self._tags.setdefault(SPACES.sub(b" ", found.group()), (number, found.start(), found.end()))
        return self._tags

    def tagged(self, text):
        """`text` as pieces, like plain(), with one thing more (P3b): a whole tag that a stock picture of the set has,
        white space apart, is taken from the unit word by word, whatever its words' length, and only the white space
        that differs is ours. So a tag a tool cut out of a stock picture and joined again on one line is the unit's in
        every byte that says anything."""
        pieces, done = [], 0
        for found in TAG.finditer(text):
            raw = found.group()
            hit = self.tags().get(SPACES.sub(b" ", raw)) if len(raw) >= TAG_LEAST else None
            if hit is None:
                continue
            number, start, end = hit
            stock = self.sources[number][1][start:end]
            if stock == raw and len(raw) >= RUN:
                continue                                     # plain() takes it as one stretch
            pieces.extend(self.plain(text[done:found.start()]))
            ours, theirs = WORDS.findall(raw), list(WORDS.finditer(stock))
            if len(ours) != len(theirs):
                raise ValueError("a tag and the stock tag it equals do not split alike: %r" % raw[:60])
            begin = None
            for word, other in zip(ours, theirs):
                if word == other.group():
                    begin = start + other.start() if begin is None else begin
                    last = start + other.end()
                    continue
                if begin is not None:                        # white space of ours where the stock picture has another
                    pieces.append(("s", number, begin, last))
                    begin = None
                if word.strip():
                    raise ValueError("a tag and the stock tag it equals differ in more than white space: %r" % raw[:60])
                pieces.append(("l", word))
            if begin is not None:
                pieces.append(("s", number, begin, last))
            done = found.end()
        pieces.extend(self.plain(text[done:]))
        return pieces

    def longest(self, text, at):
        """The longest stretch of a stock picture that `text` has from `at`: (bytes, picture, place), or (0, 0, 0)."""
        best = (0, 0, 0)
        for number, place in self.where.get(text[at:at + RUN], ())[:64]:
            data = self.sources[number][1]
            size = RUN
            limit = min(len(text) - at, len(data) - place)
            while size < limit and text[at + size] == data[place + size]:
                size += 1
            if size > best[0]:
                best = (size, number, place)
        return best

    def plain(self, text):
        """`text` as pieces: every stretch of RUN bytes or more that a stock picture has as it is, and ours between."""
        pieces, literal, k = [], 0, 0
        while k < len(text):
            size, number, place = self.longest(text, k) if k + RUN <= len(text) else (0, 0, 0)
            if size >= RUN:
                if k > literal:
                    pieces.append(("l", text[literal:k]))
                pieces.append(("s", number, place, place + size))
                k += size
                literal = k
            else:
                k += 1
        if len(text) > literal:
            pieces.append(("l", text[literal:]))
        return pieces

    def origins(self, text):
        """Where in the stock pictures `text` may have been cut from: up to eight (picture, lowest place, highest
        place) of its beginning, those that most of its stretches of RUN bytes point at."""
        votes = {}
        for at in range(len(text) - RUN + 1):
            hits = self.where.get(text[at:at + RUN], ())
            for number, place in hits:                       # a stretch many places have (a tag's words) counts little
                key = (number, (place - at) >> 8)
                low, high, count = votes.get(key, (place - at, place - at, 0.0))
                votes[key] = (min(low, place - at), max(high, place - at), count + 1.0 / len(hits))
        best = sorted(votes.items(), key=lambda item: (-item[1][2], item[0]))[:8]
        return [(key[0], low, high) for key, (low, high, _count) in best]

    def aligned(self, text):
        """`text`, a part cut from a stock picture and changed by a tool (ids out, a colour set), as pieces: stretches
        of that stock picture and bytes of ours between them: every way found, the one of fewest pieces first. None
        when it shares no stretch of RUN bytes with a stock picture, or when less than half of it is a stock picture's:
        then it is a drawing of ours."""
        ways = []
        for number, first, last in self.origins(text):
            data = self.sources[number][1]
            slack = len(text) + 400
            low = max(0, first - slack)
            high = min(len(data), last + len(text) + slack)
            window = data[low:high]
            matcher = difflib.SequenceMatcher(None, window, text, autojunk=False)
            pieces, done = [], 0
            for i, j, n in matcher.get_matching_blocks():
                if n < BLOCK:
                    continue
                if j > done:
                    pieces.append(("l", text[done:j]))
                pieces.append(("s", number, low + i, low + i + n))
                done = j + n
            if done < len(text):
                pieces.append(("l", text[done:]))
            shared = sum(piece[3] - piece[2] for piece in pieces if piece[0] == "s")
            if 2 * shared >= len(text) and pieces not in [way[2] for way in ways]:
                ways.append((len(pieces), -shared, pieces))
        if not ways:
            return None                                      # a drawing of ours that has a tag's words in common with stock's
        return [way[2] for way in sorted(ways, key=lambda way: way[:2])]


class Record:
    def __init__(self, seg, ax, bx, ay, by, out):
        self.seg, self.ax, self.bx, self.ay, self.by, self.out = seg, ax, bx, ay, by, out


class Recorder:
    """The two shared steps that place a stock part, written down while an artwork tool runs."""

    def __init__(self, nested=False):
        self.records = []
        self.by_out = {}
        self.nested = nested                                 # P3b: a part placed before, found inside a later step's input

    def note(self, seg, ax, bx, ay, by, out):
        earlier = self.by_out.get(seg)
        if earlier is not None:                              # a part placed twice: the two maps are one map of the first input
            seg, ax, bx, ay, by = earlier.seg, ax * earlier.ax, ax * earlier.bx + bx, ay * earlier.ay, ay * earlier.by + by
        record = Record(seg, ax, bx, ay, by, out)
        self.records.append(record)
        self.by_out.setdefault(out, record)

    def __enter__(self):
        import signalflow_svg_ext as sf
        import svg_geometry as g
        self._g, self._sf = g, sf
        self._span, self._shift = g.transform_span, sf.shift_x

        def transform_span(text, start, end, affine):
            out = self._span(text, start, end, affine)
            seg, placed_as = text[start:end], out[start:len(out) - (len(text) - end)]
            self.note(seg, affine.ax, affine.bx, affine.ay, affine.by, placed_as)
            if self.nested:
                # A drawing of ours that holds a part placed before (the UFOs' emblem of the element kind holds the
                # letter G, placed when the character was drawn) is scaled as one: the part inside it is then the
                # first input mapped twice, and is written down as that, so that the unit makes it from its own file.
                for inner in [key for key in self.by_out if NESTED_LEAST <= len(key) < len(seg)]:
                    at = seg.find(inner)
                    while at >= 0:
                        alone = self._span(seg, at, at + len(inner), affine)
                        alone = alone[at:len(alone) - (len(seg) - at - len(inner))]
                        if alone != inner and alone in placed_as:
                            self.note(inner, affine.ax, affine.bx, affine.ay, affine.by, alone)
                        at = seg.find(inner, at + 1)
            return out

        def shift_x(segment, dx):
            out = self._shift(segment, dx)
            self.note(segment, 1.0, float(dx), 1.0, 0.0, out)
            return out

        g.transform_span, sf.shift_x = transform_span, shift_x
        return self

    def __exit__(self, *_exc):
        self._g.transform_span, self._sf.shift_x = self._span, self._shift
        return False


def _b(text):
    return text.encode("utf-8")


def _thousandths(value):
    return int(round(value * 1000))


def _box_centre(seg):
    import duck_svg
    points = duck_svg.element_points(seg)
    if not points:
        return 0.0, 0.0
    xs, ys = [p[0] for p in points], [p[1] for p in points]
    return (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0


def placed(index, record):
    """The pieces that make a placed part on the unit, or None when the part is a drawing of ours. Also what kind."""
    seg = record.seg
    if record.ax != record.ay:
        # the digits of tape.svg: stock's own elements as they are, in a group of ours with stock's matrix moved
        if seg.count(STROKE_SET) < 1 or STROKE_STOCK in seg:
            raise ValueError("a part with two scales that is not a track digit of tape.svg")
        ways = index.aligned(_b(seg.replace(STROKE_SET, STROKE_STOCK)))
        if ways is None:
            raise ValueError("a part with two scales shares nothing with a stock picture")
        pieces = ways[0]
        if any(piece[0] == "l" and re.search(rb"[0-9]", re.sub(rb"#?[0-9A-Fa-f]{6}", b"", piece[1])) for piece in pieces):   # a colour may be ours
            raise ValueError("a track digit is not stock's own numbers: %r" % seg[:80])
        matrix = '<g transform="matrix(%s,0,0,%s,%s,%s)">' % tuple(picture_delta_number(v) for v in (record.ax, record.ay, record.bx, record.by))
        return [("l", _b(matrix))] + pieces + [("l", b"</g>")], "matrix"
    ways = index.aligned(_b(seg))
    if ways is None:
        return None, "ours"
    scale = _thousandths(record.ax)
    cx, cy = (0.0, 0.0) if abs(record.ax * 1000 - scale) < 1e-6 else _box_centre(seg)
    dx = _thousandths(record.bx + (record.ax - scale / 1000.0) * cx)
    dy = _thousandths(record.by + (record.ay - scale / 1000.0) * cy)
    whole = picture_map.map_range(_b(seg), scale, dx, dy)
    for pieces in ways:                                      # a way that has a number of ours where geometry is, is not one
        out, made = [], bytearray()
        for piece in pieces:
            if piece[0] == "l":
                out.append(piece)
                made += piece[1]
                continue
            data = index.sources[piece[1]][1][piece[2]:piece[3]]
            mapped = picture_map.map_range(data, scale, dx, dy)
            if mapped == data and len(data) < RUN:
                out.append(("l", data))                      # a few bytes of a tag's own words, no geometry in them
            else:
                out.append(("m", piece[1], piece[2], piece[3], scale, dx, dy))
            made += mapped
        if bytes(made) == whole:
            return out, "mapped"
    raise ValueError("a placed part does not come out of its stretches as it does mapped whole: %r" % seg[:80])


def picture_delta_number(value):
    text = ("%.3f" % value).rstrip("0").rstrip(".")
    return "0" if text in ("-0", "") else text


def pieces_for(text, index, records, tags=False):
    """The finished picture `text` (bytes) as pieces, and a count of what was done. `tags` (P3b): whole stock tags are
    taken from the unit word by word (Index.tagged)."""
    counts = {"placed": 0, "matrix": 0, "ours": 0, "not in the picture": 0}
    plans = {}
    for record in records:
        out = _b(record.out)
        if out in plans or not out:
            continue
        pieces, kind = placed(index, record)
        plans[out] = (pieces, kind)
    taken = []                                               # (start, end, pieces)
    used = bytearray(len(text))
    for out in sorted(plans, key=len, reverse=True):
        pieces, kind = plans[out]
        if pieces is None:
            counts["ours"] += 1
            continue
        at, found = text.find(out), 0
        while at >= 0:
            if not any(used[at:at + len(out)]):
                used[at:at + len(out)] = b"\x01" * len(out)
                taken.append((at, at + len(out), pieces))
                counts["placed" if kind == "mapped" else "matrix"] += 1
                found += 1
            at = text.find(out, at + 1)
        if not found:
            counts["not in the picture"] += 1
    taken.sort()
    pieces, at = [], 0

    def plain(start, end):
        pieces.extend(index.tagged(text[start:end]) if tags else index.plain(text[start:end]))

    for start, end, part in taken:
        plain(at, start)
        pieces.extend(part)
        at = end
    plain(at, len(text))
    # texts of ours that came to stand side by side (a tag's words around a colour of ours) are looked at once more
    # as one text: what they have of a stock picture in a row, is taken from the unit as well
    out = []
    for piece in picture_delta.Doc(index.sources, pieces).joined():
        out.extend((index.tagged(piece[1]) if tags else index.plain(piece[1])) if piece[0] == "l" else [piece])
    return out, counts


def instructions(doc):
    """The set of instructions of the second kind for a Doc, with up to SOURCES stock pictures."""
    pieces = doc.joined()
    used = sorted({piece[1] for piece in pieces if piece[0] != "l"})
    if len(used) > SOURCES:
        raise ValueError("more stock pictures than a set of instructions takes")
    number = {source: k for k, source in enumerate(used)}
    made = doc.render()
    out = bytearray(picture_make.MARK2 + b"size %d\n" % len(made))
    for source in used:
        name, data = doc.sources[source]
        out += b"from %s %d %08x\n" % (name.encode("ascii"), len(data), zlib.crc32(data) & 0xFFFFFFFF)
    for piece in pieces:
        if piece[0] == "l":
            out += b"a %d\n" % len(piece[1]) + piece[1] + b"\n"
        elif piece[0] == "s":
            out += b"c %d %d %d\n" % (number[piece[1]], piece[2], piece[3] - piece[2])
        else:
            out += b"m %d %d %d %d %d %d\n" % (number[piece[1]], piece[2], piece[3] - piece[2], piece[4], piece[5], piece[6])
    stock = {name: data for name, data in doc.sources}
    if picture_make.make(bytes(out), stock.get, most=SOURCES) != made:
        raise ValueError("the instructions do not make the picture")
    return bytes(out)


MATRIX_GROUP = re.compile(r'<g transform="matrix\(([-\d.]+),0,0,([-\d.]+),([-\d.]+),([-\d.]+)\)">(.*?)</g>', re.S)


def normalised(made):
    """The unit's picture with the digits' matrix groups worked into the numbers, as the tool wrote them."""
    import svg_geometry as g

    def bake(found):
        ax, ay, bx, by = (float(found.group(k)) for k in (1, 2, 3, 4))
        inner = found.group(5)
        if "<g" in inner or STROKE_STOCK not in inner:
            raise ValueError("a matrix group that is not a track digit")
        inner = inner.replace(STROKE_STOCK, STROKE_SET)
        return g.transform_span(inner, 0, len(inner), g.Affine(ax, bx, ay, by))

    return MATRIX_GROUP.sub(bake, made.decode("utf-8")).encode("utf-8")


def _shape(groups):
    """A path's commands and its numbers freed of place and size: an absolute coordinate counted from the path's first
    point, every number divided by the largest. None for a shape of fewer than eight numbers: a stem, a box."""
    if not groups or groups[0][0] not in ("M", "m") or len(groups[0][1]) != 2:
        return None
    x0, y0 = groups[0][1]
    letters, numbers = [], []
    for command, values in groups[1:]:
        letters.append(command)
        if command == "H":
            numbers += [v - x0 for v in values]
        elif command == "V":
            numbers += [v - y0 for v in values]
        elif command.isupper():
            numbers += [v - (x0 if k % 2 == 0 else y0) for k, v in enumerate(values)]
        else:
            numbers += values
    largest = max((abs(v) for v in numbers), default=0.0)
    if len(numbers) < 8 or largest == 0.0:
        return None
    return "".join(letters), [v / largest for v in numbers]


def _shapes(text):
    """[(the shape, its first bytes)] for every path and every line of several points in a picture's text."""
    out = []
    for d in re.findall(r'\sd="([^"]*)"', text):
        try:
            shape = _shape(picture_delta.path_groups(d))
        except (KeyError, IndexError, ValueError, TypeError):      # the reader's ValueError: no path data, so no shape
            continue
        if shape:
            out.append((shape, d[:60]))
    for points in re.findall(r'\spoints="([^"]*)"', text):
        values = [float(v) for v in picture_delta.NUMBER.findall(points)]
        if len(values) >= 10 and len(values) % 2 == 0:
            shape = _shape([("M", values[:2]), ("L", values[2:])])
            if shape:
                out.append((shape, points.strip()[:60]))
    return out


def scaled_stock_shapes(ours, sources):
    """The shapes in `ours` that are a shape of a stock picture at another size or place: the same commands and,
    freed of place and size, the same numbers within a hundredth. This is what the search of tags and shapes cannot
    see: a stock shape whose every number was changed. [(the shape's first bytes, the stock picture)]."""
    stock = {}
    for name, data in sources:
        for (letters, numbers), _first in _shapes(data.decode("latin-1")):
            stock.setdefault((letters, len(numbers)), []).append((numbers, name))
    found = []
    for (letters, numbers), first in _shapes(ours.decode("latin-1")):
        for other, name in stock.get((letters, len(numbers)), ()):
            if all(abs(a - b) <= 0.01 for a, b in zip(numbers, other)):
                found.append((first, name))
                break
    return found


# ---- the pictures ---------------------------------------------------------------------------------------------------

def _duck(tracks):
    def run(display):
        import duck_svg
        path = lambda name: os.path.join(display, name)      # noqa: E731
        return duck_svg.build(path("signalflow.svg"), path("op_font_34.svg"), path("tape.svg"), path("param_popup.svg"), path("tempo.svg"), tracks)
    return run


def _train(display):
    import fx_train_svg
    return fx_train_svg.build(os.path.join(display, "op_font_34.svg"), os.path.join(display, "param_popup.svg"))


def _presets(display):
    import fx_presets_svg
    return fx_presets_svg.build(os.path.join(display, "op_font_34.svg"))


def _save(display):
    import fx_presets_svg
    with open(os.path.join(display, "save.svg"), encoding="utf-8") as f:
        stock = f.read()
    return fx_presets_svg.build_save(os.path.join(display, "op_font_34.svg"), stock)


# name of the set: (the tool's run, the stock pictures it reads, the picture the packs carried until P3)
PICTURES = {
    "signalflow.svg": (_duck(8), ("signalflow.svg", "op_font_34.svg", "tape.svg", "param_popup.svg", "tempo.svg"), "candidates/content-h5b/display/signalflow.svg"),
    "signalflow4.svg": (_duck(4), ("signalflow.svg", "op_font_34.svg", "tape.svg", "param_popup.svg", "tempo.svg"), "candidates/content-j5g/display/signalflow4.svg"),
    "fxtrain.svg": (_train, ("op_font_34.svg", "param_popup.svg"), "candidates/content-i7/display/fxtrain.svg"),
    "fxpresets.svg": (_presets, ("op_font_34.svg",), "candidates/content-h5b/display/fxpresets.svg"),
    "fxsave.svg": (_save, ("save.svg", "op_font_34.svg"), "candidates/content-h5b/display/fxsave.svg"),
}


def _presets_b(display):
    import fx_presets_svg
    return fx_presets_svg.build(os.path.join(display, "op_font_34.svg"), stock_order=True)


# Step P3b (analysis/picture_maker_study.md, section 13): the three sets in which the measure by where each byte comes
# from (tools/picture_provenance.py) still found something of a stock picture. Each takes one or three stock pictures
# more, so that the unit supplies what the pack carried: the background every stock screen has (undo_popup.svg, the
# smallest picture that has it; power_popup.svg for the preset list, which has the file's head as well), the key hint
# pills (tbuttons.svg), the preset browser's cursor bar and slider (presetbrowser.svg). The preset list's tool writes
# the bar and the first pill as those pictures write them (fx_presets_svg.build with stock_order); the third name is
# then that tool's own picture, kept in candidates/content-p3b, where the two others are the pictures of P3.
PICTURES_B = {
    "fxtrain.svg": (_train, ("op_font_34.svg", "param_popup.svg", "undo_popup.svg"), "candidates/content-i7/display/fxtrain.svg"),
    "fxpresets.svg": (_presets_b, ("op_font_34.svg", "tbuttons.svg", "presetbrowser.svg", "power_popup.svg"), "candidates/content-p3b/display/fxpresets.svg"),
    "fxsave.svg": (_save, ("save.svg", "op_font_34.svg"), "candidates/content-h5b/display/fxsave.svg"),
}


def taken_b(name, display):
    """(the Doc, the tool's picture, the counts) for one of the pictures of P3b: the table above, a part placed before
    found inside a later step's input, and whole stock tags taken word by word."""
    return taken(name, display, PICTURES_B, nested=True, tags=True)


def taken(name, display, pictures=None, nested=False, tags=False):
    """(the Doc, the picture the packs carried, the counts) for one of the pictures of P3."""
    run, needs, carried_path = (pictures or PICTURES)[name]
    sources = []
    for needed in needs:
        with open(os.path.join(display, needed), "rb") as f:
            sources.append((needed, f.read()))
    with open(os.path.join(ROOT, carried_path), "rb") as f:
        carried = f.read()
    with Recorder(nested) as recorder:
        text = _b(run(display))
    if text != carried:
        raise ValueError("%s: the artwork tool does not write the picture the pack carries" % name)
    index = Index(sources)
    pieces, counts = pieces_for(text, index, recorder.records, tags)
    doc = picture_delta.Doc(sources, pieces)
    made = doc.render()
    differences, (count, other, largest) = picture_delta.same_picture(normalised(made), carried, tolerance=TOLERANCE)
    if differences:
        raise ValueError("%s is another picture than the pack carries: %s" % (name, "; ".join(differences[:5])))
    counts.update({"numbers": count, "numbers written otherwise": other, "largest difference": largest})
    return doc, carried, counts


def cmd_build(args, write=True):
    import stock_search
    display = stock_search.Display(args.display)
    every = []
    for stock_name in display.files:
        with open(os.path.join(args.display, stock_name), "rb") as f:
            every.append((stock_name, f.read()))
    for name in args.picture or sorted(PICTURES):
        doc, carried, counts = taken(name, args.display)
        data = instructions(doc)
        ours = doc.ours()
        measure = display.measure(ours)
        stock_bytes = measure["stock tags"] + measure["stock shapes"] + measure["stock shapes, moved"]
        scaled = scaled_stock_shapes(ours, every)
        scaled_before = scaled_stock_shapes(carried, every)
        before = display.measure(carried)
        pieces = doc.joined()
        print("%s: %d bytes of instructions for a picture of %d (%d carried until now): %d stretches kept, %d mapped, %d texts of ours in %d bytes"
              % (name, len(data), len(doc.render()), len(carried), sum(1 for p in pieces if p[0] == "s"), sum(1 for p in pieces if p[0] == "m"),
                 sum(1 for p in pieces if p[0] == "l"), len(ours)))
        print("    placed parts taken from the unit %d, the digits in their matrix %d, placed drawings of ours %d, steps whose result is not in the picture %d"
              % (counts["placed"], counts["matrix"], counts["ours"], counts["not in the picture"]))
        print("    against the tool's picture, part for part: %d numbers, %d written otherwise, the largest difference %.4f"
              % (counts["numbers"], counts["numbers written otherwise"], counts["largest difference"]))
        print("    stock's in what was carried: %d bytes of tags, %d of shapes, %d shapes at another size or place; in what the set carries: %d, %d and %d"
              % (before["stock tags"], before["stock shapes"] + before["stock shapes, moved"], len(scaled_before), measure["stock tags"],
                 measure["stock shapes"] + measure["stock shapes, moved"], len(scaled)))
        for d, source in scaled[:8]:
            print("        %s... (%s)" % (d, source))
        if stock_bytes or scaled:
            raise ValueError("%s: %d bytes and %d shapes of what the instructions carry are a stock picture's" % (name, stock_bytes, len(scaled)))
        if write:
            path = os.path.join(args.out_dir, name + ".make")
            if os.path.exists(path):
                raise ValueError("%s exists; this tool does not overwrite" % path)
            os.makedirs(args.out_dir, exist_ok=True)
            with open(path, "xb") as f:
                f.write(data)
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="command", required=True)
    for command, text in (("build", "write the instructions for each picture"), ("show", "say what would be written")):
        one = sub.add_parser(command, help=text)
        one.add_argument("--display", required=True, help="the folder of stock pictures")
        one.add_argument("--picture", action="append", help="one of the pictures; all of them when not given")
        if command == "build":
            one.add_argument("--out-dir", required=True)
    args = ap.parse_args(argv)
    try:
        return cmd_build(args, write=args.command == "build")
    except ValueError as e:
        print("error: %s" % e)
        return 2


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""The instructions of the second kind for the pictures a pack shows: how each is put together from stock pictures.

  python -I tools/picture_delta.py build --recipe "packs/p1/8 tracks.json" --display "op-1field backup/content/display" \\
         --out-dir candidates/content-p2/display
  python -I tools/picture_delta.py show --recipe "packs/p1/8 tracks.json" --display "op-1field backup/content/display"

A picture a pack shows in place of a stock picture is, with few exceptions, the stock picture changed: parts moved
or scaled, numbers set, parts doubled, text of ours put in. The artwork tools (tools/tape_svg_lanes.py and the
others) make such a picture as text on the PC, and until P2 the pack carried the result. Here the same changes are
made on a `Doc`, which remembers where every byte comes from: a stretch of a stock picture as it is, a stretch with
its geometry mapped (tools/picture_map.py), or text of ours. What comes out is a set of instructions of the second
kind (tools/picture_make.py), by which the unit puts the picture together from its own stock pictures: the pack
then holds offsets, lengths and text of ours, and no byte of a stock picture.

Each picture's steps below follow its artwork tool step by step, with the tool's own constants and functions where
they give a number or a text of ours. A picture is then held against the one the pack carried until now: byte for
byte where the tool rewrote geometry with tools/svg_geometry.py or not at all (the tape screen, the mixer, the four
tape types); the same picture part for part, every number within half a thousandth, where the tool had a rewriting
of its own (the signal flow screen, the help screen), since the unit has one way of writing a number.

`build` writes `<name>.make` for each picture of the recipe that has steps here and refuses to overwrite; `show`
prints what it would write. Reads the stock pictures and the pack's pictures; standard library only.
"""

import argparse
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import picture_make  # noqa: E402
import picture_map  # noqa: E402


def _text(data):
    return data.decode("latin-1")              # a byte a character: an offset in the text is an offset in the bytes


def _raw(text):
    return text.encode("latin-1") if isinstance(text, str) else bytes(text)


class Doc:
    """A picture in the making. Its pieces, in their order, are each a stretch of a stock picture as it is
    ("s", stock picture, from, to), such a stretch with its geometry mapped ("m", ..., scale, dx, dy) or text of ours
    ("l", bytes). Places are counted in the picture as it stands, as the artwork tools count them in their text."""

    def __init__(self, sources, pieces=None):
        self.sources = list(sources)            # [(name, bytes)]; the first is the picture the work starts from
        self.pieces = list(pieces) if pieces is not None else [("s", 0, 0, len(self.sources[0][1]))]
        self._made = {}
        self._text = None

    def bytes_of(self, piece):
        if piece[0] == "l":
            return piece[1]
        if piece not in self._made:
            data = self.sources[piece[1]][1][piece[2]:piece[3]]
            self._made[piece] = data if piece[0] == "s" else picture_map.map_range(data, piece[4], piece[5], piece[6])
        return self._made[piece]

    def render(self):
        return b"".join(self.bytes_of(piece) for piece in self.pieces)

    @property
    def text(self):
        if self._text is None:
            self._text = _text(self.render())
        return self._text

    def _cut(self, at):
        """The number of the piece that begins at `at`, a piece being cut in two for it where one runs across."""
        place = 0
        for number, piece in enumerate(self.pieces):
            if place == at:
                return number
            size = len(self.bytes_of(piece))
            if at < place + size:
                inside = at - place
                if piece[0] == "m":
                    raise ValueError("a mapped stretch cannot be cut: change it before it is mapped")
                if piece[0] == "s":
                    halves = [("s", piece[1], piece[2], piece[2] + inside), ("s", piece[1], piece[2] + inside, piece[3])]
                else:
                    halves = [("l", piece[1][:inside]), ("l", piece[1][inside:])]
                self.pieces[number:number + 1] = halves
                return number + 1
            place += size
        if place == at:
            return len(self.pieces)
        raise ValueError("a place behind the picture's end")

    def _as_pieces(self, new):
        if isinstance(new, (str, bytes, bytearray)):
            new = [new]
        out = []
        for item in new:
            if isinstance(item, (str, bytes, bytearray)):
                if len(item):
                    out.append(("l", _raw(item)))
            else:
                out.append(tuple(item))
        return out

    def take(self, start, end):
        """The pieces of a stretch of the picture as it stands: for a copy of it, or to put it somewhere else."""
        first = self._cut(start)
        last = self._cut(end)
        return list(self.pieces[first:last])

    def replace(self, start, end, new):
        first = self._cut(start)
        last = self._cut(end)
        self.pieces[first:last] = self._as_pieces(new)
        self._text = None

    def insert(self, at, new):
        self.replace(at, at, new)

    def sub(self, start, end):
        """A stretch of the picture as a Doc of its own, to be changed and put in somewhere as a copy."""
        return Doc(self.sources, self.take(start, end))

    def set_first(self, pattern, value, start=0, end=None, group=1):
        """The first match of `pattern` between start and end: its group replaced by `value`."""
        found = re.compile(pattern, re.S).search(self.text, start, len(self.text) if end is None else end)
        if not found:
            raise ValueError("nothing matches %s" % pattern)
        self.replace(found.start(group), found.end(group), value)

    def map(self, start, end, scale, dx, dy):
        """The stretch's stock bytes get their geometry mapped by the unit. Text of ours inside it stays as it is, so
        it may only stand inside a tag (an id), and the pieces must come out as the stretch mapped whole would."""
        first = self._cut(start)
        last = self._cut(end)
        whole = b"".join(self.bytes_of(piece) for piece in self.pieces[first:last])
        mapped = []
        for piece in self.pieces[first:last]:
            if piece[0] == "m":
                raise ValueError("a stretch is mapped twice")
            mapped.append(("m", piece[1], piece[2], piece[3], scale, dx, dy) if piece[0] == "s" else piece)
        self.pieces[first:last] = mapped
        self._text = None
        made = b"".join(self.bytes_of(piece) for piece in mapped)
        if made != picture_map.map_range(whole, scale, dx, dy):
            raise ValueError("pieces mapped one by one do not make the stretch mapped whole: text of ours stands where geometry is")

    def joined(self):
        """The pieces with neighbours joined: stretches that follow each other in their stock picture, texts of ours."""
        out = []
        for piece in self.pieces:
            if out and piece[0] == "l" and out[-1][0] == "l":
                out[-1] = ("l", out[-1][1] + piece[1])
            elif out and piece[0] == "s" and out[-1][0] == "s" and out[-1][1] == piece[1] and out[-1][3] == piece[2]:
                out[-1] = ("s", piece[1], out[-1][2], piece[3])
            elif piece[0] != "l" and piece[2] == piece[3]:
                continue
            else:
                out.append(piece)
        return out

    def instructions(self):
        """The set of instructions of the second kind that makes this picture."""
        pieces = self.joined()
        used = sorted({piece[1] for piece in pieces if piece[0] != "l"})
        if len(used) > picture_make.SOURCES:
            raise ValueError("more stock pictures than a set of instructions takes")
        number = {source: k for k, source in enumerate(used)}
        made = self.render()
        out = bytearray(picture_make.MARK2 + b"size %d\n" % len(made))
        for source in used:
            name, data = self.sources[source]
            out += b"from %s %d %08x\n" % (name.encode("ascii"), len(data), zlib.crc32(data) & 0xFFFFFFFF)
        for piece in pieces:
            if piece[0] == "l":
                out += b"a %d\n" % len(piece[1]) + piece[1] + b"\n"
            elif piece[0] == "s":
                out += b"c %d %d %d\n" % (number[piece[1]], piece[2], piece[3] - piece[2])
            else:
                out += b"m %d %d %d %d %d %d\n" % (number[piece[1]], piece[2], piece[3] - piece[2], piece[4], piece[5], piece[6])
        stock = {name: data for name, data in self.sources}
        if picture_make.make(bytes(out), stock.get) != made:
            raise ValueError("the instructions do not make the picture")
        return bytes(out)

    def ours(self):
        return b"".join(piece[1] for piece in self.pieces if piece[0] == "l")


def thousandths(value):
    """A length in pixels as whole thousandths, which it has to be."""
    whole = int(round(value * 1000))
    if abs(value * 1000 - whole) > 1e-6:
        raise ValueError("%r is no whole number of thousandths" % value)
    return whole


def element_span(text, ident):
    import svg_geometry
    return svg_geometry.element_span(text, ident)


# ---- the four tape types (tools/tape_type_svg_scale.py --scale 0.87 --about 240 0) --------------------------------

TYPE_SCALE, TYPE_ABOUT = 870, (240000, 0)       # 0.87 about the top centre of the screen


def tape_type(name, sources):
    doc = Doc([(name, sources[name])])
    root = re.search(r"<svg\b[^>]*>", doc.text, flags=re.S)
    dx = TYPE_ABOUT[0] - TYPE_SCALE * TYPE_ABOUT[0] // 1000
    dy = TYPE_ABOUT[1] - TYPE_SCALE * TYPE_ABOUT[1] // 1000
    doc.map(root.end(), len(doc.text), TYPE_SCALE, dx, dy)
    return doc


# ---- the tape screen (tools/tape_svg_lanes.py --lanes 8 --first-y 168 --spacing 7 --stroke 5 --bar-half 2.5 --digits
#      --loop-y 154 --half-pixel --icons-dy -20 --level-bottom 140) --------------------------------------------------

def tape_screen(name, sources):
    import math
    import tape_svg_lanes as tool
    lanes, first_y, spacing, stroke, bar_half, loop_y, icons_dy, level_bottom = 8, 168.0, 7.0, 5.0, 2.5, 154.0, -20.0, 140.0
    fmt, number = tool.fmt, tool.NUMBER
    offset = 0.5                                                        # --half-pixel
    top_of_lanes = first_y + offset - max(stroke / 2.0, bar_half)
    doc = Doc([(name, sources[name])])

    def block(identifier):
        return tool.group_block(doc.text, identifier)

    def set_in(span, attribute, value):
        """The first `attribute="number"` inside what span() finds now: its number replaced."""
        found = span()
        doc.set_first(r'\b%s="(%s)"' % (attribute, number), value, found.start(), found.end())

    # the four stock lane groups, each changed in place; trackrow2 stays the spacing's template
    for k, identifier in enumerate(tool.LANE_IDS):
        if identifier == "trackrow2":
            y, width = fmt(first_y + spacing + tool.TEMPLATE_NUDGE), fmt(stroke - 1)
        else:
            y, width = fmt(first_y + k * spacing + offset), fmt(stroke)
        for attribute, value in (("y1", y), ("y2", y), ("stroke-width", width)):
            set_in(lambda: block(identifier), attribute, value)
    # a copy of trackrow2 that is seen, TAPE_x5F_TR2, behind it
    found = block("trackrow2")
    copy = doc.sub(found.start(), found.end())
    copy.set_first(r'id="(trackrow2)"', "TAPE_x5F_TR2")
    label = copy.text.find(' inkscape:label="trackrow2"')
    if label >= 0:
        copy.replace(label, label + len(' inkscape:label="trackrow2"'), "")
    y = fmt(first_y + spacing + offset)
    for attribute, value in (("y1", y), ("y2", y), ("stroke-width", fmt(stroke))):
        copy.set_first(r'\b%s="(%s)"' % (attribute, number), value)
    doc.insert(found.end(), ["\n\t"] + copy.pieces)
    # lanes 5 to 8: copies of the fourth group behind it
    found = block(tool.LANE_IDS[3])
    extra = []
    for k in range(4, lanes):
        copy = doc.sub(found.start(), found.end())
        copy.set_first(r'id="(%s)"' % re.escape(tool.LANE_IDS[3]), "TAPE_x5F_TR%d" % (k + 1))
        y = fmt(first_y + k * spacing + offset)
        for attribute in ("y1", "y2"):
            copy.set_first(r'\b%s="(%s)"' % (attribute, number), y)
        extra += ["\n\t"] + copy.pieces
    doc.insert(block(tool.LANE_IDS[3]).end(), extra)

    def tag(pattern):
        found = re.search(pattern, doc.text)
        if not found:
            raise ValueError("nothing matches %s" % pattern)
        return found

    for identifier in ("track_inactive", "track_active"):
        y = fmt(first_y + tool.TEMPLATE_NUDGE)
        for attribute in ("y1", "y2"):
            set_in(lambda: tag(r'<line id="%s"[^>]*>' % identifier), attribute, y)
    ghost = tag(r'<line id="ghost_x5F_line"[^>]*/>')
    ghost_y = float(re.search(r'\by1="(%s)"' % number, ghost.group(0)).group(1))
    if ghost_y >= top_of_lanes - 1.0:
        new_y = fmt(math.floor(top_of_lanes - 2.0))
        for attribute in ("y1", "y2"):
            set_in(lambda: tag(r'<line id="ghost_x5F_line"[^>]*/>'), attribute, new_y)
    # the loop line, its markers, the ruler's ticks and the playhead's top
    y = fmt(loop_y)
    for identifier in ("loopin", "loopout"):
        set_in(lambda: tag(r'<circle id="%s"[^>]*>' % identifier), "cy", y)
    for attribute in ("y1", "y2"):
        set_in(lambda: tag(r'<line id="loop_line"[^>]*>'), attribute, y)
    shift = loop_y - tool.STOCK_LOOP_Y
    for identifier in tool.TICK_IDS:
        for attribute in ("y1", "y2"):
            now = tag(r'<line id="%s"[^>]*/>' % identifier)
            value = float(re.search(r'\b%s="(%s)"' % (attribute, number), now.group(0)).group(1))
            set_in(lambda: tag(r'<line id="%s"[^>]*/>' % identifier), attribute, fmt(value + shift))
    top = float(re.search(r'\by2="(%s)"' % number, tag(r'<line id="centerline"[^>]*>').group(0)).group(1))
    set_in(lambda: tag(r'<line id="centerline"[^>]*>'), "y2", fmt(top + shift))
    # the three source icons up; they are mapped, so nothing inside them is changed after this
    for identifier in tool.ICON_IDS:
        start, end = element_span(doc.text, identifier)
        doc.map(start, end, 1000, 0, thousandths(icons_dy))
    # the level line's lower end, the two dots, the warning
    y = fmt(level_bottom)
    for identifier in ("mixervol-bg", "mixervol"):
        set_in(lambda: tag(r'<line id="%s"[^>]*>' % identifier), "y1", y)
    for identifier in ("dot", "dot-pan"):
        set_in(lambda: tag(r'<circle id="%s"[^>]*>' % identifier), "cy", y)
    start, end = element_span(doc.text, "clipwarning")
    doc.map(start, end, 1000, 0, thousandths(level_bottom - tool.STOCK_LEVEL_BOTTOM))
    # the digits 5 to 8, ours, at the end of the trackno group: where the tool puts them
    before = doc.text
    after = tool.add_digits(before)
    at = next(k for k in range(len(before) + 1) if k == len(before) or before[k] != after[k])
    while after[at:at + len(after) - len(before)] + before[at:] != after[at:]:
        at -= 1
    doc.insert(at, after[at:at + len(after) - len(before)])
    if doc.text != after:
        raise ValueError("the digits did not go in as the tool puts them")
    return doc


# ---- the mixer (tools/mixer_svg_ext.py) --------------------------------------------------------------------------

def mixer_screen(name, sources):
    import mixer_svg_ext as tool
    doc = Doc([(name, sources[name])])
    # the ids of the four faint track numbers
    for ident, mark in tool.DIGIT_MARKS.items():
        text = doc.text
        if text.count(mark) != 1:
            raise ValueError("the mark of %s is not there once" % ident)
        tag_start = text.rfind("<", 0, text.find(mark))
        if ident == "trackno3":
            inner = text.rfind("<g>", 0, tag_start)
            outer = text.rfind("<g>", 0, inner)
            if inner < 0 or outer < 0 or text[outer:tag_start].count("<g>") != 2:
                raise ValueError("the 3 is not in the pair of groups expected")
            doc.insert(outer + 2, ' id="%s"' % ident)
        else:
            doc.insert(tag_start + re.match(r"<\w+", text[tag_start:]).end(), ' id="%s"' % ident)
    # the digits 5 to 8, ours, each behind the stock number of its column
    for ident in ("trackno5", "trackno6", "trackno7", "trackno8"):
        text = doc.text
        holder = [e for e in tool.walk(tool.parse(text)) if tool.attr(e, "id") == tool.DIGIT_AFTER[ident]]
        if len(holder) != 1:
            raise ValueError("%s is not there once" % tool.DIGIT_AFTER[ident])
        el = holder[0]
        indent = text[text.rfind("\n", 0, el.start) + 1:el.start]
        paths = [tool.scaled_digit(p, tool.DIGIT_CENTRES[ident]) for p in tool.TAPE_DIGITS[ident]]
        if len(paths) == 1:
            new = '\n%s<path id="%s" display="none" %s d="%s"/>' % (indent, ident, tool.DIGIT_STYLE, paths[0])
        else:
            new = '\n%s<g id="%s" display="none">' % (indent, ident)
            for d in paths:
                new += '\n%s\t<path %s d="%s"/>' % (indent, tool.DIGIT_STYLE, d)
            new += '\n%s</g>' % indent
        doc.insert(el.end, new)
    # columns 3 and 4 into groups of their own at the picture's end
    text = doc.text
    root = tool.parse(text)
    moved = {2: [], 3: []}
    static = [e for e in tool.walk(root) if tool.attr(e, "id") == "static"]
    if len(static) != 1:
        raise ValueError("the static group is not there once")
    candidates = list(static[0].children)
    for ident in ("lev3", "lev4", "panbutton3", "panbutton4_1_", "pan3", "pan4", "vu3", "vu4", "vucirc3_1_", "vucirc4"):
        found = [e for e in tool.walk(root) if tool.attr(e, "id") == ident]
        if len(found) != 1:
            raise ValueError("%s is not there once" % ident)
        candidates.append(found[0])
    panmeters = [e for e in tool.walk(root) if tool.attr(e, "id") == "panmeters"][0]
    for child in panmeters.children:
        if child.name == "circle" and tool.attr(child, "id") is None:
            candidates.append(child)
    for el in candidates:
        x = tool.first_x(text, el)
        if x is None:
            continue
        column = int(x // tool.COLUMN_WIDTH)
        if column in moved:
            moved[column].append(el)
    spans = sorted(((e.start, e.end, column) for column in moved for e in moved[column]), reverse=True)
    blocks = {2: [], 3: []}
    for start, end, column in spans:                     # from the end, so that the places before stay as they are
        pieces = doc.take(start, end)
        text = doc.text
        line_start = text.rfind("\n", 0, start) + 1
        if text[line_start:start].strip() == "":
            start = line_start
        doc.replace(start, end, "")
        blocks[column].insert(0, pieces)
    groups = []
    for column in (2, 3):
        groups.append('<g id="col%d">\n' % (column + 1))
        for pieces in blocks[column]:
            groups += ["\t"] + pieces + ["\n"]
        groups.append("</g>\n")
    doc.insert(doc.text.rfind("</svg>"), groups)
    return doc


# ---- the signal flow screen with eight meters (tools/signalflow_svg_ext.py --meters 8 --centred) -------------------

def signalflow_screen(name, sources):
    import signalflow_svg_ext as tool
    count, centre, step = 8, tool.CENTRE, tool.STEP
    doc = Doc([(name, sources[name])])
    targets = tool.meter_targets(count, centre, step)
    originals = [doc.sub(s, e) for s, e in tool.meter_elements(doc.text, 4)]        # meter 4 as stock draws it, for the copies
    moves = []
    for k in (1, 2, 3, 4):
        dx = thousandths(round(targets[k - 1] - tool.STOCK_X[k - 1], 6))
        moves += [(s, e, dx) for s, e in tool.meter_elements(doc.text, k)]
    for s, e, dx in sorted(moves, reverse=True):                                    # from the end
        doc.map(s, e, 1000, dx, 0)
    extra = []
    for k in range(5, count + 1):
        dx = thousandths(round(targets[k - 1] - tool.STOCK_X[3], 6))
        for prefix, original in zip(("tapein", "tapevu", "tapevubase"), originals[:3]):
            copy = Doc(doc.sources, original.pieces)
            if copy.text.count('id="%s4' % prefix) != 1:
                raise ValueError("the id %s4 is not once in the part copied" % prefix)
            copy.set_first(r'id="(%s4[^"]*)"' % prefix, "%s%d" % (prefix, k))
            copy.map(0, len(copy.text), 1000, dx, 0)
            extra += [" "] + copy.pieces
        for original in originals[3:]:
            copy = Doc(doc.sources, original.pieces)
            if 'id="' in copy.text:
                raise ValueError("an arc of meter 4 has an id")
            copy.map(0, len(copy.text), 1000, dx, 0)
            extra += [" "] + copy.pieces
    start, end = tool.element_span(doc.text, 'id="tapevubase4')
    doc.insert(end, extra)
    layout = tool.bracket_layout(targets, centre)
    for k in (0, 1, 2, 3):                                                          # the stock brackets keep their parts and get paths of ours
        s, e = tool.element_span(doc.text, 'id="tape%d_' % k)
        if doc.text[s:e].count(' d="') != 1:
            raise ValueError("bracket tape%d is not one path" % k)
        doc.set_first(r'\bd="([^"]*)"', tool.bracket_d(*layout[k]), s, e)
    s, e = tool.element_span(doc.text, 'id="tape3_')
    doc.insert(e, " " + " ".join(tool.bracket(k, *layout[k]) for k in range(4, count)))
    s, e = tool.element_span(doc.text, 'id="mixerwarn')
    doc.map(s, e, 1000, thousandths(round(targets[-1] - tool.STOCK_X[3], 6)), 0)
    return doc


# ---- the help screen with eight boxes and the tape gauge (tools/help_svg_ext.py --count 8 --font) ------------------

def help_screen(name, sources):
    import help_svg_ext as tool
    count = 8
    font_name = "op_font_34.svg"
    doc = Doc([(name, sources[name]), (font_name, sources[font_name])])
    font = Doc(doc.sources, [("s", 1, 0, len(sources[font_name]))])
    xs = tool.positions(count)
    for k, gid in ((1, "t2tape"), (2, "t3tape"), (3, "t4tape")):                    # boxes 2 to 4 to their new places
        start, end = tool.group_span(doc.text, gid)
        old_x = float(re.search(r'<rect x="([\d.]+)"', doc.text[start:end]).group(1))
        doc.map(start, end, 1000, int(round((xs[k] - old_x) * 1000)), 0)
    digits = _text(sources["#digits"])
    new = ""
    for k in range(4, count):                                                       # the new boxes, ours, each dim and lit
        paths = [tool.scale_path(d, xs[k], tool.Y_ROW) for d in re.findall(r'\bd="([^"]+)"', digits[slice(*tool.group_or_path_span(digits, "track%d" % (k + 1)))])]
        new += tool.box_group("t%dtapeoff" % (k + 1), xs[k], tool.DIM, paths) + tool.box_group("t%dtapeon" % (k + 1), xs[k], tool.LIT, paths)
    start, end = tool.group_span(doc.text, "t2tape")
    doc.insert(end, " " + new)

    def glyph(gid, colour):
        """A glyph of the font sheet as the tool takes it: its ids out, its strokes in our colour, the colon's guide
        box out. The stretches come from the font sheet on the unit; what is ours is the colour."""
        start, end = tool.element_span(font.text, gid)
        part = font.sub(start, end)
        while True:
            found = re.search(r'\s*id="[^"]*"', part.text)
            if not found:
                break
            part.replace(found.start(), found.end(), "")
        while True:
            found = re.search(r'stroke="(#000000)"', part.text)
            if not found:
                break
            part.replace(found.start(1), found.end(1), colour)
        while True:
            found = re.search(r'<rect [^>]*stroke="#FFFF00"[^>]*/>', part.text)
            if not found:
                break
            part.replace(found.start(), found.end(), "")
        return part.pieces

    # the gauge: the tool's own text, with every glyph it holds put back as the stretches it came from
    whole = tool.gauge_group(_text(sources[font_name]))
    gauge, at = [], 0
    x = tool.GAUGE_DIGIT_X
    dy = tool.GAUGE_DIGIT_Y - tool.GAUGE_FONT_DIGIT_TOP
    wanted = []
    for position in range(4):
        if position == 2:
            gid, left, top = tool.GAUGE_FONT_COLON
            wanted.append(('<g id="gaugecolon" transform="translate(%s,%s)">' % (tool.num(x - left), tool.num(tool.GAUGE_DIGIT_Y - top)), gid))
            x += tool.GAUGE_COLON_ADVANCE
        for digit, (gid, left) in enumerate(tool.GAUGE_FONT_DIGITS):
            wanted.append(('<g id="gauged%d%d" transform="translate(%s,%s)">' % (position, digit, tool.num(x - left), tool.num(dy)), gid))
        x += tool.GAUGE_DIGIT_ADVANCE
    for opening, gid in wanted:
        start = whole.index(opening, at) + len(opening)
        taken = tool.glyph(font.text, gid, tool.LIT)
        if whole[start:start + len(taken)] != taken:
            raise ValueError("the gauge does not hold the glyph %s where it is looked for" % gid)
        pieces = glyph(gid, tool.LIT)
        if _text(b"".join(font.bytes_of(piece) for piece in pieces)) != taken:
            raise ValueError("the glyph %s does not come out of its stretches as the tool takes it" % gid)
        gauge += [whole[at:start]] + pieces
        at = start + len(taken)
    gauge.append(whole[at:])
    start, end = tool.group_span(doc.text, "t%dtapeon" % count)
    doc.insert(end, [" "] + gauge)
    return doc


STEPS = {
    "tape.svg": tape_screen,
    "mixer.svg": mixer_screen,
    "signalflow.svg": signalflow_screen,
    "help.svg": help_screen,
    "tape_4track.svg": tape_type,
    "tape_vintage.svg": tape_type,
    "tape_cassette.svg": tape_type,
    "tape_minidisk.svg": tape_type,
}
EXACT = ("tape.svg", "mixer.svg", "tape_4track.svg", "tape_vintage.svg", "tape_cassette.svg", "tape_minidisk.svg")
NEEDS = {"help.svg": ("op_font_34.svg",)}


# ---- two pictures held against each other part for part ------------------------------------------------------------

NUMBER = re.compile(r"-?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")
ONE_NUMBER = ("x", "x1", "x2", "cx", "y", "y1", "y2", "cy", "r", "rx", "ry", "width", "height")
ARGUMENTS = {"M": 2, "L": 2, "H": 1, "V": 1, "C": 6, "S": 4, "Q": 4, "T": 2, "A": 7, "Z": 0}


def path_groups(d):
    """Path data as [(command, [numbers])], a command letter for every group, the further pairs of M as L. Data that
    ends inside a group gives that group with the numbers it has. ValueError, with the words that say why, for data
    that is no path data: a number before the first command, a number behind a close, a command where a group still
    wants a number. (Until 2026-11-16 the first was an AttributeError and the second was never left: a close takes no
    number, so the place in the data did not move.)"""
    out, command = [], None
    tokens = re.findall(r"[MmLlHhVvCcSsQqTtAaZz]|" + NUMBER.pattern, d)
    k = 0
    while k < len(tokens):
        if tokens[k].isalpha():
            command = tokens[k]
            k += 1
            if command in "Zz":
                out.append((command, []))
                continue
        elif command is None:
            raise ValueError("path data that begins with a number")
        elif command in "Zz":
            raise ValueError("path data with a number after a close")
        elif command in ("M", "m"):
            command = "L" if command == "M" else "l"
        count = ARGUMENTS[command.upper()]              # one or more from here on: every turn of the loop moves the place
        try:
            values = [float(t) for t in tokens[k:k + count]]
        except ValueError:                              # a command letter among them
            raise ValueError("path data with a command where a number is wanted") from None
        out.append((command, values))
        k += count
    return out


def same_picture(made, proven, tolerance=0.0005):
    """What differs between two pictures read part for part: [] when they are the same picture. Parts, their names
    and attributes must agree; a number may differ by `tolerance`. Two paths whose texts differ and of which one is no
    path data are a difference, in the words path_groups refuses it with. Also (numbers held, numbers that differ at
    all, the largest difference)."""
    differences, counted = [], [0, 0, 0.0]

    def numbers(a, b, where):
        if len(a) != len(b):
            differences.append("%s: %d numbers against %d" % (where, len(a), len(b)))
            return
        for x, y in zip(a, b):
            counted[0] += 1
            if x != y:
                counted[1] += 1
                counted[2] = max(counted[2], abs(x - y))
                if abs(x - y) > tolerance + 1e-9:
                    differences.append("%s: %r against %r" % (where, x, y))

    def walk(a, b, where):
        if a.tag != b.tag:
            differences.append("%s: <%s> against <%s>" % (where, a.tag, b.tag))
            return
        if sorted(a.attrib) != sorted(b.attrib):
            differences.append("%s: attributes %s against %s" % (where, sorted(a.attrib), sorted(b.attrib)))
            return
        for key in a.attrib:
            x, y = a.attrib[key], b.attrib[key]
            here = "%s/@%s" % (where, key)
            if x == y:
                counted[0] += len(NUMBER.findall(x)) if key in ONE_NUMBER or key in ("points", "d", "transform") else 0
            elif key in ONE_NUMBER or key in ("points", "transform"):
                if NUMBER.sub("#", x).replace(",", " ").split() != NUMBER.sub("#", y).replace(",", " ").split() and key == "transform":
                    differences.append("%s: %r against %r" % (here, x, y))
                else:
                    numbers([float(t) for t in NUMBER.findall(x)], [float(t) for t in NUMBER.findall(y)], here)
            elif key == "d":
                try:
                    p, q = path_groups(x), path_groups(y)
                except ValueError as refused:               # one of the two is no path data: not the same path, and the place is said
                    differences.append("%s: %s" % (here, refused))
                else:
                    if [c for c, _n in p] != [c for c, _n in q]:
                        differences.append("%s: other commands" % here)
                    else:
                        numbers([n for _c, group in p for n in group], [n for _c, group in q for n in group], here)
            else:
                differences.append("%s: %r against %r" % (here, x, y))
        if (a.text or "").strip() != (b.text or "").strip() or (a.tail or "").strip() != (b.tail or "").strip():
            differences.append("%s: other text" % where)
        if len(a) != len(b):
            differences.append("%s: %d parts against %d" % (where, len(a), len(b)))
            return
        for number, (x, y) in enumerate(zip(a, b)):
            walk(x, y, "%s/%s[%d]" % (where, x.tag.split("}")[-1], number))

    walk(ET.fromstring(made), ET.fromstring(proven), "")
    return differences, tuple(counted)


# ---- the pictures of a pack -----------------------------------------------------------------------------------------

def sources_for(name, display, digits=None):
    """The stock pictures a picture's steps read, by name; `#digits` is the picture our tape digits are read from."""
    sources = {}
    for needed in (name,) + NEEDS.get(name, ()):
        with open(os.path.join(display, needed), "rb") as f:
            sources[needed] = f.read()
    if digits is not None:
        sources["#digits"] = digits
    return sources


def pictures_of(recipe_path, display):
    """For every picture of a pack's recipe that has steps here: (name, the Doc, the picture the pack carries now)."""
    with open(recipe_path, encoding="utf-8") as f:
        recipe = json.load(f)
    carried = {}
    for picture in recipe["pictures"]:
        with open(os.path.join(ROOT, picture["file"]), "rb") as f:
            carried[picture["name"]] = f.read()
    out = []
    for picture in recipe["pictures"]:
        name = picture["name"]
        if name not in STEPS:
            continue
        proven = carried[name]
        if picture_make.is_instructions(proven):
            raise ValueError("%s is carried as instructions already: give the recipe that carries the picture" % name)
        doc = STEPS[name](name, sources_for(name, display, carried.get("tape.svg")))
        out.append((name, doc, proven))
    return out


def held(name, doc, proven):
    """A line that says how the picture made stands against the one carried; ValueError when it is another picture."""
    made = doc.render()
    if name in EXACT:
        if made != proven:
            raise ValueError("%s is not the picture the pack carries, byte for byte" % name)
        return "the picture the pack carries, byte for byte"
    differences, (count, other, largest) = same_picture(made, proven)
    if differences:
        raise ValueError("%s is another picture than the pack carries: %s" % (name, "; ".join(differences[:5])))
    return "the picture the pack carries part for part: of %d numbers %d are written otherwise, the largest difference %.4f" % (count, other, largest)


def cmd_build(args, write=True):
    import stock_search
    display = stock_search.Display(args.display)
    for name, doc, proven in pictures_of(args.recipe, args.display):
        verdict = held(name, doc, proven)
        data = doc.instructions()
        ours = doc.ours()
        measure = display.measure(ours) if ours else {"stock tags": 0, "stock shapes": 0, "stock shapes, moved": 0}
        stock_bytes = measure["stock tags"] + measure["stock shapes"] + measure["stock shapes, moved"]
        if stock_bytes:
            raise ValueError("%s: %d bytes of what the instructions carry are a stock picture's" % (name, stock_bytes))
        pieces = doc.joined()
        print("%s: %d bytes of instructions (%d kept, %d mapped, %d of ours in %d bytes) for a picture of %d (%d carried now); %s"
              % (name, len(data), sum(1 for p in pieces if p[0] == "s"), sum(1 for p in pieces if p[0] == "m"), sum(1 for p in pieces if p[0] == "l"),
                 len(ours), len(doc.render()), len(proven), verdict))
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
        one.add_argument("--recipe", required=True, help="the recipe of the pack whose pictures are meant, as it carries them now")
        one.add_argument("--display", required=True, help="the folder of stock pictures")
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

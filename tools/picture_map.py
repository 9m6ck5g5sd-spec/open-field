#!/usr/bin/env python3
"""The geometry of a stretch of a picture moved or scaled, as text: the unit's own steps, in Python.

  from picture_map import map_range
  made = map_range(stretch, 870, 31200, 0)        # x' = 0.870 x + 31.200, y' = 0.870 y + 0: 0.87 about (240, 0)

This is what `kit`'s picture maker does to a stretch a set of instructions names with `m` (tools/picture_make.py,
module/build_p2_map.cpp; analysis/picture_maker_study.md, section 9). It is `tools/svg_geometry.py`'s
`transform_span`, which has made the scaled tape pictures since F8, written again so that the unit can do the same:

- **Whole numbers only.** A number of the picture is read as a sign, a whole part and up to six decimals; the scale
  is a whole number of thousandths (1 to 4000) and the offsets are whole thousandths; a result is rounded to three
  decimals (four inside a transform), a half away from zero. `svg_geometry` works in binary floating point, so where
  a result falls exactly on a half its last digit may be the other one: a thousandth of a pixel.
- **Bytes, no patterns.** A tag is walked once from left to right; an attribute is taken where its name follows a
  byte that is no letter, digit, underscore or hyphen and is followed by `="`.
- **What is mapped**: `x x1 x2 cx` and `y y1 y2 cy` (a point's coordinate: scaled and offset), `r rx ry width height`
  (a length: scaled), `points`, `d` (absolute commands as points, relative ones as lengths; written again as the
  command and its numbers joined by commas, a command letter before every group), `transform="translate(a,b)"`
  (lengths, four decimals). A tag with `transform="matrix(a b c d e f)"` takes the map into its matrix and its
  children are left as they are. The root `<svg ...>` tag, comments, the prologue and closing tags are left alone.
- **What is refused**, with the words the unit logs: a number with an exponent or more than six decimals or a whole
  part over 9,999; an arc in a path; a path that begins with a number, has too few numbers for a command or a
  number after its close; a list of points with an odd count; any other transform; a part with a matrix that does
  not end inside the stretch.

Standard library only; reads nothing, writes nothing.
"""

MOST_WHOLE = 9999               # the largest whole part of a number
MOST_SCALE = 4000               # the largest scale, in thousandths: 4000 x 999,999 still fits 32 bits
MOST_OFFSET = 99999999          # the largest offset, in thousandths either way

WORD = frozenset(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")
NAME = WORD | frozenset(b":.-")
COMMANDS = frozenset(b"MmLlHhVvCcSsQqTtAaZz")
NUMBERS = {ord("M"): 2, ord("L"): 2, ord("H"): 1, ord("V"): 1, ord("C"): 6, ord("S"): 4, ord("Q"): 4, ord("T"): 2}
POINT_X = (b"x", b"x1", b"x2", b"cx")
POINT_Y = (b"y", b"y1", b"y2", b"cy")
LENGTHS = (b"r", b"rx", b"ry", b"width", b"height")
MATRIX = b'transform="matrix('
TRANSLATE = b"translate("


class Refused(ValueError):
    """A stretch the unit does not map: the text is what the unit's log line says."""


def read_number(data, at, end):
    """A number that begins at `at`: (the place behind it, negative, whole part, millionths), or None when none
    begins there. `-?(digits[.digits] | .digits)`; an exponent, a seventh decimal and a whole part over 9,999 are
    refused."""
    q = at
    negative = q < end and data[q] == 0x2D
    if negative:
        q += 1
    whole, digits = 0, 0
    while q < end and 0x30 <= data[q] <= 0x39:
        whole = whole * 10 + data[q] - 0x30
        if whole > MOST_WHOLE:
            raise Refused("a number in the picture is too large")
        digits += 1
        q += 1
    millionths, decimals = 0, 0
    if q < end and data[q] == 0x2E:
        r = q + 1
        while r < end and 0x30 <= data[r] <= 0x39:
            if decimals == 6:
                raise Refused("a number in the picture has more than six decimals")
            millionths = millionths * 10 + data[r] - 0x30
            decimals += 1
            r += 1
        if digits == 0 and decimals == 0:
            return None                                     # a point by itself is no number
        q = r
    elif digits == 0:
        return None
    if q < end and data[q] in (0x65, 0x45):                 # e or E, and what makes it an exponent
        r = q + 1
        if r < end and data[r] in (0x2B, 0x2D):
            r += 1
        if r < end and 0x30 <= data[r] <= 0x39:
            raise Refused("a number in the picture has an exponent")
    while decimals < 6:
        millionths *= 10
        decimals += 1
    return q, negative, whole, millionths


def mapped(number, scale, offset, decimals):
    """scale/1000 x number + offset/1000, rounded to `decimals` (3 or 4) a half away from zero: a whole number of
    10**-decimals. Every step fits 32 bits: scale x whole part x 10 under 4 x 10**8, scale x millionths under 2**32."""
    _q, negative, whole, millionths = number
    unit = 1000000 if decimals == 3 else 100000              # what one step of the result is, in 10**-9
    up = 1 if decimals == 3 else 10
    low = scale * millionths                                 # in 10**-9, unsigned
    quotient, rest = low // unit, low % unit
    size = scale * whole * up + quotient                     # scale x |number|, cut off, in steps of the result
    base = offset * up
    if not negative:
        base += size
    elif rest:
        base -= size + 1
        rest = unit - rest
    else:
        base -= size
    half = unit // 2
    if rest > half or (rest == half and base >= 0):
        base += 1
    return base


def written(value, decimals):
    """A whole number of 10**-decimals as text: no zero at the end, no point without decimals, no minus before 0."""
    negative = value < 0
    if negative:
        value = -value
    step = 1000 if decimals == 3 else 10000
    whole, rest = value // step, value % step
    out = bytearray()
    if negative:
        out.append(0x2D)
    out += b"%d" % whole
    if rest:
        digits = bytearray(b"%0*d" % (decimals, rest))
        while digits[-1] == 0x30:
            del digits[-1]
        out.append(0x2E)
        out += digits
    return bytes(out)


class Map:
    """x' = scale/1000 x + dx/1000, y' = scale/1000 y + dy/1000."""

    def __init__(self, scale, dx, dy):
        if not 1 <= scale <= MOST_SCALE or not -MOST_OFFSET <= dx <= MOST_OFFSET or not -MOST_OFFSET <= dy <= MOST_OFFSET:
            raise Refused("its instructions name a scale or an offset the unit does not take")
        self.scale, self.dx, self.dy = scale, dx, dy

    def x(self, number, decimals=3):
        return written(mapped(number, self.scale, self.dx, decimals), decimals)

    def y(self, number, decimals=3):
        return written(mapped(number, self.scale, self.dy, decimals), decimals)

    def length(self, number, decimals=3):
        return written(mapped(number, self.scale, 0, decimals), decimals)


def numbers_of(value):
    """Every number of an attribute's value in its order, whatever stands between them."""
    found, at, end = [], 0, len(value)
    while at < end:
        number = read_number(value, at, end)
        if number is None:
            at += 1
        else:
            found.append(number)
            at = number[0]
    return found


def one_number(value):
    """The number a value is, or None when it is anything else (`0px`)."""
    number = read_number(value, 0, len(value))
    return number if number is not None and number[0] == len(value) else None


def map_points(value, m):
    numbers = numbers_of(value)
    if len(numbers) % 2:
        raise Refused("a list of points in the picture has an odd count")
    out = bytearray()
    for k in range(0, len(numbers), 2):
        if k:
            out.append(0x20)
        out += m.x(numbers[k]) + b"," + m.y(numbers[k + 1])
    return bytes(out)


def map_path(value, m):
    """Path data written again: every command letter with its numbers joined by commas. An absolute command's
    numbers are points, a relative command's are lengths. After M the further pairs are L's, after m they are l's."""
    out = bytearray()
    at, end = 0, len(value)
    command = 0
    while at < end:
        byte = value[at]
        if byte in COMMANDS:
            command = byte
            at += 1
            if command in (0x5A, 0x7A):                     # Z z
                out.append(command)
                continue
        else:
            number = read_number(value, at, end)
            if number is None:
                at += 1                                     # a comma, a space: whatever stands between
                continue
            if command == 0:
                raise Refused("a path in the picture begins with a number")
            if command in (0x5A, 0x7A):
                raise Refused("a path in the picture has a number after its close")
            if command == 0x4D:                             # M: the pairs that follow are L's
                command = 0x4C
            elif command == 0x6D:                           # m: l's
                command = 0x6C
        kind = command & 0xDF                               # the capital
        if kind == 0x41:
            raise Refused("a path in the picture has an arc")
        relative = command != kind
        out.append(command)
        for k in range(NUMBERS[kind]):
            while True:                                     # the next number, whatever stands before it but a command
                if at >= end or value[at] in COMMANDS:
                    raise Refused("a path in the picture has a command with too few numbers")
                number = read_number(value, at, end)
                if number is not None:
                    break
                at += 1
            at = number[0]
            if k:
                out.append(0x2C)
            if kind == 0x48:                                # H: an x
                out += m.length(number) if relative else m.x(number)
            elif kind == 0x56:                              # V: a y
                out += m.length(number) if relative else m.y(number)
            elif relative:
                out += m.length(number)
            else:
                out += m.x(number) if k % 2 == 0 else m.y(number)
    return bytes(out)


def map_translate(value, m):
    """`translate(a,b)` with its two lengths scaled, to four decimals; None for a value of another form."""
    if value[:len(TRANSLATE)] != TRANSLATE:
        return None
    end = len(value)
    a = read_number(value, len(TRANSLATE), end)
    if a is None:
        return None
    at = a[0]
    between = at
    while at < end and value[at] in b", \t\r\n":
        at += 1
    if at == between:
        return None
    b = read_number(value, at, end)
    if b is None or value[b[0]:] != b")":
        return None
    return TRANSLATE + m.length(a, 4) + b"," + m.length(b, 4) + b")"


def map_value(name, value, m):
    """An attribute's value mapped, or None when it stays as it is."""
    if name in POINT_X or name in POINT_Y or name in LENGTHS:
        number = one_number(value)
        if number is None:
            return None
        return m.x(number) if name in POINT_X else m.y(number) if name in POINT_Y else m.length(number)
    if name == b"points":
        return map_points(value, m)
    if name == b"d":
        return map_path(value, m)
    if name == b"transform":
        made = map_translate(value, m)
        if made is None:
            raise Refused("a transform in the picture is of no form the unit maps")
        return made
    return None


def attributes(tag):
    """The attributes of a tag, or of a piece of one, as the unit finds them: (name's start, its end, value's start,
    value's end), where a name stands first or follows a byte that is no letter, digit, underscore or hyphen, and is
    followed by `="`; the value runs to the next `"`. A value that does not end ends the walk."""
    found = []
    at, end = 0, len(tag)
    while at < end:
        if tag[at] in NAME and (at == 0 or (tag[at - 1] not in WORD and tag[at - 1] != 0x2D)):
            stop = at
            while stop < end and tag[stop] in NAME:
                stop += 1
            if tag[stop:stop + 2] == b'="':
                close = tag.find(b'"', stop + 2)
                if close < 0:
                    break
                found.append((at, stop, stop + 2, close))
                at = close + 1
                continue
            at = stop
            continue
        at += 1
    return found


def map_tag(tag, m):
    """An opening tag with its geometry mapped."""
    out = bytearray()
    done = 0
    for start, stop, first, close in attributes(tag):
        made = map_value(tag[start:stop], tag[first:close], m)
        if made is not None:
            out += tag[done:first] + made
            done = close
    out += tag[done:]
    return bytes(out)


def map_matrix_tag(tag, m):
    """A tag with transform="matrix(a b c d e f)": the four of the linear part scaled, the last two mapped as a point,
    all to four decimals and joined by spaces. The rest of the tag stays."""
    at = tag.find(MATRIX)
    first = at + len(MATRIX)
    end = len(tag)
    six, q = [], first
    for k in range(6):
        between = q
        while q < end and tag[q] in (b", \t\r\n" if k else b" \t\r\n"):     # commas only between the numbers
            q += 1
        if k and q == between:
            raise Refused("a transform in the picture is of no form the unit maps")
        number = read_number(tag, q, end)
        if number is None:
            raise Refused("a transform in the picture is of no form the unit maps")
        six.append(number)
        q = number[0]
    while q < end and tag[q] in b" \t\r\n":
        q += 1
    if tag[q:q + 2] != b')"':
        raise Refused("a transform in the picture is of no form the unit maps")
    made = b" ".join([m.length(six[0], 4), m.length(six[1], 4), m.length(six[2], 4), m.length(six[3], 4), m.x(six[4], 4), m.y(six[5], 4)])
    return tag[:first] + made + tag[q:]


def element_end(data, start, tag_end, end):
    """Where the element ends whose opening tag is data[start:tag_end]: the tag itself when it closes itself, else
    behind the closing tag that matches it, tags of the same name counted. None when it does not end before `end`."""
    if data[tag_end - 2] == 0x2F:
        return tag_end
    stop = start + 1
    while stop < tag_end and data[stop] in WORD:
        stop += 1
    name = data[start + 1:stop]
    opening, closing = b"<" + name, b"</" + name + b">"
    depth, at = 1, tag_end
    while True:
        at = data.find(b"<", at, end)
        if at < 0:
            return None
        if data[at:at + len(closing)] == closing:
            depth -= 1
            at += len(closing)
            if depth == 0:
                return at if at <= end else None
            continue
        if data[at:at + len(opening)] == opening and (at + len(opening) >= end or data[at + len(opening)] not in WORD):
            close = data.find(b">", at, end)
            if close < 0:
                return None
            if data[close - 1] != 0x2F:
                depth += 1
            at = close + 1
            continue
        at += 1


def left_alone(tag):
    """The root's tag, <? <! and a closing tag: not mapped."""
    return tag[:4] == b"<svg" or tag[1:2] in (b"?", b"!", b"/")


def map_range(data, scale, dx, dy):
    """A stretch of a picture with its geometry mapped: x' = scale/1000 x + dx/1000, y' = scale/1000 y + dy/1000.
    The stretch may begin inside a tag and end inside one (where text of ours, an id, stands in the middle of a
    tag): what stands before a first `>` that no `<` precedes is the rest of a tag, and a last `<` that no `>`
    follows begins one; the attributes of both are mapped as a whole tag's are."""
    m = Map(scale, dx, dy)
    out = bytearray()
    at, end = 0, len(data)
    first_close, first_open = data.find(b">"), data.find(b"<")
    if first_close >= 0 and (first_open < 0 or first_close < first_open):
        out += map_tag(data[:first_close + 1], m)
        at = first_close + 1
    while True:
        start = data.find(b"<", at, end)
        if start >= 0 and data[start + 1:start + 2] == b">":                 # "<>" is no tag: a tag has a byte between its marks
            out += data[at:start + 1]
            at = start + 1
            continue
        if start < 0:
            out += data[at:]
            return bytes(out)
        out += data[at:start]
        close = data.find(b">", start + 1, end)
        if close < 0:                                                        # the stretch ends inside this tag
            tag = data[start:]
            if MATRIX in tag and not left_alone(tag):
                raise Refused("a part with a matrix does not end inside the stretch")
            out += tag if left_alone(tag) else map_tag(tag, m)
            return bytes(out)
        tag = data[start:close + 1]
        if left_alone(tag):
            out += tag
            at = close + 1
        elif MATRIX in tag:
            stop = element_end(data, start, close + 1, end)
            if stop is None:
                raise Refused("a part with a matrix does not end inside the stretch")
            out += map_matrix_tag(tag, m) + data[close + 1:stop]
            at = stop
        else:
            out += map_tag(tag, m)
            at = close + 1

#!/usr/bin/env python3
"""Pictures made on the unit: the instructions a pack carries in place of a picture made from a stock picture.

  python -I tools/picture_make.py compile --stock "op-1field backup/content/display/singlelfo.svg" \\
         --picture candidates/content-i3/display/singlelfo.svg --out candidates/content-p1/display/singlelfo.svg.make
  python -I tools/picture_make.py make --instructions candidates/content-p1/display/singlelfo.svg.make \\
         --display "op-1field backup/content/display" [--out made.svg]
  python -I tools/picture_make.py check --instructions FILE [more] --display "op-1field backup/content/display"

A pack's picture is either a picture that is ours alone, handed to the firmware's reader as it is, or a set of
instructions by which the unit makes the picture from its own stock pictures when a screen asks for it
(analysis/picture_maker_study.md). The maker on the unit is in `kit` (module/build_p1_art.cpp); `make` here is the
same steps in Python, the reference the unit's code is held against on the PC.

A set of instructions is text, one instruction a line, a line ended by a line feed:

  op1f picture 1                  the first line: what this is
  size N                          the second: the bytes of the picture that is made
  from NAME BYTES CRC             the text to work on is the unit's own content/display/NAME, which must be BYTES long
                                  and have the CRC-32 CRC (eight hex digits): a unit whose picture is another one gets
                                  its own screen and a line on the log, never a picture put together from the wrong parts
  add end BYTES                   the BYTES bytes that follow this line (and a line feed behind them) are put in before
                                  the last closing tag of the picture, `</svg`

Step P1 has these and no more: a stock picture with groups of ours added, which is what the five LFO pages are.
Anything else in a set of instructions is refused, by the unit too.

Step P2 has a second kind, told by its first line, for a picture that is put together from stretches of stock
pictures and text of ours, a stretch moved or scaled where the picture needs it (tools/picture_delta.py writes them):

  op1f picture 2                  the first line
  size N                          the second, as above
  from NAME BYTES CRC             a stock picture the instructions take stretches from; up to four, numbered from 0
                                  in their order, a megabyte together, each checked by its length and CRC-32 as above
  c S OFFSET LENGTH               LENGTH bytes of the stock picture S from OFFSET, as they are
  m S OFFSET LENGTH SCALE DX DY   the same stretch with its geometry mapped: x' = SCALE/1000 x + DX/1000,
                                  y' = SCALE/1000 y + DY/1000 (tools/picture_map.py: whole numbers, three decimals)
  a BYTES                         the BYTES bytes that follow this line (and a line feed behind them), which are ours

So "shrink the tape" is one `m` over the whole picture, and "draw that part of theirs here" is a `c` or an `m` of the
part's stretch. The pack holds numbers and text of ours; the stretches are read from the unit's own files.

`compile` writes the instructions for a picture that is a stock picture with one stretch of ours before its closing
tag, and refuses any other picture. `check` holds that what a set of instructions carries is ours: no tag and no
shape of any stock picture (tools/stock_search.py's measure), and that it makes a picture.

Reads its inputs; `compile` and `make --out` write one file each and refuse to overwrite it.
"""

import argparse
import os
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

MARK = b"op1f picture 1\n"
MARK2 = b"op1f picture 2\n"         # the second kind (P2): stretches of stock pictures, kept or mapped, and text of ours
CLOSING = b"</svg"
SOURCES = 4                     # the stock pictures one set of the second kind takes stretches from, at most
LARGEST = 1 << 20               # the largest picture the unit makes: a megabyte
NAME_BYTES = 32                 # a stock picture's name with its ending, a zero byte counted


class Refused(ValueError):
    """A set of instructions that cannot be carried out: the text is what the unit's log line says."""


def is_instructions(data):
    return data[:len(MARK)] in (MARK, MARK2)


def instructions_for_added(stock_name, stock, picture):
    """The instructions for `picture`, which is the stock picture `stock` with one stretch of ours put in before its
    closing tag. ValueError for any other picture."""
    at = stock.rfind(CLOSING)
    if at < 0:
        raise ValueError("The stock picture has no closing tag")
    ours = len(picture) - len(stock)
    if ours <= 0 or picture[:at] != stock[:at] or picture[at + ours:] != stock[at:]:
        raise ValueError("The picture is not the stock picture with one stretch of ours before its closing tag")
    name = stock_name.encode("ascii")
    if not 0 < len(name) < NAME_BYTES or any(not 0x20 < c < 0x7f for c in name):
        raise ValueError("A stock picture's name is one to %d plain characters" % (NAME_BYTES - 1))
    if len(picture) > LARGEST:
        raise ValueError("The picture is larger than the unit makes")
    return (MARK + b"size %d\n" % len(picture) + b"from %s %d %08x\n" % (name, len(stock), zlib.crc32(stock) & 0xffffffff)
            + b"add end %d\n" % ours + picture[at:at + ours] + b"\n")


HEX = b"0123456789abcdef"


class _Lines:
    """A set of instructions read line by line, as the unit reads it."""

    def __init__(self, data, mark=MARK):
        if data[:len(mark)] != mark:
            raise Refused("it is no set of instructions")
        self.data, self.at = data, len(mark)

    def more(self):
        return self.at < len(self.data)

    def line(self):
        end = self.data.find(b"\n", self.at)
        if end < 0:
            raise Refused("its instructions end inside a line")
        words = self.data[self.at:end].split(b" ")
        self.at = end + 1
        return words

    def size(self):
        first = self.line()
        if len(first) != 2 or first[0] != b"size":
            raise Refused("its instructions do not begin with the picture's size")
        size = _number(first[1])
        if not 0 < size <= LARGEST:
            raise Refused("its instructions name a size the unit does not make")
        return size


def _number(text):
    if not 0 < len(text) <= 9 or any(not 0x30 <= c <= 0x39 for c in text):
        raise Refused("its instructions name a number that is none")
    return int(text)


def _plain_name(text):
    return 0 < len(text) < NAME_BYTES and all(0x20 < c < 0x7f and c not in b"/\\" for c in text)


def _signed(text):
    """A whole number that may have a minus before it: an offset."""
    return -_number(text[1:]) if text[:1] == b"-" else _number(text)


def _stock(words, read_stock):
    """The stock picture a `from` line names, checked as the unit checks it."""
    stock = read_stock(words[1].decode("ascii"))
    if stock is None:
        raise Refused("the unit's own picture could not be opened")
    if len(stock) != _number(words[2]):
        raise Refused("the unit's own picture is not as long as the instructions say")
    if zlib.crc32(stock) & 0xffffffff != int(words[3], 16):
        raise Refused("the unit's own picture is not the one the instructions were made for")
    return stock


def _stretch(sources, words):
    """The stretch a `c` or an `m` line names: (the stock picture, offset, length)."""
    number, offset, length = _number(words[1]), _number(words[2]), _number(words[3])
    if number >= len(sources):
        raise Refused("its instructions name a stock picture they did not take")
    if offset > len(sources[number]) or length > len(sources[number]) - offset:
        raise Refused("its instructions name a stretch outside the stock picture")
    return sources[number], offset, length


def make2(data, read_stock, blocks=None, most=SOURCES):
    """The picture a set of instructions of the second kind makes, by the unit's steps in the unit's order. `most` is
    the count of stock pictures the kit holds for one set: four in the kit of P2, eight from the kit of P3 on."""
    import picture_map
    lines = _Lines(data, MARK2)
    size = lines.size()
    out = bytearray()
    sources = []
    held = 0
    while lines.more():
        words = lines.line()
        if len(words) == 4 and words[0] == b"from":
            if not _plain_name(words[1]) or len(words[3]) != 8 or any(c not in HEX for c in words[3]):
                raise Refused("its instructions name a stock picture badly")
            named = _number(words[2])
            if len(sources) == most:
                raise Refused("its instructions take more stock pictures than the unit holds")
            if named > LARGEST - held:                          # stock's allocator aborts when it has no room: what a set asks for is bounded
                raise Refused("its instructions take stock pictures larger than the unit holds at once")
            sources.append(_stock(words, read_stock))
            held += named
        elif len(words) == 4 and words[0] == b"c":
            source, offset, length = _stretch(sources, words)
            if len(out) + length > size:
                raise Refused("its instructions make more than the size they name")
            out += source[offset:offset + length]
        elif len(words) == 7 and words[0] == b"m":
            scale, dx, dy = _number(words[4]), _signed(words[5]), _signed(words[6])       # the numbers first, in their order
            _number(words[1]), _number(words[2]), _number(words[3])
            source, offset, length = _stretch(sources, words)
            try:
                made = picture_map.map_range(source[offset:offset + length], scale, dx, dy)
            except picture_map.Refused as refused:
                raise Refused(str(refused))
            if len(out) + len(made) > size:
                raise Refused("its instructions make more than the size they name")
            out += made
        elif len(words) == 2 and words[0] == b"a":
            count = _number(words[1])
            if count + 1 > len(data) - lines.at or data[lines.at + count:lines.at + count + 1] != b"\n":
                raise Refused("its instructions end inside what they add")
            if len(out) + count > size:
                raise Refused("its instructions make more than the size they name")
            block = data[lines.at:lines.at + count]
            out += block
            if blocks is not None:
                blocks.append(block)
            lines.at += count + 1
        else:
            raise Refused("its instructions hold one this kit does not know")
    if len(out) != size:
        raise Refused("its instructions do not make the size they name")
    return bytes(out)


def make(data, read_stock, blocks=None, most=SOURCES):
    """The picture a set of instructions makes, by the unit's own steps in the unit's own order, so that a set the
    unit refuses is refused here for the same reason. `read_stock(name)` answers a stock picture's bytes, or None
    when the unit has none of that name. `blocks`, a list, is given what the instructions add of their own."""
    if data[:len(MARK2)] == MARK2:
        return make2(data, read_stock, blocks, most)
    lines = _Lines(data)
    size = lines.size()
    work = None
    while lines.more():
        words = lines.line()
        if len(words) == 4 and words[0] == b"from":
            if not _plain_name(words[1]) or len(words[3]) != 8 or any(c not in HEX for c in words[3]):
                raise Refused("its instructions name a stock picture badly")
            count = _number(words[2])
            if work is not None:
                raise Refused("its instructions take a stock picture twice")
            if count > size:
                raise Refused("its instructions name a stock picture larger than the result")
            stock = read_stock(words[1].decode("ascii"))
            if stock is None:
                raise Refused("the unit's own picture could not be opened")
            if len(stock) != count:
                raise Refused("the unit's own picture is not as long as the instructions say")
            if zlib.crc32(stock) & 0xffffffff != int(words[3], 16):
                raise Refused("the unit's own picture is not the one the instructions were made for")
            work = bytearray(stock)
        elif len(words) == 3 and words[0] == b"add" and words[1] == b"end":
            count = _number(words[2])
            if count + 1 > len(data) - lines.at or data[lines.at + count:lines.at + count + 1] != b"\n":
                raise Refused("its instructions end inside what they add")
            if work is None:
                raise Refused("its instructions add to a picture before they take one")
            at = bytes(work).rfind(CLOSING)
            if at < 0:
                raise Refused("the picture has no closing tag to add before")
            if len(work) + count > size:
                raise Refused("its instructions make more than the size they name")
            block = data[lines.at:lines.at + count]
            work[at:at] = block
            if blocks is not None:
                blocks.append(block)
            lines.at += count + 1
        else:
            raise Refused("its instructions hold one this kit does not know")
    if work is None or len(work) != size:
        raise Refused("its instructions do not make the size they name")
    return bytes(work)


def own_text(data, read_stock, most=SOURCES):
    """What a set of instructions carries of its own: the blocks it adds, one behind the other."""
    blocks = []
    make(data, read_stock, blocks, most)
    return b"".join(blocks)


def stock_reader(folder):
    def read_stock(name):
        path = os.path.join(folder, name)
        if os.sep in name or "/" in name or not os.path.isfile(path):
            return None
        with open(path, "rb") as f:
            return f.read()
    return read_stock


def write_new(path, data):
    if os.path.exists(path):
        raise ValueError("%s exists; this tool does not overwrite" % path)
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "xb") as f:
        f.write(data)


def cmd_compile(args):
    with open(args.stock, "rb") as f:
        stock = f.read()
    with open(args.picture, "rb") as f:
        picture = f.read()
    data = instructions_for_added(os.path.basename(args.stock), stock, picture)
    blocks = []
    if make(data, lambda name: stock if name == os.path.basename(args.stock) else None, blocks) != picture:
        raise ValueError("The instructions do not make the picture")
    write_new(args.out, data)
    print("wrote %s: %d bytes of instructions for a picture of %d, %d of them ours; from %s, %d bytes, CRC-32 %08x"
          % (args.out, len(data), len(picture), sum(map(len, blocks)), os.path.basename(args.stock), len(stock), zlib.crc32(stock) & 0xffffffff))
    return 0


def cmd_make(args):
    with open(args.instructions, "rb") as f:
        data = f.read()
    picture = make(data, stock_reader(args.display))
    if args.out:
        write_new(args.out, picture)
    print("%s makes a picture of %d bytes, CRC-32 %08x%s" % (args.instructions, len(picture), zlib.crc32(picture) & 0xffffffff,
                                                           ", written to " + args.out if args.out else ""))
    return 0


def cmd_check(args):
    import stock_search
    display = stock_search.Display(args.display)
    bad = 0
    for path in args.instructions:
        with open(path, "rb") as f:
            data = f.read()
        blocks = []
        picture = make(data, stock_reader(args.display), blocks, args.sources)
        ours = b"".join(blocks)
        measure = display.measure(ours)
        stock_bytes = measure["stock tags"] + measure["stock shapes"] + measure["stock shapes, moved"]
        bad += 1 if stock_bytes else 0
        print("%s: %d bytes of instructions, %d of them ours in %d tags, %d bytes of them a stock picture's; makes %d bytes, CRC-32 %08x"
              % (path, len(data), len(ours), measure["tags"], stock_bytes, len(picture), zlib.crc32(picture) & 0xffffffff))
    return 1 if bad else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="command", required=True)
    one = sub.add_parser("compile", help="the instructions for a stock picture with a stretch of ours added")
    one.add_argument("--stock", required=True, help="the stock picture")
    one.add_argument("--picture", required=True, help="the picture as the artwork tool made it")
    one.add_argument("--out", required=True)
    one.set_defaults(run=cmd_compile)
    two = sub.add_parser("make", help="the picture a set of instructions makes, as the unit makes it")
    two.add_argument("--instructions", required=True)
    two.add_argument("--display", required=True, help="the folder of stock pictures")
    two.add_argument("--out")
    two.set_defaults(run=cmd_make)
    three = sub.add_parser("check", help="hold that what the instructions carry is ours, and that they make a picture")
    three.add_argument("--instructions", required=True, nargs="+")
    three.add_argument("--display", required=True)
    three.add_argument("--sources", type=int, default=SOURCES, help="the stock pictures the kit holds for one set: 4 (the kit of P2), 8 (from P3 on)")
    three.set_defaults(run=cmd_check)
    args = ap.parse_args(argv)
    try:
        return args.run(args)
    except ValueError as e:
        print("error: %s" % e)
        return 2


if __name__ == "__main__":
    sys.exit(main())

"""tools/publish_check.py: the check that refuses a file with something of the stock firmware in it (docs/14, row G).

  python -I -m unittest discover -s tools/tests -p "test_publish_check.py"

analysis/publish_check_study.md. Held here, on a small made-up stock image, a made-up stock picture and a made-up
profile, so that nothing in this file is the stock firmware's:

- the names: a file that never goes is told by its folder, its ending or its name, and is not opened; a firmware
  image and an update package under any name are told by their first bytes;
- a binary: sixteen bytes of the image in a row are a finding and fifteen are not; the bytes of a place of change
  are found by themselves, taken from the image and not from the profile; small numbers among zeros say nothing; a row
  of addresses is let through and counted;
- shared text: a phrase quoted is let through and counted (a finding until 2026-11-17), with the white space at its
  ends not counted; one file that quotes many phrases, or many bytes of them, holds the image's texts in bulk, and that
  is a finding, on each side of each bound; one name is let through; a row of strings in the image's order is a
  finding, a phrase among them or not; short strings that meet by their padding are nothing; a text on the list a
  person has read is let through;
- a pack: an entry of a table of stock changes with the image's bytes is a finding, with the count of bytes; an entry
  of the new kind is not; the bytes of a place of change in the body are found;
- a text file: stock bytes written as hex, in each of the five ways of writing them; a row of plain numbers and a
  hash are not hex; a habit of the instruction set is not a copy;
- a listing: lines with the image's own bytes; a line that begins with a stock code address; a listing of code of
  ours is searched as the bytes it shows;
- quoted instructions: two beside a stock code address are a finding, one is let through, none beside an address is
  not counted; in an assembly source only the comments are read;
- a log read from the unit, and log lines quoted in a note;
- pictures: a picture file, a picture inside another file, a stock tag loose in a text, a set of instructions; a
  picture of pixels is not judged, and `--strict` counts it;
- the command: its three answers, the list of files, the record it writes and does not overwrite, the part that
  needs no stock image, the same bytes met twice;
- two firmwares in one run: each profile's places are read in its own firmware's records, so that a file with the
  newer firmware's bytes of a place is refused, which it would not be were the place read in the older records;
- the tree as it stands (skipped where the stock image is not there): today's pack files are refused for their
  tables and for nothing of a stock picture; the base passes; sources; this step's own three files pass; and the
  dev-mode page's reused code computes the table it carried until 2026-11-16, which is the firmware's.
"""

import contextlib
import hashlib
import io
import json
import os
import random
import shutil
import struct
import sys
import tempfile
import unittest
import zlib
from unittest import mock

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))

import pack_build  # noqa: E402
import picture_make  # noqa: E402
import picture_provenance  # noqa: E402
import publish_check as pc  # noqa: E402

# ---------------------------------------------------------------- a made-up stock image

CODE_AT, HABITS_AT, TEXT_AT, WORDS_AT, PLAIN_AT = 0x8f002000, 0x8f003000, 0x84000100, 0x84000400, 0x84000500
RANDOM = random.Random(20261116)
CODE = bytes(RANDOM.randrange(1, 255) for _ in range(0x200))
SITE_AT = CODE_AT + 0x40
SITE = CODE[0x40:0x48]
SIXTEEN = CODE[0x100:0x110]
HABIT = bytes.fromhex("6701400141014201")
HABITS = b"".join(HABIT + bytes(RANDOM.randrange(1, 255) for _ in range(5)) for _ in range(pc.MANY + 4))
MESSAGE = b"The tape head is %d steps off its mark"
NAME = b"settings/bright_mode.json"
ROW = b"alpha\0\0\0bravo\0\0\0charlie\0delta\0\0\0echo"
SHORT = b"wide\0\0\0\0tall"
DECIMALS = bytes.fromhex("10203040506070809011223344556677")
LICENCE = (b"A small library, by its two authors.\n\nPermission is hereby granted, free of charge, to any person who asks for it,\n"
           b"to use this library as they like. IT COMES WITH NO PROMISE, AND ITS AUTHORS HAVE NO OTHER DEALINGS IN THE MATTER.\n")
TWO_LINES = b"Line one of the long text ends here\nand line two goes on from there"
MENU_LINE = b"9. spin the reels"
TEXT = (MESSAGE + b"\0\0" + NAME + b"\0\0\0" + ROW + b"\0\0\0\0" + SHORT + b"\0\0\0\0" + DECIMALS + b"\x7f\x80\0\0" + LICENCE + b"\0\0\0"
        + TWO_LINES + b"\0\0    " + MENU_LINE + b"\n\0")
WORDS = struct.pack("<4I", CODE_AT + 0x10, CODE_AT + 0x24, CODE_AT + 0x38, TEXT_AT + 4)
PLAIN = struct.pack("<4I", 1, 0x200, 0x30000, 0)
COUNTING = bytes(range(0x20, 0x90, 7))              # sixteen bytes that count by seven: an index table has such a row
OURS_AT = 0x8d2f4000                                # where code of ours stands: no image has code there

STOCK_LINE = b'<line fill="none" stroke="#123456" stroke-width="7" stroke-linecap="round" x1="11.25" y1="207.5" x2="66.75" y2="207.5"/>'
STOCK_PICTURE = b'<svg>\n<rect id="ground" fill="#010203" width="333" height="111"/>\n' + STOCK_LINE + b'\n<path d="M12,34l5,6l-7,8l-9,-10z"/>\n</svg>\n'
OUR_PICTURE = b'<svg>\n<circle fill="#3C6087" cx="5" cy="6" r="7"/>\n<path d="M1,2l30,4l-5,60z"/>\n</svg>\n'


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data if isinstance(data, bytes) else data.encode("utf-8"))
    return path


def make_stock(folder):
    """(the folder of memory records, the profile, the folder of stock pictures) of the made-up device."""
    records, stored = [], ((CODE_AT, CODE), (HABITS_AT, HABITS), (TEXT_AT, TEXT), (WORDS_AT, WORDS), (PLAIN_AT, PLAIN + b"\x99" + COUNTING + b"\xaa"))
    for number, (address, data) in enumerate(stored):
        name = "%03d_%08x.bin" % (number, address)
        write(os.path.join(folder, "records", name), data)
        records.append({"index": number, "target": address, "count": len(data), "argument": 0, "fill": False, "stored": len(data), "file": name})
    records.append({"index": len(stored), "target": 0x84001000, "count": 4096, "argument": 0, "fill": True, "stored": 0})
    write(os.path.join(folder, "records", "index.json"), json.dumps({"source": "a test", "bytes": 0, "records": records}))
    profile = write(os.path.join(folder, "profile.json"), json.dumps({"patches": [
        {"address": "0x%08x" % SITE_AT, "length": 8, "group": "g1"},                    # no "before": the image has the bytes
        {"address": "0x%08x" % WORDS_AT, "length": 4, "group": "a word"},
        {"kind": "heap", "name": "an edit with no address"}]}))
    write(os.path.join(folder, "display", "dial.svg"), STOCK_PICTURE)
    return os.path.join(folder, "records"), profile, os.path.join(folder, "display")


def beside(data, at, length):
    """`length` bytes of `data` from `at` between bytes of ours that are not their neighbours in the image: a zero byte
    first, so that the file is a binary."""
    return b"OURS\0" + bytes([data[at - 1] ^ 0x5a]) + data[at:at + length] + bytes([data[at + length] ^ 0x5a]) + b"\x01ours"


class WithAStock(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.folder = tempfile.mkdtemp(prefix="publish-check-")
        cls.records, cls.profile, cls.pictures = make_stock(cls.folder)
        cls.images = pc.Images([cls.records])
        cls.places = pc.read_places([cls.profile], cls.images)
        cls.display = picture_provenance.Stock(cls.pictures)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.folder, ignore_errors=True)

    def check(self, **more):
        return pc.Check(self.images, self.places, more.pop("display", self.display), **more)

    def judged(self, name, data, **more):
        path = write(os.path.join(self.folder, "files", self.id().rsplit(".", 1)[-1], name), data)
        return self.check(**more).file(path, name)

    def rules(self, name, data, **more):
        record = self.judged(name, data, **more)
        return record["verdict"], record["counts"], {rule: held["count"] for rule, held in record["notes"].items()}


# ---------------------------------------------------------------- the names


class TheNames(unittest.TestCase):
    def test_a_file_that_never_goes_is_told_by_its_path(self):
        for path in ("candidates/build-x.ldr", "x/update.TFW", "analysis/device-logs/a.txt", "analysis/device-files/day/tape01.json",
                     "op-1field backup/content/display/tape.svg", "analysis/os-disasm-full/004.txt", "analysis/boot-records/1.bin",
                     "logs/log_2026-11-16_09.54.18_0.txt", "firmware/an archive.zip", "second copy of firmware folder/x", "Analysis\\OS-Records\\1.bin"):
            self.assertIsNotNone(pc.never(path), path)
        for path in ("docs/15-lawyer-questions.md", "tools/publish_check.py", "module/a.cpp", "packs/kit.json", "firmware.md",
                     "notes/log_of_the_work.txt", "records.md", "candidates/pack-p3b/kit.pack"):
            self.assertIsNone(pc.never(path), path)

    def test_a_named_file_is_refused_without_being_opened(self):
        record = pc.Check().file(os.path.join("no", "such", "folder", "os.ldr"))
        self.assertEqual((record["verdict"], record["kind"], record["counts"]), ("refused", "named", {"name": 1}))
        self.assertIn("a firmware image", record["findings"][0]["what"])

    def test_an_image_and_a_package_under_another_name_are_told_by_their_first_bytes(self):
        folder = tempfile.mkdtemp(prefix="publish-check-")
        self.addCleanup(shutil.rmtree, folder, True)
        header = bytearray(struct.pack("<4I", 0, 0x11a00000, 0, 0x1ff0ac))
        header[3] = pc.LDR_SIGN
        check = 0
        for byte in header:
            check ^= byte
        header[2] = check                                                   # the sixteen bytes now XOR to zero
        stream = write(os.path.join(folder, "notes.bin"), bytes(header) + bytes(64))
        package = write(os.path.join(folder, "holiday.jpg"), pc.TFW_MAGIC + bytes(range(1, 200)))
        plain = write(os.path.join(folder, "plain.bin"), bytes(header[:2]) + b"\x00" + bytes(header[3:]) + bytes(64))
        self.assertTrue(pc.is_boot_stream(bytes(header)))
        check = pc.Check()
        for path, kind in ((stream, "firmware image"), (package, "update package")):
            record = check.file(path)
            self.assertEqual((record["kind"], record["verdict"], record["counts"]), (kind, "refused", {"name": 1}), path)
            self.assertIn("by its first bytes", record["findings"][0]["what"])
        self.assertEqual(check.file(plain)["kind"], "binary")               # a header whose check does not hold is no boot stream


# ---------------------------------------------------------------- a binary and a pack


class ABinary(WithAStock):
    def test_sixteen_bytes_of_the_image_are_a_finding_and_fifteen_are_not(self):
        verdict, counts, _notes = self.rules("part.bin", beside(CODE, 0x100, 16))
        self.assertEqual((verdict, counts), ("refused", {"bytes": 1}))
        record = self.judged("part.bin", beside(CODE, 0x100, 16))
        self.assertIn("16 bytes", record["findings"][0]["what"])
        self.assertIn("0x%08x" % (CODE_AT + 0x100), record["findings"][0]["what"])
        self.assertEqual(record["findings"][0]["offset"], 6)
        self.assertEqual(self.rules("part.bin", beside(CODE, 0x100, 15)), ("passed", {}, {}))
        self.assertEqual(pc.RUN, 16)

    def test_the_bytes_of_a_place_of_change_are_found_by_themselves(self):
        """Eight bytes: too short for a run to say anything, and known to be the image's. The profile names the place
        and its length only; the bytes come from the image."""
        self.assertEqual(self.places[1], [(WORDS_AT, WORDS[:4], "a word"), (SITE_AT, SITE, "g1")])
        record = self.judged("stub.bin", beside(CODE, 0x40, 8))
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"site": 1}))
        self.assertIn("0x%08x (g1)" % SITE_AT, record["findings"][0]["what"])
        self.assertIn(SITE.hex(), record["findings"][0]["what"])
        self.assertEqual(self.rules("stub.bin", beside(CODE, 0x41, 7) + beside(CODE, 0x80, 8)), ("passed", {}, {}))     # eight other bytes
        self.assertEqual(self.rules("word.bin", beside(WORDS, 0, 4) if False else b"OURS\0\x01" + WORDS[:4] + b"\x02"), ("passed", {}, {}))  # a word is an address

    def test_small_numbers_among_zeros_and_a_row_that_counts_say_nothing(self):
        self.assertEqual(self.images.stock.places(PLAIN), [(PLAIN_AT, "records")])
        self.assertEqual(self.rules("counts.bin", b"OURS\0\x07" + PLAIN + b"\x09"), ("passed", {}, {}))
        self.assertEqual(self.images.stock.places(COUNTING), [(PLAIN_AT + len(PLAIN) + 1, "records")])
        self.assertEqual(self.rules("key.bin", b"OURS\0\x07\x77" + COUNTING + b"\x55\x09"), ("passed", {}, {}))
        self.assertEqual(self.rules("test.py", "KEY = bytes.fromhex('%s')\n" % COUNTING.hex()), ("passed", {}, {}))
        self.assertEqual(pc.sort_run(COUNTING[:-1] + b"\x7e", self.images, set())[0], "bytes")        # sixteen that do not count
        for row, counts in ((struct.pack("<9H", *range(3, 12)), True), (struct.pack("<4I", 0x8c0d0100, 0x8c0d0110, 0x8c0d0120, 0x8c0d0130), True),
                            (struct.pack("<9H", 3, 4, 5, 6, 8, 9, 10, 11, 12), False), (bytes(8) + b"\x01" + bytes(7), False), (SIXTEEN, False)):
            self.assertEqual(pc.counting(row), counts, row.hex())           # half words and words count too; one step missed does not
        little = struct.pack("<4I", 1, 2, 3, 5)                             # four words that each fit a byte
        self.assertEqual(pc.sort_run(little, self.images, set())[:2], (None, None))
        self.assertEqual(pc.sort_run(struct.pack("<4I", 1, 2, 3, 0x105), self.images, set())[0], "bytes")    # five bytes of sixteen say something
        self.assertEqual(pc.SMALL, 4)

    def test_a_row_of_addresses_is_let_through_and_counted(self):
        self.assertEqual(self.rules("handlers.bin", b"OURS\0\x07" + WORDS + b"\x09"), ("passed", {}, {"addresses": 1}))
        other = struct.pack("<4I", CODE_AT + 0x10, CODE_AT + 0x24, 0x12345678, TEXT_AT + 4)
        self.assertFalse(pc.addresses_only(other, self.images))             # not in the image at all
        self.assertFalse(pc.addresses_only(SIXTEEN, self.images))           # code is no row of addresses

    def test_a_picture_that_stands_in_a_binary_is_judged(self):
        record = self.judged("module.bin", b"OURS\0\x01" + STOCK_PICTURE + b"\0" + OUR_PICTURE + b"\0\x02")
        self.assertEqual((record["verdict"], list(record["counts"])), ("refused", ["picture"]))
        self.assertEqual(len(record["findings"]), 1)                        # the stock picture, not ours
        self.assertEqual(self.rules("module.bin", b"OURS\0\x01" + OUR_PICTURE + b"\0\x02")[:2], ("passed", {}))


class SharedText(WithAStock):
    def test_a_phrase_quoted_is_let_through_and_counted(self):
        """A message, a line the unit shows. What a screen or a log says is no secret, and a guide that quotes it is
        clearer than one that talks round it: let through and counted since 2026-11-17, a finding until then."""
        record = self.judged("unit.bin", b"OURS\0\x01\0\0" + MESSAGE + b"\0\x02")
        self.assertEqual((record["verdict"], record["counts"]), ("passed", {}))
        self.assertEqual((record["notes"]["quoted"]["count"], record["notes"]["quoted"]["samples"]), (1, [repr(MESSAGE.decode())]))
        self.assertEqual(self.rules("guide.md", "Pick the entry that reads: %s\n" % MENU_LINE.decode()), ("passed", {}, {"quoted": 1}))
        self.assertIn("quoted", pc.NOTES)
        self.assertNotIn("quoted", pc.REFUSING)

    def test_one_file_that_quotes_the_images_texts_in_bulk_is_a_finding(self):
        """A phrase quoted is a quote; a list of the image's strings is a copy of a part of it. The bound is a count of
        different phrases and a count of their bytes, for one file. Held on each side of each, with the bounds set
        small for the test: the made-up image has three phrases to quote."""
        three = "It says: %s\nthen: %s\nand the entry reads: %s\n" % (MESSAGE.decode(), TWO_LINES[36:].decode(), MENU_LINE.decode())
        sizes = (len(MESSAGE), len(TWO_LINES[36:]), len(MENU_LINE))
        self.assertEqual(self.rules("guide.md", three), ("passed", {}, {"quoted": 3}))            # far below both bounds as they are
        self.assertEqual((pc.BULK_TEXTS, pc.BULK_BYTES), (64, 4096))
        for texts, size, refused in ((3, 4096, True), (4, 4096, False), (64, sum(sizes), True), (64, sum(sizes) + 1, False)):
            with mock.patch.object(pc, "BULK_TEXTS", texts), mock.patch.object(pc, "BULK_BYTES", size):
                record = self.judged("guide.md", three)
            self.assertEqual((record["verdict"], record["counts"]), ("refused", {"text": 3}) if refused else ("passed", {}), (texts, size))
            if refused:
                self.assertIn("3 different phrases of the image, %d bytes of them: its texts in bulk" % sum(sizes), record["findings"][0]["what"])
        twice = three + "Once more: %s\n" % MESSAGE.decode()                # the same phrase twice is one phrase
        with mock.patch.object(pc, "BULK_TEXTS", 4):
            self.assertEqual(self.rules("guide.md", twice), ("passed", {}, {"quoted": 4}))
        with mock.patch.object(pc, "BULK_TEXTS", 3):                        # and a second file with the same bytes has the first one's verdict
            check = self.check()
            folder = os.path.join(self.folder, "bulk")
            first, second = (check.file(write(os.path.join(folder, name), three), name) for name in ("a.md", "b.md"))
        self.assertEqual((first["counts"], second["counts"], second.get("same_as")), ({"text": 3}, {"text": 3}, "a.md"))

    def test_the_white_space_at_a_phrases_ends_does_not_count(self):
        """Three words of fourteen letters between two spaces are sixteen bytes in the image and no phrase."""
        self.assertEqual(MESSAGE[9:23], b"head is %d ste")
        self.assertEqual(pc.sort_text(b" tape head is %d", set())[:2], (None, None))     # sixteen with the space, fifteen without
        self.assertEqual(pc.sort_text(b"tape head is %d ", set())[:2], (None, None))
        self.assertEqual(pc.sort_text(b"tape head is %d s", set())[:2], (None, "quoted"))  # a whole string, as far as the caller says
        self.assertEqual(pc.PHRASE, 16)

    def test_a_short_phrase_counts_when_it_is_a_whole_string_and_a_long_one_wherever_it_lies(self):
        """A menu line or a short message is quoted whole: a phrase. A few words from a longer string are words two
        English texts share, or a few words quoted: pieces, until there are twenty-four bytes of them. Both are let
        through; only a phrase counts towards the bound on what a file quotes."""
        for text, phrase in ((MENU_LINE.decode(), True),                       # seventeen, a whole string of the image
                             ("The tape head is", False),                      # sixteen, from a string's first letter
                             ("steps off its mark", False),                    # eighteen, to its last
                             ("tape head is %d steps", False),                 # twenty-one from the middle
                             ("tape head is %d steps off", True)):             # twenty-five from the middle
            verdict, counts, notes = self.rules("notes.md", "It says: %s, we think.\n" % text)
            self.assertEqual((verdict, counts, notes), ("passed", {}, {"quoted": 1} if phrase else {"pieces": 1}), text)
        self.assertEqual(pc.sort_text(b"tape head is %d steps", set(), False, False)[:2], (None, "pieces"))
        self.assertEqual(pc.sort_text(b"tape head is %d steps", set(), True, False)[:2], (None, "pieces"))
        self.assertEqual(pc.sort_text(b"tape head is %d steps", set(), False, True)[:2], (None, "pieces"))
        self.assertEqual(pc.sort_text(b"tape head is %d steps", set(), True, True)[:2], (None, "quoted"))
        self.assertEqual(pc.sort_text(b"x\0tape head is %d steps\0y", set(), False, False)[:2], (None, "quoted"))   # zero bytes around it, in the run
        self.assertEqual(pc.sort_text(b"x\0tape head is %d steps", set(), False, False)[:2], (None, "pieces"))
        self.assertEqual((pc.LONG, pc.PAD), (24, 8))

    def test_a_strings_edge_is_a_zero_byte_with_at_most_some_white_space_before_it(self):
        self.assertEqual(pc.string_edges(MESSAGE[4:25], self.images), (False, False))
        self.assertEqual(pc.string_edges(MESSAGE[:16], self.images), (True, False))
        self.assertEqual(pc.string_edges(MENU_LINE, self.images), (True, True))       # four spaces before it, a line's end behind it
        self.assertEqual(pc.string_edges(TWO_LINES[36:56], self.images), (False, False))     # a line's end is no edge: the string goes on
        self.assertEqual(self.rules("notes.md", "It goes: and line two goes on, we think.\n"), ("passed", {}, {"pieces": 1}))
        self.assertEqual(self.rules("notes.md", "It goes: and line two goes on from there\n")[2], {"quoted": 1})   # thirty-one bytes

    def test_the_wording_of_a_public_licence_is_let_through(self):
        """The image carries the licences of the libraries it is built with. A file of ours under the same licence
        has the same words, and they are nobody's secret: told by where the run lies in the image."""
        at = TEXT_AT + TEXT.index(b"Permission is hereby")
        self.assertEqual(self.images.public, [(at, TEXT_AT + TEXT.index(b"\0", TEXT.index(b"DEALINGS IN THE")))])
        verdict, counts, notes = self.rules("LICENSE", b"Copyright (c) 2026 somebody else\n\n" + LICENCE[LICENCE.index(b"Permission"):])
        self.assertEqual((verdict, counts, list(notes)), ("passed", {}, ["licence"]))
        before = b"It is " + LICENCE[:36] + b"\n"                           # the same image text, before the licence begins
        self.assertEqual(self.rules("notes.md", before)[2], {"quoted": 1})  # a phrase like any other, and it counts towards the bound

    def test_one_name_is_let_through(self):
        self.assertEqual(self.rules("unit.bin", b"OURS\0\x01\0" + NAME + b"\0\0\x02"), ("passed", {}, {"names": 1}))
        self.assertEqual(pc.sort_text(b"stroke-miterlimit=\"10\"", set())[:2], (None, "names"))

    def test_a_row_of_names_in_the_images_order_is_a_finding(self):
        record = self.judged("unit.bin", b"OURS\0\x01\0\0" + ROW + b"\0\0\x02")
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"text": 1}))
        together = TEXT[:TEXT.index(ROW) + 5]                               # the message, the name and the row's first name, as the image has them
        self.assertEqual(self.rules("unit.bin", b"OURS\0\x01\0\0" + together + b"\0\0\x02")[:2], ("refused", {"text": 1}))   # a row all the same, with a phrase in it
        self.assertEqual(pc.sort_text(b"\0" + MESSAGE + b"\0\0" + NAME + b"\0", set())[:2], ("text", None))                # two strings that stand whole: the row comes first
        self.assertEqual(pc.sort_text(b"\0alpha\0\0\0bravo\0\0\0charlie\0delta\0", set())[:2], ("text", None))
        self.assertEqual(pc.sort_text(b"\0alpha\0\0\0bravo\0\0\0charlie\0", set())[:2], ("text", None))    # seventeen letters
        self.assertEqual(pc.sort_text(b"\0alpha\0\0\0bravo\0\0\0char", set())[:2], (None, None))          # two whole names, ten letters
        self.assertEqual(pc.sort_text(b"a\0\0\0bravo\0\0\0charlie\0delta", set())[:2], (None, None))       # the ends may be pieces of a name

    def test_short_strings_that_meet_by_their_padding_are_nothing(self):
        self.assertEqual(self.rules("unit.bin", b"OURS\0\x01" + TEXT[TEXT.index(SHORT) - 4:TEXT.index(SHORT) + len(SHORT) + 4] + b"\x02"), ("passed", {}, {}))
        self.assertEqual(pc.sort_text(b"s\0\0\0../src/main/", set())[:2], (None, None))

    def test_a_text_a_person_has_read_is_let_through(self):
        listed = write(os.path.join(self.folder, "allow.txt"), "# texts read on 2026-11-16\n%s\n\nanother text\n" % MESSAGE.decode())
        allowed = pc.read_allowed(listed)
        self.assertEqual(allowed, {MESSAGE.decode(), "another text"})
        self.assertEqual(self.rules("unit.bin", b"OURS\0\x01\0\0" + MESSAGE + b"\0\x02", allowed=allowed), ("passed", {}, {"allowed": 1}))
        self.assertEqual(self.rules("guide.md", "Pick the entry that reads: %s\n" % MESSAGE.decode(), allowed=allowed), ("passed", {}, {"allowed": 1}))


LINK = 0x8d2f36cc


def a_pack(folder, entries, code=b""):
    code = bytes([0x67, 0x01, 0x40, 0x01, 0x52, 0x00, 0x10, 0x00]) + code
    code += bytes(-len(code) % 4)
    table = b"".join(entries)
    body = code + table + struct.pack("<I", 0xffffffff)
    start = LINK + len(code)
    data = pack_build.make3(body, LINK, len(body), 64, "small", 0x33657797, (start, start + len(table)), exports=[("op1f_small", 0)])
    return write(os.path.join(folder, "small.pack"), data), pack_build.HEADER3_BYTES + len(code)


class APack(WithAStock):
    def test_a_table_entry_with_the_images_bytes_is_a_finding_and_one_of_the_new_kind_is_not(self):
        old = struct.pack("<5I", 4, SITE_AT, LINK, *struct.unpack("<2I", SITE))
        word = struct.pack("<5I", 0, WORDS_AT, LINK + 4, struct.unpack("<I", WORDS[:4])[0], 0)
        new = struct.pack("<5I", 0x00080014, SITE_AT, LINK, zlib.crc32(SITE), LINK + 0x40)
        other = struct.pack("<5I", 4, SITE_AT + 0x20, LINK, 0x11223344, 0x55667788)       # not what the image has there
        path, table = a_pack(os.path.join(self.folder, "old"), [old, word, other])
        record = self.check().file(path, "small.pack")
        self.assertEqual((record["kind"], record["verdict"], record["counts"]), ("pack", "refused", {"table": 12}))
        self.assertEqual([(item["offset"], item["what"].rsplit(": ", 1)[1]) for item in record["findings"]],
                         [(table + 12, SITE.hex()), (table + 32, WORDS[:4].hex())])
        path, _table = a_pack(os.path.join(self.folder, "new"), [new])
        self.assertEqual(self.check().file(path, "small.pack")["verdict"], "passed")

    def test_the_bytes_of_a_place_of_change_in_the_body_are_found(self):
        new = struct.pack("<5I", 0x00080014, SITE_AT, LINK, zlib.crc32(SITE), LINK + 0x40)
        path, _table = a_pack(os.path.join(self.folder, "replay"), [new], code=b"\x11\x22" + SITE + b"\x33\x44")
        record = self.check().file(path, "small.pack")
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"site": 1}))
        self.assertEqual(record["findings"][0]["offset"], pack_build.HEADER3_BYTES + 10)
        self.assertIn("in the body", record["findings"][0]["what"])

    def test_a_pack_file_that_does_not_hold_together_is_searched_as_a_plain_binary(self):
        """A damaged pack is kept in the tree for a test of the base. It must not end the check of the other files."""
        old = struct.pack("<5I", 4, SITE_AT, LINK, *struct.unpack("<2I", SITE))
        path, table = a_pack(os.path.join(self.folder, "damaged"), [old])
        with open(path, "rb") as f:
            data = bytearray(f.read())
        data[pack_build.HEADER3_BYTES + 2] ^= 0x10                          # one bit of the body: the checksum no longer holds
        write(path, bytes(data))
        record = self.check().file(path, "small.pack")
        self.assertEqual((record["kind"], record["verdict"], record["counts"]), ("pack", "refused", {"site": 1}))
        self.assertEqual(record["findings"][0]["offset"], table + 12)       # the same eight bytes, found as a place's
        self.assertIn("cannot be read as one", record["notes"]["no pack"]["samples"][0])

    def test_a_picture_that_stands_whole_in_a_packs_body_is_judged(self):
        """The packs of before J5e carried their pictures inside the body. The first run over the tree found none of
        them: only the pictures a pack lists were looked at."""
        new = struct.pack("<5I", 0x00080014, SITE_AT, LINK, zlib.crc32(SITE), LINK + 0x40)
        path, _table = a_pack(os.path.join(self.folder, "art"), [new], code=b"\x11\x22\0" + STOCK_PICTURE + b"\0" + OUR_PICTURE + b"\0")
        record = self.check().file(path, "small.pack")
        self.assertEqual((record["verdict"], list(record["counts"])), ("refused", ["picture"]))
        self.assertEqual(len(record["findings"]), 1)                        # the stock picture, and not ours beside it
        self.assertIn("in the body at byte", record["findings"][0]["what"])
        path, _table = a_pack(os.path.join(self.folder, "ours"), [new], code=b"\x11\x22\0" + OUR_PICTURE + b"\0")
        self.assertEqual(self.check().file(path, "small.pack")["verdict"], "passed")

    def test_a_run_of_the_image_in_the_body_is_found(self):
        new = struct.pack("<5I", 0x00080014, SITE_AT, LINK, zlib.crc32(SITE), LINK + 0x40)
        path, _table = a_pack(os.path.join(self.folder, "run"), [new], code=b"\x11" + SIXTEEN + b"\x22" + b"\0\0" + MESSAGE + b"\0")
        record = self.check().file(path, "small.pack")
        self.assertEqual((record["counts"], record["notes"]["quoted"]["count"]), ({"bytes": 1}, 1))     # the code is found; the message is a quote
        path, _table = a_pack(os.path.join(self.folder, "row"), [new], code=b"\x11\x22\0\0" + ROW + b"\0\0")
        self.assertEqual(self.check().file(path, "small.pack")["counts"], {"text": 1})                  # and a row of the image's names is found in a pack too


# ---------------------------------------------------------------- a text file


class Hex(WithAStock):
    def test_the_bytes_of_a_place_of_change_in_each_way_of_writing_them(self):
        low, high = struct.unpack("<2I", SITE)
        ways = {
            "one string": '    "before": "%s",\n' % SITE.hex(),
            "one string with 0x": "the eight bytes 0x%s\n" % SITE.hex().upper(),
            "words as the memory holds them": "    .long 4, 0x%08x, op1f_x, 0x%08x, 0x%08x    /* a place */\n" % (SITE_AT, low, high),
            "words on lines of their own": "    .long 0x%08x\n    .long 0x%08x\n" % (low, high),
            "half words": "w0 w1 w2 w3: %s\n" % " ".join("%04x" % value for value in struct.unpack("<4H", SITE)),
            "bytes in a list": "STOCK = bytes([%s])\n" % ", ".join("0x%02x" % byte for byte in SITE),
            "bytes with spaces": "it reads %s there\n" % " ".join("%02x" % byte for byte in SITE),
            "escapes": 'STOCK = b"%s"\n' % "".join("\\x%02x" % byte for byte in SITE),
        }
        for way, text in ways.items():
            record = self.judged("notes.txt", "A line of ours.\n" + text + "Another line.\n")
            self.assertEqual((record["verdict"], record["counts"]), ("refused", {"site": 1}), way)
            self.assertEqual(record["findings"][0]["line"], 2, way)

    def test_other_bytes_of_the_image_written_as_hex(self):
        record = self.judged("notes.md", "Its first bytes are `%s`.\n" % CODE[0x150:0x15c].hex())
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"hex": 1}))
        self.assertIn("12 bytes", record["findings"][0]["what"])
        self.assertEqual(self.rules("notes.md", "Its first bytes are `%s`.\n" % CODE[0x150:0x157].hex()), ("passed", {}, {}))   # seven
        self.assertEqual(pc.HEX, 8)

    def test_a_row_of_plain_numbers_and_a_hash_are_not_hex(self):
        plain = " ".join("%02x" % byte for byte in DECIMALS)                # 10 20 30 ... : the image has these bytes
        self.assertEqual(self.images.stock.places(DECIMALS), [(TEXT_AT + TEXT.index(DECIMALS), "records")])
        self.assertEqual(self.rules("table.txt", "the steps: %s\n" % plain), ("passed", {}, {}))
        self.assertEqual(self.rules("table.txt", "the steps: %s\n" % " ".join("0x%02x" % byte for byte in DECIMALS))[1], {"hex": 1})
        digest = "f2e8f676973fffc4fb1a4aec35f497802309641612bad08201c8af4fb76fde15"
        self.assertEqual(self.rules("hashes.txt", "the image: %s\n" % digest), ("passed", {}, {}))

    def test_a_habit_of_the_instruction_set_is_not_a_copy(self):
        """Eight bytes of register saves stand at many places of the image: hex of code of ours that has them too."""
        self.assertGreaterEqual(len(self.images.stock.places(HABIT)), pc.MANY)
        self.assertEqual(self.rules("test.py", "EXPECTED = bytes.fromhex('%s')\n" % (HABIT.hex() + "0000")), ("passed", {}, {}))
        follows = HABITS[len(HABIT):len(HABIT) + 2]                         # what stands behind the first of them in the image:
        self.assertEqual(len(self.images.stock.places(HABIT + follows)), 1)  # ten bytes at one place, and still the habit
        self.assertEqual(self.rules("test.py", "EXPECTED = bytes.fromhex('%s')\n" % (HABIT + follows).hex()), ("passed", {}, {}))
        self.assertEqual(self.rules("test.py", "EXPECTED = bytes.fromhex('%s')\n" % CODE[0x150:0x15a].hex())[1], {"hex": 1})   # ten at one place

    def test_what_a_text_writes_as_hex(self):
        self.assertEqual(pc.hex_written("no hex here, 12 34 and 2026 only"), [])
        self.assertEqual([held for held, _at in pc.hex_written("x = 0x11223344, 0x55667788")],
                         [bytes.fromhex("4433221188776655"), bytes.fromhex("1122334455667788")])
        self.assertEqual(pc.hex_written("0x11223344 and 0x55667788"), [])   # a word between them: two numbers, not a row
        held, offsets = pc.hex_written("see\n" + "ab " * 8)[0]
        self.assertEqual((held, offsets[:3]), (b"\xab" * 8, [4, 7, 10]))


class Listings(WithAStock):
    def lines(self, address, data, sizes, text="R0 = R1;"):
        out, at = [], 0
        for size in sizes:
            out.append("%08x:\t%s \t%s" % (address + at, " ".join("%02x" % byte for byte in data[at:at + size]), text))
            at += size
        return "\n".join(out) + "\n"

    def test_lines_with_the_images_own_bytes(self):
        record = self.judged("dump.txt", "Disassembly of section .text:\n\n" + self.lines(CODE_AT + 0x60, CODE[0x60:], (2, 4, 2, 8)))
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"listing": 4}))
        self.assertIn("lines 3 to 6", record["findings"][-1]["what"])
        shorter = self.judged("dump.txt", " %x:\t%02x %02x       \tR0 = R1;\n" % (CODE_AT + 0x60, CODE[0x60], CODE[0x61]))
        self.assertEqual(shorter["counts"], {"listing": 1})                 # the form of the full listings, one line of two bytes

    def test_a_listing_of_code_of_ours_passes_and_is_searched_as_the_bytes_it_shows(self):
        ours = bytes(RANDOM.randrange(1, 255) for _ in range(24))
        self.assertEqual(self.rules("build.dis.txt", self.lines(OURS_AT, ours, (4, 2, 2, 8, 8))), ("passed", {}, {}))
        replay = ours[:6] + SITE + ours[6:12]                              # the place's bytes over three lines
        record = self.judged("build.dis.txt", self.lines(OURS_AT, replay, (4, 4, 4, 4, 4)))
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"site": 1}))
        self.assertEqual(record["findings"][0]["line"], 1)
        wrong = self.lines(CODE_AT + 0x60, ours, (4, 4))                    # a stock address, other bytes: not the image's
        self.assertEqual(self.rules("build.dis.txt", wrong), ("passed", {}, {}))

    def test_a_line_that_begins_with_a_stock_code_address(self):
        text = "The entry, read whole:\n\n%08x: LINK 0x10;\n%08x: [--SP] = (R7:4, P5:4);   the saved registers\n" % (SITE_AT, SITE_AT + 4)
        record = self.judged("study.md", text)
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"listing": 2}))
        self.assertEqual([item["line"] for item in record["findings"]], [3, 4])
        told = "`0x%08x`: the entry of the handler, which keeps its first argument and returns at once.\n" % SITE_AT
        self.assertEqual(self.rules("study.md", told), ("passed", {}, {}))
        ours = "%08x: LINK 0x10;\n" % OURS_AT
        self.assertEqual(self.rules("study.md", ours), ("passed", {}, {}))


class Quotes(WithAStock):
    def test_two_instructions_beside_a_stock_code_address_are_a_finding_and_one_is_let_through(self):
        two = "The handler 0x%08x begins `P1 = R0; LINK 0x10; [--SP] = (R7:4, P5:4);` and goes on at 0x%08x.\n" % (SITE_AT, SITE_AT + 8)
        record = self.judged("study.md", two)
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"quote": 1}))
        self.assertIn("3 instructions", record["findings"][0]["what"])
        one = "It keeps its third argument (`R5 = R2` at `0x%08x`) and branches on it.\n" % (SITE_AT + 0x16)
        self.assertEqual(self.rules("study.md", one), ("passed", {}, {"one instruction": 1}))
        self.assertEqual(pc.QUOTE, 2)

    def test_instructions_with_no_stock_address_beside_them_are_not_counted(self):
        ours = "New routines write `SP += -12` after their pushes and `SP += 12` before their pops; `R0 = [P5 + 0x10]` is read first.\n"
        self.assertEqual(self.rules("methods.md", ours), ("passed", {}, {}))
        module = "The stub at 0x%08x does `R0 = R1; R1 = R2;` and calls the C function.\n" % OURS_AT
        self.assertEqual(self.rules("methods.md", module), ("passed", {}, {}))
        told = "The handler at 0x%08x is called with the buffer and the length, and its answer is not used.\n" % SITE_AT
        self.assertEqual(self.rules("methods.md", told), ("passed", {}, {}))

    def test_in_an_assembly_source_only_the_comments_are_read(self):
        source = ("/* The entry of the handler.\n"
                  " *      handler 0x%08x:  P1 = R0; LINK 0x10;  -> continue at 0x%08x\n"
                  " */\n"
                  "op1f_entry:\n"
                  "    R0 = [SP++]; RETS = [SP++]; JUMP.a 0x%08x;     // back into the handler\n"
                  "    P1 = R0; LINK 0x10;\n") % (SITE_AT, SITE_AT + 8, SITE_AT + 8)
        record = self.judged("entry.s", source)
        self.assertEqual((record["counts"], [item["line"] for item in record["findings"]]), ({"quote": 1}, [2]))
        self.assertEqual(self.judged("entry.txt", source)["counts"], {"quote": 2})          # as a note, every line is read

    def test_what_stands_in_comments_and_what_counts_as_an_instruction(self):
        self.assertEqual(pc.comments_only(["a /* b */ c // d", "e /* f", "g */ h", "i"]), [" b   d", " f", "g ", ""])
        for text, count in (("P1 = R0; LINK 0x10; [--SP] = (R7:4, P5:4);", 3), ("R0 = [0x08001234]", 1), ("CALL.a 0x8d2f4100 with R0 = 0x1234", 2),
                            ("LINK 0; UNLINK; RTS", 3), ("CC = R0 <= 7 (IU)", 1), ("IF CC JUMP 0x8d2f4050", 1), ("B[P4] = R0; W[P1 + 2] = R1", 2),
                            ("values[k] = 0; index = 4; names[2] = other", 0), ("the call of the handler returns", 0), ("R0 == 0 and R1 != 2", 0)):
            self.assertEqual(pc.statements(text), count, text)


LOG = "".join("  %2d.%03d.%03d: I main_thread[%d]: step %d of the start done\n" % (4 + k, 238 + k, 573 + k, 348 + k, k) for k in range(6))


class Logs(WithAStock):
    def test_a_log_read_from_the_unit_and_log_lines_quoted_in_a_note(self):
        record = self.judged("session.txt", LOG)
        self.assertEqual((record["verdict"], record["counts"]), ("refused", {"log": 6}))
        note = "What the start wrote:\n\n" + LOG[:LOG.index("\n", LOG.index("\n") + 1) + 1] + "\n".join("A line of ours, number %d." % k for k in range(8)) + "\n"
        self.assertEqual(self.rules("result.md", note), ("passed", {}, {"log lines": 2}))
        self.assertEqual((pc.LOG_LINES, pc.LOG_SHARE), (3, 2))


def a_set(lines):
    """A set of instructions of the second kind from dial.svg: [("c", offset, length) | ("a", bytes)]."""
    body, size = b"", 0
    for line in lines:
        if line[0] == "c":
            body += b"c 0 %d %d\n" % (line[1], line[2])
            size += line[2]
        else:
            body += b"a %d\n" % len(line[1]) + line[1] + b"\n"
            size += len(line[1])
    return picture_make.MARK2 + b"size %d\n" % size + b"from dial.svg %d %08x\n" % (len(STOCK_PICTURE), zlib.crc32(STOCK_PICTURE)) + body


class Pictures(WithAStock):
    def test_a_picture_file(self):
        self.assertEqual(self.rules("ours.svg", OUR_PICTURE)[:2], ("passed", {}))
        record = self.judged("theirs.svg", STOCK_PICTURE)
        self.assertEqual((record["verdict"], list(record["counts"])), ("refused", ["picture"]))
        self.assertIn("dial.svg", record["findings"][0]["what"])
        mixed = OUR_PICTURE.replace(b"</svg>", STOCK_LINE + b"\n</svg>")
        self.assertEqual(list(self.judged("mixed.svg", mixed)["counts"]), ["picture"])

    def test_a_picture_inside_another_file_and_a_stock_tag_loose_in_a_text(self):
        page = b"<html><body><p>Our page.</p>\n" + STOCK_PICTURE + b"</body></html>\n"
        self.assertEqual(list(self.judged("page.html", page)["counts"]), ["picture"])
        tool = b"PILL = '" + STOCK_LINE + b"'\n"
        record = self.judged("tool.py", tool)
        self.assertEqual((record["verdict"], list(record["counts"])), ("refused", ["picture"]))
        self.assertEqual(record["findings"][0]["line"], 1)
        ours = b"DOT = '<circle fill=\"#3C6087\" cx=\"5\" cy=\"6\" r=\"7\"/>'\n"
        self.assertEqual(self.rules("tool.py", ours), ("passed", {}, {}))

    def test_a_set_of_instructions(self):
        at = STOCK_PICTURE.index(STOCK_LINE)
        whole = a_set([("c", 0, len(STOCK_PICTURE))])
        self.assertEqual(self.rules("dial.svg", whole)[:2], ("passed", {}))                 # everything from the unit's own picture
        carried = a_set([("c", 0, at), ("a", STOCK_LINE), ("c", at + len(STOCK_LINE), len(STOCK_PICTURE) - at - len(STOCK_LINE))])
        self.assertEqual(list(self.judged("dial.svg", carried)["counts"]), ["picture"])    # the same picture, the line carried
        broken = a_set([("c", 0, len(STOCK_PICTURE) + 1)])
        record = self.judged("dial.svg", broken)
        self.assertEqual((record["verdict"], record["counts"]), ("not judged", {}))
        self.assertIn("cannot be made", record["not_judged"][0])

    def test_a_text_that_only_looks_like_a_picture_does_not_stop_the_check(self):
        """A test's broken paths and a tool's template: a path is looked at before it is handed over to the measure,
        whose own path reader did not come back from a number behind a close until 2026-11-16 (it refuses one now)."""
        for value, reads in ((b"M1,2l3,4z", True), (b"m1,2 3,4", True), (b"M1,2z5,5", False), (b"5,5L1,2", False), (b"%s", False), (b"M1,2x3", False), (b"", False)):
            self.assertEqual(pc.path_data(value), reads, value)
        cases = b'CASES = ((b\'<path d="M1,2z5,5"/>\', "a number after its close"), (b\'<path d="5,5L1,2"/>\', "begins with a number"))\n'
        self.assertEqual(self.rules("test_cases.py", cases), ("passed", {}, {}))
        template = b'PAGE = \'<svg><path d="%s"/><path d="M1,2z5,5"/></svg>\'\n'
        self.assertEqual(self.rules("tool.py", template), ("passed", {}, {}))
        carried = b"PAGE = '<svg><path d=\"M1,2z5,5\"/>" + STOCK_LINE + b"</svg>'\n"     # no picture, and a stock tag in it all the same
        self.assertEqual(list(self.judged("tool.py", carried)["counts"]), ["picture"])
        self.assertEqual(pc.svg_regions(b"a <svg>1</svg> b <svg x='1'>2</svg > c <svg> never closed"), [(2, 14), (17, 36)])

    def test_without_the_stock_pictures_a_picture_is_not_judged(self):
        record = self.judged("theirs.svg", STOCK_PICTURE, display=None)
        self.assertEqual((record["verdict"], record["counts"]), ("not judged", {}))

    def test_a_picture_of_pixels_is_not_judged_and_strict_counts_it(self):
        record = self.judged("screen.png", b"\x89PNG\r\n\x1a\n" + bytes(64))
        self.assertEqual((record["kind"], record["verdict"]), ("pixels", "not judged"))
        self.assertEqual((pc.answer([record]), pc.answer([record], strict=True)), (0, 1))


# ---------------------------------------------------------------- the command


class TheCommand(WithAStock):
    def run_main(self, *arguments, forms=False):
        said = io.StringIO()
        fixed = [] if forms else ["--stock", self.records, "--profile", self.profile, "--display", self.pictures]
        with contextlib.redirect_stdout(said):
            code = pc.main(fixed + list(arguments))
        return code, said.getvalue()

    def tree(self, name):
        folder = os.path.join(self.folder, name)
        write(os.path.join(folder, "docs", "guide.md"), "How to copy a file onto the unit.\n")
        write(os.path.join(folder, "src", "unit.cpp"), "int answer() { return 42; }\n")
        write(os.path.join(folder, ".git", "objects", "x.ldr"), b"not looked at")
        return folder

    def test_a_folder_with_nothing_of_the_image_answers_0(self):
        code, said = self.run_main(self.tree("clean"), "--passed")
        self.assertEqual(code, 0, said)
        self.assertIn("2 file(s): 0 refused, 2 passed, 0 not judged", said)
        self.assertIn("answer: 0 (nothing of the stock firmware was found)", said)
        self.assertNotIn(".git", said)

    def test_a_finding_answers_1_and_says_what_and_where(self):
        folder = self.tree("found")
        write(os.path.join(folder, "src", "table.s"), "    .long 4, 0x%08x, op1f_x, 0x%08x, 0x%08x\n" % ((SITE_AT,) + struct.unpack("<2I", SITE)))
        write(os.path.join(folder, "old", "os.ldr"), b"never opened")
        code, said = self.run_main(folder)
        self.assertEqual(code, 1, said)
        self.assertIn("REFUSED    %s" % os.path.join(folder, "src", "table.s").replace("\\", "/"), said)
        self.assertIn("site     line 1: the 8 stock bytes of the place of change 0x%08x (g1): %s" % (SITE_AT, SITE.hex()), said)
        self.assertIn("name    : never goes, by its ending .ldr: a firmware image", said)
        self.assertIn("4 file(s): 2 refused, 2 passed, 0 not judged", said)
        self.assertNotIn("guide.md", said)                                  # a file that passed is counted, not printed

    def test_a_list_of_files_and_the_record(self):
        folder = self.tree("listed")
        bad = write(os.path.join(folder, "notes.md"), "Its bytes are %s.\n" % SITE.hex())
        listed = write(os.path.join(folder, "files.txt"), "%s\n%s\n" % (os.path.join(folder, "docs", "guide.md"), bad))
        out = os.path.join(folder, "record.json")
        code, _said = self.run_main("--list", listed, "--json", out)
        self.assertEqual(code, 1)
        with open(out, encoding="utf-8") as f:
            record = json.load(f)
        self.assertEqual((record["answer"], record["summary"]["files"], record["summary"]["refused"], record["rules"]["RUN"]), (1, 2, 1, 16))
        self.assertEqual([one["verdict"] for one in record["files"]], ["passed", "refused"])
        code, said = self.run_main("--list", listed, "--json", out)
        self.assertEqual(code, 2)
        self.assertIn("does not overwrite", said)

    def test_what_cannot_be_checked_answers_2(self):
        self.assertEqual(self.run_main()[0], 2)                              # no file
        self.assertEqual(self.run_main(os.path.join(self.folder, "no such file.md"))[0], 2)
        said = io.StringIO()
        with contextlib.redirect_stdout(said):
            code = pc.main(["--stock", os.path.join(self.folder, "no records"), self.tree("nostock")])
        self.assertEqual(code, 2, said.getvalue())

    def test_the_part_that_needs_no_stock_image_says_so(self):
        folder = self.tree("forms")
        write(os.path.join(folder, "notes.md"), "Its bytes are %s.\n" % SITE.hex())       # only the image can tell
        write(os.path.join(folder, "study.md"), "%08x: LINK 0x10;\n" % SITE_AT)            # told by its form
        code, said = self.run_main(folder, "--forms-only", forms=True)
        self.assertEqual(code, 1)
        self.assertIn("needs no stock image", said)
        self.assertIn("4 file(s): 1 refused, 3 passed", said)
        os.remove(os.path.join(folder, "study.md"))
        code, said = self.run_main(folder, "--forms-only", forms=True)
        self.assertEqual(code, 0)
        self.assertIn("by the rules that need no image", said)

    def test_the_same_bytes_met_twice_are_judged_once(self):
        folder = self.tree("twice")
        for name in ("a.md", "b.md"):
            write(os.path.join(folder, name), "Its bytes are %s.\n" % SITE.hex())
        check = self.check()
        first, second = (check.file(os.path.join(folder, name), name) for name in ("a.md", "b.md"))
        self.assertEqual((first["counts"], second["counts"], second.get("same_as")), ({"site": 1}, {"site": 1}, "a.md"))
        self.assertNotIn("same_as", first)

    def test_a_named_file_is_searched_all_the_same_when_asked(self):
        path = write(os.path.join(self.folder, "deep", "device-logs", "notes.txt"), "Its bytes are %s.\n" % SITE.hex())
        shown = "device-logs/notes.txt"
        self.assertEqual(self.check().file(path, shown)["counts"], {"name": 1})
        self.assertEqual(self.check(deep=True).file(path, shown)["counts"], {"name": 1, "site": 1})


# ---------------------------------------------------------------- two firmwares in one run


def make_firmware(folder, size, code):
    """A folder of memory records of a made-up firmware whose index names `size` and two records, with `code` at
    CODE_AT."""
    records = []
    for number, (address, data) in enumerate(((CODE_AT, code), (TEXT_AT, TEXT))):
        name = "%03d_%08x.bin" % (number, address)
        write(os.path.join(folder, name), data)
        records.append({"index": number, "target": address, "count": len(data), "argument": 0, "fill": False, "stored": len(data), "file": name})
    write(os.path.join(folder, "index.json"), json.dumps({"source": "a test", "bytes": size, "records": records}))
    return folder


class TwoFirmwares(unittest.TestCase):
    """The records of two firmwares in one run: the same address holds other bytes in each. A profile that names its
    firmware's size and record count is read in that firmware's records, wherever they stand in the list; read in the
    first image that holds the place, a newer firmware's place would hold the older one's bytes, and a file with the
    newer bytes would pass."""

    @classmethod
    def setUpClass(cls):
        cls.folder = tempfile.mkdtemp(prefix="publish-two-")
        newer = random.Random(20261119)
        cls.newer_code = bytes(newer.randrange(1, 255) for _ in range(0x200))
        cls.older = make_firmware(os.path.join(cls.folder, "older"), 1000, CODE)
        cls.newer = make_firmware(os.path.join(cls.folder, "newer"), 2000, cls.newer_code)
        site = {"address": "0x%08x" % SITE_AT, "length": 8, "group": "g1"}
        cls.profiles = {name: write(os.path.join(cls.folder, name + ".json"), json.dumps(dict(named, patches=[site])))
                        for name, named in (("older", {"stock_bytes": 1000, "stock_records": 2}),
                                            ("newer", {"stock_bytes": 2000, "stock_records": 2}),
                                            ("sized", {"stock_bytes": 2000}), ("unnamed", {}),
                                            ("elsewhere", {"stock_bytes": 3000, "stock_records": 2}))}

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.folder, ignore_errors=True)

    def site_bytes(self, sources, *names):
        images = pc.Images(sources)
        return [data for _address, data, _group in pc.read_places([self.profiles[name] for name in names], images)[1]]

    def test_each_profile_reads_its_own_firmware(self):
        newer_site = self.newer_code[0x40:0x48]
        self.assertNotEqual(newer_site, SITE)
        for sources in ([self.older, self.newer], [self.newer, self.older]):
            self.assertEqual(self.site_bytes(sources, "older"), [SITE])
            self.assertEqual(self.site_bytes(sources, "newer"), [newer_site])
            self.assertEqual(self.site_bytes(sources, "sized"), [newer_site])      # a size alone matches too

    def test_a_profile_no_image_matches_is_read_in_the_first_that_holds_the_place(self):
        self.assertEqual(self.site_bytes([self.older, self.newer], "unnamed"), [SITE])
        self.assertEqual(self.site_bytes([self.newer, self.older], "elsewhere"), [self.newer_code[0x40:0x48]])

    def test_a_file_with_the_newer_firmwares_place_is_refused(self):
        """Eight bytes in a binary are fewer than a run, so only the place tells them."""
        path = write(os.path.join(self.folder, "files", "ours.bin"), beside(self.newer_code, 0x40, 8))
        images = pc.Images([self.older, self.newer])
        held = pc.Check(images, pc.read_places([self.profiles["older"], self.profiles["newer"]], images), None).file(path, "ours.bin")
        self.assertEqual((held["verdict"], held["counts"]), ("refused", {"site": 1}))
        waved = pc.Check(images, pc.read_places([self.profiles["unnamed"]], images), None).file(path, "ours.bin")
        self.assertEqual((waved["verdict"], waved["counts"]), ("passed", {}))      # read in the older records only


# ---------------------------------------------------------------- the tree as it stands


RECORDS = os.path.join(ROOT, "analysis", "os-records")
BOOT = os.path.join(ROOT, "analysis", "boot-records")
DISPLAY = os.path.join(ROOT, "op-1field backup", "content", "display")
PROFILES = [os.path.join(ROOT, "profiles", name) for name in ("op1f-1.7.0.json", "op1f-1.7.0-l1-test.json")]
TODAY = os.path.join(ROOT, "candidates", "pack-p3b")
HAVE = (all(os.path.isfile(os.path.join(folder, "index.json")) for folder in (RECORDS, BOOT)) and os.path.isdir(DISPLAY)
        and all(os.path.isfile(path) for path in PROFILES) and os.path.isfile(os.path.join(TODAY, "kit.pack")))


@unittest.skipUnless(HAVE, "the stock image's records, the stock pictures, the profiles and today's packs are needed")
class TheTreeAsItStands(unittest.TestCase):
    """analysis/publish_check_study.md: what the check says of files of 2026-11-16. The pack files are kept artifacts."""

    @classmethod
    def setUpClass(cls):
        images = pc.Images([RECORDS, BOOT])
        cls.check = pc.Check(images, pc.read_places(PROFILES, images), picture_provenance.Stock(DISPLAY))

    def record(self, *parts):
        return self.check.file(os.path.join(ROOT, *parts), "/".join(parts))

    def test_todays_pack_files_are_refused_for_their_tables(self):
        held = {name: self.record("candidates", "pack-p3b", name)["counts"].get("table", 0) for name in sorted(os.listdir(TODAY))}
        self.assertEqual(held, TABLES)
        self.assertEqual(sum(held.values()), sum(TABLES.values()))

    def test_no_pack_file_of_today_carries_anything_of_a_stock_picture_and_the_set_before_did(self):
        for name in sorted(os.listdir(TODAY)):
            self.assertNotIn("picture", self.record("candidates", "pack-p3b", name)["counts"], name)
        before = self.record("candidates", "pack-p3", "fx train.pack")
        self.assertEqual(before["counts"]["picture"], 2443)

    def test_the_base_and_the_test_pack_of_the_clean_base_pass(self):
        for parts in (("open-field", "base-1.7.0-v1.bin"), ("candidates", "pack-l1", "trampolines.pack"), ("build", "build_l1_base.bin")):
            record = self.record(*parts)
            self.assertEqual((record["verdict"], record["counts"]), ("passed", {}), parts)

    def test_the_base_before_it_is_refused_for_its_tables_and_each_byte_is_counted_once(self):
        """A binary beside its link: the tables are read by their structure (three entries, twenty bytes, as
        analysis/pack_clean_study.md has them), and the same bytes are not found a second time as a place's."""
        self.assertEqual(self.record("build", "build_j5c_base.bin")["counts"], {"table": 20})

    def test_sources(self):
        self.assertEqual(self.record("module", "build_l1_base_table.s")["verdict"], "passed")          # checksums where the bytes stood
        self.assertIn("site", self.record("module", "build_j5d_kit_table.s")["counts"])                 # the bytes as words
        maps = self.record("module", "build_k_maps.cpp")                                                # six of the image's own messages:
        self.assertEqual((maps["verdict"], maps["counts"], maps["notes"]["quoted"]["count"]), ("passed", {}, 6))     # quoted, a finding until 2026-11-17
        self.assertEqual(self.record("candidates", "pack-l2", "kit.pack")["counts"].get("text"), 2)     # two rows of the image's names, as it has them: still one
        dump = os.path.join(ROOT, "analysis", "bootloader_ldr_allstrings.txt")                          # the bootloader's strings, one a line
        if os.path.isfile(dump):
            listed = self.record("analysis", "bootloader_ldr_allstrings.txt")
            self.assertEqual((listed["verdict"], listed["counts"]), ("refused", {"text": 577}))
            self.assertIn("577 different phrases of the image, 20672 bytes of them: its texts in bulk", listed["findings"][0]["what"])
        self.assertIn("quote", self.record("module", "build_s_bench.s")["counts"])                      # a comment that quotes the entry
        self.assertIn("site", self.record("profiles", "op1f-1.7.0.json")["counts"])                     # every place's "before"

    def test_this_steps_own_files_pass(self):
        for parts in (("docs", "15-lawyer-questions.md"), ("tools", "publish_check.py"), ("tools", "tests", "test_publish_check.py"),
                      ("analysis", "publish_check_study.md")):
            if not os.path.isfile(os.path.join(ROOT, *parts)):
                continue
            record = self.record(*parts)
            self.assertEqual((record["verdict"], record["counts"]), ("passed", {}), (parts, record["findings"][:3]))

    def test_the_dev_mode_pages_code_computes_the_table_it_carried(self):
        """open-field/ferret.mjs carried the table of its frame check as 256 bytes of hex, and the same 256 bytes stand
        in the stock images; since 2026-11-16 it computes them. Here the formula's bytes are written out as the file
        had them, in a file of the test's own, and the check finds them in the images at two places: what the page
        computes is the firmware's table. The page's file has no such finding any more (the maker's name in the small
        letters of the unit's USB port is in it: quoted, and let through since 2026-11-17), and its validator passes,
        which holds the table the page computes to the SHA-256 of these same bytes."""
        table = bytearray()
        for entry in range(256):                            # a reflected CRC-8, polynomial 0xB2, every entry XORed with 0xEA
            for _ in range(8):
                entry = (entry >> 1) ^ 0xB2 if entry & 1 else entry >> 1
            table.append(entry ^ 0xEA)
        folder = tempfile.mkdtemp(prefix="publish-table-")
        self.addCleanup(shutil.rmtree, folder, ignore_errors=True)
        written_out = b'const CRC8 = Uint8Array.from("' + bytes(table).hex().encode("ascii") + b'".match(/../g).map(h => parseInt(h, 16)));\n'
        carried = self.check.file(write(os.path.join(folder, "carried.mjs"), written_out), "carried.mjs")
        self.assertEqual((carried["verdict"], carried["counts"]), ("refused", {"hex": 1}))
        self.assertTrue(carried["findings"][0]["what"].startswith("256 bytes, at "), carried["findings"])
        self.assertIn(" in the image (2 places)", carried["findings"][0]["what"])
        page = self.record("open-field", "ferret.mjs")
        self.assertEqual((page["verdict"], page["counts"], page["notes"]["quoted"]["count"]), ("passed", {}, 1), page["findings"][:3])
        with open(os.path.join(ROOT, "open-field", "ferret.mjs"), encoding="utf-8") as f:
            self.assertIn("c = c & 1 ? (c >> 1) ^ 0xb2 : c >> 1; return c ^ 0xea;", f.read())              # the formula above, as the page has it
        validator = self.record("open-field", "validate-devmode.mjs")
        self.assertEqual((validator["verdict"], validator["counts"]), ("passed", {}), validator["findings"][:3])
        with open(os.path.join(ROOT, "open-field", "validate-devmode.mjs"), encoding="utf-8") as f:
            self.assertIn('"%s"' % hashlib.sha256(bytes(table)).hexdigest(), f.read())


TABLES = {"8 tracks.pack": 584, "fx train.pack": 196, "kit.pack": 64, "lfo.pack": 64, "sidechain.pack": 48}


if __name__ == "__main__":
    unittest.main()

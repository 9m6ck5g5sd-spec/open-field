#!/usr/bin/env python3
"""Which calls made by hand-written routines give their callee no argument space, and what those callees do.

  python -I tools/stub_calls.py --listing build/<name>.dis.txt [--all] module/*.s

The Blackfin calling convention has the caller keep twelve bytes free above the return address (at [SP + 0] to
[SP + 11] when the CALL is made): the callee may store its three register arguments there, and compiled code, the
firmware's included, also uses the words as spill slots. A hand-written routine that pushes RETS and calls at once
has its saved RETS in that place. That stands only while the callee, and whatever the callee jumps to as a tail
call, never writes there. An early build of this project froze a unit on it: the compiler had turned the last call
of a function of ours into a tail call of a function of the firmware, which keeps a word right above its own frame,
and that word landed on the saved RETS of the routine around it.

The listing is the disassembly of our own linked code, which the toolchain makes:
`bfin-elf-objdump -d build/<name>.elf > build/<name>.dis.txt`.

For every CALL in a hand-written routine this prints the bytes of argument space the routine has made by then
(counted from the routine's label: a push or a pop leaves none, `SP += -N` makes N, `LINK N` makes N), and for a
call with fewer than twelve to a function of the module it reads the callee in the listing for

  - tail calls: a JUMP that leaves the function, through a register or to another function's address, which hands
    the routine's stack to that function as its caller's;
  - stores above its own frame: `[SP + N]` with N at or past the frame the prologue made, or `[FP + N]` at or past
    the same word where the function keeps FP as its frame pointer (the registers pushed before LINK lie between).

A call through a register goes to a function of the firmware, which the listing does not hold: it is printed so
that the function is read in your own copy of the firmware. Exit status 1 when a call without the space reaches a
callee with a tail call or a store above its frame.
"""

import argparse
import re
import sys

SPACE = 12

# Labels that are not entered by a call of their own and stand on an argument area all the same.
# op1f_tape_json_load_after (build_b_mixer.s) is where stock's tape json loader returns to: the trampoline before it
# put the label into RETS, so stock's own RTS comes there with SP as stock's caller had it at its CALL, and the
# twelve bytes at SP are the ones that caller kept free for the loader.
ENTRY_SPACE = {"op1f_tape_json_load_after": 12}

LABEL = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):")
PUSH = re.compile(r"\[--SP\]\s*=")
POP = re.compile(r"=\s*\[SP\+\+\]")
SP_ADD = re.compile(r"^SP\s*\+=\s*(-?\s*(?:0x[0-9a-fA-F]+|\d+))\s*;")
LINK = re.compile(r"^LINK\s+(0x[0-9a-fA-F]+|\d+)\s*;")
CALL = re.compile(r"^CALL\s+(\([A-Za-z0-9]+\)|[A-Za-z_][A-Za-z0-9_.]*)\s*;")
JUMP_OUT = re.compile(r"^JUMP(?:\.[SL])?\s+(\([A-Za-z0-9]+\)|[A-Za-z_][A-Za-z0-9_.]*)\s*;")


def number(text):
    text = text.replace(" ", "")
    return int(text, 16) if "0x" in text.lower() else int(text)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def routine_calls(path):
    """(routine, line number, callee, argument space in bytes) for every CALL of a hand-written source."""
    with open(path, encoding="utf-8") as f:
        raw = f.read()
    # comments out, the line numbers kept
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group().count("\n"), raw, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    calls, routine, space = [], None, 0
    for number_, line in enumerate(text.split("\n"), 1):
        for statement in [s.strip() + ";" for s in line.split(";") if s.strip()]:
            label = LABEL.match(statement)
            while label:
                routine = label.group(1)
                space = ENTRY_SPACE.get(routine, 0)
                statement = statement[label.end():].strip()
                label = LABEL.match(statement)
            if not statement or statement == ";" or statement.startswith("."):
                continue
            if PUSH.search(statement) or POP.search(statement) or statement.startswith("UNLINK"):
                space = 0
                continue
            link = LINK.match(statement)
            if link:
                space = number(link.group(1))
                continue
            add = SP_ADD.match(statement)
            if add:
                space = max(0, space - number(add.group(1)))
                continue
            call = CALL.match(statement)
            if call:
                calls.append((routine, number_, call.group(1), space))
    return calls


def listing_functions(path):
    """name -> (address, the body's instruction lines as (address, text)) from an objdump listing."""
    functions, name, body = {}, None, []
    head = re.compile(r"^([0-9a-f]{8}) <([^>]+)>:")
    line_re = re.compile(r"^([0-9a-f]{8}):\t[0-9a-f ]+\t(.*)$")
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            h = head.match(line)
            if h:
                if name:
                    functions[name] = (functions[name], body)
                name, body = h.group(2), []
                functions[name] = int(h.group(1), 16)
                continue
            m = line_re.match(line.rstrip("\n"))
            if m and name:
                body.append((int(m.group(1), 16), m.group(2).split("/*")[0].strip()))
    if name:
        functions[name] = (functions[name], body)
    return functions


def frame_layout(body):
    """(frame, fp_home) from a function's prologue: the bytes it takes below its entry SP, so that [SP + frame] is
    the first word of its caller's argument area while SP stands at the frame's base; and the same word's offset
    from FP, or None when the function has no LINK or uses FP as a general register afterwards."""
    before_link, link, after, linked = 0, 0, 0, False
    for _address, text in body[:16]:
        if re.match(r"(CALL|JUMP|IF |LSETUP|RTS)", text):
            break
        pushed = 0
        multi = re.match(r"\[--SP\] = \((?:R7:(\d))?(?:, )?(?:P5:(\d))?\);", text)
        if multi and (multi.group(1) or multi.group(2)):
            pushed = (4 * (8 - int(multi.group(1))) if multi.group(1) else 0) + (4 * (6 - int(multi.group(2))) if multi.group(2) else 0)
        elif text.startswith("[--SP] ="):
            pushed = 4
        elif text.startswith("LINK "):
            link, linked = 8 + number(text.split()[1].rstrip(";")), True
            continue
        elif text.startswith("SP += -"):
            pushed = number(text.split("-", 1)[1].split(";")[0])
        if linked:
            after += pushed
        else:
            before_link += pushed
    fp_home = 8 + before_link if linked else None
    if fp_home is not None and any(re.search(r"(^|\|\| )FP(\.[HL])? (=|\+=|-=) ", text) for _a, text in body[1:] if not text.startswith("FP = [SP++]")):
        fp_home = None                                  # FP holds something else in this function
    return before_link + link + after, fp_home


def frame_bytes(body):
    return frame_layout(body)[0]


def callee_findings(name, functions):
    """What a function of the module does to its caller's argument area: tail calls and stores above its frame."""
    address, body = functions[name]
    end = body[-1][0] if body else address
    frame, fp_home = frame_layout(body)
    found = []
    for at, text in body:
        jump = re.match(r"JUMP(?:\.[SL])?\s+(.*);", text)
        if jump:
            target = jump.group(1).strip()
            if target.startswith("("):
                found.append("0x%08x: tail call through %s" % (at, target))
            else:
                where = re.match(r"0x([0-9a-f]+) <([^>+]+)(\+0x[0-9a-f]+)?>", target)
                if where and where.group(2) != name:
                    found.append("0x%08x: tail call of %s" % (at, where.group(2)))
                elif not where:
                    number_ = re.match(r"0x([0-9a-f]+)", target)
                    if number_ and not address <= int(number_.group(1), 16) <= end:
                        found.append("0x%08x: jump out to %s" % (at, target))
        for store in re.finditer(r"[BW]?\[(FP|SP) \+ (0x[0-9a-f]+)\] = ", text):
            offset = int(store.group(2), 16)
            if store.group(1) == "FP" and fp_home is not None and offset >= fp_home:
                found.append("0x%08x: store to [FP + 0x%x], the caller's area starts at [FP + 0x%x]" % (at, offset, fp_home))
            if store.group(1) == "SP" and frame and offset >= frame:
                found.append("0x%08x: store to [SP + 0x%x], the frame is 0x%x" % (at, offset, frame))
    return found


def audit(sources, listing):
    """(rows, hazards): a row per CALL; a hazard per call without the space whose callee writes to the caller's area."""
    functions = listing_functions(listing)
    rows, hazards = [], []
    for path in sources:
        for routine, line, callee, space in routine_calls(path):
            findings = []
            if space < SPACE and not callee.startswith("(") and callee in functions:
                findings = callee_findings(callee, functions)
                if findings:
                    hazards.append((path, routine, line, callee, findings))
            rows.append((path, routine, line, callee, space, findings))
    return rows, hazards


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--listing", required=True, help="the module's objdump listing")
    ap.add_argument("--all", action="store_true", help="print the calls that have the space too")
    ap.add_argument("sources", nargs="+", help="the hand-written sources (module/*.s)")
    args = ap.parse_args(argv)
    rows, hazards = audit(args.sources, args.listing)
    for path, routine, line, callee, space, findings in rows:
        if space >= SPACE and not args.all:
            continue
        kind = "stock, through a register" if callee.startswith("(") else "module"
        print("%s:%d  %s -> %s  space %d  (%s)" % (path, line, routine, callee, space, kind))
        for finding in findings:
            print("    HAZARD %s" % finding)
    print("%d call(s), %d without the argument space, %d hazard(s)"
          % (len(rows), sum(1 for r in rows if r[4] < SPACE), len(hazards)))
    return 1 if hazards else 0


if __name__ == "__main__":
    sys.exit(main())

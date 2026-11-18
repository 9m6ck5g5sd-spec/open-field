#!/usr/bin/env python3
"""Assemble, compile and link a module for the OP-1 Field reservation with open-xy's Blackfin+ toolchain.

  python -I tools/module_build.py --name m2_noop --out <dir> module/m2_noop.s [more .s, .cpp or .o files]

Produces <dir>/<name>.elf, <name>.map and <name>.bin (the bytes `ldr_build.py build --module` appends),
and prints every global symbol with its address. The toolchain is `open-xy/local/toolchain/bin`
(`python open-xy/sdk.py toolchain fetch` installs it); the linker script is `linker/op1f-1.7.0.ld`, or the one
`--linker` names (since J4 the base and a pack are linked apart: `linker/op1f-1.7.0-base.ld`, `-pack.ld`).

C++ sources are compiled the way open-xy's `tooling/native.py` does it: freestanding, no exceptions,
RTTI, unwind tables or static constructors, with the driver producing assembly (`-S`) and the pinned
assembler making the object, because the bundled driver cannot find an assembler by itself. No C or
C++ library is linked: a unit must not divide by a variable (libgcc) or rely on memset/memcpy, and
the symbol of a C function carries a leading underscore unless it is given an `asm("name")` label.
Only the files named by --out and --name (plus one .s/.o per source) are written there.
"""

import argparse
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "open-xy"))
from tooling.formats import elf32, require  # noqa: E402

TOOLCHAIN = os.path.join(ROOT, "open-xy", "local", "toolchain", "bin")
LINKER = os.path.join(ROOT, "linker", "op1f-1.7.0.ld")
CPU = "-mcpu=bf707"      # the BF703 runs the same Blackfin+ ISA; open-xy's toolchain knows bf707
# open-xy's firmware C++ flags (tooling/native.py CXX_FLAGS), copied so this tool imports nothing but formats.
CXX_FLAGS = [CPU, "-O2", "-std=gnu++11", "-ffreestanding", "-fno-exceptions", "-fno-rtti",
             "-fno-threadsafe-statics", "-fno-use-cxa-atexit", "-fno-asynchronous-unwind-tables",
             "-fno-unwind-tables", "-fno-common", "-fno-tree-loop-distribute-patterns",
             "-fomit-frame-pointer", "-nostdinc++", "-Wall", "-Werror", "-Wno-unused-variable"]


def tool(name):
    for candidate in (os.path.join(TOOLCHAIN, name + ".exe"), os.path.join(TOOLCHAIN, name)):
        if os.path.isfile(candidate):
            return candidate
    raise SystemExit("toolchain binary not found: %s (run: python open-xy/sdk.py toolchain fetch)" % name)


def run(cmd):
    print("  " + " ".join(os.path.basename(c) if i == 0 else c for i, c in enumerate(cmd)))
    subprocess.run(cmd, check=True)


def compile_sources(sources, out, include=()):
    """Assemble and compile each source into <out>/<source's name>.o and answer the objects in the sources' order.
    (A function of its own since J5c: tools/pack_build.py links a pack of the new kind itself.)"""
    os.makedirs(out, exist_ok=True)
    objects = []
    for src in sources:
        if src.endswith(".o"):
            objects.append(src)
            continue
        # Objects carry the source extension (m2_measure.s.o, m2_measure.cpp.o) so an assembly and a
        # C++ unit of the same name do not overwrite each other.
        base = os.path.join(out, os.path.basename(src))
        obj = base + ".o"
        require(obj not in objects, "Two sources would produce %s" % obj)
        if src.endswith((".cpp", ".cc")):
            assembly = base + ".s"
            run([tool("bfin-elf-g++")] + CXX_FLAGS + ["-I" + d for d in include] + ["-S", "-o", assembly, src])
            run([tool("bfin-elf-as"), CPU, "-o", obj, assembly])
        else:
            run([tool("bfin-elf-as"), CPU, "-o", obj, src])
        objects.append(obj)
    return objects


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("sources", nargs="+", help=".s assembly, .cpp C++ or .o object files")
    ap.add_argument("--name", required=True)
    ap.add_argument("--out", required=True, help="output folder (created)")
    ap.add_argument("--include", action="append", default=[], help="C++ include folder (repeatable)")
    ap.add_argument("--linker", default=LINKER, help="linker script (default: the whole reservation, linker/op1f-1.7.0.ld)")
    args = ap.parse_args(argv)
    objects = compile_sources(args.sources, args.out, args.include)
    elf = os.path.join(args.out, args.name + ".elf")
    run([tool("bfin-elf-ld"), "-T", args.linker, "-Map=" + os.path.join(args.out, args.name + ".map"), "-o", elf] + objects)
    binary = os.path.join(args.out, args.name + ".bin")
    run([tool("bfin-elf-objcopy"), "-O", "binary", elf, binary])
    with open(elf, "rb") as f:
        sections, symbols = elf32(f.read())
    size = os.path.getsize(binary)
    require(size > 0, "empty module image")
    # The binary's first byte is the lowest section's first byte, and ldr_build.py loads it at the reservation's
    # first byte. A source that asks for an alignment above four moves .text up and the two no longer agree.
    start = symbols.get("__module_reserved_start")
    lowest = min(sec["address"] for name, sec in sections.items() if sec["size"] and name in (".text", ".rodata", ".data"))
    require(start is not None and lowest == start["value"],
            "the module's first byte is at 0x%08x, not at the reservation's start: it would be loaded at another "
            "address than it was linked for (look for an alignment above 4 in a source)" % lowest)
    print("%s: %d bytes" % (binary, size))
    for name, sym in sorted(symbols.items(), key=lambda kv: kv[1]["value"]):
        if name.startswith("__") or name.startswith(".") or name.startswith("L"):
            continue
        print("  0x%08x  %5d  %s" % (sym["value"], sym["size"], name))
    for name, sec in sections.items():
        if sec["size"] and name in (".text", ".rodata", ".data", ".bss"):
            print("  %-8s 0x%08x %6d bytes" % (name, sec["address"], sec["size"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

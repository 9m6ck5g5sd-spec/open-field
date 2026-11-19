#!/usr/bin/env python3
"""Write the page of the firmware's functions our code uses, docs/firmware-interfaces.md, from the sources.

  python -I tools/interfaces.py [--out docs/firmware-interfaces.md]
  python -I tools/interfaces.py --check
  python -I tools/interfaces.py --list

A source of ours that calls a function of the firmware binds it in two lines: the function's type, and a name for
its address.

  typedef int (*FileSizeFn)(const char* path);
  #define STOCK_FILE_SIZE ((FileSizeFn)0x8c051f9cu)

This tool reads every such pair in module/ and examples/ and joins them with the words kept in
the words file beside the profile build.json names (profiles/op1f-1.7.3-interfaces.json): for an address, a short name, what the caller hands over and gets back, and the
thread our code calls it on. The page lists the described functions by subject, and after them every function that
is bound in the sources and has no words yet, with its type and the files that bind it. So the page cannot name a
function the code does not use, and cannot leave one out.

What the words may say is what our code needs to call the function: where it stands, what it takes, what it gives
back, where it may be called. How the function works inside is not written down (CONTRIBUTING.md, "Writing about
the firmware").

`--check` writes nothing and answers 1 when the page in the tree is not what the sources and the words give, or when
the words describe an address no source binds. `--list` prints every binding with the files it stands in. Every
address belongs to the firmware of that profile, OS 1.7.3. Standard library only.
"""

import argparse
import fnmatch
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def words_path():
    """The words beside the profile build.json names: profiles/<the profile's name>-interfaces.json."""
    with open(os.path.join(ROOT, "build.json"), encoding="utf-8") as f:
        profile = json.load(f)["profile"]
    return os.path.join(ROOT, *(profile[:-len(".json")] + "-interfaces.json").split("/"))


WORDS = words_path()
PAGE = os.path.join(ROOT, "docs", "firmware-interfaces.md")
SOURCES = ("module/*.cpp", "module/*.h", "examples/*/*.cpp", "examples/*/*.h")

TYPEDEF = re.compile(r"typedef\s+([^;()]+?)\s*\(\s*\*\s*(\w+)\s*\)\s*\(([^;]*?)\)\s*;", re.S)
DEFINE = re.compile(r"^[ \t]*#define[ \t]+(\w+)[ \t]+\(\([ \t]*(\w+)[ \t]*\)[ \t]*(0x[0-9a-fA-F]{6,8})[uU]?[ \t]*\)", re.M)


def tidy(text):
    return " ".join(text.split())


def is_source(path):
    """Whether a path of the tree is a source this tool reads."""
    return any(fnmatch.fnmatch(path, pattern) for pattern in SOURCES)


def bindings_in(sources):
    """{address: {"names": [...], "types": {(returns, arguments): count}, "files": [...]}} over sources given as
    (path in the tree, text), in the order of their paths: every name defined as a function type's value at a fixed
    address."""
    found = {}
    for where, text in sorted(sources):
        types = {name: (tidy(returns), tidy(arguments)) for returns, name, arguments in TYPEDEF.findall(text)}
        for name, kind, address in DEFINE.findall(text):
            if kind not in types:
                continue
            held = found.setdefault(int(address, 16), {"names": [], "types": {}, "files": []})
            if name not in held["names"]:
                held["names"].append(name)
            held["types"][types[kind]] = held["types"].get(types[kind], 0) + 1
            if where not in held["files"]:
                held["files"].append(where)
    return found


def bindings(root=ROOT):
    """The same over the sources of a tree on the disk."""
    sources = []
    for pattern in SOURCES:
        for path in glob.glob(os.path.join(root, *pattern.split("/"))):
            with open(path, encoding="utf-8") as f:
                sources.append((os.path.relpath(path, root).replace("\\", "/"), f.read()))
    return bindings_in(sources)


def plain_name(names):
    """A name for a call that has no words yet: the sources' own, in small letters."""
    name = sorted(names, key=lambda n: (len(n), n))[0]
    return re.sub(r"^stock_", "", name.lower())


def call(binding, name):
    """A binding as one line of C: what is handed over and what comes back."""
    (returns, arguments), _count = sorted(binding["types"].items(), key=lambda item: (-item[1], item[0]))[0]
    return "%s %s(%s)" % (returns, name, arguments or "void")


def cell(text):
    return text.replace("|", "\\|").replace("\n", " ")


def units(files):
    return ", ".join("`%s`" % os.path.basename(path) for path in files[:4]) + (" and %d more" % (len(files) - 4) if len(files) > 4 else "")


def render(found, words):
    """The page's text."""
    described = words["functions"]
    stray = sorted(address for address in described if int(address, 16) not in found)
    if stray:
        raise ValueError("the words describe an address no source binds: %s" % ", ".join(stray))
    groups = words["groups"]
    for address, held in described.items():
        if held["group"] not in groups:
            raise ValueError("%s is in a group the words do not name: %s" % (address, held["group"]))
    left = sorted(address for address in found if "0x%08x" % address not in described)
    lines = [
        "# The firmware's functions our code uses",
        "",
        "Made by `tools/interfaces.py` from the sources and from `%s`; change those, not this page." % os.path.relpath(WORDS, ROOT).replace("\\", "/"),
        "",
    ]
    lines += words["about"]
    lines += [
        "",
        "There are %d functions bound in the sources. %d are described below, by subject. The other %d are listed at the end with their type and the files that bind them: their words are still to be written."
        % (len(found), len(found) - len(left), len(left)),
        "",
    ]
    for key, group in groups.items():
        mine = [address for address, held in described.items() if held["group"] == key]      # in the order the words have them
        if not mine:
            continue
        lines += ["## %s" % group["title"], ""]
        if group.get("about"):
            lines += [group["about"], ""]
        lines += ["| Address | The call | What the caller hands over and gets back | Called from |", "| --- | --- | --- | --- |"]
        for address in mine:
            held = described[address]
            lines.append("| `%s` | `%s` | %s | %s |" % (address, cell(call(found[int(address, 16)], held["name"])), cell(held["what"]), cell(held["thread"])))
        lines.append("")
    lines += [
        "## Bound in the sources, not described yet",
        "",
        "Each of these is called by the files named. The type is what our code hands over and takes back; the name is the sources' own. Read the file's comments at the binding before you use one, and add its words to `%s` when you have." % os.path.relpath(WORDS, ROOT).replace("\\", "/"),
        "",
        "| Address | The call | Bound in |",
        "| --- | --- | --- |",
    ]
    for address in left:
        lines.append("| `0x%08x` | `%s` | %s |" % (address, cell(call(found[address], plain_name(found[address]["names"]))), units(found[address]["files"])))
    lines.append("")
    return "\n".join(lines)


def read_words(path=WORDS):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", help="the page to write (default docs/firmware-interfaces.md)")
    ap.add_argument("--check", action="store_true", help="write nothing: is the page in the tree what the sources and the words give?")
    ap.add_argument("--list", action="store_true", help="print every binding with the files it stands in")
    args = ap.parse_args(argv)
    try:
        found = bindings()
        if args.list:
            for address in sorted(found):
                held = found[address]
                print("0x%08x  %s  [%s]" % (address, call(held, plain_name(held["names"])), ", ".join(os.path.basename(path) for path in held["files"])))
            print("%d function(s) bound" % len(found))
            return 0
        text = render(found, read_words())
        path = args.out or PAGE
        if args.check:
            held = None
            if os.path.isfile(path):
                with open(path, encoding="utf-8", newline="") as f:
                    held = f.read()
            print("the page of the firmware's functions is %s" % ("what the sources and the words give" if held == text else "not what the sources and the words give: write it anew"))
            return 0 if held == text else 1
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        print("wrote %s: %d functions" % (os.path.relpath(path, ROOT).replace("\\", "/"), len(found)))
    except (OSError, ValueError, KeyError) as problem:
        print("error: %s" % problem)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())

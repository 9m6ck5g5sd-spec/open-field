#!/usr/bin/env python3
"""The check before a commit: nothing of the firmware goes into this repository.

  python -I tools/commit_check.py            (what hooks/pre-commit runs)

It takes the tree as it would be committed (the index, not the working folder), writes it into a folder of its own
and runs `tools/publish_check.py --strict` on every file of it. The commit goes through only when the check answers
0: nothing of the stock firmware found, and no file left unjudged. Any other answer refuses the commit and shows
what was found and where.

The check holds files against the maintainer's own copy of the firmware, which is never in this repository. Say
where it is, once, in the repository's local settings (they are not committed and do not travel with a clone):

  git config core.hooksPath hooks
  git config --add openfield.stock    <folder of records made from your own os.ldr>
  git config --add openfield.stock    <folder of records made from your own bootloader.ldr>
  git config openfield.display        <a copy of your own unit's content/display folder>

`tools/ldr_dump_records.py FILE --out FOLDER` makes a folder of records. Without these settings the check cannot be
made, and a commit is refused: this hook never lets a commit through unchecked. A file `publish-allow.txt` at the top
of the tree, if there is one, is handed to the check as its list of texts a person has read and accepted.

Each run adds a line to `.git/open-field-check.log`: the time, the tree's id, the answer and the number of files.
docs/checking.md has the rules of the check and what it cannot see. Standard library only.
"""

import datetime
import os
import shutil
import subprocess
import sys
import tempfile


def git(*words):
    done = subprocess.run(["git"] + list(words), capture_output=True, text=True)
    if done.returncode:
        raise RuntimeError("git %s: %s%s" % (" ".join(words), done.stdout, done.stderr))
    return done.stdout.strip()


def settings():
    stock = subprocess.run(["git", "config", "--get-all", "openfield.stock"], capture_output=True, text=True).stdout.split("\n")
    display = subprocess.run(["git", "config", "--get", "openfield.display"], capture_output=True, text=True).stdout.strip()
    return [line.strip() for line in stock if line.strip()], display


def tree_profiles(scratch):
    """The profiles of the tree to be committed: the one build.json names, which must be there, and every other
    profiles/op1f-*.json beside it (their words for the firmware's functions are no profile). [] when build.json's
    is missing."""
    import json
    try:
        with open(os.path.join(scratch, "build.json"), encoding="utf-8") as f:
            named = os.path.join(scratch, *json.load(f)["profile"].split("/"))
    except (OSError, ValueError, KeyError):
        return []
    if not os.path.isfile(named):
        return []
    out = [named]
    folder = os.path.join(scratch, "profiles")
    for name in sorted(os.listdir(folder)):
        path = os.path.join(folder, name)
        if name.startswith("op1f-") and name.endswith(".json") and not name.endswith("-interfaces.json") and path != named:
            out.append(path)
    return out


def main():
    top = git("rev-parse", "--show-toplevel")
    os.chdir(top)
    stock, display = settings()
    problems = []
    if not stock:
        problems.append("no openfield.stock: the folders of records of your own firmware files")
    problems += ["openfield.stock names %s, which holds no index.json" % folder for folder in stock if not os.path.isfile(os.path.join(folder, "index.json"))]
    if not display:
        problems.append("no openfield.display: a copy of your own unit's content/display folder")
    elif not os.path.isdir(display):
        problems.append("openfield.display names %s, which is no folder" % display)
    if problems:
        print("open-field: the commit is refused, because the check before a commit cannot be made:")
        for problem in problems:
            print("  " + problem)
        print("tools/commit_check.py and docs/checking.md say how to set them.")
        return 2
    tree = git("write-tree")
    scratch = tempfile.mkdtemp(prefix="open-field-commit-")
    code = 2
    try:
        git("checkout-index", "--all", "--force", "--prefix=" + scratch.replace("\\", "/") + "/")
        files = sum(len(names) for _folder, _folders, names in os.walk(scratch))
        tool = os.path.join(scratch, "tools", "publish_check.py")
        if not files:
            print("open-field: nothing is staged")
            return 1
        profiles = tree_profiles(scratch)
        if not os.path.isfile(tool) or not profiles:
            print("open-field: the commit is refused: the tree to be committed lacks tools/publish_check.py or the profile build.json names, so it cannot be checked")
            return 2
        # -B: the check must leave no cache file in the folder it is about to walk
        command = [sys.executable, "-I", "-B", tool, "--strict"]
        for profile in profiles:
            command += ["--profile", profile]
        examples = os.path.join(scratch, "examples")        # an example pack's places are places of change too
        if os.path.isdir(examples):
            for name in sorted(os.listdir(examples)):
                if os.path.isfile(os.path.join(examples, name, "profile.json")):
                    command += ["--profile", os.path.join(examples, name, "profile.json")]
        command += ["--display", display]
        for folder in stock:
            command += ["--stock", folder]
        allow = os.path.join(scratch, "publish-allow.txt")
        if os.path.isfile(allow):
            command += ["--allow", allow]
        done = subprocess.run(command + [scratch], capture_output=True, text=True, cwd=scratch)
        code = done.returncode
        shown = done.stdout.replace(scratch + os.sep, "").replace(scratch.replace("\\", "/") + "/", "")
        if code:
            print(shown.rstrip())
            print(done.stderr.rstrip())
            print("open-field: the commit is refused: the publication check answered %d, and only 0 lets a commit through." % code)
        else:
            print("open-field: the publication check answered 0 for the %d files of the tree to be committed." % files)
        with open(os.path.join(git("rev-parse", "--git-dir"), "open-field-check.log"), "a", encoding="utf-8") as f:
            f.write("%s tree %s answer %d files %d\n" % (datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"), tree, code, files))
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    return code


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as problem:
        print("open-field: the commit is refused: %s" % problem)
        sys.exit(2)

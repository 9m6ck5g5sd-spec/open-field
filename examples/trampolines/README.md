# The example pack: trampolines

The smallest pack that changes the firmware, kept here as the model for a pack that has to stand in the firmware's own code. [A first pack](../../docs/first-pack.md), "A pack that changes the firmware", walks through it step by step. A pack that only needs what the base offers is better made like `wink`, `yoyo` or `list`, which name no address of the firmware ([the base](../../docs/base.md)).

It adds nothing you can see or hear. It stands at the first instruction of two functions of the firmware that the start-up calls once each, counts, and writes one line into the unit's log. It needs no other pack, the base neither. It has run on a unit.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, and what it is built from |
| `profile.json` | Its two places in the firmware: an address, a length and a checksum each. No byte of the firmware |
| `trampolines_table.s` | Its table of changes, made from `profile.json` by `tools/table_build.py`. Do not edit it |
| `trampolines.s` | The two routines the firmware arrives in, and their rooms |
| `trampolines.cpp` | The function that writes the line into the log |

At every start the loader checks each place by its checksum, copies the instructions the jump will stand on into the pack's rooms, and writes the two jumps.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/trampolines/recipe.json --profile examples/trampolines/profile.json --out out/trampolines.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

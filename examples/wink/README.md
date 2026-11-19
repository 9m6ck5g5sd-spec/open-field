# The example pack: wink

The example of level 1 of the base: a call every frame, and one picture. It puts a small red dot of ours on the tempo screen and blinks it, half a second on and half a second off. It changes no place of the firmware and names no address of it. [A first pack](../../docs/first-pack.md) walks through it step by step, and [the base](../../docs/base.md) says what level 1 offers.

**To see it**: with `wink.pack` and `base.pack` in `packs`, press the metronome key. The dot is at the top of the tempo screen. On every other screen the pack does nothing.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, its picture, and its card: level 1, the frame function `wink_frame`, its pictures |
| `wink.cpp` | The frame function. It finds the dot by its name in the tempo screen's document, keeps it, and shows or hides it by the base's clock |
| `wink_card.s` | The card, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `wink_pictures.s` | The directory of its pictures, made from the recipe by `tools/pack_build.py pictures`. Do not edit it |
| `tempo.svg.make` | The instructions for its picture: the unit's own tempo screen, with the dot `wink_dot` of ours added before its last tag |

It uses the base's helpers `op1f_screen`, `op1f_node`, `op1f_show` and `op1f_log`, and needs the mark `op1f_level_1`: without `base.pack` it is left out, with a line in the log.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/wink/recipe.json --profile profiles/op1f-1.7.3.json --out out/wink.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

# The example pack: dub

The example of levels 6 and 7 of the base: an echo on everything the unit plays, worked out on the sound itself, its values kept with each tape. It changes no place of the firmware and names no address of it. [Level 6](../../docs/level-6.md) and [level 7](../../docs/level-7.md) say what those levels offer, and quote this pack throughout.

**To hear it**: with `dub.pack` and `base.pack` in `packs`, hold **shift** and press the **metronome** key, in any mode. Its page comes up: the word `dub`, three numbers and three bars in the knobs' colours. **It starts dry**, with the level at 0, so the unit sounds exactly as before until you turn the third knob.

| On its page | What happens |
| --- | --- |
| Turn the blue knob | `time`: the echo comes 1 to 8 sixteenth notes after the sound, in time with the tempo |
| Turn the second knob | `back`: how much of the echo comes back again, 0 to 6 tenths |
| Turn the third knob | `level`: how loud the echo is, 0 to 10. At 0 the sound goes out exactly as it came in |
| Shift and the metronome key | The page closes |

While its page is up it takes all four knobs, so nothing under it moves without you seeing it. If the base couldn't give it its memory, or switched its stage off, the page says so in red and the pack does nothing to the sound.

**Its values go with each tape.** Load a tape and `dub` takes that tape's echo. A new tape starts dry. A copy of a tape keeps the original's echo. A tape from before has none, and takes the values `dub` keeps in its own folder, `user/pf/dub/dub.bin`, which the computer doesn't see.

Shift and the metronome key open `sketch` and `yoyo` too. `dub`'s file sorts first, so while it's on your unit those two have no way in.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 7, the frame function `dub_frame`, the input function `dub_input`, one folder that is not shown, and three asks: one megabyte of working memory, a sound stage after the master effect that waits for that memory and declares 40,000 cycles a call, and the member `dub_echo` kept with the tape |
| `dub.cpp` | The stage, `dub_process`, which runs the echo on the audio thread, the input function, the frame function, which paints the page and reads and sets the tape's values, and `dub_changed`, which notes when a tape was loaded or written |
| `dub_card.s` | The card, its folder and its asks, made from the recipe by `tools/pack_build.py card`. Do not edit it |

It uses the base's helpers `op1f_memory`, `op1f_stage`, `op1f_value_get`, `op1f_value_set`, `op1f_page_show`, `op1f_page_close`, `op1f_paint_box`, `op1f_paint_text`, `op1f_read`, `op1f_write` and `op1f_log`, and needs the mark `op1f_level_7`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/dub/recipe.json --profile profiles/op1f-1.7.3.json --out out/dub.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

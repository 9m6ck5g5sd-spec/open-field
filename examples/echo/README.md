# The example pack: echo

The example of level 5 of the base: a sequencer of its own, in the unit's own list, that repeats what you play. It changes no place of the firmware and names no address of it. [Level 5](../../docs/level-5.md) says what level 5 offers, and quotes this pack throughout.

**To hear it**: with `echo.pack` and `base.pack` in `packs`, open the synth's (or the drum's) list of sequencers and pick `echo`, at the end of the list after the unit's own seven. Switch it on and play. Every key sounds at the velocity you struck it, and then comes back in time, each repeat a quarter softer than the one before. A key you hold is never cut short by its echo, and a key played again sounds fresh. MIDI notes come back too, including those outside the keyboard's two octaves.

| On its page | What happens |
| --- | --- |
| Turn the blue knob | How many repeats, 0 to 7 (3 at first) |
| Turn the second knob | How far apart they are: a quarter, an eighth (at first), a sixteenth or a thirty-second note |
| The sequencer key | Switches the sequencer off and on, as with the unit's own. While the unit's SEQ OFF notice is up, the page shows dimmed under it |

Its page is the base's blank page, painted by the pack: the two values and a row of fading dots for each note that is echoing.

**Its folder.** One folder that the computer doesn't see, `user/pf/echo/`, where `echo.bin` keeps the two values. It's read at the first frame, and written when the values have changed and then stood a second, or when the page goes, never while the disk mode is open.

If `echo.pack` is taken off the unit, the synth and the drum start their stock sequencer instead. Put it back and the base picks `echo` again where it was chosen before.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 5, the frame function `echo_frame`, the input function `echo_input`, one folder that is not shown, and one ask, a sequencer named `echo` for the synth and the drum, with all MIDI notes, its `tick`, `key`, `run` and `shown` |
| `echo.cpp` | The sequencer's functions (`key` takes each key and fills a small queue, `tick` empties it and plays the repeats, `run` empties it at a stop), the input function, which takes the two knobs on its page, and the frame function, which paints the page and keeps `echo.bin` |
| `echo_card.s` | The card, its folder and its ask, made from the recipe by `tools/pack_build.py card`. Do not edit it |

It uses the base's helpers `op1f_note_on`, `op1f_note_off`, `op1f_mask`, `op1f_unmask`, `op1f_page_stand_in`, `op1f_paint_box`, `op1f_paint_mask`, `op1f_paint_text`, `op1f_read`, `op1f_write` and `op1f_log`, and needs the mark `op1f_level_5`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/echo/recipe.json --profile profiles/op1f-1.7.3.json --out out/echo.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

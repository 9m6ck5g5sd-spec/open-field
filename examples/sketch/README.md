# The example pack: sketch

The example of level 4 of the base: a drawing board, painted on the frame at every frame. It changes no place of the firmware and names no address of it. [Level 4](../../docs/level-4.md) says what level 4 offers, and quotes this pack throughout.

**To see it**: with `sketch.pack` and `base.pack` in `packs`, hold **shift** and press the **metronome** key, in any mode. Its page comes up: a black board, a small blue cross in the middle, and the title `sketch 0` at the top with an ochre bar under it as wide as the words.

| On its page | What happens |
| --- | --- |
| Turn the blue knob | The cross moves left and right, and leaves a line behind it |
| Turn the second knob | The cross moves up and down, the same |
| Press the red knob | The board is wiped, and `sketch.bin` deleted |
| Press the third knob | The same board on the base's blank page, with no title. Press again to go back |
| Shift and the metronome key | The page closes |

The board keeps up to 256 lines. When a new one comes past that, the oldest goes. The title counts them.

**Its folder.** The card lists one folder, `sketch`, shown in the disk mode. When the page closes and the board has changed, the pack writes `sketch.bin` there, and reads it back at the next start, so your drawing survives. While the disk mode is open it waits, and writes when the disk mode has closed.

Shift and the metronome key open `dub` and `yoyo` too. The pack whose file sorts first gets them: with `dub.pack` on the unit, `sketch` has no way in, and with `sketch.pack` there, `yoyo` has none. Keep one of them on the unit at a time.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, its picture, and its card: level 4, the frame function `sketch_frame`, its pictures, the input function `sketch_input`, one folder `{}` named after the pack, and no asks |
| `sketch.cpp` | The input function, which takes the way in and, while the page is up, the four knobs' turns and presses, and the frame function, which draws the lines, sets and measures the title, paints the board and the cross, and reads, writes and deletes `sketch.bin` |
| `sketch_card.s` | The card and its list of folders, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `sketch_pictures.s` | The directory of its pictures, made from the recipe by `tools/pack_build.py pictures`. Do not edit it |
| `sketchpad.svg.make` | The instructions for its picture: the head and the last tag of the unit's own tempo screen, by reference, around a black box and one placeholder for the title, `sketch_title` |

It uses the base's helpers `op1f_picture`, `op1f_page_show`, `op1f_page_close`, `op1f_text_set`, `op1f_text_width`, `op1f_paint_line`, `op1f_paint_box`, `op1f_paint_mask`, `op1f_read`, `op1f_write`, `op1f_remove` and `op1f_log`, and needs the mark `op1f_level_4`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/sketch/recipe.json --profile profiles/op1f-1.7.3.json --out out/sketch.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

When `sketch.pack` is deleted, its folder `user/pf/sketch/` stays on the disk and is read by nothing. Delete it in the bootloader's drive mode if you no longer want it.

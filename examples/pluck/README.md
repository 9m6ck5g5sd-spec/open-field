# The example pack: pluck

The example of levels 9 and 10 of the base: a synth engine of its own in the unit's own list of engines, a plucked string. It changes no place of the firmware and names no address of it. [Level 9](../../docs/level-9.md) says what an engine gets, [level 10](../../docs/level-10.md) how its page shows the values as the sound plays them, and both quote this pack.

**To hear it**: with `pluck.pack` and `base.pack` in `packs`, hold **shift** and press **T1** on a synth sound, and pick `pluck`. The list is sorted by name, so it's right after `phase`. Play: every key is a plucked string, louder and brighter the harder you play, duller the softer. The unit's own envelope, play mode, glide, effect and LFOs work on it as on any engine.

| On its page (T1) | What happens |
| --- | --- |
| Turn the blue knob | `tone`: how bright the pluck is. 60 when picked |
| Turn the second knob | `decay`: how long the string rings. 70 |
| Turn the third knob | `damping`: how dark it gets as it rings. 40 |
| Turn the red knob | `pick`: where it's plucked, from the middle of the string towards its end. 25 |
| Shift and the blue knob | `body`: a little warmth. 30 |
| Shift and the second knob | `drift`: a slow wander of the pitch. 10 |
| Shift and the third knob | `noise`: a breath of noise that keeps the string moving. 0 |
| Shift and the red knob | `level`. 80 |

Every value runs from 0 to 100. Its page is the base's blank page, on which it paints the eight values in two rows, in the knobs' colours, as the sound plays them. Aim an LFO at `drift` and hold a key: the number moves.

**Its sounds.** Two of its own, `nylon` and `harp`. The base writes them into a folder `pluck` of the unit's sound list the first time the pack runs, and the folder shows after the next start. It writes them only when the unit has room for both and 20 more of yours, since the unit takes 500 user sounds and no more. Delete the folder and they come back only with the pack's next version.

**Without the pack.** A sound saved with `pluck` is, for the unit, a sound of its own engine `dsynth`. With open-field and no `pluck.pack` it's silent, and T1 shows `'pluck' is not on this unit`. On the original firmware it plays `dsynth`. Put the pack back and it plays `pluck` again.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 10, the frame function `pluck_frame`, no input function, no folder, and one ask, an engine named `pluck` of one channel, declaring 33,000 cycles a voice and 16 KiB of memory a voice, with its eight values, its two sounds, its `reset` and its `shown` |
| `pluck.cpp` | The engine, `pluck_render`, which fills one voice's block on the audio thread, `pluck_reset`, which clears a voice's string on the display thread, `pluck_shown`, and the frame function, which paints the page |
| `pluck_card.s` | The card, its ask and its sounds, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `nylon.json`, `harp.json` | The two sounds, as the unit writes a synth sound. The pack tool reads them from beside the recipe |

It uses the base's helpers `op1f_page_stand_in`, `op1f_param_heard`, `op1f_paint_box`, `op1f_paint_text` and `op1f_log`, and needs the mark `op1f_level_10`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/pluck/recipe.json --profile profiles/op1f-1.7.3.json --out out/pluck.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

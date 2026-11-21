# The example pack: fold

The example of levels 9 and 10 of the base: a synth engine of its own in the unit's own list of engines, a sine folded over itself, which renders two channels. It changes no place of the firmware and names no address of it. [Level 9](../../docs/level-9.md) says what an engine gets, [level 10](../../docs/level-10.md) how its page shows the values as the sound plays them, and both quote this pack.

**To hear it**: with `fold.pack` and `base.pack` in `packs`, hold **shift** and press **T1** on a synth sound, and pick `fold`. The list is sorted by name, so it's right after `fm`. Play: a hollow, buzzy tone, brighter the harder you play and the higher `fold`, a little different on the left and the right. The unit's own envelope, play mode, glide, effect and LFOs work on it as on any engine.

| On its page (T1) | What happens |
| --- | --- |
| Turn the blue knob | `fold`: how many times the wave folds over, 0 to 100. 40 when picked |
| Turn the second knob | `symmetry`: folds the left and the right apart, -50 to 50. 0 |
| Turn the third knob | `sub`: a sine an octave down, 0 to 100. 20 |
| Turn the red knob | `drive`: pushes the sum into a soft limit, 0 to 100. 30 |

Its page is the base's blank page, on which it paints the four values in the knobs' colours, as the sound plays them.

**Its memory.** None. Its two phases a voice are words of its own data, which is safe because the base renders one voice at a time.

**Its sounds.** Two of its own, `hollow` and `buzz`. The base writes them into a folder `fold` of the unit's sound list the first time the pack runs, and the folder shows after the next start. It writes them only when the unit has room for both and 20 more of yours, since the unit takes 500 user sounds and no more.

**Without the pack.** A sound saved with `fold` is, for the unit, a sound of its own engine `dsynth`. With open-field and no `fold.pack` it's silent, and T1 shows `'fold' is not on this unit`. On the original firmware it plays `dsynth`. Put the pack back and it plays `fold` again.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 10, the frame function `fold_frame`, no input function, no folder, and one ask, an engine named `fold` of two channels, declaring 25,000 cycles a voice and no memory, with its four values, its two sounds and its `shown` |
| `fold.cpp` | The engine, `fold_render`, which fills one voice's block, left and right in turn, on the audio thread, `fold_shown`, and the frame function, which paints the page |
| `fold_card.s` | The card, its ask and its sounds, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `hollow.json`, `buzz.json` | The two sounds, as the unit writes a synth sound. The pack tool reads them from beside the recipe |

It uses the base's helpers `op1f_page_stand_in`, `op1f_param_heard`, `op1f_paint_box`, `op1f_paint_text` and `op1f_log`, and needs the mark `op1f_level_10`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/fold/recipe.json --profile profiles/op1f-1.7.3.json --out out/fold.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

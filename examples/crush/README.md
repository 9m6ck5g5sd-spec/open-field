# The example pack: crush

The example of levels 8 and 10 of the base: an effect of its own in the unit's own lists of effects, which makes a sound rougher, with fewer bits and a lower sample rate. It changes no place of the firmware and names no address of it. [Level 8](../../docs/level-8.md) says what an effect gets, [level 10](../../docs/level-10.md) how its page shows the values as the sound plays them, and both quote this pack.

**To hear it**: with `crush.pack` and `base.pack` in `packs`, hold **shift** and press **T3** on a synth or drum sound, and pick `crush`. The list is sorted by name, so it's near the top, just before `cwo`. Play, and the sound is rougher. It works on the master too, from the mixer's list of effects.

| On its page (T3) | What happens |
| --- | --- |
| Turn the blue knob | `bits`: how many bits the sound keeps, 16 (nearly clean) down to 2. 8 when picked |
| Turn the second knob | `rate`: the sample rate, halved at each step, from 0 (the unit's own) to 6. 2 when picked |
| Turn the third knob | `drive`: how hard the sound is pushed into the steps, 0 to 4, twice as loud at each step. 0 when picked |
| Turn the red knob | `mix`: how much of what comes out is crushed, 0 (the sound as it came) to 100. 100 when picked |
| T3 | Switches the effect off and on, as with the unit's own |

Its page is the base's blank page with its name, on which it paints the four values in the knobs' colours, as the sound plays them. Aim an LFO at one and you'll see the number move. It paints `off` while the effect is switched off. The page takes no key and no knob: the unit turns the values, and the page shows where they are.

**Without the pack.** `crush` stands on the unit's `punch`. A sound saved with it keeps punch's name and four knobs, switched off. With open-field and no `crush.pack`, or on the original firmware, that sound plays dry. Put the pack back and it plays `crush` again.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 10, the frame function `crush_frame`, no input function, no folder, and one ask, an effect named `crush` standing on `punch`, declaring 31,000 cycles a call, with its four values and its `shown` |
| `crush.cpp` | The effect, `crush_process`, which crushes each side apart on the audio thread and keeps nothing between blocks, `crush_shown`, which notes when its page is up, and the frame function, which paints the page |
| `crush_card.s` | The card and its ask, made from the recipe by `tools/pack_build.py card`. Do not edit it |

It uses the base's helpers `op1f_param_heard`, `op1f_paint_text` and `op1f_log`, and needs the mark `op1f_level_10`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/crush/recipe.json --profile profiles/op1f-1.7.3.json --out out/crush.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

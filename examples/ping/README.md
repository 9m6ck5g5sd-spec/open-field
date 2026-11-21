# The example pack: ping

The example of levels 8 and 10 of the base: an effect of its own in the unit's own lists of effects, an echo in time with the tempo that bounces left and right. It changes no place of the firmware and names no address of it. [Level 8](../../docs/level-8.md) says what an effect gets, [level 10](../../docs/level-10.md) how its page shows the values as the sound plays them, and both quote this pack.

**To hear it**: with `ping.pack` and `base.pack` in `packs`, go to the mixer, open the master effect's list as always and pick `ping`, between `phone` and `punch`. Everything you play now echoes on the beat, left, then right, then left. It works on a single sound too: shift and T3 on a synth or drum sound.

| On its page (T3) | What happens |
| --- | --- |
| Turn the blue knob | `time`: how far behind the echo comes, 1 to 4 sixteenth notes (a sixteenth, an eighth, a dotted eighth, a quarter), in time with the tempo. 3 when picked |
| Turn the second knob | `feedback`: how much of each echo comes back as the next, 0 to 90 hundredths. 40 when picked |
| Turn the third knob | `spread`: how far the echoes bounce, 0 (both sides alike) to 100 (left, right, left, all the way). 100 when picked |
| Turn the red knob | `mix`: how much of what comes out is echo, 0 (none) to 100. 30 when picked |
| T3 | Switches the effect off and on. Switched off, the last echoes ring out and nothing new goes in |

Its page is the base's blank page with its name, on which it paints the four values in the knobs' colours, as the sound plays them, and `off` while the effect is switched off. The page takes no key and no knob.

**Its memory.** A megabyte of working memory for each place it plays at, about three seconds of two lines. Its `reset` clears it on the display thread before the first block at a place. Once its lines are silent and it's switched off, it does nothing at all.

**Without the pack.** `ping` stands on the unit's `delay`. A sound or the master saved with it keeps delay's name and four knobs, switched off. With open-field and no `ping.pack`, or on the original firmware, it plays dry. Put the pack back and it plays `ping` again.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, and its card: level 10, the frame function `ping_frame`, no input function, no folder, and one ask, an effect named `ping` standing on `delay`, with tails, declaring 42,000 cycles a call and a megabyte of memory a place, with its four values, its `reset` and its `shown` |
| `ping.cpp` | The effect, `ping_process`, which runs the two lines on the audio thread, `ping_reset`, which clears a place's memory, `ping_shown`, and the frame function, which paints the page |
| `ping_card.s` | The card and its ask, made from the recipe by `tools/pack_build.py card`. Do not edit it |

It uses the base's helpers `op1f_param_heard`, `op1f_paint_text` and `op1f_log`, and needs the mark `op1f_level_10`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/ping/recipe.json --profile profiles/op1f-1.7.3.json --out out/ping.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

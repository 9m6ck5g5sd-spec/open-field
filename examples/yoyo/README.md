# The example pack: yoyo

The example of level 2 of the base: a page of its own, with keys and a knob. It changes no place of the firmware and names no address of it. [The base](../../docs/base.md), "Level 2: a page with keys and knobs", says what level 2 offers.

**To see it**: with `yoyo.pack` and `base.pack` in `packs`, hold **shift** and press the **metronome** key, in any mode. Its page opens: a grey string hanging from a short bar, and a blue yoyo on it.

| On its page | What happens |
| --- | --- |
| Turn the blue knob | The yoyo moves up or down, four pixels a step, 56 at most either way |
| Press the blue knob | The yoyo goes back to the middle |
| Shift and the metronome key | The page closes |
| A mode key | The page closes, and the mode's own screen comes back |

Every other key and knob goes on to the firmware as it always did, and acts on the screen the page hides. The metronome key alone stays the firmware's. With `yoyo.pack` on the unit, shift and the metronome key are the pack's.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, its picture, and its card: level 2, the frame function `yoyo_frame`, its pictures, the input function `yoyo_input` |
| `yoyo.cpp` | The input function, which takes the page's keys and the blue knob while the page is shown, and the frame function, which makes the page's document once and moves the yoyo, 240 pixels a second at most |
| `yoyo_card.s` | The card, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `yoyo_pictures.s` | The directory of its pictures, made from the recipe by `tools/pack_build.py pictures`. Do not edit it |
| `yoyo.svg.make` | The instructions for its picture, a picture of the pack's own: the head and the last tag of the unit's own tempo screen, by reference, around a drawing of ours |

The input function may run on the audio thread, so it writes no line and waits for nothing: it writes a few words that the frame function reads, and opens or closes the page. It uses the base's helpers `op1f_picture`, `op1f_node`, `op1f_move`, `op1f_ms`, `op1f_page_open`, `op1f_page_close` and `op1f_log`, and needs the mark `op1f_level_2`.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/yoyo/recipe.json --profile profiles/op1f-1.7.3.json --out out/yoyo.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

# The example pack: list

The example of level 3 of the base: a folder of its own on the unit's disk, shown to the computer in the disk mode, files read and written there, and words on its page. It changes no place of the firmware and names no address of it. [The base](../../docs/base.md), "Level 3: folders, files and text", says what level 3 offers.

**Its folder.** The card lists one folder, `list`. The base makes it on the unit's disk as `user/pf/list/` and shows it in the disk mode, beside `packs`, `fx` and `lfo`. Copy files into it from the computer, or delete them there.

**To see it**: with `list.pack` and `base.pack` in `packs`, hold **shift** and press the **help** key, in any mode. Its page opens: a folder sign, the title `list: 3 files` (with `, 1 to 7` behind it when there are more than seven), and the names of up to seven files of the folder, in the firmware's own letters.

| On its page | What happens |
| --- | --- |
| Turn the blue knob | The names scroll, a row a step |
| Shift and the help key | The page closes |
| A mode key | The page closes, and the mode's own screen comes back |

**When the page closes**, the pack writes `list.txt` into the same folder: the folder's path and the names it showed, one a line. It writes at the first close of a start, and later only when the names changed. While the disk mode is open it waits, and writes when the disk mode has closed. So the computer finds a file the pack made at the next disk mode. The first close writes `list.txt` into the folder, which changes the names, so the second close writes again.

With `list.pack` on the unit, shift and the help key are the pack's, and no longer do what the firmware does with them.

| File | What it is |
| --- | --- |
| `recipe.json` | The pack's name and version, its sources, its picture, and its card: level 3, the frame function `list_frame`, its pictures, the input function `list_input`, and one folder, `{}`: named after the pack, at the place of its name, shown |
| `list.cpp` | The input function, which takes the page's keys and the blue knob while the page is shown, and the frame function, which reads the folder when the page opens and when the disk mode closes under it, sets the page's words, and writes `list.txt` |
| `list_card.s` | The card and its list of folders, made from the recipe by `tools/pack_build.py card`. Do not edit it |
| `list_pictures.s` | The directory of its pictures, made from the recipe by `tools/pack_build.py pictures`. Do not edit it |
| `list.svg.make` | The instructions for its picture: the head and the last tag of the unit's own tempo screen, by reference, around a drawing of ours with eight placeholders for the words, `list_title` and `list_row1` to `list_row7` |

It uses the base's helpers `op1f_picture`, `op1f_text`, `op1f_list`, `op1f_path`, `op1f_write`, `op1f_page_open`, `op1f_page_close` and `op1f_log`, and needs the mark `op1f_level_3`. The page shows the first 64 names of the folder.

Build it, from the top of the repository, into a file that is not there yet (the tool overwrites nothing):

```bash
python -I tools/pack_build.py build3 --recipe examples/list/recipe.json --profile profiles/op1f-1.7.3.json --out out/list.pack
```

`python -I tools/build_all.py` builds it too, into `out/examples/`, and holds it to the size and SHA-256 kept in `packs/built.json`.

When `list.pack` is deleted, its folder `user/pf/list/` stays on the disk and is read by nothing. Delete it in the bootloader's drive mode if you no longer want it.

# Level 5: a sequencer of your own, and the notes heard

With level 5 your pack can add a sequencer to the unit's own list, right after the firmware's seven, without changing a single place of the firmware. You hand over a name and a few functions. The base puts the name in the list, calls you at every pulse of the clock and for every key, plays the notes you ask for, ends the ones you leave sounding, and shows your page in place of the sequencer's screen. Level 5 also lets any pack listen to every note the instrument is asked to play.

The words of the interface are in the header `module/build_r6e_card.h`. Where this page and the header disagree, the header is right.

## The example: `echo`

[`echo`](../examples/echo/README.md) is a sequencer that repeats what you play. Pick `echo` in the sequencer list of the synth or the drum and switch it on. Every key you play sounds, then comes back in time, each repeat a bit softer than the one before. On its page the **blue** knob sets how many repeats (0 to 7), the **second** knob how far apart they are, from a quarter note down to a thirty-second.

`echo` takes every key and plays the note itself. So a key you hold is never cut short by an echo, and a key you play again sounds fresh. MIDI notes outside the keyboard's two octaves are echoed too. The code quoted below is `echo`'s own.

## Asks

Level 4 gave the card a list of **asks**. Level 5 defines the first two kinds:

| Kind | In the recipe | What the ask's table is |
| --- | --- | --- |
| A sequencer | `"kind": "sequencer"` | an `Op1fSequencer` |
| The notes heard | `"kind": "notes"` | an `Op1fNotes` |

Every kind's table starts with the same head, `Op1fKindHead`: its size, a name, flags, and a `shown` function for a kind that has a page. Later levels add kinds in the same form, and a kind's table only ever grows at its end.

You never write an ask's table by hand. The recipe lists the asks in the card, and the pack tool writes the card, the list and each table, naming your functions by their symbols. This is `echo`'s card:

```json
"card": {
  "level": 5,
  "frame": "echo_frame",
  "pick": null,
  "pictures": false,
  "input": "echo_input",
  "folders": [ { "shown": false } ],
  "asks": [
    {
      "kind": "sequencer",
      "name": "echo",
      "synth_only": false,
      "all_midi": true,
      "shown": "echo_shown",
      "tick": "echo_tick",
      "key": "echo_key",
      "run": "echo_run",
      "octave": null,
      "playing": null
    }
  ]
}
```

A card of level 5 is 64 bytes, the same as level 4's. Its source makes `op1f_level_5` a name your pack needs, so on an older base the loader leaves your pack out and says which mark was missing.

The pack tool refuses what the base would leave aside. The base checks again at every start anyway, and an ask that isn't in order is left aside with a line in the log. Your pack still runs, without that ask.

## A sequencer's name

- One to fifteen characters of `a`-`z`, `0`-`9`, `-` and `_`. It's what the owner sees in the list, and what the unit saves as their choice.
- Not one of the firmware's seven: `pattern`, `endless`, `tombola`, `finger`, `sketch`, `arpeggio`, `hold`.
- **Where two packs ask for the same name, the earlier file keeps it**, in the order of the files' names. The later pack still runs, without that sequencer.
- Sixteen sequencers of packs in all. The seventeenth is left aside.

The packs' sequencers come at the end of the list, after the seven, in the order of the files' names and then of each card's asks. The firmware numbers them at each start, so nothing ever saves a number. The unit saves the name. A sequencer with `"synth_only": true` isn't in the drum's list.

The list has never been tried on a unit with more than eight rows in it.

## The five functions

`tick` or `key` must be there. The other three may be null.

| Function | When the base calls it | Thread |
| --- | --- | --- |
| `tick(now)` | At every pulse of the clock, 96 to a quarter note, while the sequencer is on, its instrument is the one the unit is on, and no sound is being loaded | The sequencer thread |
| `key(event)` | For every key of the keyboard and every MIDI note, down and up, while the sequencer is on, before the instrument plays it | Whichever thread brings the key: the key thread, **the audio thread**, or a MIDI handler's |
| `run(host, how)` | `OP1F_RUN_ON` when the sequencer is switched on. `OP1F_RUN_STOP` when it must stop sounding: switched off, another sequencer picked for its instrument, or the unit changing instrument | The key thread, a MIDI handler's, or the start's |
| `octave(host, steps)` | At the pulse after an octave key, while the sequencer is on | The sequencer thread |
| `playing(host)` | When the unit draws its "playing" mark. Not 0 shows it. With no function the mark shows | The display thread |

And the head's `shown(where, how)`, for the page (below), on the display thread.

`host` is the instrument, 0 the synth and 1 the drum. Each instrument has its own chosen sequencer, as with the firmware's own. If your sequencer is chosen for both, you get calls for both, one at a time, and only the instrument the unit is on ticks.

**What these functions may do.** `tick`, `key`, `run` and `octave` keep to the rules of an input function: no line in the log, no waiting, nothing of the firmware's, and back quickly. `key` may run on the audio thread, and a slow `key` delays the sound. They may call `op1f_note_on`, `op1f_note_off`, `op1f_note_bend`, `op1f_seq_restart`, `op1f_mask`, `op1f_unmask` and `op1f_ms`. They count, and your frame function says what they counted.

`shown` and `playing` come on the display thread, outside your frame function. They note what they're told in single words and do nothing else. No painting, no text, no file, no page.

**One of them can arrive while another is halfway through**, or while your frame function is. A `key` on the audio thread can come in the middle of a `tick`. Share single words between them, or keep what they share inside a section of `op1f_mask` (below).

## What `tick` is handed

| Field | What |
| --- | --- |
| `bytes` | 32 at level 5. A later base may hand more, so read what you know |
| `host` | The instrument the unit is on |
| `pulse` | The firmware's own count of pulses, 0 to 47. It wraps, and it goes back to 0 when the tape starts or a sequencer restarts the clock |
| `count` | The base's count of your ticks since the sequencer was switched on or last stopped, from 0. **It's never set back otherwise**, so time your notes on it |
| `tempo` | Beats a minute in hundredths. 12000 is 120 |
| `us` | Microseconds a pulse, at that tempo |
| `clock` | `OP1F_CLOCK_OWN` the unit's own, `OP1F_CLOCK_TAPE` the tape's while it plays, `OP1F_CLOCK_MIDI` one from outside |
| `jumped` | Not 0 when the firmware set its count back since the last tick |

A quarter note is 96 pulses, an eighth 48, a sixteenth 24, a thirty-second 12. `echo` keeps its steps in a table of those four numbers.

## What `key` is handed

| Field | What |
| --- | --- |
| `what` | `OP1F_DOWN`, or `OP1F_UP` for a key whose down your `key` took |
| `host` | The instrument |
| `key` | The keyboard's key, 0 to 23 from the left. For a MIDI note, the note minus 53. **An up is matched to its down by `key`**: the octave may have moved between the two, and `note` with it |
| `note` | The note the instrument would play for it now, octave counted, 0 to 127 |
| `velocity` | 1 to 127 for a down, 0 for an up |
| `shift` | Not 0 when the key went down while shift was held |
| `from` | `OP1F_FROM_KEYS` or `OP1F_FROM_MIDI` |

`key` answers not 0 to take the key. The instrument doesn't play it, and the key's up comes to `key` too. Answer 0 and the instrument plays it as usual.

**Take the key if you want its up.** You hear a key's up only when you took its down. A down you let through belongs to the instrument: it plays the note and ends it itself when the key comes up, and your sequencer hears nothing of that up. That goes for the keyboard and for MIDI alike. A pack that answers 0 and waits for the up waits forever. So a sequencer that needs to know how long a key is held takes the key and plays the note itself, `op1f_note_on` at the down and `op1f_note_off` at the up. If you only want to hear when notes end, without taking any keys, ask for the notes heard (below).

`echo` takes every key. At a down it ends its own sounding note of that number if it has one, starts the note itself at the velocity you played, and puts the key into its queue. At the up it ends the note that down started, unless another key still holds the same note.

```cpp
extern "C" int echo_key(const Op1fKey* e) {
    ...                                     // a down or an up of a note 0 to 127, or nothing
    int place = e->key + PLACE_OF_KEY;      // a MIDI key may lie anywhere: held only within the list's bounds
    if (place < 0 || place >= (int)PLACES) return 0;
    ...
    u32 was = op1f_mask();
    ...                                     // the key held or let go by its number, the queue filled
    if (!down) {
        op1f_unmask(was);
        if (ended) end_note(ended - 1u);
        return 1;
    }
    ...
    op1f_unmask(was);
    if (ended) end_note(ended - 1u);
    if (ending) op1f_note_off(&op1f_card_echo, note, 0);
    if (!op1f_note_on(&op1f_card_echo, note, e->velocity, 0)) ++refused;
    return 1;
}
```

A key whose number falls outside `echo`'s list is left to the instrument, with no echo, because `echo` couldn't match its up.

While a sequencer is on, MIDI notes outside the keyboard's two octaves are dropped, as with those of the firmware's sequencers that don't take them. With `"all_midi": true` they come to `key` too, with a `key` below 0 or above 23. `echo` asks for them because it repeats whatever is played. **Never index a table by `key` without checking its bounds first.**

## Notes out

| Helper | What it does |
| --- | --- |
| `op1f_note_on(card, note, velocity, flags)` | A note started on the instrument the unit is on, as a key would. `note` 0 to 127, `velocity` 1 to 127 |
| `op1f_note_off(card, note, flags)` | A note your pack started, ended |

Both send the note out over MIDI as well, on the unit's own MIDI settings, unless `flags` is `OP1F_NOTE_NO_MIDI`.

**From where.** Only from your `tick`, `key`, `run`, `octave` and input function, while the base is calling them, and only while the instrument the unit is on has your sequencer chosen. Never from your frame function, never from `shown` or `playing`, never from a note heard. Any other call plays nothing. The base counts it and puts the count in the log under your pack's name.

**What they answer.** 1 when the note was handed on. 0 when it wasn't: the card not in use, a note above 127, a velocity of 0 or above 127, a note your pack already has sounding (note on), a note it doesn't have sounding (note off), or a call from somewhere it may not come from. **Handed on isn't the same as heard.** The synth has six voices and a seventh note takes one of them, and while a sound is loading the instrument plays nothing.

**The list the base keeps.** The base keeps a list of the notes your pack started on each instrument. That's why a second note on of a note that's still sounding is refused. **End a note before you start it again.** `echo` does this when a key goes down on a note it has sounding, and when a new repeat lands on a note another repeat still holds.

**What ends a note:**

- Your own `op1f_note_off`.
- **Every stop.** When the sequencer is switched off, another sequencer is picked, or the unit changes instrument, the base calls your `run` with `OP1F_RUN_STOP`. Once that returns, the base ends every note your pack left sounding. A note whose off you never send sounds until then.
- The instrument itself, when a seventh note takes a voice on the synth. The note stays in the base's list, and its later off does nothing.
- With a sustain pedal held, the instrument may let an ended note ring until the pedal comes up. For the base it has ended.

**Your notes and the keys the owner holds.** The instrument ends a note by its number, whoever started it, and doesn't start a note that's already sounding. A key the instrument plays for itself isn't in the base's list, so your note off of that note would end the held key too. A pack that takes its keys, as `echo` does, has every note in the base's list, and the list keeps them apart. `echo` never ends a note it didn't start. A repeat that falls due while its note is still sounding gets 0 from `op1f_note_on`, and `echo` skips that repeat and counts it. So no echo ever cuts a held key, and the repeats still due after the key comes up play as they should. Since the key's own note is the pack's, the base ends it at a stop, even while the owner still holds the key.

The octave keys don't move the notes your pack started. If you want yours to follow, end them and start them again yourself, in `octave`.

## Three more helpers

| Helper | What it does |
| --- | --- |
| `op1f_note_bend(card, note, cents)` | One of your sounding notes moved in pitch, on the synth: `cents` hundredths of a semitone, -4800 to 4800, 0 back to the note itself. The note keeps its number, so end it as you started it. Where the instrument plays the same note for a key as well, that one moves with it. Answers 0 on the drum |
| `op1f_seq_restart(card)` | The clock's count set back to its start, as the firmware's own sequencers do when a first key goes down. Not while a clock from outside leads. The next tick's `pulse` starts over and its `jumped` says so. `count` carries on |
| `op1f_mask()`, `op1f_unmask(was)` | A short section in which nothing else of the unit runs. `op1f_mask` answers what you hand back to `op1f_unmask` |

`op1f_note_bend` and `op1f_seq_restart` come from the same places as the note helpers, with the same refusals.

**A masked section lasts a few microseconds.** Never call a helper of the base's inside one, and never wait inside one. Decide inside the section, leave it, then make the call. `echo`'s tick takes its keys in one at a time like this:

```cpp
u32 was = op1f_mask();
...                          // one waiting key taken off the queue and put into a free place, or into the
                             // oldest echo's place; whether that echo's note was sounding noted
op1f_unmask(was);
if (ending) end_note(note);  // the call, after the section
```

Never switch interrupts off yourself. `op1f_mask` holds on every thread, the audio thread among them.

## The page that stands in

While your sequencer is the one chosen and the sequencer's screen is up, **your page stands in for that screen**. You don't open or close it. The base does.

```cpp
op1f_page_stand_in(&op1f_card_echo, 0, 0);    // ask 0, the base's blank page, which echo paints
```

| Argument | What |
| --- | --- |
| `ask` | The ask's number in your card's list, from 0 |
| `document` | One your card made with `op1f_picture`, or 0 for the base's blank page, which your frame function paints |

Call it from your frame function. It takes effect at the next frame, and the document stands until you call it again with another. It answers 1, or 0 for a card not in use, an ask with no page, a document your card didn't make, or a call from elsewhere. Until your pack calls it, the base's blank page stands in with your sequencer's name at the top in light grey.

While your page is up:

- **The frame's `page` is your card.** Paint on the frame's `canvas` when it is. The frame's `mode`, and the input's, is 7 while the sequencer's screen is up.
- **Every key and knob goes to your input function first**, with `mine` not 0, as for any page of yours. What you let go reaches the sequencer's own handling (the sequencer key switches it on and off, the octave keys move the octave), then the firmware's keys. The keyboard keys you let go reach the instrument, and so your `key`.
- **Mind the thread, as on any page.** The keys 13 to 53 (the keys that play notes, and shift, 38) may reach your input function on the audio thread. Every other key and every turn comes on the key thread. The input's `thread` says which ([level 4](level-4.md)).
- A page your pack shows over it with `op1f_page_show` lies over it. When that one closes, the stand-in shows again.

`echo` takes the blue and second knob's turns while its page is up, and lets everything else go:

```cpp
extern "C" int echo_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2 || !e->mine || e->what != (u32)OP1F_TURN) return 0;
    if (e->key == (u32)KNOB_BLUE) turned_repeats = turned_repeats + (u32)(e->turn >> 10);     // 1024 a step; more than 0 up
    else if (e->key == (u32)KNOB_SECOND) turned_step = turned_step + (u32)(e->turn >> 10);
    else return 0;
    ++turns;
    return 1;
}
```

**`shown(where, how)`** is called at each frame where what it says changes, before the frame functions of that frame. From the frame where it says `OP1F_SHOWN_UP`, the frame's `page` is your card. `where` is the instrument. `how` is 0 when your page has left the screen, otherwise `OP1F_SHOWN_UP` with these bits beside it:

| Bit | What |
| --- | --- |
| `OP1F_SHOWN_OFF` | The instrument's sequencer is switched off |
| `OP1F_SHOWN_COVERED` | The unit's own SEQ OFF notice lies over your page (below) |

A page covered by another pack's page is told 0, and `OP1F_SHOWN_UP` again when that page closes.

**The octave keys**, 36 and 37, move the instrument's octave as they do for every sequencer. To see them on your page and still let them work, answer `OP1F_SEEN` for them in your input function ([level 4](level-4.md), "Watching a key"). To hear the move itself, give your sequencer an `octave` function. It's handed the steps the octave moved, and answers not 0 to have the notes of keys held on the keyboard follow now, or 0 to leave them where they are.

## When the unit says SEQ OFF

Switch the sequencer off while its screen is up and the unit shows its own SEQ OFF notice for a moment. Over the firmware's own sequencers it darkens the screen and draws the notice on top. **Over your page it looks the same**, and you have nothing to do for it.

While the notice is up, `shown` says `OP1F_SHOWN_COVERED` and the frame's `page` still names your card. Paint as usual. After every pack's frame function, the base darkens the whole frame, what you painted included, and draws the unit's notice over it. When the notice goes, your page is back in its own colours at the next frame.

`echo` paints its page whatever `shown` says, so its dots and numbers show dimmed under the notice.

## The owner's choice, kept

The unit saves the chosen sequencer by its name. The base keeps the owner's choice too, in a small file of its own, `user/pf/.seq`, so the choice survives a start without your pack:

- **Your pack taken off the unit.** At the next start the firmware doesn't know the name. It says so in its log and starts `endless`. The base's file keeps your sequencer's name.
- **Your pack back.** At the next start the base finds that the firmware fell back to `endless` and picks your sequencer again. The log says so.
- **A choice the owner makes while your pack is away** is a choice like any other. The base keeps it, and it stands when your pack comes back.

**The one case where your sequencer comes back without being asked**: the owner picks `endless` while your pack **and the base** are both off the unit. Nothing of the base saw that pick. When both come back, the base finds `endless` and its own file naming your sequencer, and picks yours again.

## Your sequencer's settings

A sequencer keeps what it keeps (settings, patterns) in its own folders ([level 3](base.md#level-3-folders-files-and-text)), read in its frame function and held in words. Nothing goes into the base's files or the firmware's own settings. `echo` keeps its two values in `echo.bin`, in a folder the computer doesn't see. It reads the file at its first frame, and writes it once the values have changed and then stayed put for a second, or when its page goes, and never while the disk mode is open.

If you'd rather keep a sequencer's settings with each sound or each tape, that's [level 7](level-7.md).

## The notes heard

Any pack may ask to be told **every note the instrument is asked to start and to end**, whoever asks: the keyboard, MIDI, a sequencer of the firmware's or of a pack's. It's for a pack that listens, such as a recorder or a picture that moves with the notes. A pack that wants to change what's played is a sequencer, and has `key`.

```json
"asks": [ { "kind": "notes", "name": "meter", "note": "meter_note" } ]
```

The base calls `note(event)` for each note, before the instrument plays it, **on whatever thread asks for the note**, the audio thread among them. The rules of an input function apply. It answers nothing and changes nothing. It may not start or end a note itself.

| Field | What |
| --- | --- |
| `what` | `OP1F_NOTE_BEGINS` or `OP1F_NOTE_ENDS` |
| `host` | The instrument the unit is on. Another number while it's on neither: the note then only goes out over MIDI |
| `note` | 0 to 127 |
| `velocity` | 1 to 127 for a start, 0 for an end |
| `flags` | `OP1F_NOTE_NO_MIDI` when the note isn't sent out over MIDI as well |

**What a listener isn't told.** It hears what the instrument is asked, not the sound:

- The instrument also ends notes by itself, and no call comes for those: when the unit changes instrument, when a sound is loaded, when a seventh note takes a voice.
- An end may come twice, or for a note that never started.
- A start comes even where the instrument then plays nothing: a note already sounding, a sound being loaded.

If you keep a list of sounding notes, treat it as an upper bound and clear it when the frame's `mode` changes. Eight asks in all may hear the notes. The ninth is left aside.

## The lines the base writes

| Line begins | What it says |
| --- | --- |
| `seq: '<name>' of '<pack>' is in the list of sequencers as number <n>` | Your sequencer's place in the list, once a start |
| `seq: '<pack>' asks for a sequencer '<name>', which ...: left aside` | Why an ask was left aside |
| `seq: the synth's sequencer is '<name>'` | A switch (the drum's says `the drum's`) |
| `seq: '<name>' put back for the synth` | Your sequencer picked again after a start without it |
| `seq: '<name>' this period:` | Through the base's quiet gate: ticks, keys (taken), notes on and off, notes the base ended at a stop, calls refused |
| `page: '<pack>' stands in for the sequencer's screen of the synth` | Your page went up. When it no longer stands in, a line says why |
| `page: the unit's own notice drawn over the page of '<pack>', the page darkened under it` | The SEQ OFF notice came up over your page |
| `notes: '<name>' of '<pack>' hears the notes` | A listener, once a start |

A count of refused calls that isn't 0 means your pack called a note helper from somewhere it may not, or for a note it didn't have sounding.

## Limits, in one place

| | |
| --- | --- |
| Sequencers of packs | 16 in all |
| A sequencer's name | 1 to 15 characters of `a`-`z`, `0`-`9`, `-`, `_` |
| Asks that hear the notes | 8 in all |
| Notes in the base's list | 128 an instrument |
| Velocity | 1 to 127 |
| Pages that stand in | One a sequencer |
| A masked section | A few microseconds |

## Moving a sequencer that changes the firmware over to an ask

Before level 5, a sequencer pack had to stand in the firmware's own code: at the player switch, at the two lookups between a sequencer's name and its number, at the two places that answer the sequencer's controller, at the factory catalog's read and write, and often at the list, the pick, the note functions, the key and knob posters and the frame. **A base of level 5 holds the sequencer's places itself**, and the note functions' entries with them.

**When two packs change the same place, the loader keeps the one whose file name sorts first** and leaves the other out. The base is a pack too, `base.pack`. So a pack whose file name sorts after `base.pack` and changes one of the base's places is the one left out, with a line in the log and `1 left out` on the start screen. But a pack whose name sorts before `base.pack` (any name that starts with a digit, a capital letter or an `a`) keeps the place, and **the base is left out instead**, and with it every pack that needs the base. Nothing of open-field runs at that start. A pack that adds its sequencer through an ask changes none of those places, whatever its name. An older pack that still changes them has to come off the unit, whatever its name, or move to an ask, in five steps.

1. **Delete from your table** every entry at those places: the player switch, the name lookups, the controller's getters, the catalog's read and write, the list and the pick. Delete the entries at the two note functions, the key and knob posters and the frame loop too. The base offers each of them in another form.
2. **Delete the code that served them**: the player built with the firmware's constructor and its copied table of functions, the controller's copy, any guard that compared the firmware's tables with copies your pack carried, the type number and list id you chose, the row you added to the catalog and the code that kept it out of the factory file. The base gives your sequencer its number at each start, and adds and removes its row itself.
3. **Write each piece again in its new form:**

| Your pack did | It becomes |
| --- | --- |
| A player's key down and key up slots | `key`: not 0 takes the key (its up comes to `key`, and you play the note), 0 lets the instrument play it (and you hear no up) |
| The player's slot called at each step of the clock, its own counts of ticks | `tick`, with `count`, `tempo`, `us`, `clock` and `jumped` |
| The player's start, stop and reset slots | `run`, with `OP1F_RUN_ON` and `OP1F_RUN_STOP` |
| Reading the instrument word and the type words | `host`, handed to every call |
| The controller's screen slot, opening your page from inside it | `op1f_page_stand_in` once, from your frame function. `shown` says when it's on the screen |
| A marker pixel in your picture, read back from the frame buffer | `shown`, and the frame's `page` |
| Painting into the frame buffer from a hook of your own | Your frame function paints on the frame's `canvas` while `page` is your card |
| The controller's key and knob slots answering "not mine", hooks at the key and knob posters | Your input function, while `mine` is set: take what your page uses |
| Taking the octave keys yourself | `OP1F_SEEN` to see them, `octave` to hear the move |
| Calls of the firmware's note functions | `op1f_note_on` and `op1f_note_off` |
| Hooks on the note functions, to hear the notes | An ask of the kind `notes` |
| Your own queue of notes, drained with interrupts off | `op1f_mask` and `op1f_unmask` around what `key` and `tick` share, the note calls after the section |
| Ending and starting a note that two chords share | The same: end it, then start it. The base refuses a second note on of a sounding note |
| Settings kept in a stock player's memory | A folder of your own (level 3), or [level 7](level-7.md) |
| `synth_only` in the catalog row | `"synth_only": true` in the ask |
| The firmware's logger | `op1f_log`, from your frame function only |

4. **Write the recipe**: the card at level 5, the ask with your functions' symbols, and the folders you keep. The pack tool writes the card's source, as for any card ([the base](base.md), "The card").
5. **Build and check.** Your table should now be empty, and the pack tool's `check` says the pack changes no place of the firmware and prints your ask. On the unit, your sequencer is at the end of the list, and the log says where.

A pack that also offers names to other packs keeps them. The loader's names between packs work as before.

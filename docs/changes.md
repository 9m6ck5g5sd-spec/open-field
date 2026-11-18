# The change list

Every place where open-field changes OS 1.7.0+0 of the OP-1 Field, and nothing else. This page is made from `profiles/op1f-1.7.0.json` by `tools/change_list.py`; change the profile, not the page.

It holds addresses and our own words. It quotes no instruction of the firmware and gives none of its bytes. For each place the profile keeps a checksum of the firmware's own bytes there, so that the loader can tell the place before it changes it; a unit with another firmware is left as it is.

There are 131 places. 2 of them are changed in the firmware file, by the builder, on the owner's own computer. The others are changed in the running firmware, in memory, at every start: 138 changes in 7 tables. A place that two packs can use is listed under each.

## In the firmware file, by the builder

| Address | Bytes | What stands there | What is changed |
| --- | --- | --- | --- |
| `0x8c0b3a88` | 8 | In the start-up, right behind the step that mounts the disk: the load of an address for the call that follows. | A call to our start-up stub, which runs the loader (it looks for pack files and makes their changes) and then does the load. |
| `0x8d4e36f8` | 8 | The size the firmware gives its application heap at the start. | The builder makes it 2 MiB smaller. The memory that is left over is where our code is loaded. |

The builder also adds one record to the file: the loader, our own code, loaded into the memory the first change sets aside. The packs, the base among them, are files on the unit.

## In the running firmware, at every start

### The loader (3)

The loader's own three changes: the loader is our small piece inside the firmware file. It makes them at every start that runs at least one pack.

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x8490f810` | a word of data | `op1f_base_root_child` | A word in the table of the disk mode's root folder: the function that answers a folder's name with what that folder does. | Ours answers for the folder `packs`, asks a pack that has folders of its own (`fx`, `lfo`), and hands every other name to the firmware's function. |
| `0x8fcaddea` | a call | `op1f_base_populate_stub` | In the function that builds the disk mode's folder tree: the call that makes the last of the firmware's own folders, with the step behind it. | A call to our stub, which makes the firmware's folder as before, then the folder `packs` with the pack files in it, then the folders a pack has of its own. |
| `0x8c0b3b14` | a call | `op1f_base_text_stub` | In the start-up, where the version text is drawn on the start screen: the load of the frame buffer's address. | A call to our stub, which does the load and puts words behind the version text: open-field and the release base.pack offers, or that the unit starts as stock. |

### The base, `base.pack` (14)

The pack `base`, the file base.pack: the shared pack every other pack relies on. Every unit with open-field needs it. Its last five changes hand keys, knobs and a screen to a pack's page (level 2).

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x8c0d848c` | the load of an address | `op1f_audio_handler` | Where the audio start-up registers the handler of an audio block: the load of the handler's address. | The address becomes our wrapper's. It measures the time an audio block takes and calls the firmware's handler. |
| `0x8c0ca0d0` | the load of an address | `op1f_audio_handler` | Where the loopback controller registers the handler of an audio block, the first of two places: the load of the handler's address. | The address becomes our wrapper's, as at the audio start-up. |
| `0x8c0ca2f8` | the load of an address | `op1f_audio_handler` | Where the loopback controller registers the handler of an audio block, the second of two places: the load of the handler's address. | The address becomes our wrapper's, as at the audio start-up. |
| `0x8fc88c64` | a jump, with a trampoline of 8 bytes | `op1f_svg_load` | The entry of the picture loader, which loads a screen's picture by its file name. | A jump to our loader. A picture for which a pack carries instructions is put together from the unit's own pictures and handed to the firmware's reader; every other name goes on in the firmware's function. |
| `0x84013670` | a word of data | `op1f_fx_screen_synth` | A word in the synth controller's table: the function that says which screen to show. | Ours answers with our own screen while the effect train or the LFO page is open, and with the firmware's answer otherwise. |
| `0x8400e8b0` | a word of data | `op1f_fx_screen_drum` | A word in the drum controller's table: the function that says which screen to show. | Ours answers with our own screen while the effect train is open, and with the firmware's answer otherwise. |
| `0x8c0b113c` | a call | `op1f_cost_tick` | In the display thread's frame loop: the call of the firmware's own work of a frame before the draw. | A call to our wrapper, which takes the time, makes the firmware's call as it was and takes the time again: what a frame costs. |
| `0x8c0b114c` | a call | `op1f_cost_draw` | In the display thread's frame loop: the call that draws the screens. | A call to our wrapper, which takes the time, makes the firmware's call as it was and takes the time again: what a frame costs. |
| `0x8c0b1320` | a jump, with a trampoline of 8 bytes | `op1f_input_down_entry` | The start of the function that hands a key that goes down to the mode shown. | A jump to ours, which offers the key to the packs first: the pack whose page shows, then the others in the order of their files. A key a pack takes goes no further; any other goes on into the firmware's function, whose first instructions ours runs from its room. |
| `0x8c0b1390` | a jump, with a trampoline of 12 bytes | `op1f_input_shift_entry` | The start of the function that hands a key that goes down while shift is held to the mode shown. | The same for a key that goes down while shift is held. |
| `0x8c0b13c4` | a jump, with a trampoline of 12 bytes | `op1f_input_up_entry` | The start of the function that hands a key that comes up to the mode shown. | A jump to ours, which hands a key that comes up to whoever took it when it went down: the pack, or the firmware's function. |
| `0x8c0b1438` | a jump, with a trampoline of 8 bytes | `op1f_input_turn_place` | In the function that hands a knob's turn to the mode shown: the call right behind its own bookkeeping of the knob. | A jump to ours, which offers the turn to the packs as a key is offered; a turn a pack takes goes no further, any other goes on as before. |
| `0x8c0b1db0` | a call | `op1f_page_screen` | In the function that asks which screen is to show: the load right behind the mode's answer, where the screen's number is in hand. | A call to ours, which does the load and, while a pack's page is open in the mode shown, answers the screen the base lends packs in place of the mode's. |
| `0x8c0b116e` | a call | `op1f_gfx_frame_hook` | In the display thread's frame loop: the call that writes the frame buffer out of the cache before it is shown. | A call to our frame hook, which does our own work of a frame and then makes the firmware's call. |

### `8 tracks` (73)

The pack `8 tracks`: tape tracks 5 to 8, and tapes the original firmware still reads.

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x8fc213c0` | a jump | `op1f_load_block_map` | The entry of the function that loads a tape's block map. | Replaced by ours, which reads the firmware's file for tracks 1 to 4 and a file of our own for tracks 5 to 8. |
| `0x8fc1f90c` | a jump | `op1f_tape_chunks_init` | The entry of the function that sets up the tape's cache of chunks. | Replaced by ours, which lays the cache out for eight tracks. |
| `0x8fc1fca0` | a jump | `op1f_chunks_service` | The entry of the tape cache's service loop, which loads and writes chunks in the background. | Replaced by ours, which serves eight tracks. |
| `0x8fc20a0c` | a jump | `op1f_snapshots_reset` | The entry of the function that empties the undo history. | Replaced by ours, which also empties the records of tracks 5 to 8. |
| `0x8fc21280` | a jump | `op1f_save_block_map` | The entry of the function that saves a tape's block map. | Replaced by ours, which writes the firmware's file as the firmware writes it and a file of our own for tracks 5 to 8. |
| `0x8fc21638` | a jump | `op1f_take_snapshot_ex` | The entry of the function that takes an undo snapshot. | Replaced by ours, which calls the firmware's own snapshot code and carries the records of tracks 5 to 8 along. |
| `0x11a0822e` | a call | `op1f_read_block_ext` | In the tape's audio driver: the call that reads a block of tape. | A call to ours, which makes the firmware's call and then fills the buffers of tracks 5 to 8. |
| `0x11a0826a` | a call | `op1f_mixer_ext` | In the tape's audio driver: the call of the tape mixer. | A call to ours, which mixes tracks 5 to 8 into the tape's two sums and then makes the firmware's call. |
| `0x8c0c3212` | a jump, with a trampoline of 8 bytes | `op1f_tape_key_down` | The entry of the tape controller's key-down handler. | A jump to ours. The keys that select tracks 5 to 8 (the tape key held with a track key) are dealt with; every other key goes on in the firmware's handler through a trampoline. |
| `0x8c0c3a52` | a jump, with a trampoline of 8 bytes | `op1f_tape_shift_key_down` | The entry of the tape controller's handler for a key pressed with shift. | A jump to ours, which deals with shift and a track key for tracks 5 to 8; everything else goes on in the firmware's handler through a trampoline. |
| `0x8d513188` | a jump | `op1f_gui_selected_track` | The entry of the getter the screens ask for the selected track. | Replaced by ours, which answers with the selected track of all eight. |
| `0x8d4ecc3e` | a jump | `op1f_clip_valid` | The entry of the check a clip's record has to pass. | Replaced by ours: the same checks with eight tracks allowed. |
| `0x11a05888` | a call | `op1f_track_output` | In the audio core: the call that gets the selected track's output buffer for input monitoring. | A call to ours: the firmware's buffer for tracks 1 to 4, a buffer of our own for tracks 5 to 8. |
| `0x11a02b30` | a call | `op1f_monitor_track_p0` | In the audio core, where input monitoring picks the selected track's mixer strip: the load of the selected track's number. | A call to our stub, which does the load and points the code at our own strip for tracks 5 to 8. |
| `0x11a02d0a` | a call | `op1f_meter_track_p1` | In the audio core, where the selected track's level meter is fed: the load of the selected track's number. | A call to our stub, which does the load and points the code at our own meter words for tracks 5 to 8. |
| `0x8c0c123e` | a call | `op1f_saved_track_r1` | In the function that writes a tape's settings file: the load of the selected track's number. | A call to our stub, which does the load and never hands on a track above the fourth, so that the file names no track the original firmware lacks. |
| `0x8fc200ac` | a jump, with a trampoline of 8 bytes | `op1f_tape_io_read` | The entry of the function that reads audio from a tape track. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; tracks 5 to 8 are read by our code. |
| `0x8fc2021c` | a jump, with a trampoline of 8 bytes | `op1f_tape_io_write` | The entry of the function that writes audio to a tape track. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; tracks 5 to 8 are written by our code. |
| `0x8f00928c` | a jump, with a trampoline of 8 bytes | `op1f_xfade_start_edge` | The entry of the helper that crossfades the start edge of a clip. | A jump to ours, which lets all eight tracks through to the firmware's helper (through a trampoline) and turns any other number away. |
| `0x8f009344` | a jump, with a trampoline of 8 bytes | `op1f_xfade_stop_edge` | The entry of the helper that crossfades the end edge of a clip. | A jump to ours, which lets all eight tracks through to the firmware's helper (through a trampoline) and turns any other number away. |
| `0x8c0c24a4` | a jump, with a trampoline of 8 bytes | `op1f_tape_encoder` | The entry of the tape controller's knob handler. | A jump to ours, which looks at the turn first and goes on in the firmware's handler through a trampoline. |
| `0x8fc8272c` | a call | `op1f_tape_glyph_off` | In the tape screen's update: where the number of the track shown before is hidden. | A call to our stub, which hides the number through the firmware's table for tracks 1 to 4 and through a table of ours for tracks 5 to 8. |
| `0x8fc82740` | a call | `op1f_tape_glyph_on` | In the tape screen's update: where the number of the selected track is looked up to be shown. | A call to our stub, which remembers the track and answers with the firmware's table for tracks 1 to 4 and a table of ours for tracks 5 to 8. |
| `0x8d5156c4` | a jump | `op1f_track_unmuted` | The entry of the getter that says whether a track is unmuted. | Replaced by ours: the firmware's own flag for tracks 1 to 4, ours for tracks 5 to 8. |
| `0x8fc2188c` | a jump | `op1f_xfade_mark_loaded_range` | The entry of the function that marks a stretch of the loop crossfade as loaded. | Replaced by ours: the same tests for eight tracks. |
| `0x8fc21968` | a jump | `op1f_xfade_mark_dirty` | The entry of the function that marks tracks' loop crossfade as out of date. | Replaced by ours: the same marks for eight tracks. |
| `0x8f01dce0` | a jump, with a trampoline of 8 bytes | `op1f_fade_out` | The entry of the function that writes the fade-out at a clip's edge. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; for tracks 5 to 8 our code writes the fade. |
| `0x8f01ddf0` | a jump, with a trampoline of 8 bytes | `op1f_fade_in` | The entry of the function that writes the fade-in at a clip's edge. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; for tracks 5 to 8 our code writes the fade. |
| `0x8f00ad0c` | a jump, with a trampoline of 8 bytes | `op1f_post_lift` | The entry of the function that queues a lift. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; for tracks 5 to 8 ours makes ready what that function expects and joins it behind its test of the track. |
| `0x8f01ef00` | a jump, with a trampoline of 8 bytes | `op1f_lift` | The entry of the function that lifts audio off a track. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; tracks 5 to 8 are lifted by our code. |
| `0x8f01f548` | a jump, with a trampoline of 8 bytes | `op1f_drop_copy` | The entry of the function that drops a clip as a copy. | A jump to ours. A drop between tracks 1 to 4 goes on in the firmware's function through a trampoline; one that touches a track of 5 to 8 is made by our code. |
| `0x8f01f2a0` | a jump, with a trampoline of 8 bytes | `op1f_drop_mix` | The entry of the function that drops a clip mixed into what is on the tape. | A jump to ours. A drop between tracks 1 to 4 goes on in the firmware's function through a trampoline; one that touches a track of 5 to 8 is made by our code. |
| `0x8f01dc4c` | a jump, with a trampoline of 8 bytes | `op1f_clear_range` | The entry of the function that clears a stretch of a track. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; tracks 5 to 8 are cleared by our code. |
| `0x8f003044` | a jump, with a trampoline of 12 bytes | `op1f_clip_to_sample` | The entry of the function that turns the clipboard into a sample. | A jump to ours. A clipboard of tracks 1 to 4 goes on in the firmware's function through a trampoline; one that holds a track of 5 to 8 is turned into the sample by our code. |
| `0x8c0c1408` | a call | `op1f_tape_json_set_mixer` | In the function that writes a tape's settings file: the call that stores the mixer's values. | A call to ours, which makes the firmware's call and then stores the levels and pans of tracks 5 to 8 under a name of our own. |
| `0x8c0c2adc` | a jump, with a trampoline of 8 bytes | `op1f_tape_json_load` | The entry of the function that loads a tape's settings file. | A jump to ours, which lets the firmware's function run (through a trampoline) and then reads our own values back. |
| `0x8c0bb5c0` | a jump, with a trampoline of 8 bytes | `op1f_mixer_key_down` | The entry of the mixer controller's key-down handler. | A jump to ours, which notes that the mixer key is held (it shows the page of tracks 5 to 8) and goes on in the firmware's handler through a trampoline. |
| `0x8c0bb4c8` | a jump, with a trampoline of 12 bytes | `op1f_mixer_key_up` | The entry of the mixer controller's key-up handler. | A jump to ours, which notes that the mixer key is let go and goes on in the firmware's handler through a trampoline. |
| `0x8c0bb078` | a jump, with a trampoline of 8 bytes | `op1f_mixer_shift_key_down` | The entry of the mixer controller's handler for a key pressed with shift. | A jump to ours. While the page of tracks 5 to 8 shows, shift and a track key mutes one of them; otherwise the firmware's handler goes on through a trampoline. |
| `0x8c0ba95c` | a jump, with a trampoline of 8 bytes | `op1f_mixer_encoder` | The entry of the mixer controller's knob handler. | A jump to ours. While the page of tracks 5 to 8 shows, the knobs set their levels and, with shift, their pans; otherwise the firmware's handler goes on through a trampoline. |
| `0x8fc2e6ba` | a call | `op1f_mixer_values` | In the mixer screen's update: the call that fetches the tracks' levels and pans. | A call to ours: the values of tracks 5 to 8 while their page shows, the firmware's otherwise. |
| `0x8fc2e7c0` | a call | `op1f_mixer_levels_p4` | In the mixer screen's update: the load of the address of the four level numbers. | A call to our stub: the address of our own numbers while the page of tracks 5 to 8 shows, the firmware's otherwise. |
| `0x8fc2e81e` | a call | `op1f_mixer_track_unmuted` | In the mixer screen's update: the call that asks whether the first track is unmuted, for its colour. | A call to ours: the answer for track 5 while the page of tracks 5 to 8 shows, the firmware's otherwise. |
| `0x8fc2e87a` | a call | `op1f_mixer_track_unmuted` | In the mixer screen's update: the call that asks whether the second track is unmuted, for its colour. | A call to ours: the answer for track 6 while the page of tracks 5 to 8 shows, the firmware's otherwise. |
| `0x8fc2e8c0` | a call | `op1f_mixer_track_unmuted` | In the mixer screen's update: the call that asks whether the third track is unmuted, for its colour. | A call to ours: the answer for track 7 while the page of tracks 5 to 8 shows, the firmware's otherwise. |
| `0x8fc2e8fc` | a call | `op1f_mixer_track_unmuted` | In the mixer screen's update: the call that asks whether the fourth track is unmuted, for its colour. | A call to ours: the answer for track 8 while the page of tracks 5 to 8 shows, the firmware's otherwise. |
| `0x8fc2e948` | a call | `op1f_mixer_vu` | In the mixer screen's update: the call that fetches the four meters' values. | A call to ours: the meters of tracks 5 to 8 while their page shows, the firmware's otherwise. |
| `0x8fc21194` | a jump | `op1f_diff_range` | The entry of the function that finds the stretch of tape in which two undo snapshots differ. | Replaced by ours, which looks at all eight tracks, so that undo and redo reload what changed on tracks 5 to 8 too. |
| `0x8c0c1cc6` | a jump | `op1f_lift_all_bound` | In the tape controller's lift of every track: the test that ends the loop over the tracks. | A jump to our stub, which makes the test for eight tracks and goes on in the loop. |
| `0x8f009fec` | a jump, with a trampoline of 8 bytes | `op1f_lift_all` | The entry of the handler of the command that lifts every track. | A jump to ours. A lift of tracks 1 to 4 alone goes on in the firmware's handler through a trampoline; one that takes a track of 5 to 8 is made by our code. |
| `0x8f009a20` | a jump, with a trampoline of 8 bytes | `op1f_drop_all` | The entry of the handler of the command that drops every track. | A jump to ours. A drop of tracks 1 to 4 alone goes on in the firmware's handler through a trampoline; one that holds a track of 5 to 8 is made by our code. |
| `0x8f00de9c` | a jump | `op1f_midi_bound` | In the handler of MIDI messages for the mixer: the test of the channel. | A jump to our stub. The first four channels go on in the firmware's handler; the next four set the level, pan or mute of tracks 5 to 8. |
| `0x8fcad910` | a jump | `op1f_mtp_populate_bound` | In the function that fills a tape's folder in the disk mode: the test that ends the loop over the track files. | A jump to our stub, which makes the test for eight tracks, so that a tape's folder lists eight track files. |
| `0x8fcb9d66` | a jump | `op1f_mtp_name_bound` | In the disk mode's search of a track file by its name: right behind the comparison with the fourth name. | A jump to our stub, which also takes the names of the track files 5 to 8. |
| `0x8fcb9a94` | a jump | `op1f_mtp_file_track_bound` | Where the disk mode makes a track file: the test of the track's number. | A jump to our stub, which makes the test for eight tracks. |
| `0x8d4fa514` | a jump | `op1f_mtp_partition_bound` | In the helper that names a tape's storage: the test of the track's number. | A jump to our stub, which makes the test for eight tracks. |
| `0x8fc20c60` | a jump, with a trampoline of 8 bytes | `op1f_unmap_chunk` | The entry of the function that takes a chunk out of a track's block map. | A jump to ours. Tracks 1 to 4 go on in the firmware's function through a trampoline; for tracks 5 to 8 our code takes the chunk out of our own map. |
| `0x8fc2ca3c` | a call | `op1f_help_track` | In the help screen's update: the call that asks for the selected track. | A call to ours, which answers as the firmware's getter does and lights the legend's box of a track of 5 to 8. |
| `0x8fc33bce` | a call | `op1f_signalflow_unmuted` | In the signal flow screen's update: the call that asks whether the selected track is unmuted. | A call to ours, which answers for all eight tracks and points the screen's level hint at the right value for tracks 5 to 8. |
| `0x8fc33bb8` | a call | `op1f_signalflow_vu` | In the signal flow screen's update: the call that fetches the meters' values. | A call to ours, which makes the firmware's call and then moves the meters of tracks 5 to 8. |
| `0x8fc810f4` | a call | `op1f_clip_bar_outer` | In the tape screen's overlay that shows while shift is held: the call that draws a clip's outer line. | A call to our stub, which makes the line thinner and half a pixel lower and then makes the firmware's call: eight lanes fit where four stood. |
| `0x8fc8111e` | a call | `op1f_clip_bar_inner` | In the tape screen's overlay that shows while shift is held: the call that draws a clip's inner line. | A call to our stub, which makes the line thinner and half a pixel lower and then makes the firmware's call. |
| `0x8fc821a0` | a call | `op1f_clip_rect` | In the tape screen's drawing of a clip's bar: the call that fills the bar's rectangle. | A call to our stub, which makes the bar five rows high where it was seven and then makes the firmware's call. |
| `0x8f008c54` | a jump | `op1f_clipboard_write` | The entry of the function that writes the clipboard's file. | Replaced by ours, which writes the file as the firmware writes it for tracks 1 to 4, and a file of our own for what the clipboard holds of tracks 5 to 8. |
| `0x8c0b4110` | a call | `op1f_clipboard_load` | In the start-up: the call that loads the clipboard. | A call to ours, which makes the firmware's call and then reads our own file. |
| `0x8d4ed7d8` | a call | `op1f_clips_key_clear` | Where the tape's list of clips is emptied: the load of the list's name. | A call to our stub, which has our own list of the clips of tracks 5 to 8 emptied too and then does the load. |
| `0x8d4ed99e` | a call | `op1f_clips_key_push` | Where a clip is added to the tape's list of clips: the load of the list's name. | A call to our stub, which names the firmware's list for a clip of tracks 1 to 4 and a list of our own for a clip of tracks 5 to 8. |
| `0x8d4eddb4` | a call | `op1f_clips_key_update` | Where a clip is changed in the tape's list of clips: the load of the list's name. | A call to our stub, which names the firmware's list for a clip of tracks 1 to 4 and a list of our own for a clip of tracks 5 to 8. |
| `0x8d4ee384` | a call | `op1f_clips_key_erase` | Where a clip is taken out of the tape's list of clips: the load of the list's name. | A call to our stub, which names the firmware's list for a clip of tracks 1 to 4 and a list of our own for a clip of tracks 5 to 8. |
| `0x8c0b3d1c` | a call | `op1f_clips_load` | In the start-up's tape step: the call that loads the tape's clips. | A call to ours, which makes the firmware's call and then loads our own list of the clips of tracks 5 to 8. |
| `0x8c0d6168` | a call | `op1f_clips_load` | In the loading of a tape: the call that loads the tape's clips. | A call to ours, which makes the firmware's call and then loads our own list of the clips of tracks 5 to 8. |
| `0x8c0d6338` | a call | `op1f_tape_seconds` | In the tape list: the call that asks how long the highlighted tape is. | A call to ours, which counts the clips of all eight tracks. |
| `0x8c0d7946` | a call | `op1f_tape_clone` | In the tape list's copy of a tape: the call that copies the tape. | A call to ours, which makes the firmware's call and then copies tracks 5 to 8. |

### `fx train` (30)

The pack `fx train`: up to four effects on an instrument, their presets, and an instrument that keeps its effects.

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x11a05284` | a call | `op1f_fx_synth_t0` | In the synth's audio stage: the load of the effect's object, right before the firmware runs the sound's effect. | A call to our stub, which has the firmware leave its own effect out here: the train runs the effects. |
| `0x11a052b6` | a call | `op1f_fx_synth_t1` | In the synth's audio stage: the first load behind the place where the sound's effect is run. | A call to our stub, which runs the effects of the synth's train and then does the load. |
| `0x11a03e04` | a call | `op1f_fx_drum_t0` | In the drum's audio stage: the load of the effect's object, right before the firmware runs the sound's effect. | A call to our stub, which has the firmware leave its own effect out here: the train runs the effects. |
| `0x11a03e38` | a call | `op1f_fx_drum_t1` | In the drum's audio stage: the first load behind the place where the sound's effect is run. | A call to our stub, which runs the effects of the drum's train and then does the load. |
| `0x84013658` | a word of data | `op1f_fx_key_down` | A word in the synth controller's table: its key-down handler. | Ours deals with the keys of the effect train and of the LFO page and hands every other key to the firmware's handler. |
| `0x8401365c` | a word of data | `op1f_fx_key_shift` | A word in the synth controller's table: its handler for a key pressed with shift. | Ours deals with the keys of the effect train and of the LFO page and hands every other key to the firmware's handler. |
| `0x84013660` | a word of data | `op1f_fx_key_up` | A word in the synth controller's table: its key-up handler. | Ours deals with the keys of the effect train and of the LFO page and hands every other key to the firmware's handler. |
| `0x84013664` | a word of data | `op1f_fx_encoder` | A word in the synth controller's table: its knob handler. | Ours deals with the knobs of the effect train and of the LFO page and hands every other turn to the firmware's handler. |
| `0x8400e898` | a word of data | `op1f_fx_key_down` | A word in the drum controller's table: its key-down handler. | Ours deals with the keys of the effect train and hands every other key to the firmware's handler. |
| `0x8400e89c` | a word of data | `op1f_fx_key_shift` | A word in the drum controller's table: its handler for a key pressed with shift. | Ours deals with the keys of the effect train and hands every other key to the firmware's handler. |
| `0x8400e8a0` | a word of data | `op1f_fx_key_up` | A word in the drum controller's table: its key-up handler. | Ours deals with the keys of the effect train and hands every other key to the firmware's handler. |
| `0x8400e8a4` | a word of data | `op1f_fx_encoder` | A word in the drum controller's table: its knob handler. | Ours deals with the knobs of the effect train and hands every other turn to the firmware's handler. |
| `0x8401e730` | a word of data | `op1f_fx_patch_apply` | A word in a table of the code that loads sounds: the function that applies a sound loaded step by step. | Ours makes the firmware's call and then brings the effect train up to date. |
| `0x8401f62c` | a word of data | `op1f_fx_apply_synth` | A word in the table of the synth's sound slots: the function that puts a slot's settings onto the sound engine. | Ours makes the firmware's call and then brings the slot's effect train up to date. |
| `0x8401f608` | a word of data | `op1f_fx_apply_drum` | A word in the table of the drum's sound slots: the function that puts a slot's settings onto the sound engine. | Ours makes the firmware's call and then brings the slot's effect train up to date. |
| `0x8d516c3e` | a call | `op1f_spring_base_p4` | In the audio code of the effect spring: the load of the address of its working memory. | A call to our stub, which takes the address from a word of ours, so that each spring on the train has working memory of its own. |
| `0x8d5172da` | a call | `op1f_spring_base_r6` | In the audio code of the effect spring, on a second path: the load of the address of its working memory. | A call to our stub, which takes the address from a word of ours, as on the first path. |
| `0x11a066be` | a call | `op1f_mother_base_p4` | In the audio code of the effect mother: the load of the address of its working memory. | A call to our stub, which takes the address from a word of ours, so that each mother on the train has working memory of its own. |
| `0x11a054d0` | a call | `op1f_lfofx_pre_synth` | In the synth's audio stage, where the sound's LFO is applied to its effect: the load of the address of the LFO's settings. | A call to our stub, which does the load and has the call that follows go through a wrapper of ours, so that the sound's LFO can turn any effect of the train. |
| `0x11a0401e` | a call | `op1f_lfofx_pre_drum` | In the drum's audio stage, where the sound's LFO is applied to its effect: the load of the address of the LFO's settings. | A call to our stub, which does the load and has the call that follows go through a wrapper of ours, so that the sound's LFO can turn any effect of the train. |
| `0x8fc5a076` | a call | `op1f_lfo_param_name` | In the drawing of the value LFO's page: the call that asks for the name over a knob. | A call to ours, which answers with the names of the effect the LFO turns and makes the firmware's call for everything else. |
| `0x8fc578e8` | a call | `op1f_lfo_param_name` | In the drawing of the element LFO's page: the call that asks for the name over a knob. | A call to ours, which answers with the names of the effect the LFO turns and makes the firmware's call for everything else. |
| `0x8fc58b68` | a call | `op1f_lfo_param_name` | In the drawing of the random LFO's page: the call that asks for the name over a knob. | A call to ours, which answers with the names of the effect the LFO turns and makes the firmware's call for everything else. |
| `0x8fc5b1ec` | a call | `op1f_lfo_param_name` | In the drawing of the velocity LFO's page: the call that asks for the name over a knob. | A call to ours, which answers with the names of the effect the LFO turns and makes the firmware's call for everything else. |
| `0x8d4e9d18` | a call | `op1f_patch_read_preset` | In the loading of a synth sound: the call that reads the sound's file. | A call to ours, which makes the firmware's call and keeps the effect train and the LFOs the file carries. |
| `0x8d4ebc3a` | a call | `op1f_patch_read_sampler` | In the loading of a sampler sound: the call that reads the sound's file. | A call to ours, which makes the firmware's call and keeps the effect train and the LFOs the file carries. |
| `0x8d4eb004` | a call | `op1f_patch_read_drum` | In the loading of a drum sound: the call that reads the sound's file. | A call to ours, which makes the firmware's call and keeps the effect train and the LFOs the file carries. |
| `0x8d4f2a06` | a call | `op1f_patch_assign_stub` | In the function that puts a loaded sound onto a sound slot: the call that copies the sound's settings. | A call to our stub, which makes the firmware's call and tells the effect train that another instrument is on the slot. |
| `0x8c0cd7ec` | a call | `op1f_patch_save_synth_stub` | In the saving of an instrument: the call that writes a synth sound's file. | A call to our stub, which adds the slot's effect train and LFOs to what is written and then makes the firmware's call. |
| `0x8c0cd83a` | a call | `op1f_patch_save_drum_stub` | In the saving of an instrument: the call that writes a drum sound's file. | A call to our stub, which adds the slot's effect train and LFOs to what is written and then makes the firmware's call. |

### `lfo` (10)

The pack `lfo`: four LFOs on a synth sound, their page and their presets.

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x11a05284` | a call | `op1f_lfo_alone_fx` | In the synth's audio stage: the load of the effect's object, right before the firmware runs the sound's effect. | Only when the pack `fx train` is away: a call to our stub, which runs our LFOs on the sound's effect and then does the load. |
| `0x84013658` | a word of data | `op1f_lfo_alone_key_down` | A word in the synth controller's table: its key-down handler. | Only when the pack `fx train` is away: ours deals with the keys of the LFO page and hands every other key to the firmware's handler. |
| `0x8401365c` | a word of data | `op1f_lfo_alone_key_shift` | A word in the synth controller's table: its handler for a key pressed with shift. | Only when the pack `fx train` is away: ours deals with the keys of the LFO page and hands every other key to the firmware's handler. |
| `0x84013660` | a word of data | `op1f_lfo_alone_key_up` | A word in the synth controller's table: its key-up handler. | Only when the pack `fx train` is away: ours deals with the keys of the LFO page and hands every other key to the firmware's handler. |
| `0x84013664` | a word of data | `op1f_lfo_alone_encoder` | A word in the synth controller's table: its knob handler. | Only when the pack `fx train` is away: ours deals with the knobs of the LFO page and hands every other turn to the firmware's handler. |
| `0x11a04304` | a call | `op1f_lfo4_env_stub` | In the synth's audio stage, inside the loop over the voices, before a voice's envelope is made: the load of a buffer's address. | A call to our stub, which runs our LFOs for the voice (their part at the envelope) and then does the load. |
| `0x11a04590` | a call | `op1f_lfo4_eng_stub` | In the synth's audio stage, inside the loop over the voices, before a voice's level is applied: the load of an address. | A call to our stub, which runs our LFOs for the voice (their part at the sound engine) and then does the load. |
| `0x11a04ce6` | a call | `op1f_lfo4_reset_synth` | In the synth's audio stage, where a voice begins a new note: the call that starts the voice's LFO anew. | A call to ours, which makes the firmware's call and then starts our LFOs anew for the same voice. |
| `0x8f006848` | a call | `op1f_lfo4_reset_synth` | Where the synth gives a new note to a voice: the call that starts the voice's LFO anew. | A call to ours, which makes the firmware's call and then starts our LFOs anew for the same voice. |
| `0x8f006a16` | a call | `op1f_lfo4_reset_synth` | Where the synth starts a note on several voices: the call that starts a voice's LFO anew. | A call to ours, which makes the firmware's call and then starts our LFOs anew for the same voice. |

### `sidechain` (8)

The pack `sidechain`: one tape track turned down by another, or by the beat.

| Address | Change | Function of ours | What stands there | What ours does |
| --- | --- | --- | --- | --- |
| `0x11a0822e` | a call | `op1f_duck_read_block` | In the tape's audio driver: the call that reads a block of tape. | Only when the pack `8 tracks` is away: a call to ours, which runs the sidechain around the firmware's call. |
| `0x8c0c1408` | a call | `op1f_duck_json_set` | In the function that writes a tape's settings file: the call that stores the mixer's values. | Only when the pack `8 tracks` is away: a call to ours, which makes the firmware's call and then stores the sidechain's settings under a name of our own. |
| `0x8c0c2adc` | a jump, with a trampoline of 8 bytes | `op1f_duck_json_load` | The entry of the function that loads a tape's settings file. | Only when the pack `8 tracks` is away: a jump to ours, which lets the firmware's function run (through a trampoline) and then reads the sidechain's settings back. |
| `0x8fc33bb8` | a call | `op1f_duck_vu` | In the signal flow screen's update: the call that fetches the meters' values. | Only when the pack `8 tracks` is away: a call to ours, which makes the firmware's call and then draws the sidechain's page. |
| `0x84010058` | a word of data | `op1f_duck_key_down` | A word in the signal flow controller's table: its key-down handler. | Ours deals with the keys of the sidechain's page; the firmware's own handling runs for every other key. |
| `0x8401005c` | a word of data | `op1f_duck_key_shift` | A word in the signal flow controller's table: its handler for a key pressed with shift. | Ours: shift and the mixer key change between the sidechain's page and the firmware's signal flow screen. |
| `0x84010060` | a word of data | `op1f_duck_key_up` | A word in the signal flow controller's table: its key-up handler. | Ours deals with the keys of the sidechain's page; the firmware's own handling runs for every other key. |
| `0x84010064` | a word of data | `op1f_duck_encoder` | A word in the signal flow controller's table: its knob handler. | Ours: the knobs set the sidechain's source, amount, hold and release, and four more values with shift. |

## Words

- **A word of data**: four bytes that hold the address of a function. Ours is put there.
- **A call**, **a jump**: eight bytes of code become a call of, or a jump to, a function of ours.
- **The load of an address**: eight bytes of code that load a function's address load ours.
- **A trampoline**: where a jump stands at the entry of a function of the firmware, the instructions it displaces are needed still. The loader copies them, from the running firmware, into a room of the pack and puts a jump back behind them. Our function ends its way into the firmware there. So no pack holds a copy of those instructions.
- **L1 code** and **DDR**: the two kinds of memory the firmware's code runs from.

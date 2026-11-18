# The firmware's functions our code uses

Made by `tools/interfaces.py` from the sources and from `profiles/op1f-1.7.0-interfaces.json`; change those, not this page.

This page lists the functions of the firmware that open-field's own code calls, for OS 1.7.0. For each it says where the function stands, what the caller hands over, what comes back and which thread our code calls it on. It says nothing of how a function works inside: that is not needed to call it, and it is not written down in this repository ([CONTRIBUTING.md](../CONTRIBUTING.md), "Writing about the firmware").

**Every address belongs to one firmware file**, OS 1.7.0. On another version each one has to be found again ([methods](methods.md), "Moving the work to another firmware version").

**How a source binds a function.** Two lines: the function's type, and a name for its address. The type is ours; nothing of the firmware is copied.

```cpp
typedef int (*FileSizeFn)(const char* path);
#define STOCK_FILE_SIZE ((FileSizeFn)0x8c051f6cu)
```

**How arguments travel.** The first three go in the registers R0, R1 and R2, the rest on the stack. A result comes back in R0. A function that gives back a structure gets the address for it in P0. The compiler does all of that when the type is right, so a wrong type is a crash: before you bind a function that is not on this page, read how the firmware itself calls it ([methods](methods.md), "Reading a caller before you bind a function").

**The threads.** *The main thread* runs the start-up. *The display thread* draws a frame some sixty times a second; the shared part calls our frame functions from it. *The key thread* hands key presses and knob turns to the mode that is shown. *The audio thread* makes the sound, a block of 64 samples at a time, and must never wait: no log line, no file, no memory asked for there. Where the last column says "ours", it names the threads our code calls the function on today; that it is safe on another thread has not been shown.

There are 159 functions bound in the sources. 47 are described below, by subject. The other 112 are listed at the end with their type and the files that bind them: their words are still to be written.

## The log

The unit writes a log into the folder `logs` of its disk. [Methods](methods.md) says how to copy a log off the unit and how to read one.

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c0539f0` | `void log(int level, const char* file, int line, const char* format, ...)` | Writes one line into the unit's log. `level`: 3 for a line of information, 2 for a warning, 1 for an error. `file`: a text in the form of a source path; the log shows what stands between its last slash and its dot as the line's tag, so `op1f/mypack.cpp` gives the tag `mypack`. A text without a slash and a dot gives a line with no tag. `line`: a number shown in brackets behind the tag. `format` and the values behind it: as `printf` takes them. A line is cut at about 260 characters, the time and the tag counted. | Any thread but the audio thread: keep a count there and print it from a frame function. Ours: the main thread at the start, the display thread, the key thread, the tape's background thread. |

## Files on the unit's disk

A path is a C text, counted from the top of the unit's disk, with no slash in front: `user/pk/kit.pack`. There are two families of calls. Use one family for a file from its open to its close.

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c051f6c` | `int file_size(const char* path)` | The size of the file at a path, in bytes; -1 when there is no such file. It writes nothing into the log, so it is the way to ask whether a file is there. | Not the audio thread. Ours: the main thread at the start, the display thread, the disk mode's thread. |
| `0x8fc88ac0` | `int file_open(const char* path, const char* mode)` | Opens a file. `mode` is a text as C's `fopen` takes it; our code hands over the firmware's own text for reading, `rb`, which stands at `0x848daa84`. Gives back a descriptor, a small number that may be 0. A negative number means the open failed, and the function has then written a line into the log itself. | Not the audio thread. Ours: the main thread at the start and when a screen is made, and the display thread. |
| `0x8fc88a90` | `int file_read(void* buffer, int size, int count, int descriptor)` | Reads `count` items of `size` bytes into the buffer. Gives back the number of items read. The descriptor is the fourth argument, so it travels on the stack. | As `file_open`. |
| `0x8fc88a7c` | `int file_close(int descriptor)` | Closes a file opened with `file_open`. | As `file_open`. |
| `0x8c0a3280` | `int file_seek(int descriptor, u32 offset_low, u32 offset_high, int from)` | Goes to a place in a file opened with `file_open`. The offset is 64 bits wide and is handed over as two words, the low one first. `from`: 0 counts from the file's start, 1 from the present place, 2 from the file's end. Gives back the low word of the new place. | As `file_open`. |
| `0x8fc20690` | `i32 fopen(const char* path, const char* mode)` | The second family, which the tape's code uses. Opens a file with a mode as C has it (`rb`, `w+b`). Gives back a descriptor, which may be 0; a negative number when the open failed, after a line of its own in the log. To ask whether a file is there, use `file_size`, which writes no line. | Not the audio thread. |
| `0x8fc2065c` | `i32 fread(void* destination, i32 size, i32 count, i32 file)` | Reads `count` items of `size` bytes from a file opened with `fopen`. Gives back the number of items read. | As `fopen`. |
| `0x8fc205c8` | `i32 fwrite(const void* source, i32 size, i32 count, i32 file)` | Writes `count` items of `size` bytes to a file opened with `fopen`. Gives back the number of items written, and writes a line into the log itself when it fails. | As `fopen`. |
| `0x8fc205b4` | `void fclose(i32 file)` | Closes a file opened with `fopen`. | As `fopen`. |
| `0x8fc20804` | `void rename(const char* old_path, const char* new_path)` | Gives a file another path, over a file that is there already. Our code writes a file under a spare name and renames it over the real one, so that a power cut leaves the old file or the new one, never half of one. | As `fopen`. |
| `0x8fcba530` | `u32 file_mtime(const char* path)` | A number for when the file was last written; 0 when there is no such file. Our code only compares two such numbers: of two files, the one written later never has the smaller number. What the number counts in has not been established. | Not the audio thread. |
| `0x8c05203c` | `int make_folders(const char* path)` | Makes the folders of a path that are missing. Only a part that a slash ends is made: `user/fx/` makes both folders, `user/fx` makes `user` alone. | Not the audio thread. |
| `0x8fcbd194` | `int file_remove(const char* path)` | Removes the file at a path. Gives back 0 when the file is gone. | Ours: the disk mode's thread. |
| `0x8c051df8` | `void* dir_open(const char* path)` | Opens a folder to list it. The path has no slash at its end. Gives back a handle, or 0 when the folder cannot be opened. | Not the audio thread. Ours: the main thread at the start, the disk mode's thread. |
| `0x8c0a4820` | `void* dir_next(void* handle)` | The next entry of a folder opened with `dir_open`, or 0 at the end. The entry's name is the text that starts 11 bytes into what comes back. Do not count on the order of the entries: our code sorts the names itself. | As `dir_open`. |
| `0x8c0a5424` | `int dir_close(void* handle)` | Closes a folder opened with `dir_open`. | As `dir_open`. |

## JSON values and JSON files

The firmware keeps its settings as JSON, and so does our code. A JSON value is a small object of 12 bytes in memory the caller provides (our code keeps 16). It owns what it holds: make it with `json_ctor`, give it back with `json_dtor`. What `json_at`, `json_get` and `json_lookup` give back is a reference into another value, not a value of the caller's to destroy. A value has a kind: 0 nothing (null), 1 an integer, 2 an unsigned number, 3 a number with a fraction, 4 a text, 5 true or false, 6 an array, 7 an object. A number read from a file has the kind 2 or 3, never 1: do not test for 1, ask `json_int`.

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c085b78` | `void json_ctor(void* json, int kind)` | Makes a value of a kind in the caller's 12 bytes. Kind 0 makes a null, which becomes an array at its first `json_push`. | Not the audio thread. Ours: the main thread at the start, the display thread, the key thread. |
| `0x8c0844b0` | `void json_dtor(void* json)` | Gives back what a value holds. Call it for every value made with `json_ctor`, and for none that is a reference. | As `json_ctor`. |
| `0x8c0534a0` | `int json_read_file(const char* path, void* json)` | Reads a JSON file and parses it into a value the caller has made. Not 0 when the file was read and parsed. | As `json_ctor`. |
| `0x8c053138` | `int json_write_file(void* path_string, void* json)` | Writes a value as a JSON file. The path is a text object of the firmware (`string_ctor` below), not a C text. The file is written under a spare name in the folder `tmp` and then renamed over the target. 0 comes back when that failed: look at the answer, and make the target's folder first. | As `json_ctor`. |
| `0x8c084810` | `int json_int(void* json)` | A number as an `int`. It takes all three kinds of number. For any other kind it gives back 0 and writes a line into the log. | As `json_ctor`. |
| `0x8c084b64` | `int json_bool(void* json)` | A value of the kind true-or-false as 1 or 0. | As `json_ctor`. |
| `0x8c084bf2` | `int json_size(void* json)` | How many elements an array has. Ask it of an array only. | As `json_ctor`. |
| `0x8c084eb0` | `void* json_at(void* json, int index)` | An array's element by its number, from 0: a reference into the array. | As `json_ctor`. |
| `0x8c085038` | `int json_has(void* json, const char* key)` | Not 0 when an object has a member of that name. | As `json_ctor`. |
| `0x8c084f44` | `void* json_get(void* json, const char* key)` | An object's member by its name: a reference into the object. Our code asks `json_has` first and never asks for a member that is not there. | As `json_ctor`. |
| `0x8c085e54` | `void* json_lookup(void* json, const char* key)` | An object's member by its name, as `json_get` gives it; a member that is not there is made, as a null. The way to add a member: look it up, then assign to it. | As `json_ctor`. |
| `0x8c084588` | `void json_assign(void* target, void* value)` | Gives a value the content of another. The target may be a reference, such as a member got with `json_lookup`. | As `json_ctor`. |
| `0x8c085d24` | `void json_push(void* array, void* value)` | Puts a copy of a value at an array's end. A null becomes an array at its first push. | As `json_ctor`. |
| `0x8c083aec` | `void json_from_int(void* json, int value)` | Makes a value an integer (the kind 1). | As `json_ctor`. |
| `0x8c083abc` | `void json_from_bool(void* json, int value)` | Makes a value true or false (the kind 5). | As `json_ctor`. |
| `0x8c084400` | `void json_from_cstr(void* json, const char* text)` | Makes a value a text (the kind 4), copied from a C text. | As `json_ctor`. |

## Texts and memory

Our code has no library of its own: no `malloc`, no `memcpy`, no division by a variable. What it needs of memory it keeps in its own data, or asks of the firmware with the calls below.

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c03e40c` | `void string_ctor(void* out, const char* text)` | Makes a text object of the firmware from a C text, in memory the caller provides (our code keeps 48 zeroed bytes for it). A text of up to fifteen characters lives inside the object, and nothing has to be freed afterwards. Our code keeps its paths that short for this reason: `user/op1f.json` fits, `user/lfo/01.json` does not. | Not the audio thread. Ours: the main thread at the start, the display thread, the key thread, the disk mode's thread. |
| `0x8fcc5744` | `void* alloc(u32 bytes)` | A block of the firmware's own heap, the one its screens and pictures come from. It never gives back 0: when the heap has no room, the firmware writes an error into the log and stops. So bound what you ask for (our picture maker asks for a megabyte at most) and give every block back with `free` at `0x8fcc4e10`. | Not the audio thread. Ours: the main thread when a screen is made, and the display thread. |
| `0x8fcc4e10` | `void free(void* block)` | Gives back a block got with `alloc` at `0x8fcc5744`. | As `alloc`. |
| `0x8fcc3c48` | `void* heap_malloc(int heap_index, u32 size)` | A block of one of two large heaps that the firmware itself hardly uses, by the heap's number, 0 or 1. Our code takes large blocks there once and keeps them: the tape's cache for tracks 5 to 8 from heap 1, the effect train's private regions from heap 0. Gives back 0 when the heap has no room. | Not the audio thread. Ours: the tape's set-up at the start, and the display thread. |
| `0x8fcc33dc` | `u32 heap_free(int heap_index)` | How many bytes of such a heap are not in use, by the heap's number. | Ours: the display thread. |

## Screens, pictures and texts

A screen of the firmware is an object with a picture: an SVG text, parsed into a document whose parts can be found by their id, shown, hidden, moved and given a text. Our own screens are made the same way.

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c040260` | `void* parse_svg(char* text)` | Parses a picture, an SVG text that a zero byte ends, into a document. Gives back the document, or 0. Hand it only what the firmware's own pictures use: the path commands M, L, H, V, C, S and z in capitals or small letters, and no attribute its own files lack. A path command it does not know makes it hang, and the unit with it. | Ours: the main thread when a screen is made, and the display thread. |
| `0x8fc248a8` | `void screen_ctor(void* obj, int id, const char* name)` | Makes a screen object in memory the caller provides (our code keeps 64 bytes) and registers it under a number. Register a screen once: a number that is taken stops the firmware. In OS 1.7.0 the numbers 37, 38, 77 and 85 are free, and our code uses 37, 38 and 77. Keep a number below 82: the firmware draws the screens in the order of their numbers, and a screen above 82 is drawn over the shutdown countdown. The object's second word is its document: put the parsed picture there. `name` may be 0. | Ours: the display thread, once. |
| `0x8fc241d0` | `void* find_node(void* screen, const char* id, int optional)` | Finds a part of a screen's picture by its id. With `optional` not 0 it gives back 0 for a part that is not there, and our code always asks that way. The screen must have its document: look at the object's second word before you ask. | Ours: the display thread. |
| `0x8c03db96` | `void apply_matrix(void* node, const u32* matrix)` | Gives a part of a picture a transform: nine words, each a number with ten bits behind the point, `[a, b, 0, c, d, 0, e, f, 1024]`, for x' = a·x + c·y + e and y' = b·x + d·y + f. Keep the third, sixth and ninth word at 0, 0 and 1024. | Ours: the display thread. |
| `0x8fc23fcc` | `void* label_from_node(void* screen, void* node, int align)` | Turns a part of a picture into a label, an object whose text can be set while the unit runs. The part is a placeholder: it gives the label its place and its colour. The label takes the part's place in the document and comes back; keep it, since the id finds it no more. Our code passes 0 for `kind`: the text then starts at the placeholder's left edge. | Ours: the display thread, once for each label. |
| `0x8fc38ebc` | `void label_text(void* label, const char* text, int max_width)` | Sets a label's text from a C text. A text wider than `max_width` pixels is shortened and gets a mark at its end. An empty text hides the label. Send small letters and digits: the firmware's font has the letters at the small letters' codes and its own signs at the capitals'. Each call asks the heap for memory, so set a text only when it changes; setting the text a label already has changes nothing. | Ours: the display thread. |
| `0x8fc3668c` | `LabelBox label_bounds(void* label)` | The box of a label's text as it was last laid out: left, right, top and bottom, each a number with ten bits behind the point. It is the box of the shapes without their stroke, so an upright stroke is 0 wide. The box comes back as a structure, by value: bind the function as one that returns the structure, and the compiler hands over the address for it in P0. | Ours: the display thread. |

## Keys and the screen that is shown

| Address | The call | What the caller hands over and gets back | Called from |
| --- | --- | --- | --- |
| `0x8c0b1d8c` | `void request_screen(void)` | Asks the firmware to show the screen the present mode wants now. The firmware does this itself after every key; our code calls it after a change that no key press follows, such as a timer running out. It takes no argument. From the display thread, call it with interrupts masked for the length of the call: the firmware stops when two threads are inside it at once. | Ours: the key thread, and the display thread with interrupts masked. |
| `0x8fc231b4` | `int shift(void)` | Not 0 while the shift key is held. | Ours: the key thread, the display thread. |

## Bound in the sources, not described yet

Each of these is called by the files named. The type is what our code hands over and takes back; the name is the sources' own. Read the file's comments at the binding before you use one, and add its words to `profiles/op1f-1.7.0-interfaces.json` when you have.

| Address | The call | Bound in |
| --- | --- | --- |
| `0x040201c4` | `void* memcpy(void* destination, const void* source, u32 count)` | `build_b_storage.cpp` |
| `0x04020218` | `void* memmove(void* destination, const void* source, u32 count)` | `build_b_storage.cpp` |
| `0x04020fd8` | `i32 float_to_int(u32 a)` | `build_e_screens.cpp`, `build_g_duck.cpp` |
| `0x04021310` | `u32 rom_float_to_int(u32)` | `build_b_mix.cpp` |
| `0x04021854` | `u32 float_from_int(i32 value)` | `build_b_mixer.cpp`, `build_e_screens.cpp`, `build_k_clip.cpp`, `build_k_clips.cpp` |
| `0x040218a0` | `u32 rom_value_to_float(u32)` | `build_b_mix.cpp` |
| `0x040218e0` | `u32 rom_float_sub(u32, u32)` | `build_b_mix.cpp` |
| `0x040218e2` | `u32 rom_float_add(u32 a, u32 b)` | `build_b_mix.cpp`, `build_e_screens.cpp`, `build_g_duck.cpp` |
| `0x040219e4` | `u32 rom_float_div(u32 a, u32 b)` | `build_b_mix.cpp`, `build_b_mixer.cpp`, `build_e_screens.cpp` |
| `0x04021ac0` | `u32 rom_float_mul(u32 a, u32 b)` | `build_b_mix.cpp`, `build_e_screens.cpp`, `build_g_duck.cpp` |
| `0x04021bdc` | `u32 rom_half_to_float(u32)` | `build_b_mix.cpp` |
| `0x04022ea8` | `void* memset(void* destination, int value, u32 count)` | `build_b_edit.cpp`, `build_b_storage.cpp` |
| `0x8c051f58` | `int rename(const char* from, const char* to)` | `build_m3_folders.cpp` |
| `0x8c052024` | `int is_folder(const char* path)` | `build_m3_folders.cpp` |
| `0x8c052714` | `void string_clear(void* string)` | `build_d_clip.cpp` |
| `0x8c053a20` | `void assert(const char* message)` | `build_m4_loader.cpp` |
| `0x8c0567c0` | `u32 kb_request(void* link, u32 type, const void* payload, u32 bytes, void* answer, u32 timeout_ms)` | `build_m4_loader.cpp` |
| `0x8c0828b0` | `int json_read_raw(void* a, void* path_string, void* json, int flag)` | `build_i_patch.cpp` |
| `0x8c08434c` | `void json_from_float(void* json, u32 float_bits)` | `build_k_clip.cpp` |
| `0x8c08445c` | `void json_from_string(void* json, const void* string)` | `build_k_clip.cpp` |
| `0x8c084752` | `void json_to_string(void* out_string, void* json)` | `build_h_chain.cpp`, `build_i_lfopresets.cpp` |
| `0x8c0b6404` | `int drum_screen_id(void* self)` | `build_h_train.cpp` |
| `0x8c0bc4e8` | `void mode_switch(int mode)` | `build_g_duck.cpp` |
| `0x8c0c1420` | `void* tape_model(void)` | `build_b_mix.cpp`, `build_b_record.cpp`, `build_g_duck.cpp` |
| `0x8c0ce984` | `int synth_key_up(void* self, int key)` | `build_j5i_lfo_alone.cpp` |
| `0x8c0cef50` | `int synth_key_shift(void* self, const int* event)` | `build_j5i_lfo_alone.cpp` |
| `0x8c0cf4cc` | `int synth_key_down(void* self, const int* event)` | `build_i_lfo4.cpp`, `build_j5i_lfo_alone.cpp` |
| `0x8c0d4fbc` | `int synth_screen_id(void* self)` | `build_h_train.cpp` |
| `0x8c0d50a8` | `int synth_encoder(void* self, int index, int delta)` | `build_j5i_lfo_alone.cpp` |
| `0x8c0dc764` | `void draw_char(u16* framebuffer, int character)` | `build_m1_measure.cpp` |
| `0x8d4e37bc` | `void* alloc_aligned(u32 alignment, u32 bytes)` | `build_k_copy.cpp` |
| `0x8d4e383c` | `void free_aligned(void* block)` | `build_k_copy.cpp` |
| `0x8d4e5854` | `void* heap_alloc(u32 bytes)` | `build_m1_loader_disk.cpp`, `build_m3_disk.cpp`, `build_m3_folders.cpp` |
| `0x8d4e5864` | `void free(void* block)` | `build_h_chain.cpp`, `build_i_lfopresets.cpp`, `build_k_clip.cpp`, `build_k_clips.cpp` and 1 more |
| `0x8d4e8bbc` | `void patch_apply(void* descriptor)` | `build_h_chain.cpp` |
| `0x8d4ecb7c` | `i32 clips_idle(void)` | `build_b_record.cpp`, `build_g_duck.cpp` |
| `0x8d4ecc80` | `void clip_fill(i32* record, i32 track, i32 start, i32 stop, const i32* paddings)` | `build_k_clips.cpp` |
| `0x8d4ecddc` | `void* clips(void)` | `build_d_clip.cpp` |
| `0x8d4ecebe` | `void* clip_record(void* clips, i32 id, void* record)` | `build_d_clip.cpp` |
| `0x8d4ed438` | `i32 clip_at(u32 track, i32 position)` | `build_d_clip.cpp` |
| `0x8d4edb90` | `void clips_relink(void* store)` | `build_k_clips.cpp` |
| `0x8d4ede24` | `void clip_set(void* store, i32 id, const i32* record, i32 to_document)` | `build_k_clips.cpp` |
| `0x8d4edf1c` | `void clips_load(void* store)` | `build_k_clips.cpp` |
| `0x8d4ee398` | `void clip_remove(void* store, i32 id)` | `build_k_clips.cpp` |
| `0x8d4f12d8` | `int db_clear(int db, const char* key)` | `build_k_clips.cpp` |
| `0x8d4f13bc` | `void db_set(int db, const char* key, void* value)` | `build_b_mixer.cpp`, `build_g_duck.cpp`, `build_j5g_duck_alone.cpp`, `build_k_clips.cpp` and 1 more |
| `0x8d4f1a60` | `void* tape_doc(void)` | `build_b_mixer.cpp`, `build_g_duck.cpp`, `build_j5g_duck_alone.cpp`, `build_k_clips.cpp` |
| `0x8d4f2934` | `void lfo_pick(void* record, int id)` | `build_i_lfo4.cpp` |
| `0x8d4f2a7c` | `void set_params_doc(void* record, const u32* words)` | `build_h_chain.cpp` |
| `0x8d4f2b04` | `void set_active_doc(void* record, int value)` | `build_h_chain.cpp` |
| `0x8d4f2b64` | `void set_lfo_words_doc(void* record, const u32* words)` | `build_i_lfo4.cpp` |
| `0x8d4f2bec` | `void set_lfo_active_doc(void* record, int value)` | `build_i_lfo4.cpp` |
| `0x8d4f3d0c` | `void apply_drum(void* descriptor)` | `build_h_chain.cpp` |
| `0x8d4f523c` | `void apply_synth(void* descriptor)` | `build_h_chain.cpp` |
| `0x8d4fa7b8` | `void clone(int source, int destination, ProgressFn progress, u32 fourth)` | `build_k_copy.cpp` |
| `0x8d4ff2d8` | `void close_partition(i32 handle)` | `build_b_edit.cpp`, `build_k_copy.cpp` |
| `0x8d4ff528` | `i32 partition_read(i32 handle, u32 byte_offset, u32 length, void* buffer)` | `build_b_edit.cpp`, `build_b_storage.cpp`, `build_k_copy.cpp` |
| `0x8d4ffa28` | `i32 partition_write(i32 handle, u32 byte_offset, u32 length, void* buffer)` | `build_b_edit.cpp`, `build_b_storage.cpp`, `build_k_copy.cpp` |
| `0x8d4ffe34` | `void partition_sync(void)` | `build_b_edit.cpp` |
| `0x8d4ffea8` | `i32 open_partition(const char* name)` | `build_b_edit.cpp`, `build_k_copy.cpp` |
| `0x8d503148` | `void lfo_on_set(u32 slot, int value)` | `build_i_lfo4.cpp` |
| `0x8d5032d6` | `void reset_synth(u32 voice)` | `build_i_lfo4.cpp` |
| `0x8d503694` | `void page_copy(u32 slot, u32 type, int cls)` | `build_i_lfo4.cpp` |
| `0x8d5041d0` | `void build_producer(int index, int type)` | `build_h_chain.cpp` |
| `0x8d51117c` | `int random_param(int type, int index, int bias)` | `build_h_chain.cpp` |
| `0x8d515734` | `void* mixer_values(void)` | `build_b_mixer.cpp` |
| `0x8d515ad0` | `void* vu_values(void)` | `build_b_mixer.cpp`, `build_e_screens.cpp`, `build_j5g_duck_alone.cpp` |
| `0x8d51634c` | `const char* param_name(int kind, int index)` | `build_i_lfopage.cpp` |
| `0x8f0025fe` | `void sample_finalize(void* sample)` | `build_b_edit.cpp` |
| `0x8f004fe8` | `u32 db_to_amp(u32 db)` | `build_e_screens.cpp`, `build_g_duck.cpp` |
| `0x8f005014` | `u32 vu_curve(u32 x)` | `build_b_mixer.cpp`, `build_e_screens.cpp` |
| `0x8f008774` | `void vector_assign(void* destination, const void* source)` | `build_d_clip.cpp`, `build_k_clip.cpp` |
| `0x8f008c54` | `void write_clipboard_json(void)` | `build_d_clip.cpp` |
| `0x8f0093ec` | `void lift_worker(u32 track, i32 start, i32 end)` | `build_d_clip.cpp` |
| `0x8f009630` | `void drop_worker(const void* record, u32 source, u32 destination, i32 position, i32 end, i32 mix)` | `build_d_clip.cpp` |
| `0x8f009f70` | `void split(u32 track, i32 position)` | `build_d_clip.cpp` |
| `0x8f00b2d8` | `void clipboard_load(void)` | `build_k_clip.cpp` |
| `0x8f01c8fc` | `i32* track_output(int track)` | `build_b_record.cpp` |
| `0x8f01d144` | `int read_block(int speed, void* state, int rolling)` | `build_b_play.cpp`, `build_j5g_duck_alone.cpp` |
| `0x8f01d9f0` | `void mixer(int speed, void* state)` | `build_b_play.cpp` |
| `0x8f01e4a4` | `void set_position(i32 position)` | `build_d_clip.cpp` |
| `0x8fc1f780` | `Slot* slot_for_logical(int logical)` | `build_b_play.cpp`, `build_b_xfade.cpp`, `build_g_duck.cpp` |
| `0x8fc1fdf8` | `void invalidate_range(i32 start, i32 end)` | `build_b_edit.cpp` |
| `0x8fc1fe94` | `i32 tape_handle(void)` | `build_b_storage.cpp` |
| `0x8fc1fea8` | `int tape_index(void)` | `build_b_storage.cpp`, `build_k_clips.cpp`, `build_k_sums.cpp`, `build_l3_maps.cpp` |
| `0x8fc200ac` | `i32 tape_io_read(u32 track, u32 byte_offset, u32 length, void* buffer)` | `build_b_edit.cpp`, `build_b_storage.cpp` |
| `0x8fc2021c` | `i32 tape_io_write(u32 track, u32 byte_offset, u32 length, void* buffer)` | `build_b_edit.cpp`, `build_b_storage.cpp` |
| `0x8fc20578` | `void mark_map_dirty(void)` | `build_b_storage.cpp`, `build_k_sums.cpp` |
| `0x8fc209a8` | `void set_current_snapshot(i32 index)` | `build_b_storage.cpp` |
| `0x8fc20d14` | `i32 count_free(void)` | `build_b_storage.cpp` |
| `0x8fc20d48` | `u32 alloc_chunk(void)` | `build_b_storage.cpp` |
| `0x8fc20f24` | `i32 drop_oldest(void)` | `build_b_storage.cpp` |
| `0x8fc21000` | `i32 take_snapshot(void)` | `build_b_storage.cpp` |
| `0x8fc243cc` | `u8* colour_pointer(void* screen, const char* id, int optional)` | `build_e_screens.cpp` |
| `0x8fc2470c` | `u32* screens(void)` | `build_i_lfopage.cpp`, `build_m3_cards.cpp` |
| `0x8fc25b88` | `u32 draw(u32, u32, u32)` | `build_p3b_cost.cpp` |
| `0x8fc33b20` | `i32 meter_fixed(u32 amp)` | `build_e_screens.cpp`, `build_g_duck.cpp` |
| `0x8fc7ee58` | `void flash_track(int track)` | `build_b_mix.cpp` |
| `0x8fc7ee6c` | `void clear_region(void)` | `build_d_clip.cpp` |
| `0x8fc7ee9c` | `void lift_region(i32 track, i32 start, i32 end, i32 flag)` | `build_d_clip.cpp` |
| `0x8fca00e2` | `void notify(void* object)` | `build_b_mix.cpp`, `build_b_mixer.cpp`, `build_b_record.cpp`, `build_g_duck.cpp` |
| `0x8fca4b20` | `u32 tick(u32, u32, u32)` | `build_p3b_cost.cpp` |
| `0x8fca5ce8` | `int fx_type_of(void* string)` | `build_h_chain.cpp` |
| `0x8fcab080` | `void* root_child(void* behaviour, const char* name)` | `build_m1_loader_disk.cpp`, `build_m3_disk.cpp` |
| `0x8fcaccf0` | `void* make_folder(void* parent, const char* name)` | `build_m1_loader_disk.cpp`, `build_m3_disk.cpp`, `build_m3_folders.cpp` |
| `0x8fcacf48` | `void* add_file(void* folder, const char* name)` | `build_m1_loader_disk.cpp`, `build_m3_disk.cpp`, `build_m3_folders.cpp` |
| `0x8fcb8a8c` | `void close(int handle)` | `build_m3_folders.cpp` |
| `0x8fcb8aa0` | `int write(const void* data, u32 size, u32 count, int handle)` | `build_m3_folders.cpp` |
| `0x8fcb8b34` | `int read(void* buffer, u32 size, u32 count, int handle)` | `build_m3_folders.cpp` |
| `0x8fcb8b68` | `int open(const char* path, const char* mode)` | `build_m3_folders.cpp` |
| `0x8fcb8d98` | `void fs_file_ctor(void* obj, const char* path)` | `build_m1_loader_disk.cpp`, `build_m3_disk.cpp`, `build_m3_folders.cpp` |
| `0x8fcc58a0` | `void array_ctor(void* storage, int count, int size, void* ctor, void* dtor)` | `build_h_chain.cpp` |

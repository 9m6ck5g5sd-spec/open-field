// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R3: what a pack shows the base, and what the base offers a pack, at level 5. A sibling of
// module/build_r1_card.h, which stays as it was for the packs built against level 4: every word of levels 1 to 4
// stands here at the same place with the same meaning, and level 5 adds behind it.
//
// The words. The loader is our piece inside the firmware file: at the start it reads the pack files, places them
// and joins them by their names. The base is the shared pack, the file base.pack. A pack that wants something of
// the base carries a card: a small table in its own body that says what it wants. The pack file's header names the
// card's place in its last word; tools/pack_build.py writes the card's source from the pack's recipe (`card`) and
// the header's word (`build3`). The loader hands the base the cards of the packs that run, in the order of their
// files' names, and the base serves each pack through its card and knows none of them by name.
//
// Level 1: a call once a frame, pictures the pack carries for screens of the firmware, a picture of the pack's own,
// and six helpers, so that a pack with a card names no address of the firmware. A picture such a pack carries is a
// picture as it is, or instructions of either kind by which the unit makes it from its own pictures, up to eight of
// them in a set.
//
// Level 2: a page of the pack's own, with keys and knobs. Only one page is on the screen at a time, so the base
// lends the one it has: a pack makes a document from a picture of its own (op1f_picture), finds its parts by their
// names (op1f_node), opens and closes its page through the base (op1f_page_open, op1f_page_close) and dresses it
// from its frame function. A pack never names a screen's number. And every key and knob event is offered to the
// packs before the firmware's own code for the mode: first to the pack whose page is shown, then to every card that
// has an input function, in the order of the files' names, until one takes it. A taken event does not reach the
// firmware; one that nobody takes goes on as it always did. A pack picks its own keys, anywhere: a combination in a
// mode, or a key on another screen. Nobody keeps a list of them: where two packs pick the same keys, the pack of the
// earlier file gets them. The base offers a key to the packs before the mode's own code, where our own features
// stand, so a pack that takes a key one of them uses has it first. And while a pack's page is shown, every key and
// knob it does not take still goes to the mode's own code, which acts on the screen the page hides: in the mixer
// the other knobs move levels and pans nobody sees, over the effect train they move effects one hears. A pack that
// wants its page to own the knobs takes all four while its page is shown.
//
// Level 3: folders of the pack's own on the unit's disk, files read and written there through the base, each folder
// shown to the computer in the disk mode if the pack asks, and text on the pack's page.
//
//   Folders: none, one or several. Most packs need none: a pack whose card says no folder gets none, and nothing of
//   the disk is touched for it. A pack that keeps something (its settings, files the computer brings) lists its
//   folders in its card, up to eight. Each has
//     a name     what the computer shows in the disk mode, beside synth, drum, tapes, mixdown, packs, fx and lfo;
//     a place    where the folder lies on the unit's disk: user/pf/ and the place, a folder of its own;
//     shown      whether the computer sees it. A folder that is not shown is a pack's private folder (its settings).
//   tools/pack_build.py writes the list from the recipe and fills what the recipe leaves out: a folder's name is the
//   pack's own when the pack has one folder and does not name it; its place is its name; it is shown unless the
//   recipe says not. A pack names a folder to the helpers by its number in that list, from 0.
//   The base makes each folder at its first frame (user/pf/ and the place, with the slash behind: the folder maker
//   makes the part before each slash), and says one line a folder. A name or a place is one to fifteen characters
//   of A-Z, a-z, 0-9, space, '_', '-' and '.', not beginning with a space, a dot or a dash and not ending with a
//   space or a dot; the tool refuses any other and the base does not use it. Where two folders want one thing, the
//   earlier keeps it, in the order of the packs' file names and then of each pack's list, capitals counted as small
//   letters (the computer's disk cannot keep two names that differ only in capitals):
//     a name the firmware's (synth, drum, tapes, mixdown), the loader's (packs), the base's (fx, lfo) or an earlier
//       folder's: the folder is not shown, and its line says so; the pack still has it on the disk;
//     a place an earlier folder has: the folder is not the pack's, its line says so, and the helpers answer
//       OP1F_NO_FOLDER for it.
//   In the disk mode the computer sees a shown folder's files, every name the disk holds there but those beginning
//   with a dot and the folders inside it, which are not shown; it may read them, bring new ones and delete them
//   (a delete takes the file off the disk). The firmware builds what the computer sees once, when the disk mode
//   starts: a file a pack writes while the disk mode is open is seen at the next disk mode.
//
//   Files: op1f_list, op1f_read and op1f_write work on the files of a pack's own folder, by their names. A pack's
//   file is bytes: the base reads and writes them as they are. A write goes to a file of the base's in tmp/ first
//   and is then renamed over the pack's file, as the firmware writes its own files, so a file is the old one or the
//   new one, never half of each; and it is held by the file's size afterwards, since the firmware does not say when
//   a rename fails. Every write that fails, and the first ones that land, are said on the log by the pack's name.
//   While the disk mode is open (the frame's `disk`), the computer owns the folders: a write is refused with
//   OP1F_NOT_NOW, and a read sees what the disk holds. A pack that keeps its settings in memory reads its file again
//   when `disk` goes back to 0, since the computer may have brought a new one.
//
//   Text: op1f_text writes a text into a part of a document op1f_picture made, in the firmware's own letters, as the
//   firmware's lists show theirs. The part is a placeholder in the pack's picture: a group (<g id="...">) holding a
//   path with a stroke, whose top-left is where the text begins and whose stroke colour the letters take. The
//   letters are small letters only, the firmware's font having no capitals of its own: a capital is shown as its
//   small letter, and a character the font has not as a question mark.
//
// Level 4: what a page needs. Pages that stack: a page shown over another in the same mode lies over it, and the one
// under it shows again when it closes; a page of the base's own, black, for a pack that paints its page itself; text
// set left, centred or to the right, its width measured, and the text moved as a part; painting on the frame's
// picture with boxes, lines, blocks of pixels and text in the base's own small font; a file of a pack's folder
// removed; and, for an input function, the thread an event came on, and a key watched: a pack may see a key's down
// and up and still let the key go on to the others and to the firmware.
//
//   Painting. Each frame the screen is drawn whole, and then the base calls the packs' frame functions: the frame's
//   `canvas` is that frame's picture, already drawn. What a pack paints on it lies over everything for
//   that frame and is gone at the next, so a pack paints again at every frame. Only from the pack's own frame
//   function, with the `canvas` it was handed in that call: any other call paints nothing and is counted, and the
//   base says the count on the log by the pack's name. A pack that paints its whole page opens the base's blank page
//   (op1f_page_show with no document) and paints on it: the screen behind is black, every frame.
//
//   The asks. A card of level 4 may carry a list of things it asks of the base. Level 4 defines no kind of ask yet:
//   a later level adds kinds (a sequencer, an effect, an engine, a point of the audio path, a sound's file) with their
//   helpers, and never a field of the card. A base that does not know a kind uses the card and leaves that ask aside,
//   with one line.
//
// Level 5: sequencers, and notes heard. The card's list of asks gets its first two kinds; the card itself, and what a
// frame function and an input function are handed, stay as level 4 has them.
//
//   A sequencer. A pack adds a sequencer of its own to the unit's list: its ask hands over a name and up to five
//   functions, and the pack changes no place of the firmware. The base puts the name at the end of the list of
//   sequencers, for the synth and the drum or for the synth alone. While the owner has chosen it for an instrument
//   and switched it on, the base calls the pack at every pulse of the clock and for every key of the keyboard and
//   every MIDI note that goes down (and for the up of each one the pack took: a key it lets the instrument play is
//   ended by the instrument, and the pack is not told), plays the notes the pack asks for, ends the ones it left
//   sounding when the sequencer stops, and shows a page of the pack's in place of the sequencer's screen. The unit
//   keeps the owner's choice by the sequencer's name, and the base keeps it across a start without the pack. Several
//   packs may each add sequencers, sixteen in all; one is chosen for the synth and one for the drum at a time, as
//   with the firmware's own seven. Where two packs ask for one name, the earlier keeps it, in the order of the files'
//   names; the later pack still runs, without that sequencer.
//
//   Notes heard. Any pack may ask to be told every note the instrument is asked to begin and to end, whoever asks:
//   the keyboard, MIDI, a sequencer of the firmware's or of a pack's. It listens only: a pack that wants to change
//   what is played is a sequencer. What it hears are the asks, not the sound. The instrument also ends notes by
//   itself (when the unit changes instrument or a sound is loaded, when a seventh note takes a voice), and no call
//   comes for those; an end may come twice, or for a note that never began; and a begin comes also where the
//   instrument then plays nothing (a note already sounding, a sound being loaded). A pack that keeps a list of
//   sounding notes takes it as an upper bound, and clears it when the frame's `mode` changes.
//
// A pack that uses level 3 needs the name op1f_level_3, the mark of a base that offers it, a pack that uses level 4
// the name op1f_level_4, and a pack that uses level 5 the name op1f_level_5; the card's source names it, so nothing is
// written by hand. A base of level 5 offers op1f_level_1 to op1f_level_4 too: a pack built for an earlier level runs on
// it as it is. Where no pack offers the mark the loader leaves the pack out and says so.
//
// A card only grows. A later level adds fields behind the last one and raises `bytes`; a base reads the part it
// knows and takes a field the card is too short for as 0. What a frame function is handed grows the same way.
//
// The threads. A frame function runs on the display thread. An input function runs on the thread that handles keys,
// never on the display thread; for the keys of the keyboard (the ones that play notes) the firmware may hand a key
// in at once on the thread that brought it, which can be the audio thread. So an input function does what the audio
// thread may do: it writes no line on the log, it waits for nothing, it calls nothing of the firmware's, and it comes
// back quickly. It counts, and the frame function says it. The two share single words, never a structure: either can
// find the other half way through its work. Level 3's files and text are the frame function's alone: the base
// answers OP1F_NOT_NOW (op1f_text: 0) to a call it gets while it is not calling that pack's frame function, and to
// a call from any other thread while it is (it knows the display thread by its stack), so that nothing of the disk
// and no text is touched from the thread that handles keys or from the audio thread. A file is
// read or written while the frame function waits: the picture stands still for as long as it takes, some
// milliseconds for a small file. Read a file once and keep what it says; write it when something changed, not at
// every frame. Level 4's text and painting are the frame function's too (the display thread), and so is
// op1f_remove; op1f_page_show is taken from the frame function and from the input function, as op1f_page_open is.
// From level 4 an input function is told the thread (the input's `thread`): every turn and every key outside 13 to
// 53 come on the firmware's key thread, where a handler may do what the mode's own code does there; the keys 13 to
// 53 (the keys that play notes, and shift among them) may come on the audio thread, and for them the rules above
// stand.
//
// Level 5's calls come on other threads still. A sequencer's `tick` and `octave` come on the firmware's sequencer
// thread; its `key` on the thread that brings the key (the key thread, the audio thread, a MIDI handler's); its `run`
// on the key thread, a MIDI handler's or the thread of the start; a `note` of the notes heard on whatever thread asks
// for the note. For all of these the rules of an input function stand: no line on the log, no wait, nothing of the
// firmware's, quickly back; they count, and the frame function says it. One of them may come while another, or the
// frame function, stands interrupted: what they share is single words, or anything inside a section of op1f_mask.
// `shown` and `playing` come on the display thread, outside the pack's frame function: they keep what they are told
// in single words and do nothing else (no painting, no text, no file, no page: those are the frame function's).
//
// Nothing here names an address of the firmware. The file is compiled for the unit and, with OP1F_HOST defined, by
// the PC's compiler for the tests. The PC's build must be one with addresses of four bytes, as the unit's are: the
// sizes and places below are held when the file is compiled, and a build they do not hold for stops.

#ifndef OP1F_BUILD_R3_CARD_H
#define OP1F_BUILD_R3_CARD_H

// The name a function or an object has in the linked pack. The module's compiler takes it as written here; the
// PC's compiler needs none, and the tests' stand-ins are found by their C names.
#ifdef OP1F_HOST
#define OP1F_NAMED(x)
#else
#define OP1F_NAMED(x) asm(x)
#endif

// OP1F_CARD_MARK is a card's first word. OP1F_CARD_LEVEL_1_BYTES, OP1F_CARD_LEVEL_2_BYTES and OP1F_CARD_LEVEL_3_BYTES
// are the sizes of a card of level 1, 2 and 3. OP1F_CARDS_MOST is how many cards the base's list holds: as many as
// the loader takes pack files. OP1F_RELEASE_BYTES is the room of the release text a base carries under the name
// op1f_release: the text and zeros behind it. OP1F_FOLDERS_MOST is how many folders a card may list,
// OP1F_FOLDER_TEXT the room of a folder's name and of its place (fifteen characters and the end), OP1F_FILE_NAME
// the room of a file's name in a pack's folder (thirty-nine characters and the end).
enum { OP1F_CARD_MARK = 0x44524143 /* "CARD" */, OP1F_CARD_LEVEL_1_BYTES = 44, OP1F_CARD_LEVEL_2_BYTES = 48, OP1F_CARD_LEVEL_3_BYTES = 56, OP1F_CARDS_MOST = 32,
       OP1F_RELEASE_BYTES = 16, OP1F_FOLDERS_MOST = 8, OP1F_FOLDER_TEXT = 16, OP1F_FILE_NAME = 40 };

// Level 4. OP1F_CARD_LEVEL_4_BYTES is the size of a card of level 4; OP1F_ASKS_MOST is how many asks a card may list,
// OP1F_ASK_BYTES_MOST the most bytes one ask's table may have.
enum { OP1F_CARD_LEVEL_4_BYTES = 64, OP1F_ASKS_MOST = 16, OP1F_ASK_BYTES_MOST = 4096 };

// Level 5. A card of level 5 is a card of level 4's size: OP1F_CARD_LEVEL_5_BYTES. OP1F_SEQUENCERS_MOST is how many
// sequencers the packs may add in all, OP1F_NOTES_MOST how many asks may hear the notes, OP1F_KIND_NAME the room of an
// ask's name (fifteen characters and the end).
enum { OP1F_CARD_LEVEL_5_BYTES = 64, OP1F_SEQUENCERS_MOST = 16, OP1F_NOTES_MOST = 8, OP1F_KIND_NAME = 16 };

// What level 3's helpers answer when they do not do what was asked (each is less than 0):
//   OP1F_NO_FILE     op1f_read, op1f_remove: the folder holds no file of that name
//   OP1F_TOO_LARGE   op1f_read: the file is larger than the room handed in (nothing is read); op1f_path: the room
//                    handed in is too small for the path
//   OP1F_NOT_NOW     the call did not come from this pack's frame function, or (op1f_write, op1f_remove) the disk mode
//                    is open
//   OP1F_NO_FOLDER   the card is not one the base has in use, or the folder's number is not one of its folders the
//                    base made
//   OP1F_BAD_NAME    a file's name is empty, longer than thirty-nine characters, holds a slash or a character that
//                    is not plain, or begins with a dot
//   OP1F_FAILED      the disk did not do it: the line on the log says which step
enum { OP1F_NO_FILE = -1, OP1F_TOO_LARGE = -2, OP1F_NOT_NOW = -3, OP1F_NO_FOLDER = -4, OP1F_BAD_NAME = -5, OP1F_FAILED = -6 };

struct Op1fCard;

// What a card's frame function is handed, the same for every card of a frame.
//   bytes    the size of what is handed: 28 at level 1, 32 at level 2, 36 at level 3, 40 at level 4. A later base
//            may hand more; read what you know
//   frame    the number of the frame, counted by the base
//   ms       milliseconds since the base's first frame: what op1f_ms answers. It moves once a frame
//   mode     the mode the unit is in, by the firmware's own number: 0 synth, 1 drum, 2 tape, 3 mixer. A pack with a
//            sequencer (level 5) also sees 7 here: the sequencer's screen is up
//   screen   the number of the screen that is shown
//   shift    not 0 while shift is held
//   popup    not 0 while the power popup is on the screen: the last moment to say something on the log
//   page     level 2: the card whose page the GUI shows now, 0 when it shows none. Compare it with your own card's
//            address: it is yours when they are the same. The base takes a page asked for at its next frame and the
//            GUI shows it at the frame after, from which on `page` names it; it stops when the page is closed, when
//            the mode changes, or when the firmware's own page of the base (the effect train, the LFOs) is opened
//            over it. Level 4: while a page shown with op1f_page_show lies over another, `page` names the one on top
//   disk     level 3: not 0 while the disk mode is open (the COM key, the computer connected): the computer owns the
//            folders and op1f_write answers OP1F_NOT_NOW. When it goes back to 0 the computer may have brought,
//            changed or deleted files in a shown folder
//   canvas   level 4: this frame's picture as the display will show it, already drawn, for the op1f_paint_ helpers,
//            which take it; 0 when the base offers none. It lives for the call only: never keep it, and never write
//            it yourself (its layout is the base's to know). What a pack paints lies over everything for this frame
//            and is gone at the next: paint again at every frame
struct Op1fFrame { unsigned bytes, frame, ms, mode, screen, shift, popup; const Op1fCard* page; unsigned disk; void* canvas; };   // 40 bytes at level 4

// What happened, for an input function (level 2).
//   OP1F_DOWN        a key went down
//   OP1F_DOWN_SHIFT  a key went down while shift was held. The firmware hands these on apart, and so does the base
//   OP1F_UP          a key came up. An up goes where its down went: to the pack that took the down, whatever its
//                    input function answers, and never to another; to the firmware where the firmware had the down.
//                    So a pack sees the up of every key whose down it took, and of no other
//   OP1F_TURN        a knob was turned
enum { OP1F_DOWN = 1, OP1F_DOWN_SHIFT = 2, OP1F_UP = 3, OP1F_TURN = 4 };

// What an input function answers, besides 0 (let the event go on) and any other number (take it):
//   OP1F_SEEN        level 4: the event goes on to the other packs and to the firmware as if the answer had been 0,
//                    and the pack is handed that key's up too, before the up goes where its down went. For a turn it
//                    is the same as 0. A base of level 3 or earlier takes it as any answer not 0: the event is taken.
//                    So a pack that answers it needs op1f_level_4
enum { OP1F_SEEN = 2 };

// The thread an event came on (the input's `thread`, level 4).
//   OP1F_KEY_THREAD  the firmware's key thread: every turn, and every key outside 13 to 53 (the keys under the
//                    screen, the mode keys, the help and metronome keys, the knobs' presses and clicks). There a
//                    handler may do what the mode's own code does there
//   0                the keys 13 to 53: the keys that play notes, and shift (38) among them. They may come on the
//                    audio thread: a handler of such a key does only what the audio thread may do (see "The threads"
//                    above)
enum { OP1F_KEY_THREAD = 1 };

// What an input function is handed (level 2), one event at a time.
//   bytes    the size of what is handed: 36 at level 2, 40 at level 4. A later base may hand more; read what you know
//   what     OP1F_DOWN, OP1F_DOWN_SHIFT, OP1F_UP or OP1F_TURN
//   key      the key's number as the firmware counts its keys (0 the help key, 1 the metronome key, 2 to 5 the
//            synth, drum, tape and mixer keys, 6 to 9 the four keys under the screen, 38 shift, 54 to 57 the knobs'
//            clicks, 61 to 64 the knobs pressed down and let go); for a turn, the knob: 0 blue, 1 the second, 2 the
//            third, 3 red. The firmware makes a click (54 to 57, a down and then an up) when a knob that was pressed
//            comes up without having been turned
//   turn     for a turn: 1024 a step of the knob, more than 0 one way and less the other, as the firmware's own pages
//            take it: a value goes up for more than 0. 0 for a key
//   mode     the mode the unit is in: 0 synth, 1 drum, 2 tape, 3 mixer; and, as in the frame, 7 while the sequencer's
//            screen is up
//   screen   the number of the screen that is shown
//   shift    not 0 while shift is held
//   mine     not 0 when this pack's page is the one the base shows
//   record   the firmware's own record of a key that went down, sixteen bytes: the key's number, the value (127, or
//            how hard a key of the keyboard was struck), and two words of the clock at the press. 0 for an up and a
//            turn, which the firmware hands on as numbers. It lives only for the call (for a knob's click it stands
//            in the firmware's own stack): copy what you need, never keep the pointer
//   thread   level 4: OP1F_KEY_THREAD or 0, by the key's number, as above
struct Op1fInput { unsigned bytes, what, key; int turn; unsigned mode, screen, shift, mine; const unsigned* record; unsigned thread; };   // 40 bytes at level 4

// One of a pack's folders (level 3), constant data in the pack's body, written by tools/pack_build.py from the
// recipe. Each text is zero padded and ended within its sixteen bytes.
//   name     what the computer shows in the disk mode
//   place    where the folder lies on the unit's disk, inside user/pf/
//   shown    not 0 for a folder the computer sees in the disk mode
struct Op1fFolder { char name[OP1F_FOLDER_TEXT]; char place[OP1F_FOLDER_TEXT]; unsigned shown; };   // 36 bytes

// One thing a pack asks of the base (level 4), constant data in the pack's body, written by tools/pack_build.py from
// the recipe's `asks`. Level 4 defines no kind; level 5 defines two, a sequencer and the notes heard (below); a later
// level adds kinds (an effect, an engine, a point of the audio path, a sound's file), and their helpers, and never a
// field of the card.
//   kind     what is asked; 0 is no kind
//   bytes    the size of what `what` points at, at most OP1F_ASK_BYTES_MOST
//   what     a table of that kind's form in the pack's body, or 0
struct Op1fAsk { unsigned kind, bytes; const void* what; };   // 12 bytes

// A pack's card: constant data in the pack's body, written by tools/pack_build.py from the recipe under the name
// op1f_card_ and the pack's name in small letters, digits and underscores (op1f_card_wink). Its addresses are the
// pack's own and move with it; each is 0 where the pack has no such thing.
//   frame     called once a frame on the display thread, in the order of the packs' file names, before the base
//             dresses its own screens. It is handed the frame (above) and answers nothing. It must come back
//             quickly, every time: the picture stands still for as long as it runs. The base times every call and
//             prints the time on the log by the pack's name. A function that never comes back stops the display
//             for good, and no line names the pack. A function written in assembly keeps the registers the
//             calling convention says a function keeps
//   pictures  the pack's directory of pictures (tools/pack_build.py `pictures`): pictures for screens of the
//             firmware, by the file names the firmware asks for, and the pack's own. The packs the base knows by name
//             are asked first, then the cards in their order: the first pack that has a name serves it. A name the
//             firmware never asks for is the pack's alone. No picture may be larger than a megabyte: the base does
//             not use a card whose directory holds one
//   pick      asked before the pack's directory is looked into: handed the name a screen asks for, it answers the
//             name to look for in this pack's directory, or 0 for nothing of this pack's now. With no such function
//             the directory is looked into by the name itself. It is asked every time the firmware asks for a
//             picture: while the firmware makes its screens at the start, before the first frame, and again later,
//             when a tape is loaded and when a page of the base is opened. A later call is not always on the
//             display thread, and may come while the pack's own frame function stands interrupted. So it only
//             reads; what it shares with the frame function is single words, never a structure, which it could
//             find half written; and it takes no time
//   input     level 2, read only from a card that says level 2 or later: offered every key and knob event, on the
//             thread that handles keys (see "The threads" above),
//             before the firmware's code for the mode. It answers not 0 to take the event: then the firmware never
//             sees it, and no pack after this one is asked. It answers 0 to let it go on. It is asked first when the
//             pack's page is the one shown, else in the order of the files' names. An up is not offered: it goes where
//             its down went. Level 4, from a card that says level 4 or later and is of level 4's size: it may answer
//             OP1F_SEEN (above) to see a key and let it go on
//   folders   level 3, read only from a card that says level 3 or later and is of level 3's size: the pack's list of
//             folders (above), 0 for none
//   folder_count  level 3: how many folders the list holds, 0 to OP1F_FOLDERS_MOST; 0 for none
//   asks      level 4, read only from a card that says level 4 or later and is of level 4's size: the pack's list of
//             asks (above), 0 for none. The base does not use a card whose list is not in order (more than
//             OP1F_ASKS_MOST, a count with no list or a list with no count) or does not lie in its pack (the list,
//             or an ask's table with its bytes, outside the pack's body, or not on a word). Level 5: an ask whose
//             table lies in its place but is not in order for its kind (its size, its name, a flag the base does not
//             know, a function that is not the pack's) is left aside with a line, and the card is used
//   ask_count level 4: how many asks the list holds, 0 to OP1F_ASKS_MOST; 0 for none
struct Op1fCard {
    unsigned mark, bytes, level, version;        // the mark; 44 at level 1, 48 at level 2, 56 at level 3, 64 at levels 4 and 5; the highest level used; the pack's version
    char name[16];                               // the pack's name, zero padded
    void (*frame)(const Op1fFrame* now);         // +32: once a frame, on the display thread; 0 for none
    const void* pictures;                        // +36: the pack's directory of pictures; 0 for none
    const char* (*pick)(const char* name);       // +40: which of its pictures a name stands for now; 0: the name itself
    int (*input)(const Op1fInput* event);        // +44, level 2: a key or a knob; not 0 when the pack took it; 0 for none
    const Op1fFolder* folders;                   // +48, level 3: the pack's folders; 0 for none
    unsigned folder_count;                       // +52, level 3: how many
    const Op1fAsk* asks;                         // +56, level 4: the pack's asks; 0 for none
    unsigned ask_count;                          // +60, level 4: how many, 0 to OP1F_ASKS_MOST
};

// The list the base offers under the name op1f_cards, in its zeroed data, and the loader fills once the packs are
// joined: one slot for each pack that runs and has a card, in the order of the files' names. `first` is the pack's
// first address and `end` the one behind its last (its body and its zeroed data), so that the base can hold every
// address of a card to the pack it came with. `count` is the slots filled; `more` is the cards that found no slot.
// A pack with a card has no business with the list: it is between the loader and the base.
struct Op1fCardSlot { const Op1fCard* card; unsigned first, end; };
struct Op1fCards { unsigned count, more; Op1fCardSlot slots[OP1F_CARDS_MOST]; };   // 392 bytes

// Level 5. The kinds of ask a base of level 5 knows.
//   OP1F_ASK_SEQUENCER  a sequencer: what `what` points at is an Op1fSequencer
//   OP1F_ASK_NOTES      the notes heard: what `what` points at is an Op1fNotes
// Kinds to come keep the same form: a head (below), then the functions the base calls for that kind.
enum { OP1F_ASK_SEQUENCER = 1, OP1F_ASK_NOTES = 2 };

// What `shown` is told (the head's function, below): bits, 0 when the page is not on the screen.
//   OP1F_SHOWN_UP       the thing's page is the one on the screen
//   OP1F_SHOWN_OFF      the thing is switched off (a sequencer: its instrument's sequencer is off)
//   OP1F_SHOWN_COVERED  the unit's own notice lies over the page (a sequencer: the firmware's "sequencer off"), so
//                       that a page need not paint one of its own
enum { OP1F_SHOWN_UP = 1, OP1F_SHOWN_OFF = 2, OP1F_SHOWN_COVERED = 4 };

// The head of every kind's table (level 5): constant data in the pack's body, like the card, written by
// tools/pack_build.py from the recipe's `asks`.
//   bytes  the table's size as the pack was built: a later level grows a kind's table behind, never inside
//   name   the thing's name: 1 to 15 characters of a-z, 0-9, '-' and '_', zero padded. For a sequencer it is what
//          the owner sees in the list of sequencers and what the unit saves as the choice; for a kind with nothing
//          to show, what the base's lines on the log call it
//   flags  the kind's flags; a bit the base does not know makes it leave the ask aside
//   shown  for a kind with a page: called on the display thread, outside the pack's frame function, at each frame
//          at which what it says changed: `how` is OP1F_SHOWN_UP and the bits beside it while the thing's page is
//          the one on the screen, 0 when it no longer is. `where` says which of the thing's places (a sequencer: its
//          instrument, 0 the synth, 1 the drum). It comes before the frame functions of the same frame: from the
//          frame at which it says OP1F_SHOWN_UP, the frame's `page` is the pack's card. It keeps what it is told and
//          does no more. 0 for none
struct Op1fKindHead { unsigned bytes; char name[OP1F_KIND_NAME]; unsigned flags; void (*shown)(unsigned where, unsigned how); };   // 28 bytes

// A sequencer's flags.
//   OP1F_SEQ_SYNTH_ONLY  not in the drum's list
//   OP1F_SEQ_ALL_MIDI    MIDI notes beyond the keyboard's two octaves come to `key` too. Without it such a note is
//                        dropped while this sequencer is on, as with those of the firmware's own that do not take them
enum { OP1F_SEQ_SYNTH_ONLY = 1, OP1F_SEQ_ALL_MIDI = 2 };

// Which clock leads (the tick's `clock`): the unit's own, the tape's while it plays, or one from outside over MIDI.
enum { OP1F_CLOCK_OWN = 0, OP1F_CLOCK_TAPE = 1, OP1F_CLOCK_MIDI = 2 };

// What `tick` is handed, once a pulse of the clock, 96 pulses a quarter note.
//   bytes    the size of what is handed: 32 at level 5. A later base may hand more; read what you know
//   host     0 the synth, 1 the drum: the instrument the unit is on, the only one whose sequencer ticks
//   pulse    the firmware's own count of pulses, 0 to 47
//   count    the base's count of this sequencer's ticks since it was switched on or last stopped, from 0. It is not
//            set back when the firmware's count is
//   tempo    beats a minute in hundredths (12000 is 120)
//   us       microseconds a pulse at that tempo
//   clock    OP1F_CLOCK_OWN, OP1F_CLOCK_TAPE or OP1F_CLOCK_MIDI
//   jumped   not 0 when the firmware set its count back since the last tick (the tape started, a key restarted it)
struct Op1fTick { unsigned bytes, host, pulse, count, tempo, us, clock, jumped; };   // 32 bytes

// Where a key came from (the key's `from`).
enum { OP1F_FROM_KEYS = 0, OP1F_FROM_MIDI = 1 };

// What `key` is handed: a key of the keyboard or a MIDI note, before the instrument plays it: every down, and the up
// of each down this sequencer took. Where its `key` answered 0 for a down, the instrument plays the note and ends it
// itself when the key comes up, and the sequencer is not told of that up: a sequencer that wants to know how long a
// key is held takes the key and plays the note itself (op1f_note_on at the down, op1f_note_off at the up), as the
// firmware's own sequencers do; or it lets the key play and asks to hear the notes as well.
//   bytes     the size of what is handed: 32 at level 5
//   what      OP1F_DOWN, or OP1F_UP for a key or note whose down it took
//   host      0 the synth, 1 the drum
//   key       the keyboard's key, 0 to 23 from the left; for a MIDI note, the note - 53 (with OP1F_SEQ_ALL_MIDI it
//             may be below 0 or above 23). An up is matched to its down by `key`: the octave may have moved between
//             the two, and `note` with it
//   note      the note the instrument would play for it now, the octave counted, 0 to 127
//   velocity  1 to 127 for a down, 0 for an up
//   shift     not 0 when the key went down while shift was held; 0 for an up
//   from      OP1F_FROM_KEYS or OP1F_FROM_MIDI
struct Op1fKey { unsigned bytes, what, host; int key; unsigned note, velocity, shift, from; };   // 32 bytes

// `run`'s `how`.
//   OP1F_RUN_ON    the sequencer was switched on
//   OP1F_RUN_STOP  stop sounding now: it was switched off, another sequencer was chosen for its instrument, or the
//                  unit changes instrument. A STOP may come twice, or with nothing sounding. Once the pack's `run`
//                  has come back the base ends every note the pack left sounding. A sequencer that is still on is
//                  ticked again when its instrument is the unit's again
enum { OP1F_RUN_STOP = 0, OP1F_RUN_ON = 1 };

// A sequencer: what its ask points at. `tick` or `key` must be there; the other three may be 0. What a sequencer
// keeps (its settings, its patterns) is its own, in the pack's folders.
//   tick     called each pulse while the sequencer is on, its instrument is the one the unit is on and no sound is
//            being loaded
//   key      called for a key of the keyboard or a MIDI note while the sequencer is on. For a down it answers not 0 to
//            take the key: the instrument does not play it, and the key's up comes to `key` too. It answers 0 to let
//            the instrument play it as usual: then no up comes for it (above). What it answers for an up is not used
//   run      called when the sequencer is switched on and when it must stop sounding (above)
//   octave   called at the pulse after an octave key, while on, with the steps the octave moved. It answers not 0 (or
//            is not there) to have the notes of the keys held on the keyboard follow the move now, 0 to leave them.
//            The notes the pack began never move by themselves. The octave keys are 36 and 37 as the firmware counts
//            its keys: a pack that wants to see them go down itself watches them from its input function (OP1F_SEEN)
//   playing  asked when the unit draws its "playing" mark: not 0 shows it. Not there: 1
struct Op1fSequencer {
    Op1fKindHead head;                                // +0
    void (*tick)(const Op1fTick* now);                // +28
    int  (*key)(const Op1fKey* event);                // +32
    void (*run)(unsigned host, unsigned how);         // +36
    int  (*octave)(unsigned host, int steps);         // +40
    int  (*playing)(unsigned host);                   // +44
};   // 48 bytes at level 5

// What a pack that hears the notes is told (the note's `what`).
enum { OP1F_NOTE_ENDS = 0, OP1F_NOTE_BEGINS = 1 };

// A note's flags: for op1f_note_on and op1f_note_off, and in what a pack that hears the notes is handed.
//   OP1F_NOTE_NO_MIDI  not sent out over MIDI as well
enum { OP1F_NOTE_NO_MIDI = 1 };

// What a pack that hears the notes is handed, one note at a time, as the instrument is asked and before it plays.
//   bytes     the size of what is handed: 24 at level 5
//   what      OP1F_NOTE_BEGINS or OP1F_NOTE_ENDS
//   host      0 the synth, 1 the drum: the instrument the unit is on, which is asked. Another number while the unit
//             is on neither: the note then goes out over MIDI only
//   note      0 to 127
//   velocity  1 to 127 for a note that begins, 0 for one that ends
//   flags     OP1F_NOTE_NO_MIDI when the note is not sent out over MIDI as well
struct Op1fNote { unsigned bytes, what, host, note, velocity, flags; };   // 24 bytes

// The notes heard: what its ask points at. The head's `flags` is 0 and its `shown` is 0: there is nothing to show.
//   note     called for each note, on whatever thread asks for it. It answers nothing and changes nothing: it may
//            not begin or end a note itself
struct Op1fNotes {
    Op1fKindHead head;                                // +0
    void (*note)(const Op1fNote* event);              // +28
};   // 32 bytes at level 5

#ifdef __GNUC__
#define OP1F_PLACE_IN(type, field) __builtin_offsetof(type, field)
#else
#include <stddef.h>
#define OP1F_PLACE_IN(type, field) offsetof(type, field)
#endif
static_assert(sizeof(unsigned) == 4 && sizeof(void*) == 4, "a card is laid out for words and addresses of four bytes, as the unit's are");
// Level 3's sizes of what a frame function is handed and of a card are held by the places of level 4's words behind
// them: `canvas` at +36 is level 3's 36 bytes, `asks` at +56 a card of level 3's 56.
static_assert(sizeof(Op1fFrame) == 40, "what a frame function is handed is 40 bytes at level 4");
static_assert(OP1F_PLACE_IN(Op1fFrame, popup) == 24 && OP1F_PLACE_IN(Op1fFrame, page) == 28 && OP1F_PLACE_IN(Op1fFrame, disk) == 32,
              "levels 1 and 2's 32 bytes stand as they were, and the disk behind them");
static_assert(OP1F_PLACE_IN(Op1fFrame, canvas) == 36, "level 3's 36 bytes stand as they were, and the canvas behind them");
// Level 2's size of what an input function is handed is held by the place of level 4's `thread` behind it.
static_assert(sizeof(Op1fInput) == 40 && OP1F_PLACE_IN(Op1fInput, record) == 32, "what an input function is handed is 40 bytes at level 4");
static_assert(OP1F_PLACE_IN(Op1fInput, thread) == 36, "level 2's 36 bytes stand as they were, and the thread behind them");
static_assert(sizeof(Op1fFolder) == 36, "a folder of a card's list is 36 bytes");
static_assert(sizeof(Op1fAsk) == 12, "an ask is 12 bytes");
static_assert(sizeof(Op1fCard) == OP1F_CARD_LEVEL_4_BYTES, "a card of level 4 is 64 bytes");
static_assert(OP1F_PLACE_IN(Op1fCard, frame) == 32, "a card's frame function stands at +32");
static_assert(OP1F_PLACE_IN(Op1fCard, pictures) == 36, "a card's directory of pictures stands at +36");
static_assert(OP1F_PLACE_IN(Op1fCard, pick) == 40, "a card's choice among its pictures stands at +40");
static_assert(OP1F_PLACE_IN(Op1fCard, input) == OP1F_CARD_LEVEL_1_BYTES, "a card's input function stands at +44, behind level 1");
static_assert(OP1F_PLACE_IN(Op1fCard, folders) == OP1F_CARD_LEVEL_2_BYTES && OP1F_PLACE_IN(Op1fCard, folder_count) == 52, "a card's folders stand at +48, behind level 2");
static_assert(OP1F_PLACE_IN(Op1fCard, asks) == OP1F_CARD_LEVEL_3_BYTES && OP1F_PLACE_IN(Op1fCard, ask_count) == 60, "a card's asks stand at +56, behind level 3");
static_assert(sizeof(Op1fCardSlot) == 12, "a slot of the list of cards is 12 bytes");
static_assert(sizeof(Op1fCards) == 8 + 12 * OP1F_CARDS_MOST && sizeof(Op1fCards) == 392, "the list of cards is 392 bytes");
static_assert(sizeof(Op1fCard) == OP1F_CARD_LEVEL_5_BYTES, "a card of level 5 is a card of level 4's size, 64 bytes");
static_assert(sizeof(Op1fKindHead) == 28 && OP1F_PLACE_IN(Op1fKindHead, name) == 4 && OP1F_PLACE_IN(Op1fKindHead, flags) == 20 && OP1F_PLACE_IN(Op1fKindHead, shown) == 24,
              "the head of an ask's table is 28 bytes");
static_assert(sizeof(Op1fTick) == 32 && sizeof(Op1fKey) == 32, "a tick and a key are 32 bytes each at level 5");
static_assert(sizeof(Op1fSequencer) == 48 && OP1F_PLACE_IN(Op1fSequencer, tick) == 28 && OP1F_PLACE_IN(Op1fSequencer, playing) == 44,
              "a sequencer's table is 48 bytes at level 5, its functions behind the head");
static_assert(sizeof(Op1fNote) == 24, "a note heard is 24 bytes at level 5");
static_assert(sizeof(Op1fNotes) == 32 && OP1F_PLACE_IN(Op1fNotes, note) == 28, "the table of the notes heard is 32 bytes at level 5");

// op1f_text_set's alignment (level 4): the text begins at the placeholder's left (OP1F_LEFT), is centred on it
// (OP1F_CENTRE), or ends at its right (OP1F_RIGHT).
enum { OP1F_LEFT = 0, OP1F_CENTRE = 1, OP1F_RIGHT = 2 };

// A colour for the op1f_paint_ helpers, from its red, green and blue, each 0 to 255: the display's sixteen bits a
// pixel, five of red, six of green, five of blue. The unit's own colours are the field palette's; its knobs, for
// instance, blue OP1F_COLOUR(0x3c, 0x60, 0x87), ochre OP1F_COLOUR(0x8a, 0x65, 0x3c), grey OP1F_COLOUR(0xa2, 0xa4, 0xa6)
// and red OP1F_COLOUR(0xe5, 0x28, 0x17): take them, and their light and dark ones, from the unit's own palette.svg.
#define OP1F_COLOUR(r, g, b) ((((unsigned)(r) & 0xf8u) << 8) | (((unsigned)(g) & 0xfcu) << 3) | (((unsigned)(b) & 0xffu) >> 3))

// What the base offers a pack with a card. Each is a name the pack needs once it calls it, and the loader joins it
// to the base's function. Every one of these names sorts behind op1f_level_1 to op1f_level_5, and the loader looks
// for a pack's needed names in their order: a pack whose base is too old is left out for the mark, which says what is
// missing, not for a helper.

// A line on the unit's log. `level`: 1 an error, 2 a warning, 3 for information. `file`: a source path; the log
// shows the name between its last slash and its dot in front of the line, and a bare word shows nothing. `line`: a
// number shown with that name. `format` and what follows it: as printf takes them. Answers nothing. From the frame
// function, never from an input function: an input function may run on the audio thread. A line every frame fills
// the log.
extern "C" void op1f_log(int level, const char* file, int line, const char* format, ...) OP1F_NAMED("op1f_log");

// The clock: milliseconds since the base's first frame. It moves once a frame. From any thread.
extern "C" unsigned op1f_ms() OP1F_NAMED("op1f_ms");

// The document of one of the firmware's screens, by the screen's number: the picture the firmware made for that
// screen, whose parts the next three work on. Answers 0 for a number outside 1 to 85, for a number no screen has,
// and for a screen that has no document yet: ask again at a later frame. Display thread.
extern "C" void* op1f_screen(int number) OP1F_NAMED("op1f_screen");

// A part of a document, by the name it has in the picture (its id). Answers the part, or 0 when the document has
// none of that name; 0 too for no document or no name. Display thread.
extern "C" void* op1f_node(void* document, const char* name) OP1F_NAMED("op1f_node");

// A part shown (`on` not 0) or hidden (0), with everything inside it. `part` is one op1f_node answered, never 0.
// Answers nothing. Display thread.
extern "C" void op1f_show(void* part, int on) OP1F_NAMED("op1f_show");

// A part moved, turned or scaled: its matrix set. `matrix` is nine whole numbers in 1024ths, [a, b, 0, c, d, 0,
// e, f, 1024]: the six numbers of a picture's matrix(a b c d e f), so [1024, 0, 0, 0, 1024, 0, 1024 x, 1024 y,
// 1024] is a move of x and y pixels. `part` is one op1f_node answered, never 0. Answers nothing. Display thread.
extern "C" void op1f_move(void* part, const int* matrix) OP1F_NAMED("op1f_move");

// One of the pack's own pictures, by its name in the pack's own directory and nobody else's, made into a document
// as the firmware makes one of a file. `card` is the pack's own card, one the base has in use. Answers the
// document, or 0 when the card is not in use, the pack carries no picture of that name, or it could not be made.
// Ask once for a picture and keep the answer, also where the answer was 0: each call for a picture the pack
// carries reads the pack's file and takes memory, and one that cannot be made is an error line on the log at
// every call. The base keeps the first eight documents a card made: those are the ones op1f_page_open and
// op1f_text take. Display thread.
extern "C" void* op1f_picture(const Op1fCard* card, const char* name) OP1F_NAMED("op1f_picture");

// Level 2. The pack's page opened, in the mode the unit is in, with a document op1f_picture made: the base shows it
// from the next frame on, in place of whatever the mode would show, until it is closed. `card` is the pack's own
// card, one the base has in use; a pack with no input function may open a page too, from its frame function, and
// the keys then go on to the firmware as they always did. The page opened last is the one shown: another
// pack's page that was open is closed. The page is closed when the mode changes, when the firmware's own page of
// the base (the effect train, the LFOs) is opened over it, and by op1f_page_close. Answers not 0 when the base took
// the request, 0 when it did not (no such card, no document, the base not ready yet). A request taken can still be
// dropped before the page is ever shown, with no line: when the mode changes before the base's next frame, or when
// the train's or the UFOs' page is opened in that frame. The frame's `page` is what says the page shows. Open a page
// on a key, not at every frame: since level 3 the base refuses an open from a frame function while the train's or
// the UFOs' page is shown, and takes one from a key. `document` must be one op1f_picture made for this card: since
// level 3 the base answers 0 for any other. From the frame function or the input function. The document stays the
// pack's: the base never frees it, and the pack may dress it at every frame, shown or not.
extern "C" int op1f_page_open(const Op1fCard* card, void* document) OP1F_NAMED("op1f_page_open");

// Level 2. The pack's page closed, when it is the one open; nothing otherwise. The mode's own screen comes back.
// From the frame function or the input function. From level 4's base on, a close is seen by the firmware's next
// request for a screen, so the mode's own screen is back at the next frame; and where the page lay over another shown
// with op1f_page_show, that page shows again in its place.
extern "C" void op1f_page_close(const Op1fCard* card) OP1F_NAMED("op1f_page_close");

// Level 4. The pack's page shown, in the mode the unit is in, over whatever page was there: pages stack. When the
// page on top closes (op1f_page_close by its pack), the page under it shows again; a mode change closes them all,
// and so does the firmware's own page of the base (the effect train, the LFOs) opened over them. A page opened with
// op1f_page_open behaves as it always did: a page shown over it closes it. `document` is one this card made with
// op1f_picture, or 0 for the base's own blank page: black, the whole screen, for a pack that paints its page itself
// on the frame's `canvas`; the same page for every pack, and its text is not a pack's to set. `flags` must be 0 (kept
// for a later level). Answers 1 when the base took the request, 0 when it did not: the card not in use, a document
// this card did not make, a flag set, the blank page not to be made (the log says so once), or a call from the frame
// function while the train's or the UFOs' page is shown, as op1f_page_open. Taken is not shown: the frame's `page`
// says it. From the frame function or the input function.
extern "C" int op1f_page_show(const Op1fCard* card, void* document, unsigned flags) OP1F_NAMED("op1f_page_show");

// Level 3. The path on the unit's disk of a file of one of the pack's folders (`name`), or of the folder itself
// (`name` 0: with the slash behind), written into `out` with its end. Answers the path's characters, or less than
// 0: OP1F_NO_FOLDER, OP1F_BAD_NAME, OP1F_TOO_LARGE (`room` too small; nothing written). For a line on the log or a
// page; the helpers below take the folder and the name, never a path. From any thread, once the base's first frame
// has made the folders.
extern "C" int op1f_path(const Op1fCard* card, unsigned folder, const char* name, char* out, unsigned room) OP1F_NAMED("op1f_path");

// Level 3. The names of the files in one of the pack's folders, in the order the disk lists them: each name and a
// zero behind it, one after another in `names`, as many as fit whole in `room`. Folders inside the folder and names
// that begin with a dot are left out, and so is a name of more than thirty-nine characters. Answers how many names
// it wrote, or less than 0: OP1F_NOT_NOW, OP1F_NO_FOLDER, OP1F_FAILED. From the pack's own frame function only.
extern "C" int op1f_list(const Op1fCard* card, unsigned folder, char* names, unsigned room) OP1F_NAMED("op1f_list");

// Level 3. A file of one of the pack's folders read whole into `buffer`. Answers the bytes read (0 for an empty
// file), or less than 0: OP1F_NO_FILE (no line), OP1F_TOO_LARGE (the file has more than `room` bytes: nothing is
// read), OP1F_NOT_NOW, OP1F_NO_FOLDER, OP1F_BAD_NAME, OP1F_FAILED. From the pack's own frame function only.
extern "C" int op1f_read(const Op1fCard* card, unsigned folder, const char* name, void* buffer, unsigned room) OP1F_NAMED("op1f_read");

// Level 3. `bytes` bytes of `data` written as a file of one of the pack's folders, in place of what the file held,
// or as a new file. Answers `bytes` when the file landed (its size held afterwards), or less than 0: OP1F_NOT_NOW
// (also while the disk mode is open), OP1F_NO_FOLDER, OP1F_BAD_NAME, OP1F_FAILED (the line says which step: the
// temporary file opened, written, renamed over the pack's file, or the size afterwards). From the pack's own frame
// function only. At most a megabyte.
extern "C" int op1f_write(const Op1fCard* card, unsigned folder, const char* name, const void* data, unsigned bytes) OP1F_NAMED("op1f_write");

// Level 4. A file of one of the pack's folders taken off the disk. Answers 0 when the file is gone, or less than 0:
// OP1F_NO_FILE (the folder held no file of that name), OP1F_NOT_NOW (not from the pack's own frame function, or the
// disk mode is open), OP1F_NO_FOLDER, OP1F_BAD_NAME, OP1F_FAILED (the disk did not take it off; the line says so).
// From the pack's own frame function only.
extern "C" int op1f_remove(const Op1fCard* card, unsigned folder, const char* name) OP1F_NAMED("op1f_remove");

// Level 3. A text written into a part of a document op1f_picture made (any card's, one the base keeps): the part, a
// placeholder group of the pack's picture (above), becomes a text at its first use and stays one; the text is set
// again only when it changed. `width`: the text is shortened with a mark where it is wider than so many pixels, 0
// for the screen's width. An empty text shows nothing. Answers 1 when the text is in place, 0 when it is not: no
// such document or part, a part that is no group with a stroke, the base's room for texts full (sixty-four on a base
// of level 3; on a base of level 4, 256 for one pack and 512 for all), or a call that did not come from a frame
// function. From a frame function only. On a base of level 4 it is op1f_text_set with OP1F_LEFT.
extern "C" int op1f_text(void* document, const char* part, const char* text, int width) OP1F_NAMED("op1f_text");

// Level 4. op1f_text with an alignment: `align` OP1F_LEFT (the text begins at the placeholder's left), OP1F_CENTRE
// (centred on it) or OP1F_RIGHT (it ends at its right). A part's alignment is the one of its first use: a later call
// with another alignment sets the text and keeps the first alignment, and the log says so; a pack that wants a part
// in two alignments uses two placeholders. Answers as op1f_text, and 0 for any other `align`. From a frame function
// only.
extern "C" int op1f_text_set(void* document, const char* part, const char* text, int width, unsigned align) OP1F_NAMED("op1f_text_set");

// Level 4. The width in pixels of the text a part holds, as it was laid out: 0 for an empty text, -1 when no text of
// that part was made (by op1f_text or op1f_text_set). The width is the letters' own, without their stroke: a text
// of one upright stroke measures 0. From a frame function only.
extern "C" int op1f_text_width(void* document, const char* part) OP1F_NAMED("op1f_text_width");

// Level 4. The text a part became, as a part: op1f_move moves it, op1f_show shows and hides it. Answers 0 when no
// text of that part was made. A text moved by a matrix needs nothing more. From a frame function only.
extern "C" void* op1f_text_node(void* document, const char* part) OP1F_NAMED("op1f_text_node");

// Level 4: painting on the frame's picture. `canvas` is the frame's `canvas`, handed in the same call; the picture is
// 480 pixels wide and 224 tall, x from the left and y from the top, and everything is cut where it leaves it. A
// colour is OP1F_COLOUR's, its low sixteen bits used. Each helper paints only from the pack's own frame function and
// with this frame's canvas: any other call paints nothing and is counted, and the base says the count on the log by
// the pack's name. A `canvas` of 0 (the frame offers none) paints nothing and is not counted.

// A filled box, `w` pixels wide and `h` tall, its top-left at (x, y). Nothing for a `w` or an `h` of 0 or less.
extern "C" void op1f_paint_box(void* canvas, int x, int y, int w, int h, unsigned colour) OP1F_NAMED("op1f_paint_box");

// A line from (x0, y0) to (x1, y1), both ends painted, drawn with a square pen `width` pixels wide (1 to 16) whose
// middle follows the line. Nothing for another width, and nothing for an end outside -32,768 to 32,767.
extern "C" void op1f_paint_line(void* canvas, int x0, int y0, int x1, int y1, int width, unsigned colour) OP1F_NAMED("op1f_paint_line");

// A block of pixels put at (x, y): `w` by `h` pixels of sixteen bits, row by row from the top, `stride` pixels from
// the start of one row to the start of the next. A pixel equal to `key` (0 to 65,535) is left out, so that what lies
// under it shows; a `key` of -1 leaves none out. Nothing for a `w` or an `h` of 0 or less or more than 480, for
// `pixels` 0, or for another key.
extern "C" void op1f_paint_block(void* canvas, int x, int y, int w, int h, const unsigned short* pixels, int stride, int key) OP1F_NAMED("op1f_paint_block");

// A colour laid over what is there through a block of cover, its top-left at (x, y): `w` by `h` bytes, row by row from
// the top, `stride` bytes from the start of one row to the start of the next, each saying how much of `colour` lies
// over its pixel, 0 none and 255 all, so that a pack paints letters and shapes with soft edges of its own. Each of the
// pixel's three parts is mixed by itself: the part + (((the colour's part - the part) x weight) >> 8), the weight being
// the cover + (cover >> 7), 0 to 256; 0 leaves the pixel as it is and 255 gives the colour exactly. Nothing for a `w`
// or an `h` of 0 or less or more than 480, or for `cover` 0.
extern "C" void op1f_paint_mask(void* canvas, int x, int y, int w, int h, const unsigned char* cover, int stride, unsigned colour) OP1F_NAMED("op1f_paint_mask");

// A text in the base's own font, its top-left at (x, y): seven pixels tall and five wide a character, with one
// pixel of space after each but the last, every pixel of the font a square of `scale` by `scale` (1 to 8). The
// characters are the plain ones, capitals and small letters, digits and signs; any other is a question mark. At most
// 128 characters; the rest is not painted. Answers the width in pixels the text takes, (6 x characters - 1) x scale,
// the same whether it was painted, cut or not painted at all, so that a pack measures a text with it; 0 for no text,
// an empty one or a scale outside 1 to 8.
extern "C" int op1f_paint_text(void* canvas, int x, int y, const char* text, unsigned colour, int scale) OP1F_NAMED("op1f_paint_text");

// Level 5: notes out, for a pack with a sequencer. `card` is the pack's own card.

// A note begun on the instrument the unit is on, as a key does, and sent out over MIDI on the unit's own MIDI
// settings unless `flags` says OP1F_NOTE_NO_MIDI. `note` 0 to 127, `velocity` 1 to 127. Only from the pack's `tick`,
// `key`, `run`, `octave` and its input function, while the base is calling them, and only while the instrument the
// unit is on is one whose chosen sequencer is this card's: any other call plays nothing and is counted, and the base
// says the count on the log by the pack's name. Never from the frame function, and never from a `note` of the notes
// heard. The base keeps a list of the notes a pack began on each instrument; when the sequencer stops (switched off,
// another chosen, the unit changing instrument) it ends every one the pack left, once the pack's `run` has come back.
// Answers 1 when the note was handed on, 0 when not: the card not in use, a note above 127, a velocity of 0 or above
// 127, a note this pack already has sounding (end it first), or a call from elsewhere. Handed on is not heard: the
// synth has six voices and a seventh note takes one of them, and while a sound is being loaded the instrument plays
// nothing. A note a pack began is not moved by the octave keys: a pack that wants its notes to follow ends and begins
// them in `octave`.
extern "C" int op1f_note_on(const Op1fCard* card, unsigned note, unsigned velocity, unsigned flags) OP1F_NAMED("op1f_note_on");

// A note this pack began ended. From the same calls as op1f_note_on. Answers 1 when the end was handed on, 0 when
// not: the card not in use, a note this pack has not sounding, or a call from elsewhere. While a sustain pedal is
// held the instrument may let the note ring until the pedal comes up; for the base it has ended.
extern "C" int op1f_note_off(const Op1fCard* card, unsigned note, unsigned flags) OP1F_NAMED("op1f_note_off");

// A sounding note of this pack's moved in pitch, on the synth: `cents` hundredths of a semitone, up for more than 0
// and down for less, -4800 to 4800 (beyond: the nearer of the two), 0 back to the note itself. The note keeps its
// number: it is ended with op1f_note_off as it was begun. Where the instrument plays the same note for a key as well,
// that one moves with it. From the same calls as op1f_note_on. Answers 1 when the pitch was set, 0 when not: the unit
// is not on the synth, a note this pack has not sounding, or a call from elsewhere.
extern "C" int op1f_note_bend(const Op1fCard* card, unsigned note, int cents) OP1F_NAMED("op1f_note_bend");

// Level 5. The clock's count set back to its start, as the firmware's own sequencers do when a first key goes down,
// where the firmware allows it (not while a clock from outside leads). From the same calls as op1f_note_on, with the
// same refusals. Answers 1 when it was set back, 0 when not. The next tick's `pulse` starts again and its `jumped`
// says so; its `count` goes on.
extern "C" int op1f_seq_restart(const Op1fCard* card) OP1F_NAMED("op1f_seq_restart");

// Level 5. A short section in which nothing else of the unit runs: for what a pack shares between its calls on
// different threads (a queue its `key` fills and its `tick` empties). op1f_mask answers what op1f_unmask must be
// handed. A few microseconds between the two, and never a call of the base's or a wait inside. From any thread.
extern "C" unsigned op1f_mask() OP1F_NAMED("op1f_mask");
extern "C" void op1f_unmask(unsigned was) OP1F_NAMED("op1f_unmask");

// Level 5. The document that stands in for the firmware's screen of one of the pack's asks while that screen would be
// up (a sequencer: the sequencer's screen of an instrument whose chosen sequencer it is): one this card made with
// op1f_picture, or 0 for the base's blank page, on which the pack paints from its frame function. `ask` is the ask's
// number in the card's list, from 0. It takes effect at the next frame and stands until it is called again with
// another document, whatever the page does in between; which instrument's screen the page stands in for at a moment
// is what the ask's `shown` says in `where`. Until a pack calls it, the base's blank page stands in with the ask's
// name on it. The pack does not open
// or close this page: the base does, and the ask's `shown` says when it is on the screen. A page a pack shows over it
// (op1f_page_show) lies over it, and it shows again when that one closes. While it is on the screen the frame's `page`
// is this card, and every key and knob is offered to this pack's input function first, as for any page. Answers 1, or
// 0: the card not in use, no such ask of a kind with a page, a document this card did not make, or a call that did
// not come from the pack's frame function. From the frame function.
extern "C" int op1f_page_stand_in(const Op1fCard* card, unsigned ask, void* document) OP1F_NAMED("op1f_page_stand_in");

#endif

// Step M3 (docs/base.md): what a pack shows the base, and what the base offers a pack,
// at level 3. A sibling of module/build_m2_card.h, which stays as it was for the packs built against level 2: every
// word of levels 1 and 2 stands here at the same place with the same meaning, and level 3 adds behind it.
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
// A pack that uses level 3 needs the name op1f_level_3, the mark of a base that offers it; the card's source names
// it, so nothing is written by hand. A base of level 3 offers op1f_level_1 and op1f_level_2 too: a pack built for an
// earlier level runs on it as it is. Where no pack offers the mark the loader leaves the pack out and says so.
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
// every frame.
//
// Nothing here names an address of the firmware. The file is compiled for the unit and, with OP1F_HOST defined, by
// the PC's compiler for the tests. The PC's build must be one with addresses of four bytes, as the unit's are: the
// sizes and places below are held when the file is compiled, and a build they do not hold for stops.

#ifndef OP1F_BUILD_M3_CARD_H
#define OP1F_BUILD_M3_CARD_H

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

// What level 3's helpers answer when they do not do what was asked (each is less than 0):
//   OP1F_NO_FILE     op1f_read: the folder holds no file of that name
//   OP1F_TOO_LARGE   op1f_read: the file is larger than the room handed in (nothing is read); op1f_path: the room
//                    handed in is too small for the path
//   OP1F_NOT_NOW     the call did not come from this pack's frame function, or (op1f_write) the disk mode is open
//   OP1F_NO_FOLDER   the card is not one the base has in use, or the folder's number is not one of its folders the
//                    base made
//   OP1F_BAD_NAME    a file's name is empty, longer than thirty-nine characters, holds a slash or a character that
//                    is not plain, or begins with a dot
//   OP1F_FAILED      the disk did not do it: the line on the log says which step
enum { OP1F_NO_FILE = -1, OP1F_TOO_LARGE = -2, OP1F_NOT_NOW = -3, OP1F_NO_FOLDER = -4, OP1F_BAD_NAME = -5, OP1F_FAILED = -6 };

struct Op1fCard;

// What a card's frame function is handed, the same for every card of a frame.
//   bytes    the size of what is handed: 28 at level 1, 32 at level 2, 36 at level 3. A later base may hand more;
//            read what you know
//   frame    the number of the frame, counted by the base
//   ms       milliseconds since the base's first frame: what op1f_ms answers. It moves once a frame
//   mode     the mode the unit is in, by the firmware's own number: 0 synth, 1 drum, 2 tape, 3 mixer
//   screen   the number of the screen that is shown
//   shift    not 0 while shift is held
//   popup    not 0 while the power popup is on the screen: the last moment to say something on the log
//   page     level 2: the card whose page the GUI shows now, 0 when it shows none. Compare it with your own card's
//            address: it is yours when they are the same. The base takes a page asked for at its next frame and the
//            GUI shows it at the frame after, from which on `page` names it; it stops when the page is closed, when
//            the mode changes, or when the firmware's own page of the base (the effect train, the LFOs) is opened
//            over it
//   disk     level 3: not 0 while the disk mode is open (the COM key, the computer connected): the computer owns the
//            folders and op1f_write answers OP1F_NOT_NOW. When it goes back to 0 the computer may have brought,
//            changed or deleted files in a shown folder
struct Op1fFrame { unsigned bytes, frame, ms, mode, screen, shift, popup; const Op1fCard* page; unsigned disk; };   // 36 bytes at level 3

// What happened, for an input function (level 2).
//   OP1F_DOWN        a key went down
//   OP1F_DOWN_SHIFT  a key went down while shift was held. The firmware hands these on apart, and so does the base
//   OP1F_UP          a key came up. An up goes where its down went: to the pack that took the down, whatever its
//                    input function answers, and never to another; to the firmware where the firmware had the down.
//                    So a pack sees the up of every key whose down it took, and of no other
//   OP1F_TURN        a knob was turned
enum { OP1F_DOWN = 1, OP1F_DOWN_SHIFT = 2, OP1F_UP = 3, OP1F_TURN = 4 };

// What an input function is handed (level 2), one event at a time.
//   bytes    the size of what is handed: 36 at level 2. A later base may hand more; read what you know
//   what     OP1F_DOWN, OP1F_DOWN_SHIFT, OP1F_UP or OP1F_TURN
//   key      the key's number as the firmware counts its keys (0 the help key, 1 the metronome key, 2 to 5 the
//            synth, drum, tape and mixer keys, 6 to 9 the four keys under the screen, 38 shift, 54 to 57 the knobs'
//            clicks, 61 to 64 the knobs pressed down and let go); for a turn, the knob: 0 blue, 1 the second, 2 the
//            third, 3 red. The firmware makes a click (54 to 57, a down and then an up) when a knob that was pressed
//            comes up without having been turned
//   turn     for a turn: 1024 a step of the knob, more than 0 one way and less the other, as the firmware's own pages
//            take it: a value goes up for more than 0. 0 for a key
//   mode     the mode the unit is in: 0 synth, 1 drum, 2 tape, 3 mixer
//   screen   the number of the screen that is shown
//   shift    not 0 while shift is held
//   mine     not 0 when this pack's page is the one the base shows
//   record   the firmware's own record of a key that went down, sixteen bytes: the key's number, the value (127, or
//            how hard a key of the keyboard was struck), and two words of the clock at the press. 0 for an up and a
//            turn, which the firmware hands on as numbers. It lives only for the call (for a knob's click it stands
//            in the firmware's own stack): copy what you need, never keep the pointer
struct Op1fInput { unsigned bytes, what, key; int turn; unsigned mode, screen, shift, mine; const unsigned* record; };   // 36 bytes at level 2

// One of a pack's folders (level 3), constant data in the pack's body, written by tools/pack_build.py from the
// recipe. Each text is zero padded and ended within its sixteen bytes.
//   name     what the computer shows in the disk mode
//   place    where the folder lies on the unit's disk, inside user/pf/
//   shown    not 0 for a folder the computer sees in the disk mode
struct Op1fFolder { char name[OP1F_FOLDER_TEXT]; char place[OP1F_FOLDER_TEXT]; unsigned shown; };   // 36 bytes

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
//             its down went
//   folders   level 3, read only from a card that says level 3 or later and is of level 3's size: the pack's list of
//             folders (above), 0 for none
//   folder_count  level 3: how many folders the list holds, 0 to OP1F_FOLDERS_MOST; 0 for none
struct Op1fCard {
    unsigned mark, bytes, level, version;        // the mark; 44 at level 1, 48 at level 2, 56 at level 3; the highest level used; the pack's version
    char name[16];                               // the pack's name, zero padded
    void (*frame)(const Op1fFrame* now);         // +32: once a frame, on the display thread; 0 for none
    const void* pictures;                        // +36: the pack's directory of pictures; 0 for none
    const char* (*pick)(const char* name);       // +40: which of its pictures a name stands for now; 0: the name itself
    int (*input)(const Op1fInput* event);        // +44, level 2: a key or a knob; not 0 when the pack took it; 0 for none
    const Op1fFolder* folders;                   // +48, level 3: the pack's folders; 0 for none
    unsigned folder_count;                       // +52, level 3: how many
};

// The list the base offers under the name op1f_cards, in its zeroed data, and the loader fills once the packs are
// joined: one slot for each pack that runs and has a card, in the order of the files' names. `first` is the pack's
// first address and `end` the one behind its last (its body and its zeroed data), so that the base can hold every
// address of a card to the pack it came with. `count` is the slots filled; `more` is the cards that found no slot.
// A pack with a card has no business with the list: it is between the loader and the base.
struct Op1fCardSlot { const Op1fCard* card; unsigned first, end; };
struct Op1fCards { unsigned count, more; Op1fCardSlot slots[OP1F_CARDS_MOST]; };   // 392 bytes

#ifdef __GNUC__
#define OP1F_PLACE_IN(type, field) __builtin_offsetof(type, field)
#else
#include <stddef.h>
#define OP1F_PLACE_IN(type, field) offsetof(type, field)
#endif
static_assert(sizeof(unsigned) == 4 && sizeof(void*) == 4, "a card is laid out for words and addresses of four bytes, as the unit's are");
static_assert(sizeof(Op1fFrame) == 36, "what a frame function is handed is 36 bytes at level 3");
static_assert(OP1F_PLACE_IN(Op1fFrame, popup) == 24 && OP1F_PLACE_IN(Op1fFrame, page) == 28 && OP1F_PLACE_IN(Op1fFrame, disk) == 32,
              "levels 1 and 2's 32 bytes stand as they were, and the disk behind them");
static_assert(sizeof(Op1fInput) == 36, "what an input function is handed is 36 bytes at level 2");
static_assert(sizeof(Op1fFolder) == 36, "a folder of a card's list is 36 bytes");
static_assert(sizeof(Op1fCard) == OP1F_CARD_LEVEL_3_BYTES, "a card of level 3 is 56 bytes");
static_assert(OP1F_PLACE_IN(Op1fCard, frame) == 32, "a card's frame function stands at +32");
static_assert(OP1F_PLACE_IN(Op1fCard, pictures) == 36, "a card's directory of pictures stands at +36");
static_assert(OP1F_PLACE_IN(Op1fCard, pick) == 40, "a card's choice among its pictures stands at +40");
static_assert(OP1F_PLACE_IN(Op1fCard, input) == OP1F_CARD_LEVEL_1_BYTES, "a card's input function stands at +44, behind level 1");
static_assert(OP1F_PLACE_IN(Op1fCard, folders) == OP1F_CARD_LEVEL_2_BYTES && OP1F_PLACE_IN(Op1fCard, folder_count) == 52, "a card's folders stand at +48, behind level 2");
static_assert(sizeof(Op1fCardSlot) == 12, "a slot of the list of cards is 12 bytes");
static_assert(sizeof(Op1fCards) == 8 + 12 * OP1F_CARDS_MOST && sizeof(Op1fCards) == 392, "the list of cards is 392 bytes");

// What the base offers a pack with a card. Each is a name the pack needs once it calls it, and the loader joins it
// to the base's function. Every one of these names sorts behind op1f_level_1, op1f_level_2 and op1f_level_3, and the
// loader looks for a pack's needed names in their order: a pack whose base is too old is left out for the mark,
// which says what is missing, not for a helper.

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
// From the frame function or the input function.
extern "C" void op1f_page_close(const Op1fCard* card) OP1F_NAMED("op1f_page_close");

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

// Level 3. A text written into a part of a document op1f_picture made (any card's, one the base keeps): the part, a
// placeholder group of the pack's picture (above), becomes a text at its first use and stays one; the text is set
// again only when it changed. `width`: the text is shortened with a mark where it is wider than so many pixels, 0
// for the screen's width. An empty text shows nothing. Answers 1 when the text is in place, 0 when it is not: no
// such document or part, a part that is no group with a stroke, the base's room for texts full (sixty-four), or a
// call that did not come from a frame function. From a frame function only.
extern "C" int op1f_text(void* document, const char* part, const char* text, int width) OP1F_NAMED("op1f_text");

#endif

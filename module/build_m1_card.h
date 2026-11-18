// Step M1 (docs/base.md): what a pack shows the base, and what the base offers a pack.
//
// The words. The loader is our piece inside the firmware file: at the start it reads the pack files, places them
// and joins them by their names. The base is the shared pack, the file base.pack. A pack that wants something of
// the base carries a card: a small table in its own body that says what it wants. The pack file's header names the
// card's place in its last word; tools/pack_build.py writes the card's source from the pack's recipe (`card`) and
// the header's word (`build3`). The loader hands the base the cards of the packs that run, in the order of their
// files' names, and the base serves each pack through its card and knows none of them by name.
//
// Level 1, which this file describes: a call once a frame, pictures the pack carries for screens of the firmware,
// a picture of the pack's own, and six helpers, so that a pack with a card names no address of the firmware. A
// picture such a pack carries is a picture as it is, or instructions of either kind by which the unit makes it
// from its own pictures, up to eight of them in a set: level 1 says all of that, and the pack names no other mark
// for its pictures. A pack that uses level 1 needs the name op1f_level_1, the mark of a base that reads cards; the
// card's source names it, so nothing is written by hand. Where no pack offers the mark the loader leaves the pack
// out and says so.
//
// A card only grows. A later level adds fields behind the last one and raises `bytes`; a base reads the part it
// knows and takes a field the card is too short for as 0. What a frame function is handed grows the same way.
//
// Nothing here names an address of the firmware. The file is compiled for the unit and, with OP1F_HOST defined, by
// the PC's compiler for the tests. The PC's build must be one with addresses of four bytes, as the unit's are: the
// sizes and places below are held when the file is compiled, and a build they do not hold for stops.

#ifndef OP1F_BUILD_M1_CARD_H
#define OP1F_BUILD_M1_CARD_H

// The name a function or an object has in the linked pack. The module's compiler takes it as written here; the
// PC's compiler needs none, and the tests' stand-ins are found by their C names.
#ifdef OP1F_HOST
#define OP1F_NAMED(x)
#else
#define OP1F_NAMED(x) asm(x)
#endif

// OP1F_CARD_MARK is a card's first word. OP1F_CARD_LEVEL_1_BYTES is the size of a card of level 1. OP1F_CARDS_MOST
// is how many cards the base's list holds: as many as the loader takes pack files. OP1F_RELEASE_BYTES is the room of
// the release text a base carries under the name op1f_release: the text and zeros behind it.
enum { OP1F_CARD_MARK = 0x44524143 /* "CARD" */, OP1F_CARD_LEVEL_1_BYTES = 44, OP1F_CARDS_MOST = 32, OP1F_RELEASE_BYTES = 16 };

// What a card's frame function is handed, the same for every card of a frame.
//   bytes    the size of what is handed: 28 at level 1. A later base may hand more; read what you know
//   frame    the number of the frame, counted by the base
//   ms       milliseconds since the base's first frame: what op1f_ms answers. It moves once a frame
//   mode     the mode the unit is in, by the firmware's own number: 0 synth, 1 drum, 2 tape, 3 mixer
//   screen   the number of the screen that is shown
//   shift    not 0 while shift is held
//   popup    not 0 while the power popup is on the screen: the last moment to say something on the log
struct Op1fFrame { unsigned bytes, frame, ms, mode, screen, shift, popup; };   // 28 bytes at level 1

// A pack's card: constant data in the pack's body, written by tools/pack_build.py from the recipe under the name
// op1f_card_ and the pack's name in small letters, digits and underscores (op1f_card_wink). The three addresses
// are the pack's own and move with it; each is 0 where the pack has no such thing.
//   frame     called once a frame on the display thread, in the order of the packs' file names, before the base
//             dresses its own screens. It is handed the frame (above) and answers nothing. It must come back
//             quickly, every time: the picture stands still for as long as it runs. The base times every call and
//             prints the time on the log by the pack's name. A function that never comes back stops the display
//             for good, and no line names the pack. A function written in assembly keeps the registers the
//             calling convention says a function keeps
//   pictures  the pack's directory of pictures (tools/pack_build.py `pictures`): pictures for screens of the
//             firmware, by the file names the firmware asks for. The packs the base knows by name are asked first,
//             then the cards in their order: the first pack that has a name serves it
//   pick      asked before the pack's directory is looked into: handed the name a screen asks for, it answers the
//             name to look for in this pack's directory, or 0 for nothing of this pack's now. With no such function
//             the directory is looked into by the name itself. It is asked every time the firmware asks for a
//             picture: while the firmware makes its screens at the start, before the first frame, and again later,
//             when a tape is loaded and when a page of the base is opened. A later call is not always on the
//             display thread, and may come while the pack's own frame function stands interrupted. So it only
//             reads; what it shares with the frame function is single words, never a structure, which it could
//             find half written; and it takes no time
struct Op1fCard {
    unsigned mark, bytes, level, version;        // the mark; 44 at level 1; the highest level used; the pack's version
    char name[16];                               // the pack's name, zero padded
    void (*frame)(const Op1fFrame* now);         // +32: once a frame, on the display thread; 0 for none
    const void* pictures;                        // +36: the pack's directory of pictures; 0 for none
    const char* (*pick)(const char* name);       // +40: which of its pictures a name stands for now; 0: the name itself
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
static_assert(sizeof(Op1fFrame) == 28, "what a frame function is handed is 28 bytes at level 1");
static_assert(sizeof(Op1fCard) == OP1F_CARD_LEVEL_1_BYTES, "a card of level 1 is 44 bytes");
static_assert(OP1F_PLACE_IN(Op1fCard, frame) == 32, "a card's frame function stands at +32");
static_assert(OP1F_PLACE_IN(Op1fCard, pictures) == 36, "a card's directory of pictures stands at +36");
static_assert(OP1F_PLACE_IN(Op1fCard, pick) == 40, "a card's choice among its pictures stands at +40");
static_assert(sizeof(Op1fCardSlot) == 12, "a slot of the list of cards is 12 bytes");
static_assert(sizeof(Op1fCards) == 8 + 12 * OP1F_CARDS_MOST && sizeof(Op1fCards) == 392, "the list of cards is 392 bytes");

// What the base offers a pack with a card. Each is a name the pack needs once it calls it, and the loader joins it
// to the base's function. All but the log and the clock are for the display thread: call them from the frame
// function. Every one of these names sorts behind op1f_level_1, and the loader looks for a pack's needed names in
// their order: a pack whose base is away is left out for the mark, which says what is missing, not for a helper.

// A line on the unit's log. `level`: 1 an error, 2 a warning, 3 for information. `file`: a source path; the log
// shows the name between its last slash and its dot in front of the line, and a bare word shows nothing. `line`: a
// number shown with that name. `format` and what follows it: as printf takes them. Answers nothing. From any
// thread but the audio thread: count there, and say it from the frame function. A line every frame fills the log.
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
// every call. Display thread.
extern "C" void* op1f_picture(const Op1fCard* card, const char* name) OP1F_NAMED("op1f_picture");

#endif

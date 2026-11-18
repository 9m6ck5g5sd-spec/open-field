// Step M3, the base pack: the cards at level 3 (OS 1.7.0+0; docs/base.md). A sibling of M2's cards unit, base.pack
// 15's, which this tree does not hold: levels 1 and 2 are that unit's text, with what M2 left for this base mended,
// and level 3's part of the cards is added:
//
//   A card of level 3 and of its size may list folders (module/build_m3_card.h): the check holds the list to lie in
//   the pack on a word, whole, with eight folders at most. What the folders are, their names and places, is the
//   folders unit's (module/build_m3_folders.cpp), which the frame call asks once, at the first frame, before any
//   card's frame function, and which asks this unit which cards are in use (op1f_cards_used) and whose frame function
//   is being called (op1f_cards_calling). A frame function is handed 36 bytes: level 2's 32 and `disk`, not 0 while
//   the firmware's word [0x84d25704] says the disk mode is open (mtp::init writes it, mtp::shutdown clears it).
//   The documents a card made through op1f_picture (module/build_m3_art.cpp tells this unit, op1f_cards_made) are
//   kept, eight a card: op1f_page_open takes only one of the card's own, and the text unit (module/build_m3_text.cpp)
//   only one of a card's (op1f_cards_document). An open asked from a frame function while the train's or the UFOs'
//   page is shown is refused; one from a key is taken as before. With the base's own screen 77, a close leaves the
//   pack's document where it stands rather than putting back none. And the input's counts are made with interrupts
//   masked for the instant of each count, so that a count made on one thread is not lost to one made on another.
//
// What follows is M2's description, which holds.
//
// A pack that wants something of the base carries a card: a small table in its own body (module/build_m2_card.h).
// The loader, our piece in the firmware file, knows where each running pack's card stands and nothing of what a card
// holds. Once it has joined the packs and cleared their zeroed data it writes the cards of the packs that run, in the
// order of their files' names, into the list this unit offers under the name op1f_cards: each card's address, and
// its pack's first address and the one behind its last. From there on the base serves each such pack through its
// card and knows none of them by name. The marks op1f_level_1 and op1f_level_2 say what this unit serves:
//
//   The check (level 1, and three things more). Each card is looked at once, before the first use of any (check):
//   it lies in its pack on a word; it begins with a card's mark; its size is at least a card of level 1, in whole
//   words, inside the pack; its name is plain text with an end; its frame function, its choice among its pictures
//   and, for a card of level 2 and of its size, its input function are 0 or an even address inside the pack; its directory
//   of pictures is 0, or lies in the pack on a word with a directory's mark and all its entries, and every entry and
//   the directory's two texts end inside their room and no picture is larger than the picture loader's largest, a
//   megabyte. A card that fails is not used, and one line says which pack's and why. What a card says of its level
//   is printed and not judged: a pack that uses a later level names that level's mark, which this base does not
//   offer, and the loader leaves such a pack out before its card comes here. The list holds thirty-two cards, as
//   many as the loader takes pack files: a count above that is read as thirty-two, and the first line counts the
//   rest with the cards the loader says it found no slot for. The first use is the first picture stock asks for
//   while it makes its screens, on the main thread, before the first frame. What the check finds is written before
//   the word that says it was made, and the source forces that order (a barrier the compiler moves nothing across),
//   so a second caller, and the threads that hand keys in, read either nothing or all of it.
//
//   The frame call (level 1). Once a frame, from the measuring unit's frame function (module/build_m1_measure.cpp),
//   on the display thread: each used card's frame function, in the list's order, handed the frame's number, the
//   clock, the mode, the number of the screen that is shown, whether shift is held, whether the power popup is up,
//   and (level 2) the card whose page is shown. Each call is timed and printed by the pack's name through the gate
//   of build_q_quiet.h, as M1's unit did.
//
//   Pictures (level 1). The picture loader (module/build_m3_art.cpp) asks its four packs first, then the cards in
//   the list's order (op1f_cards_picture, op1f_cards_also), as M1's unit did. Where a second pack carries a name,
//   one line says so, once a name, for sixteen names; a name is remembered by its first 23 characters and compared
//   by them, so the line for a longer name comes once.
//
//   The page (level 2). Only one page is on the screen at a time, so the base lends the one it has, screen 77: the
//   train's and the UFOs' (module/build_h_train.cpp) when that screen is made, else one the base makes itself, a
//   screen of the base class with no document, as the train's unit makes its own. A pack opens its page with a
//   document of its own (op1f_page_open, from its frame function or its input function), and the base puts that
//   document under 77 between two frames, on the display thread, keeps the document that stood there, and asks the
//   firmware for the screen (with interrupts masked, as our units ask from this thread since J5i). From then on the
//   firmware's screen request answers 77 while the unit stays in the mode the page was opened in: a call of ours
//   stands in it (module/build_m2_input.s, op1f_page_screen) and reads one word this unit writes. The page opened
//   last is the one shown. It is closed by op1f_page_close, when the mode changes, and when the train's or the
//   UFOs' page is opened over it; then the document of before goes back under 77 and the firmware is asked again.
//   The base makes its own screen 77 only from the second frame on: the train's unit makes its screen at the first
//   frame, after this unit's frame work, and the firmware stops the unit when a second screen takes a number.
//
//   Keys and knobs (level 2). Our calls stand at the four functions every key and knob event passes before the
//   mode's own code (module/build_m2_input.s) and ask op1f_input_down, op1f_input_up and op1f_input_turn here. An
//   event is offered first to the pack whose page is shown, then to every card that has an input function, in the
//   list's order, until one takes it; a taken event does not reach the firmware. An up goes where its down went:
//   to the pack that took the down, which is told whatever it answers, or to the firmware. These run on the thread
//   that handles keys, and for the keys of the keyboard maybe on the audio thread: they write no line, they wait
//   for nothing, and they share single words with the display thread. What they count is said by the frame's lines.
//
//   The helpers. op1f_log (module/build_m1_log.s), op1f_ms, op1f_screen, op1f_node, op1f_show, op1f_move as M1's
//   unit has them, and level 2's op1f_page_open and op1f_page_close. Each wraps a call or a fact our own units have
//   used on the unit since H2 and I2:
//     the registry 0x8fc2470c() answers the table of 86 screens by their numbers (0x84c960f4), 0 for a number no
//       screen has; stock's own registration holds a number against no bound, so this unit takes 1 to 85 and
//       answers 0 for any other. A screen's document is the word at its +4, 0 until stock has loaded it. The
//       firmware reads that word at every draw, on the display thread.
//     the lookup 0x8fc241d0(screen, id, optional) reads the document at the screen's +4 and nothing else of the
//       screen, and stops the unit for a screen without a document or for no id, so
//       both are held here before it is called. It is handed the screen of the 85 that holds the document, which is
//       the call our units make; for a document no screen holds (a pack's own picture) it is handed four words of
//       which the second is the document.
//     a part's display byte stands at its +0x5c; 0x8c03db96(part, matrix) sets its matrix from nine words.
//     the base constructor 0x8fc248a8(object, number, name) registers a screen of the base class (the train's unit
//       makes its own the same way); the screen request 0x8c0b1d8c() asks the current controller and our call.
//
// The clock is the core's cycle counter summed up frame by frame, 400,000 cycles a millisecond: it counts from the
// base's first frame and moves once a frame. Nothing is divided (the module links no runtime). Nothing here runs on
// the audio thread but the three input functions, which count and call the packs' input functions and nothing of
// stock's but the shift key's getter, which stock's own dispatcher calls on the same paths. Compiled for the PC as it
// is, with OP1F_HOST defined, for tests this tree does not hold: the stand-ins for stock come in through
// mock_cards_m3.h.

#include "build_m3_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;
typedef unsigned char u8;

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef u32* (*ScreensFn)();                                                // 0x8fc2470c: the table of the screens by their numbers
typedef void* (*FindNodeFn)(void* screen, const char* id, int optional);    // 0x8fc241d0: 0 when optional and missing
typedef void (*ApplyMatrixFn)(void* node, const int* matrix);               // 0x8c03db96
typedef int (*ShiftFn)();                                                   // 0x8fc231b4: non-zero while shift is held
typedef void (*ScreenCtorFn)(void* object, int number, const char* name);  // 0x8fc248a8: Screen::Screen, registers the object by its number
typedef void (*RequestScreenFn)();                                          // 0x8c0b1d8c: the GUI asks the current controller which screen, and queues it

static const char LOG_FILE[] = "../src/main/op1f_cards.cpp";

#ifdef OP1F_HOST
#include "mock_cards_m3.h"                             // the PC's stand-ins: a clock, the logger, the registry of screens, the lookup, the matrix, the words read, the constructor, the request
#include <intrin.h>
static inline u32 stack_here() { return (u32)_AddressOfReturnAddress(); }   // M3: where the stack of the thread that runs this stands
#define OP1F_APART
#define BARRIER() ((void)0)                            // the PC's run is one thread
#define MASK_OFF(mask) ((void)(mask = 0))
#define MASK_ON(mask) ((void)(mask))
#else
#define STOCK_LOG ((LogFn)0x8c0539f0u)
#define STOCK_SCREENS ((ScreensFn)0x8fc2470cu)
#define STOCK_FIND_NODE ((FindNodeFn)0x8fc241d0u)
#define STOCK_APPLY_MATRIX ((ApplyMatrixFn)0x8c03db96u)
#define STOCK_SHIFT ((ShiftFn)0x8fc231b4u)
#define STOCK_SCREEN_CTOR ((ScreenCtorFn)0x8fc248a8u)
#define STOCK_REQUEST_SCREEN ((RequestScreenFn)0x8c0b1d8cu)
#define RD32(a) (*(volatile u32*)(a))
#define RD8(a) (*(volatile u8*)(a))
#define OP1F_APART __attribute__((noinline))
// The compiler moves no load or store across this: what the check found is stored before the word that says so.
#define BARRIER() asm volatile("" : : : "memory")
// J5i: a request from the display thread is made with interrupts masked, so that no thread that hands keys in is
// inside the request's queue meanwhile (module/build_h_chain.cpp); a word cleared after a look, the same.
#define MASK_OFF(mask) asm volatile("cli %0;" : "=d"(mask) : : "memory")
#define MASK_ON(mask) asm volatile("sti %0;" : : "d"(mask) : "memory")
// A time stamp is pinned where it is written: the read carries a memory clobber, which nothing is moved across.
static inline u32 stamp() {
    u32 c;
    asm volatile("%0 = CYCLES;" : "=d"(c) : : "memory");
    return c;
}
// M3: where the stack of the thread that runs this stands.
static inline u32 stack_here() {
    u32 sp;
    asm volatile("%0 = SP;" : "=d"(sp));
    return sp;
}
#endif

static const u32 CURRENT_MODE = 0x8400f7d4u;           // the mode: 0 synth, 1 drum, 2 tape, 3 mixer (the mode switch 0x8c0bc4e8 keeps it)
static const u32 CURRENT_SCREEN = 0x848d2138u;         // the id of the screen the GUI shows (0x8fc23a1c sets it)
static const u32 POWER_POPUP_SHOWN = 0x84c9af40u;      // the power popup's byte, 1 while the countdown popup is on the screen
static const u32 DISK_OPEN = 0x84d25704u;              // M3: not 0 while the disk mode is open (mtp::init 0x8fcba168 writes it, mtp::shutdown 0x8fcba1c8 clears it, through 0x8fcba5f8)
enum { CARD_3_BYTES = OP1F_CARD_LEVEL_3_BYTES, FOLDER_BYTES = 36, FOLDERS_MOST = OP1F_FOLDERS_MOST, DOCS = 8 };   // M3
enum { CALLING_REACH = 8192 };                         // M3: how far below the frame call's stack a call from inside a frame function may stand (a task's stack is 32 KB)
enum { FIRST_SCREEN = 1, LAST_SCREEN = 85, SCREEN_DOCUMENT = 1, NODE_DISPLAY = 0x5c, PAGE_SCREEN = 77, SCREEN_WORDS = 16 };
enum { PERIOD_FRAMES = 256, CYCLES_PER_MS = 400000, SLOW_US = 1000 };
enum { CARD_BYTES = OP1F_CARD_LEVEL_1_BYTES, CARD_2_BYTES = OP1F_CARD_LEVEL_2_BYTES, CARD_NAME = 16, MOST = OP1F_CARDS_MOST };
enum { ART_MAGIC = 0x31545241, ART_NAME = 24, ART_PACK = 16, ART_PATH = 48, ART_HEAD_BYTES = 80, ART_ENTRY_BYTES = 36, ART_MOST = 0xffff, NAMES_SAID = 16 };
enum { ART_LARGEST = 1 << 20 };                         // the picture loader's largest picture (module/build_m3_art.cpp, LARGEST)
enum { NAME_KEPT = ART_NAME - 1 };                      // a name said is remembered by its first 23 characters
enum { FOUND = 1, FOUND_PICKED = 2 };
enum { HELD_KEYS = 128 };                               // keys whose down a pack took, by the key's number: the firmware's keys are 0 to 64
enum { CLOSED_BY_PACK = 1, CLOSED_BY_MODE = 2, CLOSED_BY_TRAIN = 3, CLOSED_BY_OTHER = 4 };
static const u32 LONGEST = 0x0fffffffu;                // a call is counted as two thirds of a second at most: six digits of microseconds

// A pack's directory of pictures, as the picture loader has it (module/build_m3_art.cpp) and tools/pack_build.py
// writes it: a head of 80 bytes, then an entry of 36 bytes a picture.
struct ArtEntry {
    char name[ART_NAME];
    u32 offset, bytes, crc;
};

struct ArtDir {
    u32 magic, count, total, version;
    char pack[ART_PACK];
    char path[ART_PATH];
    ArtEntry entries[1];
};

struct Measure {
    u32 calls, sum, least, most;                       // of a period: the calls, and what they took in microseconds
};

// The list the loader fills. Zeroed data: the loader clears it with the rest of the pack's and then writes it.
extern "C" Op1fCards op1f_cards OP1F_NAMED("op1f_cards");
Op1fCards op1f_cards;

// The marks of a base that reads cards (level 1) and lends a page with keys and knobs (level 2). A pack with a card
// needs the mark of its card's level; no base of before M1 offers the first, none of before M2 the second.
extern "C" const u32 op1f_level_1 OP1F_NAMED("op1f_level_1");
const u32 op1f_level_1 = 1;
extern "C" const u32 op1f_level_2 OP1F_NAMED("op1f_level_2");
const u32 op1f_level_2 = 2;
// M3: and folders, files and text (level 3). No base before M3 offers it.
extern "C" const u32 op1f_level_3 OP1F_NAMED("op1f_level_3");
const u32 op1f_level_3 = 3;

// M3: the folders unit (module/build_m3_folders.cpp), asked once at the first frame, after the check and before any
// card's frame function: it settles the cards' folders and makes them on the disk.
extern "C" void op1f_folders_settle() OP1F_NAMED("op1f_folders_settle");

extern "C" u32 op1f_frame_counter OP1F_NAMED("op1f_frame_counter");         // the measuring unit's count of frames

// The train's unit (module/build_h_train.cpp) and the two feature packs behind it say whether the train's or the
// UFOs' page is open: the base's own names of kit, weak, as that unit has them. Away, the page is never theirs.
extern "C" int __attribute__((weak)) op1f_chain_train_open(int host) OP1F_NAMED("op1f_chain_train_open");
extern "C" int __attribute__((weak)) op1f_lfotrain_open() OP1F_NAMED("op1f_lfotrain_open");

// The one word our call in the firmware's screen request reads (module/build_m2_input.s): 0, or the mode + 1 in
// which a pack's page stands under 77 now. Written on the display thread, after the document is in place and before
// the firmware is asked; cleared before the document of before goes back.
extern "C" u32 op1f_page_screen_mode OP1F_NAMED("op1f_page_screen_mode");
u32 op1f_page_screen_mode;

// What a pack asked for (op1f_page_open, op1f_page_close), from any thread but the display thread's own frame work:
// one word, 0 or the slot + 1 in its low byte and the mode in the next, and the document of each slot, written
// before that word.
static volatile u32 page_asked;
static void* volatile page_document[MOST];

// M3: the documents each card made through op1f_picture, eight a card, written on the display thread only (where
// op1f_picture runs) and read from any thread, a word at a time.
static void* volatile made_document[MOST][DOCS];

// What the threads that hand keys in count, as totals that only grow: the display thread keeps what it saw at the
// last period and says the difference. Single words. The key thread writes most of them; a key of the keyboard can
// come on another thread, and where two meet in one count a count can be lost: they are for the log, nothing
// decides on them. The bytes of `held` are each one key's, written by the thread that hands that key in.
static struct {
    volatile u32 down, up, turns, taken;               // offered downs, ups seen, offered turns, events taken by packs
    volatile u32 took_down[MOST], took_up[MOST], took_turn[MOST];
    volatile u32 last_down, last_down_ms, last_up, last_up_ms;   // the last key down and up the firmware handed in, and when
    volatile u8 held[HELD_KEYS];                       // the slot + 1 of the pack that took a key's down, 0 for the firmware
} input;

static struct {
    u32 checked, said;                                 // the cards were looked at; the check's lines were printed
    u32 seen, in_use, more;                            // slots read; cards in use; cards that found no slot
    u32 used, framed;                                  // a bit a slot: its card is in use; and has a frame function
    u32 started;                                       // the first frame has come
    u32 last_cycles, cycle_acc, ms;                    // the clock
    int countdown;                                     // frames until the period ends
    QuietGate quiet;
    Measure measure[MOST];
    u32 names;                                         // names said to be carried by two packs
    char name_said[NAMES_SAID][ART_NAME];
    // the page: the display thread's own
    u32 shown;                                         // the slot + 1 whose document stands under 77 now, 0 for none
    u32* screen;                                       // the screen object of 77 the page is lent on
    void* kept;                                        // the document that stood under 77 before a page's
    u32 own[SCREEN_WORDS];                             // the base's own screen 77, where the train's unit made none
    u32 own_made, view_was, opened, closes;            // view_was: what the train's unit showed under 77 at the last frame (0 nothing, 1 the train, 2 the UFOs)
    u32 last_screen;                                   // the screen shown at the last frame, for the line when it changes
    u32 seen_down, seen_up, seen_turns, seen_taken;    // the input's totals at the last period
    u32 seen_took[MOST];
    u32 inputs;                                        // a bit a slot: its card is in use and has an input function (behind M1's fields, which keep their places)
    u32 calling;                                       // M3: the slot + 1 whose frame function is being called now, 0 between them (the display thread's; stored before and after an outside call, which the compiler keeps)
    u32 calling_sp;                                    // M3: where the display thread's stack stood at that call
    u32 settled;                                       // M3: the folders unit was asked to settle the folders
    u32 refused_opens;                                 // M3: opens asked from a frame function while the train's or the UFOs' page was shown
} state;

// M3: a count made with interrupts masked for its instant, so that two threads that count at once lose nothing.
static inline void count(volatile u32* c) {
    u32 mask;
    MASK_OFF(mask);
    *c = *c + 1u;
    MASK_ON(mask);
}

static inline u32 microseconds(u32 cycles) {
    if (cycles > LONGEST) cycles = LONGEST;
    return ((cycles >> 4) * 41u) >> 10;
}

// ---------------------------------------------------------------- the check

// A text that ends inside its room.
static int ended(const char* text, u32 room) {
    for (u32 i = 0; i < room; ++i) {
        if (text[i] == 0) return 1;
    }
    return 0;
}

// A directory's head and entries walked once: 0, or the entry (from 1; 0 for the head's two texts) that is not in
// order, with what it says (the picture's bytes, or the room of the text that has no end).
static OP1F_APART u32 walk(const ArtDir* dir, u32* what) {
    if (!ended(dir->pack, ART_PACK)) { *what = ART_PACK; return 0xffffffffu; }
    if (!ended(dir->path, ART_PATH)) { *what = ART_PATH; return 0xffffffffu; }
    for (u32 i = 0; i < dir->count; ++i) {
        const ArtEntry* e = &dir->entries[i];
        if (!ended(e->name, ART_NAME)) { *what = ART_NAME; return i + 1; }
        if (e->bytes > (u32)ART_LARGEST) { *what = e->bytes; return i + 1; }
    }
    return 0;
}

// The card's input function, for a card that says level 2 or later and is of level 2's size; 0 for any other: a card
// of level 1 that says more bytes has no input function the base would call.
static inline u32 input_of(const Op1fCard* card) {
    return card->bytes >= (u32)CARD_2_BYTES && card->level >= 2u ? (u32)card->input : 0;
}

// M3: the card's list of folders and their count, for a card that says level 3 or later and is of level 3's size;
// none for any other.
static inline u32 folders_of(const Op1fCard* card, u32* count) {
    int level_3 = card->bytes >= (u32)CARD_3_BYTES && card->level >= 3u;
    *count = level_3 ? card->folder_count : 0u;
    return level_3 ? (u32)card->folders : 0u;
}

// One card held against the pack it came with: 0, or why it is not used, with two numbers for the line.
static OP1F_APART const char* verdict(const Op1fCardSlot* slot, u32* a, u32* b) {
    u32 first = slot->first, end = slot->end, at = (u32)slot->card;
    u32 size = end - first;
    *a = at - first;
    *b = size;
    if (end < first || size < (u32)CARD_BYTES || at < first || at - first > size - (u32)CARD_BYTES || (at & 3u) != 0)
        return "it does not lie in its pack on a word";                             // (its place from the pack's first byte, the pack's bytes)
    const Op1fCard* card = slot->card;
    u32 room = end - at;
    if (card->mark != (u32)OP1F_CARD_MARK) {
        *a = card->mark;
        *b = 0;
        return "it does not begin with a card's mark";                              // (its first word, 0)
    }
    if (card->bytes < (u32)CARD_BYTES || (card->bytes & 3u) != 0 || card->bytes > room) {
        *a = card->bytes;
        *b = room;
        return "its size is not in order";                                          // (the size it says, the bytes from it to the pack's end)
    }
    u32 n = 0;
    while (n < (u32)CARD_NAME && card->name[n] >= 0x20 && card->name[n] < 0x7f) ++n;
    if (n == 0 || n == (u32)CARD_NAME || card->name[n] != 0) {
        *a = n;
        *b = n < (u32)CARD_NAME ? (u32)(u8)card->name[n] : 0;
        return "its name is not plain text with an end";                            // (the plain characters it begins with, the byte behind them)
    }
    u32 frame = (u32)card->frame, pick = (u32)card->pick, pictures = (u32)card->pictures, input = input_of(card);
    if (frame != 0 && (frame < first || frame >= end || (frame & 1u) != 0)) {
        *a = frame - first;
        *b = size;
        return "its frame function does not lie in its pack on an even address";    // (its place from the pack's first byte, the pack's bytes)
    }
    if (pick != 0 && (pick < first || pick >= end || (pick & 1u) != 0)) {
        *a = pick - first;
        *b = size;
        return "its choice among its pictures does not lie in its pack on an even address";
    }
    if (input != 0 && (input < first || input >= end || (input & 1u) != 0)) {
        *a = input - first;
        *b = size;
        return "its input function does not lie in its pack on an even address";
    }
    if (pictures != 0) {
        if (pictures < first || pictures >= end || end - pictures < (u32)ART_HEAD_BYTES || (pictures & 3u) != 0) {
            *a = pictures - first;
            *b = size;
            return "its directory of pictures does not lie in its pack on a word";
        }
        const ArtDir* dir = (const ArtDir*)card->pictures;
        if (dir->magic != (u32)ART_MAGIC) {
            *a = dir->magic;
            *b = 0;
            return "its directory of pictures does not begin with a directory's mark";  // (its first word, 0)
        }
        u32 behind = end - pictures - (u32)ART_HEAD_BYTES;
        if (dir->count > (u32)ART_MOST || dir->count * (u32)ART_ENTRY_BYTES > behind) {
            *a = dir->count;
            *b = behind;
            return "its directory of pictures does not end in its pack";            // (the pictures it counts, the bytes behind its head)
        }
        u32 what = 0, which = walk(dir, &what);
        if (which != 0) {
            *a = which;
            *b = what;
            return "its directory of pictures has a text with no end or a picture larger than a megabyte";   // (the picture from 1, or 4294967295 for the head; its bytes, or the text's room)
        }
    }
    u32 folder_count = 0, folders = folders_of(card, &folder_count);                  // M3: the list of folders, behind M2's reasons
    if (folder_count > (u32)FOLDERS_MOST || (folders == 0) != (folder_count == 0)) {
        *a = folder_count;
        *b = (u32)FOLDERS_MOST;
        return "its list of folders is not in order";                               // (the folders it counts, the most a card may list)
    }
    if (folders != 0 && (folders < first || folders >= end || (folders & 3u) != 0 || end - folders < folder_count * (u32)FOLDER_BYTES)) {
        *a = folders - first;
        *b = size;
        return "its list of folders does not lie in its pack on a word";            // (its place from the pack's first byte, the pack's bytes)
    }
    return 0;
}

static u32 pictures_of(const Op1fCard* card) {
    return card->pictures ? ((const ArtDir*)card->pictures)->count : 0;
}

// Every card of the list looked at, once. What is found is worked out first and written then, the same whoever
// looks; then, past a barrier, the word that says the check was made; the lines follow, each card's in the list's
// order.
static OP1F_APART void check() {
    u32 count = op1f_cards.count;
    u32 seen = count > (u32)MOST ? (u32)MOST : count;                               // the list holds thirty-two: a count above that names slots that are not there
    u32 more = op1f_cards.more + (count - seen);
    u32 used = 0, framed = 0, inputs = 0, in_use = 0, a = 0, b = 0;
    for (u32 i = 0; i < seen; ++i) {
        if (verdict(&op1f_cards.slots[i], &a, &b) != 0) continue;
        used |= 1u << i;
        ++in_use;
        if (op1f_cards.slots[i].card->frame != 0) framed |= 1u << i;
        if (input_of(op1f_cards.slots[i].card) != 0) inputs |= 1u << i;
    }
    state.seen = seen;
    state.more = more;
    state.in_use = in_use;
    state.used = used;
    state.framed = framed;
    state.inputs = inputs;
    BARRIER();                                                                      // all of the above is stored before the word that says it was made
    state.checked = 1;
    BARRIER();                                                                      // and that word before the first line
    if (state.said) return;
    state.said = 1;
    STOCK_LOG(3, LOG_FILE, 1, "cards: %u card(s) from the loader, %u in use, %u not used, %u more than the list holds", seen, in_use, seen - in_use, more);
    for (u32 i = 0; i < seen; ++i) {
        const Op1fCardSlot* slot = &op1f_cards.slots[i];
        const char* why = verdict(slot, &a, &b);
        if (why != 0) {
            STOCK_LOG(1, LOG_FILE, 2, "cards: the card of the pack at 0x%08x is not used: %s (%u, %u)", slot->first, why, a, b);
            continue;
        }
        const Op1fCard* card = slot->card;
        STOCK_LOG(3, LOG_FILE, 3, "cards: '%s' version %u, level %u: a frame call %s, %u picture(s), a choice among them %s, an input function %s",
                  card->name, card->version, card->level, card->frame ? "yes" : "no", pictures_of(card), card->pick ? "yes" : "no", input_of(card) ? "yes" : "no");
    }
}

static inline void ready() {
    if (!state.checked) check();
}

// Has the check been made? For the threads that hand keys in, which never make it (it writes lines): past this,
// what it found is read whole.
static inline int checked() {
    u32 done = state.checked;
    BARRIER();
    return done != 0;
}

// ---------------------------------------------------------------- the page

// The slot of a card the base has in use, or -1.
static int slot_of(const Op1fCard* card) {
    if (card == 0 || !checked()) return -1;
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.used >> i) & 1u) != 0 && op1f_cards.slots[i].card == card) return (int)i;
    }
    return -1;
}

// M3: is this one of the documents the card of this slot made?
static int made_by(u32 slot, const void* document) {
    for (u32 d = 0; d < (u32)DOCS; ++d) {
        if (made_document[slot][d] == document) return 1;
    }
    return 0;
}

// M3: from op1f_picture (module/build_m3_art.cpp), on the display thread: a document a card made. The first eight of
// each card are kept; the ninth and later are not, and neither op1f_page_open nor op1f_text takes them.
extern "C" void op1f_cards_made(const Op1fCard* card, void* document) OP1F_NAMED("op1f_cards_made");
extern "C" void op1f_cards_made(const Op1fCard* card, void* document) {
    int slot = slot_of(card);
    if (slot < 0 || document == 0 || made_by((u32)slot, document)) return;
    for (u32 d = 0; d < (u32)DOCS; ++d) {
        if (made_document[slot][d] == 0) {
            made_document[slot][d] = document;
            return;
        }
    }
}

// M3: the slot of the card that made this document, or -1: for the text unit.
extern "C" int op1f_cards_document(const void* document) OP1F_NAMED("op1f_cards_document");
extern "C" int op1f_cards_document(const void* document) {
    if (document == 0 || !checked()) return -1;
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.used >> i) & 1u) != 0 && made_by(i, document)) return (int)i;
    }
    return -1;
}

// M3: for the folders and text units. The cards in use, a bit a slot (0 before the check); the slot of a card in
// use, or -1; the slot + 1 whose frame function the base is calling now, 0 between the calls.
extern "C" u32 op1f_cards_used() OP1F_NAMED("op1f_cards_used");
extern "C" u32 op1f_cards_used() {
    return checked() ? state.used : 0u;
}

extern "C" int op1f_cards_slot(const Op1fCard* card) OP1F_NAMED("op1f_cards_slot");
extern "C" int op1f_cards_slot(const Op1fCard* card) {
    return slot_of(card);
}

// The slot + 1 is said only to a caller on the display thread's own stack, within CALLING_REACH below where it stood
// at the frame call: a call from an input function that runs, on the thread that handles keys or on the audio
// thread, while the display thread is inside a frame function stands on another stack and is told 0 (the review of
// M3). Another task's stack lies in another allocation of 32 KB [inferred: the tasks' stacks were not read for
// their places].
extern "C" u32 op1f_cards_calling() OP1F_NAMED("op1f_cards_calling");
extern "C" u32 op1f_cards_calling() {
    u32 calling = state.calling, at = state.calling_sp;
    if (calling == 0) return 0;
    u32 here = stack_here();
    return here <= at && at - here < (u32)CALLING_REACH ? calling : 0u;
}

extern "C" int op1f_page_open(const Op1fCard* card, void* document) {
    int slot = slot_of(card);
    if (slot < 0 || document == 0) return 0;
    if (!made_by((u32)slot, document)) return 0;                                   // M3: a document this card made, and no other
    if (op1f_cards_calling() == (u32)slot + 1u && state.view_was != 0) {           // M3: from its frame function while the train's or the UFOs' page is shown
        ++state.refused_opens;
        return 0;
    }
    u32 mode = RD32(CURRENT_MODE);
    if (mode > 0xffu) return 0;                                                     // the word holds a mode of a byte: the firmware's are below 32
    page_document[slot] = document;
    BARRIER();                                                                      // the document before the word that asks for it
    page_asked = (u32)(slot + 1) | (mode << 8);
    return 1;
}

extern "C" void op1f_page_close(const Op1fCard* card) {
    int slot = slot_of(card);
    if (slot < 0) return;
    u32 mask;
    MASK_OFF(mask);                                                                 // looked at and cleared with nothing between
    if ((page_asked & 0xffu) == (u32)(slot + 1)) page_asked = 0;
    MASK_ON(mask);
}

// The word cleared only when it is still the one looked at: a page opened meanwhile on another thread stays.
static void close_asked(u32 seen) {
    u32 mask;
    MASK_OFF(mask);
    if (page_asked == seen) page_asked = 0;
    MASK_ON(mask);
}

// The screen request, from the display thread: with interrupts masked (J5i).
static OP1F_APART void request_screen() {
    u32 mask;
    MASK_OFF(mask);
    STOCK_REQUEST_SCREEN();
    MASK_ON(mask);
}

// The screen object of 77: the one registered, else the base's own, made once from the second frame on.
static OP1F_APART u32* page_screen(u32 frame) {
    u32* screens = STOCK_SCREENS();
    u32* screen = screens ? (u32*)screens[PAGE_SCREEN] : 0;
    if (screen != 0 || state.own_made) return screen;
    if (frame < 2) return 0;                                                        // the train's unit makes its screen at the first frame, after this
    for (u32 i = 0; i < (u32)SCREEN_WORDS; ++i) state.own[i] = 0;
    STOCK_SCREEN_CTOR(state.own, PAGE_SCREEN, 0);
    state.own_made = 1;
    STOCK_LOG(3, LOG_FILE, 9, "page: screen %u made by the base for the packs' pages: no other unit made it", (u32)PAGE_SCREEN);
    return state.own;
}

static const char* closed_why(u32 why) {
    if (why == CLOSED_BY_MODE) return "the mode changed";
    if (why == CLOSED_BY_TRAIN) return "the train's or the UFOs' page was opened over it";
    if (why == CLOSED_BY_OTHER) return "another pack's page was opened";
    return "the pack closed it";
}

// Once a frame on the display thread, before the cards' frame functions: the page asked for against the page shown.
static OP1F_APART void page_frame(u32 frame, u32 mode) {
    u32 asked = page_asked;
    u32 want = 0, why = CLOSED_BY_PACK;
    if (asked != 0) {
        u32 slot = (asked & 0xffu) - 1u;
        if ((asked >> 8) != mode) {
            close_asked(asked);
            why = CLOSED_BY_MODE;
        } else if (slot < state.seen && ((state.used >> slot) & 1u) != 0 && page_document[slot] != 0) {
            want = slot + 1u;
        }
    }
    // What the train's unit shows under 77 in this mode: the UFOs where their page is open (the screen request asks
    // for them first), else the train where it is open, else nothing. Version 15 (2026-11-18): the two are told apart.
    // Version 14 took them as one yes, so that T4 pressed under a page opened over the train (the UFOs taking the
    // train's place) changed nothing it saw and the page stayed over the UFOs, as a unit showed. Both are still
    // asked each frame, as before.
    u32 chain = 0, ufos = 0;
    if (mode == 0 || mode == 1) {
        if (&op1f_chain_train_open != 0 && op1f_chain_train_open((int)mode)) chain = 1;
        if (mode == 0 && &op1f_lfotrain_open != 0 && op1f_lfotrain_open()) ufos = 1;
    }
    u32 view = ufos ? 2u : chain;
    if (view != 0 && view != state.view_was && want != 0) {                         // opened over the page, or one took the other's place under it: the page opened last is the one shown
        close_asked(asked);
        want = 0;
        why = CLOSED_BY_TRAIN;
    }
    state.view_was = view;
    if (want == state.shown) {
        if (want == 0 || state.screen == 0) return;
        if ((void*)state.screen[SCREEN_DOCUMENT] != page_document[want - 1])
            state.screen[SCREEN_DOCUMENT] = (u32)page_document[want - 1];          // the pack opened its page again with another document
        if (op1f_page_screen_mode == mode + 1u) return;
        BARRIER();                                                                  // opened again in another mode before this frame saw the change
        op1f_page_screen_mode = mode + 1u;
        request_screen();
        STOCK_LOG(3, LOG_FILE, 7, "page: '%s' opened in mode %u, its document at 0x%08x under screen %u, %s", op1f_cards.slots[want - 1].card->name, mode,
                  (u32)page_document[want - 1], (u32)PAGE_SCREEN, state.own_made ? "the base's own" : "the train's");
        return;
    }
    if (want != 0) {
        u32* screen = page_screen(frame);
        if (screen == 0) return;                                                    // the next frame
        if (state.shown == 0) {
            state.screen = screen;
            state.kept = (void*)screen[SCREEN_DOCUMENT];
        } else {
            STOCK_LOG(3, LOG_FILE, 8, "page: '%s' closed: %s; %u opened since the start", op1f_cards.slots[state.shown - 1].card->name, closed_why(CLOSED_BY_OTHER), state.opened);
        }
        screen[SCREEN_DOCUMENT] = (u32)page_document[want - 1];
        state.shown = want;
        ++state.opened;
        BARRIER();                                                                  // the document is in place before our call in the request answers 77
        op1f_page_screen_mode = mode + 1u;
        request_screen();
        STOCK_LOG(3, LOG_FILE, 7, "page: '%s' opened in mode %u, its document at 0x%08x under screen %u, %s", op1f_cards.slots[want - 1].card->name, mode,
                  (u32)page_document[want - 1], (u32)PAGE_SCREEN, state.own_made ? "the base's own" : "the train's");
        return;
    }
    op1f_page_screen_mode = 0;                                                      // our call answers the firmware's screen again
    BARRIER();
    if (state.kept != 0) state.screen[SCREEN_DOCUMENT] = (u32)state.kept;          // M3: with none kept (the base's own 77) the pack's document stays: 77 is never left without one
    u32 was = state.shown;
    state.shown = 0;
    ++state.closes;
    request_screen();
    STOCK_LOG(3, LOG_FILE, 8, "page: '%s' closed: %s; %u opened since the start", op1f_cards.slots[was - 1].card->name, closed_why(why), state.opened);
}

// The slot + 1 whose page the GUI shows now, 0 for none.
static inline u32 page_on_screen(u32 screen) {
    return screen == (u32)PAGE_SCREEN ? state.shown : 0u;
}

// ---------------------------------------------------------------- keys and knobs

// An event offered: first to the pack whose page is shown, then to every card with an input function in the list's
// order, until one takes it. The slot + 1 of the pack that took it, or 0.
static OP1F_APART u32 offer(Op1fInput* e) {
    u32 page = page_on_screen(e->screen);
    if (page != 0 && ((state.inputs >> (page - 1)) & 1u) != 0) {
        e->mine = 1;
        if (op1f_cards.slots[page - 1].card->input(e) != 0) return page;
    }
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.inputs >> i) & 1u) == 0 || i + 1 == page) continue;
        e->mine = 0;
        if (op1f_cards.slots[i].card->input(e) != 0) return i + 1;
    }
    return 0;
}

static void event_of(Op1fInput* e, u32 what, u32 key, int turn, const u32* record) {
    e->bytes = (u32)sizeof(Op1fInput);
    e->what = what;
    e->key = key;
    e->turn = turn;
    e->mode = RD32(CURRENT_MODE);
    e->screen = RD32(CURRENT_SCREEN);
    e->shift = STOCK_SHIFT() != 0 ? 1u : 0u;
    e->mine = 0;
    e->record = record;
}

// From our call at the two key-down functions (module/build_m2_input.s): the firmware's record, and whether it came
// through the shifted one. Not 0: a pack took the key, and the firmware never sees it.
extern "C" int op1f_input_down(const u32* record, int shifted) OP1F_NAMED("op1f_input_down");
extern "C" int op1f_input_down(const u32* record, int shifted) {
    if (record == 0) return 0;
    u32 key = record[0];
    input.last_down = key;
    input.last_down_ms = state.ms;
    count(&input.down);                                                             // M3: masked for the instant
    if (!checked() || state.inputs == 0) return 0;
    Op1fInput e;
    event_of(&e, shifted ? (u32)OP1F_DOWN_SHIFT : (u32)OP1F_DOWN, key, 0, record);
    u32 who = offer(&e);
    if (who == 0) return 0;
    if (key < (u32)HELD_KEYS) input.held[key] = (u8)who;
    count(&input.took_down[who - 1]);
    count(&input.taken);
    return 1;
}

// From our call at the key-up function: the key's number. An up goes where its down went.
extern "C" int op1f_input_up(int key) OP1F_NAMED("op1f_input_up");
extern "C" int op1f_input_up(int key) {
    input.last_up = (u32)key;
    input.last_up_ms = state.ms;
    count(&input.up);                                                               // M3: masked for the instant
    if ((u32)key >= (u32)HELD_KEYS || !checked()) return 0;
    u32 who = input.held[key];
    if (who == 0) return 0;                                                         // the firmware had the down
    input.held[key] = 0;
    if (who <= state.seen && ((state.inputs >> (who - 1)) & 1u) != 0) {
        Op1fInput e;
        event_of(&e, OP1F_UP, (u32)key, 0, 0);
        e.mine = page_on_screen(e.screen) == who ? 1u : 0u;
        op1f_cards.slots[who - 1].card->input(&e);                                  // told; what it answers does not matter
    }
    count(&input.took_up[who - 1]);
    count(&input.taken);
    return 1;
}

// From our call in the turn function, after the firmware's own bookkeeping: the knob (0 to 3) and the turn.
extern "C" int op1f_input_turn(int knob, int turn) OP1F_NAMED("op1f_input_turn");
extern "C" int op1f_input_turn(int knob, int turn) {
    count(&input.turns);                                                            // M3: masked for the instant
    if (!checked() || state.inputs == 0) return 0;
    Op1fInput e;
    event_of(&e, OP1F_TURN, (u32)knob, turn, 0);
    u32 who = offer(&e);
    if (who == 0) return 0;
    count(&input.took_turn[who - 1]);
    count(&input.taken);
    return 1;
}

// ---------------------------------------------------------------- the frame call

static OP1F_APART void clear_measures() {
    for (u32 i = 0; i < (u32)MOST; ++i) {
        state.measure[i].calls = 0;
        state.measure[i].sum = 0;
        state.measure[i].least = 0xffffffffu;
        state.measure[i].most = 0;
    }
}

// The lines of the frame calls: one a card with a frame function, by its pack's name; the input's, where a card has
// an input function; and how long the gate was shut.
static OP1F_APART void say(int why) {
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.framed >> i) & 1u) == 0) continue;
        const Measure* m = &state.measure[i];
        STOCK_LOG(3, LOG_FILE, 4, "cards: '%s' frame call: %u calls, %u us in all, %u least, %u most", op1f_cards.slots[i].card->name, m->calls, m->sum, m->least, m->most);
    }
    if (state.inputs != 0) {
        u32 down = input.down, up = input.up, turns = input.turns, taken = input.taken;
        STOCK_LOG(3, LOG_FILE, 10, "input: %u down, %u up, %u turns this period, %u taken by packs; since the start %u down, %u up, %u turns, %u taken; %u pages opened",
                  down - state.seen_down, up - state.seen_up, turns - state.seen_turns, taken - state.seen_taken, down, up, turns, taken, state.opened);
        for (u32 i = 0; i < state.seen; ++i) {
            if (((state.inputs >> i) & 1u) == 0) continue;
            u32 all = input.took_down[i] + input.took_up[i] + input.took_turn[i];
            if (all == state.seen_took[i]) continue;                                // nothing taken by this pack this period
            STOCK_LOG(3, LOG_FILE, 11, "input: '%s' took %u down, %u up, %u turns since the start",
                      op1f_cards.slots[i].card->name, input.took_down[i], input.took_up[i], input.took_turn[i]);
        }
    }
    if (state.quiet.silent_before)
        STOCK_LOG(3, LOG_FILE, 5, "cards quiet: these lines after %u periods without (%s); %u periods printed and %u not since the start",
                  state.quiet.silent_before, quiet_why(why), state.quiet.spoken, state.quiet.skipped);
}

// What the input's lines say was said: kept at every period, printed or not.
static OP1F_APART void input_seen() {
    state.seen_down = input.down;
    state.seen_up = input.up;
    state.seen_turns = input.turns;
    state.seen_taken = input.taken;
    for (u32 i = 0; i < state.seen; ++i) state.seen_took[i] = input.took_down[i] + input.took_up[i] + input.took_turn[i];
}

// From the measuring unit's frame function, once a frame, after our own features' frame work and before the shared
// screens are dressed: the clock, the page, then each used card's frame function in the list's order.
extern "C" void op1f_cards_frame() OP1F_NAMED("op1f_cards_frame");
extern "C" void op1f_cards_frame() {
    ready();
    u32 cycles = stamp();
    if (state.started) {
        state.cycle_acc += cycles - state.last_cycles;
        while (state.cycle_acc >= (u32)CYCLES_PER_MS) {
            state.cycle_acc -= (u32)CYCLES_PER_MS;
            ++state.ms;
        }
    } else {
        state.started = 1;
        clear_measures();
    }
    state.last_cycles = cycles;
    if (!state.settled) {                                   // M3: the cards' folders settled and made, once, before any frame function
        state.settled = 1;
        op1f_folders_settle();
    }
    if (state.framed == 0 && state.inputs == 0) return;  // no card asks for a call or for keys: nothing of stock is asked, nothing is said
    Op1fFrame now;
    now.bytes = (u32)sizeof(Op1fFrame);
    now.frame = op1f_frame_counter;
    now.ms = state.ms;
    now.mode = RD32(CURRENT_MODE);
    now.screen = RD32(CURRENT_SCREEN);
    now.shift = STOCK_SHIFT() != 0 ? 1u : 0u;
    now.popup = RD8(POWER_POPUP_SHOWN) != 0 ? 1u : 0u;
    page_frame(now.frame, now.mode);
    u32 page = page_on_screen(now.screen);
    now.page = page != 0 ? op1f_cards.slots[page - 1].card : 0;
    now.disk = RD32(DISK_OPEN) != 0 ? 1u : 0u;              // M3
    if (state.inputs != 0 && now.screen != state.last_screen && state.started > 1) {   // the line that places a screen among the keys
        u32 down_ms = input.last_down_ms, up_ms = input.last_up_ms;
        STOCK_LOG(3, LOG_FILE, 12, "input: screen %u is shown; %u keys down so far, the last %u, %u ms before; %u up, the last %u, %u ms before",
                  now.screen, input.down, input.last_down, state.ms - down_ms, input.up, input.last_up, state.ms - up_ms);
    }
    state.last_screen = now.screen;
    state.started = 2;
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.framed >> i) & 1u) == 0) continue;
        const Op1fCard* card = op1f_cards.slots[i].card;
        Measure* m = &state.measure[i];
        u32 before = stamp();
        state.calling_sp = stack_here();                    // M3: where this thread's stack stands at the call
        state.calling = i + 1;                              // M3: whose frame function runs, for the files and the text
        card->frame(&now);
        state.calling = 0;
        u32 took = microseconds(stamp() - before);
        ++m->calls;
        m->sum += took;
        if (took < m->least) m->least = took;
        if (took > m->most) m->most = took;
    }
    int why = quiet_popup(&state.quiet, now.popup != 0);    // the lines once more before the unit is off
    int period = --state.countdown <= 0;                    // a countdown, not a modulo: nothing is divided
    if (!period && !why) return;
    if (period) {
        state.countdown = PERIOD_FRAMES;
        u32 s = quiet_mix(quiet_start(), state.in_use);     // what the lines state: how many cards, and which of them had a call of a millisecond
        for (u32 i = 0; i < state.seen; ++i) {
            if ((state.used >> i) & 1u) s = quiet_mix(s, state.measure[i].most >= (u32)SLOW_US ? 1u : 0u);
        }
        s = quiet_mix(s, state.opened);                     // and how many pages were opened
        why |= quiet_period(&state.quiet, s, input.down + input.up + input.turns != state.seen_down + state.seen_up + state.seen_turns);
    }
    if (why) say(why);
    if (period) {
        clear_measures();                                   // read and cleared at every period, printed or not: a line is about one period
        input_seen();
    }
}

// ---------------------------------------------------------------- pictures

// A name against a directory's entry, whose name ends inside its 24 bytes.
static int same_name(const char* name, const char* entry) {
    for (u32 i = 0; i < (u32)ART_NAME; ++i) {
        if (name[i] != entry[i]) return 0;
        if (name[i] == 0) return 1;
    }
    return 0;
}

// A name against one remembered by its first 23 characters: the same when those are.
static int same_kept(const char* name, const char* kept) {
    for (u32 i = 0; i < (u32)NAME_KEPT; ++i) {
        if (name[i] != kept[i]) return 0;
        if (name[i] == 0) return 1;
    }
    return 1;
}

// The entry of a directory under a name, or 0. Kept a function of its own: put into its callers the compiler held
// its answer in an index register, and out of line nothing in this unit touches one.
static OP1F_APART const ArtEntry* entry_of(const ArtDir* dir, const char* name) {
    for (u32 i = 0; i < dir->count; ++i) {
        if (same_name(name, dir->entries[i].name)) return &dir->entries[i];
    }
    return 0;
}

// What a used card's pack has for a name a screen asks for, the pack's own choice asked first: the entry, or 0.
static const ArtEntry* carried(const Op1fCard* card, const char* name, int* picked) {
    if (card->pictures == 0) return 0;
    const char* wanted = name;
    if (card->pick != 0) {
        wanted = card->pick(name);
        if (wanted == 0) return 0;
    }
    *picked = wanted != name;
    return entry_of((const ArtDir*)card->pictures, wanted);
}

// Two packs carry one name: said once a name, for sixteen names. The first of the two serves it.
static OP1F_APART void twice(const char* name, const char* first, const char* second) {
    for (u32 n = 0; n < state.names; ++n) {
        if (same_kept(name, state.name_said[n])) return;
    }
    if (state.names >= (u32)NAMES_SAID) return;             // sixteen names were said: the log has what it needs
    char* kept = state.name_said[state.names];
    u32 i = 0;
    for (; i < (u32)NAME_KEPT && name[i]; ++i) kept[i] = name[i];
    kept[i] = 0;
    ++state.names;
    STOCK_LOG(2, LOG_FILE, 6, "cards: the picture %s is carried by '%s' and by '%s': '%s' serves it", name, first, second, first);
}

// The picture loader, for a name none of its four packs has: the directory and the entry of the first card that has
// it, in the list's order (FOUND, or FOUND_PICKED where the pack chose another of its pictures for the name), or 0.
// With `only`, a pack's own card: that card's directory alone, by the name itself. No file is touched here.
extern "C" int op1f_cards_picture(const char* name, const Op1fCard* only, const ArtDir** dir, const ArtEntry** entry) OP1F_NAMED("op1f_cards_picture");
extern "C" int op1f_cards_picture(const char* name, const Op1fCard* only, const ArtDir** dir, const ArtEntry** entry) {
    ready();
    int found = 0, picked = 0;
    const Op1fCard* server = 0;
    if (name == 0) return 0;
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.used >> i) & 1u) == 0) continue;
        const Op1fCard* card = op1f_cards.slots[i].card;
        if (only != 0) {
            if (card != only) continue;
            const ArtEntry* own = card->pictures ? entry_of((const ArtDir*)card->pictures, name) : 0;
            if (own == 0) return 0;
            *dir = (const ArtDir*)card->pictures;
            *entry = own;
            return FOUND;
        }
        const ArtEntry* e = carried(card, name, &picked);
        if (e == 0) continue;
        if (server != 0) {                                  // a second pack has it too: said, and the first serves it
            twice(name, server->name, card->name);
            break;
        }
        server = card;
        found = picked ? FOUND_PICKED : FOUND;
        *dir = (const ArtDir*)card->pictures;
        *entry = e;
    }
    return found;
}

// The picture loader, for a name one of its four packs serves: is it carried by a card too? For the line alone.
extern "C" void op1f_cards_also(const char* name, const char* pack) OP1F_NAMED("op1f_cards_also");
extern "C" void op1f_cards_also(const char* name, const char* pack) {
    ready();
    int picked = 0;
    if (name == 0 || pack == 0) return;
    for (u32 i = 0; i < state.seen; ++i) {
        if (((state.used >> i) & 1u) == 0) continue;
        const Op1fCard* card = op1f_cards.slots[i].card;
        if (carried(card, name, &picked) == 0) continue;
        twice(name, pack, card->name);
        return;
    }
}

// ---------------------------------------------------------------- the helpers

// The clock: milliseconds since the base's first frame. It moves once a frame.
extern "C" unsigned op1f_ms() {
    return state.ms;
}

// The document of one of the firmware's screens by its number: 0 for a number outside 1 to 85, for a number no
// screen has, and for a screen whose document stock has not loaded yet.
extern "C" void* op1f_screen(int number) {
    if (number < (int)FIRST_SCREEN || number > (int)LAST_SCREEN) return 0;
    u32* screens = STOCK_SCREENS();
    u32* screen = screens ? (u32*)screens[number] : 0;
    return screen ? (void*)screen[SCREEN_DOCUMENT] : 0;
}

// A part of a document by its name: 0 when the document has none of that name, and for no document or no name.
extern "C" void* op1f_node(void* document, const char* name) {
    if (document == 0 || name == 0) return 0;               // stock's lookup stops the unit on either
    u32* screens = STOCK_SCREENS();
    if (screens) {
        for (int number = FIRST_SCREEN; number <= (int)LAST_SCREEN; ++number) {
            u32* screen = (u32*)screens[number];
            if (screen != 0 && (void*)screen[SCREEN_DOCUMENT] == document) return STOCK_FIND_NODE(screen, name, 1);
        }
    }
    u32 held[4] = { 0, (u32)document, 0, 0 };               // no screen holds it: the lookup reads the word at +4 and nothing else
    return STOCK_FIND_NODE(held, name, 1);
}

// A part shown or hidden, with what is inside it.
extern "C" void op1f_show(void* part, int on) {
    if (part) ((u8*)part)[NODE_DISPLAY] = (u8)(on ? 1 : 0);
}

// A part's matrix set: nine words in 1024ths, [a, b, 0, c, d, 0, e, f, 1024].
extern "C" void op1f_move(void* part, const int* matrix) {
    if (part && matrix) STOCK_APPLY_MATRIX(part, matrix);
}

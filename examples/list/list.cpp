// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step M3, the third example pack, list (docs/base.md, level 3): a folder of its own that the
// computer sees in the disk mode, and a page of its own that lists the names of the files in it, through level 3 of
// the base's interface (module/build_m3_card.h). It changes no place of the firmware and names no address of it.
//
// What it shows a pack author:
//   one folder, `list`, listed in its card (examples/list/recipe.json): the base makes it on the unit's disk, user/pf/list/,
//     and shows it to the computer in the disk mode, where files can be copied into it and deleted from it;
//   shift and the help key, in any mode: its page opens, or closes when it is the one shown. On the page: the word
//     list, how many files the folder holds, and the names of up to seven of them, in the firmware's own letters
//     (op1f_text into placeholders of its picture); the blue knob scrolls when there are more;
//   when the page closes, it writes list.txt into the same folder (op1f_write): the folder's path and the names it
//     showed, one a line, so that the computer finds a file the pack made. It writes only when the names changed
//     since its last write, and while the disk mode is open it waits and writes when the disk mode has closed.
// Its picture, list.svg, is made on the unit by instructions of the second kind: the head and the last tag of the
// unit's own tempo.svg, by reference, around a drawing of ours (examples/list/list.svg.make).
//
// The folder is read (op1f_list) when the page opens and when the disk mode closes while it is open, never at every
// frame: a list is a walk of the disk's folder. The input function runs on the thread that handles keys: it writes
// three words the frame function reads (a count of the page's keys, the blue knob's steps as a total that only it
// writes, and a count of turns), calls op1f_page_open and op1f_page_close, and writes no line. The frame function
// writes the log's line through the gate of build_q_quiet.h. Nothing is divided.
//
// Compiled for the PC as it is, with OP1F_HOST defined, for tests this tree does not hold.

#include "build_m3_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;
typedef unsigned char u8;

enum { KEY_HELP = 0, KNOB_BLUE = 0, ROWS = 7, FOLDER = 0, NAMES_MOST = 64, PERIOD_FRAMES = 256, WIDTH = 400 };
enum { NAMES_ROOM = NAMES_MOST * OP1F_FILE_NAME, WRITTEN_ROOM = NAMES_ROOM + 128, LINE_ROOM = 48 };
enum { FRAME_LEVEL_3 = 36, INPUT_LEVEL_2 = 36 };

static const char LOG_FILE[] = "op1f/list.cpp";           // the log shows "list"
static const char WRITTEN[] = "list.txt";
static const char* const ROW_PARTS[ROWS] = { "list_row1", "list_row2", "list_row3", "list_row4", "list_row5", "list_row6", "list_row7" };

// The pack's own card, written by tools/pack_build.py from examples/list/recipe.json (examples/list/list_card.s).
extern "C" const Op1fCard op1f_card_list OP1F_NAMED("op1f_card_list");

// Written by the input function, read by the frame function: single words.
static volatile u32 keys, turns;                          // the page's keys taken; the blue knob's turns taken
static volatile int steps;                                // the blue knob's steps since the start, a total only the input function writes
static void* volatile document;                           // the page's document, once the frame function made it

// The frame function's own.
static struct {
    u32 asked;                                            // the picture was asked for (once, whatever came of it)
    u32 shown, frames, opened, closed;                    // the page shown at the last frame; frames it was shown; openings and closings
    u32 disk;                                             // the disk mode was open at the last frame
    int count;                                            // the names listed at the last look, or what op1f_list answered when it did not list
    u32 looks;
    const char* name[NAMES_MOST];                         // each name's place in `names`
    char names[NAMES_ROOM];
    int first, steps_seen;                                // the first row shown; the knob's total as last applied
    u32 sum, written_sum;                                 // the names' sum at the last look, and at the last write that landed
    u32 written_once;                                     // a write has landed since the start: until then any names, none among them, are to be written
    u32 to_write;                                         // a write waits: the page closed and the names changed
    u32 writes, refused, failed, texts_missed;
    char line[LINE_ROOM];
    char text[WRITTEN_ROOM];
    int countdown;
    QuietGate quiet;
} state;

// Offered every key and knob event by the base, on the thread that handles keys: not 0 to take it.
extern "C" int list_input(const Op1fInput* e) OP1F_NAMED("list_input");
extern "C" int list_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2) return 0;
    if (e->what == (u32)OP1F_DOWN_SHIFT && e->key == (u32)KEY_HELP) {          // the page's own keys, in any mode
        if (document == 0) return 0;                                           // no page could be made: the keys stay the firmware's
        ++keys;
        if (e->mine) op1f_page_close(&op1f_card_list);
        else op1f_page_open(&op1f_card_list, document);
        return 1;
    }
    if (e->what == (u32)OP1F_UP) return 1;                                     // the up of a key whose down this pack took
    if (!e->mine) return 0;                                                    // the rest only while the page is shown
    if (e->what == (u32)OP1F_TURN && e->key == (u32)KNOB_BLUE) {
        steps = steps + (e->turn >> 10);                                       // 1024 a step of the knob; more than 0 scrolls down
        ++turns;
        return 1;
    }
    return 0;
}

// The folder read: the names one after another, each with its end, and their sum.
static void look() {
    ++state.looks;
    int got = op1f_list(&op1f_card_list, FOLDER, state.names, NAMES_ROOM);
    if (got > (int)NAMES_MOST) got = NAMES_MOST;                               // short names fit the room more than 64 times: the page shows the first 64
    state.count = got;
    u32 sum = 0, at = 0;
    for (int i = 0; i < got && i < (int)NAMES_MOST; ++i) {
        state.name[i] = state.names + at;
        while (state.names[at]) sum = sum * 31u + (u8)state.names[at++];
        sum = sum * 31u + 10u;
        ++at;
    }
    state.sum = got < 0 ? 0xffffffffu : sum;
    state.first = 0;
}

static u32 put(char* out, u32 at, u32 room, const char* text) {
    while (*text && at + 1 < room) out[at++] = *text++;
    out[at] = 0;
    return at;
}

static u32 put_number(char* out, u32 at, u32 room, u32 n) {
    char digits[12];
    u32 k = 0;
    do {
        u32 tens = 0;
        while (n >= 10u) { n -= 10u; ++tens; }                                 // by subtraction: nothing is divided
        digits[k++] = (char)('0' + n);
        n = tens;
    } while (n != 0u && k < sizeof digits);
    while (k > 0 && at + 1 < room) out[at++] = digits[--k];
    out[at] = 0;
    return at;
}

// The page's words: the title and the seven rows.
static void dress() {
    void* d = document;
    u32 at = put(state.line, 0, LINE_ROOM, "list: ");
    if (state.count < 0) {
        at = put(state.line, at, LINE_ROOM, "the folder could not be read");
    } else {
        at = put_number(state.line, at, LINE_ROOM, (u32)state.count);
        at = put(state.line, at, LINE_ROOM, state.count == 1 ? " file" : " files");
        if (state.count > (int)ROWS) {
            at = put(state.line, at, LINE_ROOM, ", ");
            at = put_number(state.line, at, LINE_ROOM, (u32)state.first + 1u);
            at = put(state.line, at, LINE_ROOM, " to ");
            put_number(state.line, at, LINE_ROOM, (u32)(state.first + (int)ROWS));
        }
    }
    if (!op1f_text(d, "list_title", state.line, WIDTH)) ++state.texts_missed;
    for (int r = 0; r < (int)ROWS; ++r) {
        int i = state.first + r;
        const char* row = i < state.count ? state.name[i] : "";
        if (!op1f_text(d, ROW_PARTS[r], row, WIDTH)) ++state.texts_missed;
    }
}

// list.txt: the folder's path and the names shown, one a line. Written when a write waits and the disk mode is shut.
static void write_list() {
    char path[OP1F_FILE_NAME * 2];
    u32 at = 0;
    if (op1f_path(&op1f_card_list, FOLDER, 0, path, sizeof path) > 0) {
        at = put(state.text, 0, WRITTEN_ROOM, "the files list.pack showed in ");
        at = put(state.text, at, WRITTEN_ROOM, path);
        at = put(state.text, at, WRITTEN_ROOM, ":\n");
    }
    for (int i = 0; i < state.count; ++i) {
        at = put(state.text, at, WRITTEN_ROOM, state.name[i]);
        at = put(state.text, at, WRITTEN_ROOM, "\n");
    }
    int answer = op1f_write(&op1f_card_list, FOLDER, WRITTEN, state.text, at);
    if (answer == (int)at) {
        ++state.writes;
        state.written_sum = state.sum;
        state.written_once = 1;
        state.to_write = 0;
    } else if (answer == OP1F_NOT_NOW) {
        ++state.refused;                                                       // the disk mode is open: written when it closes
    } else {
        ++state.failed;
        state.to_write = 0;                                                    // tried, and said by the base's line: not again until the names change
    }
}

static const char* picture_said() {
    if (!state.asked) return "not asked for yet";
    return document != 0 ? "made" : "not made";
}

// Once a frame, on the display thread.
extern "C" void list_frame(const Op1fFrame* now) OP1F_NAMED("list_frame");
extern "C" void list_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_3) return;
    if (!state.asked) {                                                        // the first frame: the page's document, once
        state.asked = 1;
        document = op1f_picture(&op1f_card_list, "list.svg");
        state.count = 0;
    }
    u32 shown = now->page == &op1f_card_list ? 1u : 0u;
    if (shown && !state.shown) {                                               // the page has just opened: the folder read
        ++state.opened;
        look();
    }
    if (shown && state.disk && !now->disk) look();                             // the disk mode closed under the page: read again
    if (!shown && state.shown) {                                               // the page has just closed: list.txt, if the names changed
        ++state.closed;
        if (state.count >= 0 && (!state.written_once || state.sum != state.written_sum)) state.to_write = 1;   // the first close writes, an empty folder too
    }
    state.shown = shown;
    state.disk = now->disk;
    if (shown) {
        ++state.frames;
        int moved = steps - state.steps_seen;
        state.steps_seen += moved;
        int last = state.count > (int)ROWS ? state.count - (int)ROWS : 0;
        int first = state.first + moved;
        state.first = first < 0 ? 0 : first > last ? last : first;
        dress();
    } else {
        state.steps_seen = steps;
    }
    if (state.to_write && !now->disk) write_list();
    int why = quiet_popup(&state.quiet, now->popup != 0);
    int period = --state.countdown <= 0;
    if (!period && !why) return;
    if (period) {
        state.countdown = PERIOD_FRAMES;
        u32 s = quiet_mix(quiet_start(), state.opened);
        s = quiet_mix(s, state.closed);
        s = quiet_mix(s, (u32)state.count);
        s = quiet_mix(s, state.looks);
        s = quiet_mix(s, keys);
        s = quiet_mix(s, turns);
        s = quiet_mix(s, state.writes + state.refused + state.failed);
        why |= quiet_period(&state.quiet, s, shown);
    }
    if (!why) return;
    op1f_log(3, LOG_FILE, 1, "list: picture %s; opened %u times, closed %u; %u frames shown; %d file(s) at the last of %u looks, the first row %d; %u keys, %u turns",
             picture_said(), state.opened, state.closed, state.frames, state.count, state.looks, state.first + 1, keys, turns);
    op1f_log(3, LOG_FILE, 2, "list: list.txt written %u times, %u refused while the disk mode was open, %u failed; %u text(s) not set", state.writes, state.refused,
             state.failed, state.texts_missed);
    if (state.quiet.silent_before)                                             // how long the gate held the lines, as the base's lines say it
        op1f_log(3, LOG_FILE, 3, "list quiet: these lines after %u periods held (%s); %u printed, %u held", state.quiet.silent_before, quiet_why(why),
                 state.quiet.spoken, state.quiet.skipped);
}

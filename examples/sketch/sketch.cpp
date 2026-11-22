// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R1, the example pack `sketch`: a drawing board, through level 4 of the base's interface (module/build_r1_card.h).
// It changes no place of the firmware and names no address of it.
//
// What it shows a pack author:
//   shift and the metronome key, in any mode: its page is shown with op1f_page_show, or closed when it is the one
//     shown. Its page is its own picture, black, with the title at the top centre: `sketch` and the count of lines,
//     set centred with op1f_text_set, and a small ochre bar under it as wide as the text (op1f_text_width);
//   a small cross in the middle. The blue knob moves it left and right, the second knob up and down, four pixels a
//     step; each move draws a line from where it was. Up to 256 lines are kept (the oldest goes when a new one comes
//     past them), painted again at every frame with op1f_paint_line, two pixels wide, in the field palette's light
//     grey. The cross is painted with op1f_paint_mask from a small round shape with soft edges of the pack's own;
//   the red knob's press clears the board; the third knob's press shows the same board on the base's blank page
//     (op1f_page_show with no document), where no title stands, and the next press its own picture again;
//   while its page is shown it takes the four knobs' turns, presses and clicks, and nothing else;
//   it keeps the drawing in a folder of its own, `sketch`, shown in the disk mode: sketch.bin is read at the first
//     frame, written when the page is no longer shown and something changed (while the disk mode is open it waits),
//     and removed with op1f_remove when the board is cleared.
// Its picture, sketchpad.svg, is made on the unit by instructions of the second kind: the head and the last tag of the
// unit's own tempo.svg, by reference, around a black box and one placeholder for the title.
//
// The input function runs on the thread that handles keys: it counts in single words the frame function reads (the
// two knobs' steps as totals only it writes, the clears, the keys), calls op1f_page_show and op1f_page_close, and
// writes no line. The frame function does the rest: the lines, the file, the text and the painting. Its lines are one
// for each thing a user does that the log should place (the page shown and no longer shown, the count of lines at
// every sixteen, the board cleared and the file removed, the blank page, the file read and written), and a line of
// counts at the period through the gate of build_q_quiet.h. Nothing is divided.
//
// sketch.bin: a word "SKTC", the count of lines (0 to 256), the cross's x and y as two half-words, then each line,
// oldest first, as four half-words x0, y0, x1, y1: 12 + 8 x the count bytes, the unit's own byte order.

#include "build_r1_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

enum { KEY_METRONOME = 1, KNOB_BLUE = 0, KNOB_SECOND = 1, KNOB_THIRD = 2, KNOB_RED = 3, KNOBS = 4, PRESS0 = 61, CLICK0 = 54 };
enum { STEP_SHIFT = 2, X_LEAST = 4, X_MOST = 476, Y_LEAST = 36, Y_MOST = 220, X_START = 240, Y_START = 128 };
enum { LINES_MOST = 256, PEN = 2, SAID_EVERY = 16, FOLDER = 0, PERIOD_FRAMES = 256 };
enum { CROSS = 11, CROSS_HALF = 5, TITLE_MIDDLE = 240, BAR_Y = 25, BAR_TALL = 2, TITLE_ROOM = 24 };
enum { FRAME_LEVEL_4 = 40, INPUT_LEVEL_2 = 36 };
enum { FILE_MAGIC = 0x43544b53u /* "SKTC" */, FILE_HEAD = 12, FILE_LINE = 8, FILE_MOST = FILE_HEAD + FILE_LINE * LINES_MOST };

static const char LOG_FILE[] = "op1f/sketch.cpp";         // the log shows "sketch"
static const char SAVED[] = "sketch.bin";
static const char TITLE[] = "sketch_title";
static const unsigned LIGHT_GREY = OP1F_COLOUR(0xcd, 0xd0, 0xc2);   // the field palette's light grey, ochre and light blue
static const unsigned OCHRE = OP1F_COLOUR(0x8a, 0x65, 0x3c);
static const unsigned LIGHT_BLUE = OP1F_COLOUR(0x4c, 0x7d, 0xa8);

// The cross: two strokes with round ends through the middle, soft at their edges, as cover for op1f_paint_mask.
static const u8 CROSS_COVER[CROSS * CROSS] = {
      0,   0,   0,   0,  68, 183,  68,   0,   0,   0,   0,
      0,   0,   0,   0, 187, 255, 187,   0,   0,   0,   0,
      0,   0,   0,   0, 191, 255, 191,   0,   0,   0,   0,
      0,   0,   0,   0, 191, 255, 191,   0,   0,   0,   0,
     68, 187, 191, 191, 239, 255, 239, 191, 191, 187,  68,
    183, 255, 255, 255, 255, 255, 255, 255, 255, 255, 183,
     68, 187, 191, 191, 239, 255, 239, 191, 191, 187,  68,
      0,   0,   0,   0, 191, 255, 191,   0,   0,   0,   0,
      0,   0,   0,   0, 191, 255, 191,   0,   0,   0,   0,
      0,   0,   0,   0, 187, 255, 187,   0,   0,   0,   0,
      0,   0,   0,   0,  68, 183,  68,   0,   0,   0,   0,
};

// The pack's own card, written by tools/pack_build.py from its recipe (examples/sketch/sketch_card.s).
extern "C" const Op1fCard op1f_card_sketch OP1F_NAMED("op1f_card_sketch");

// Written by the input function, read by the frame function: single words.
static volatile u32 keys, turns, presses;                 // the way in's keys taken; the knobs' turns and presses taken
static volatile int steps_x, steps_y;                     // the blue and the second knob's steps since the start, totals only the input function writes
static volatile u32 clears;                               // the red knob's presses since the start
static volatile u32 blank;                                // 1 while the board is shown on the base's blank page
static void* volatile document;                           // the page's document, once the frame function made it

struct Line { u16 x0, y0, x1, y1; };

// The frame function's own.
static struct {
    u32 asked;                                            // the picture was asked for and the file read (once)
    u32 shown, opened, closed;                            // the page shown at the last frame; how often it came and went
    int x, y;                                             // the cross
    Line lines[LINES_MOST];                               // a ring: the oldest at `first`
    u32 first, count, drawn, said;                        // the ring; lines drawn since the start; the count at the last line that said it
    int seen_x, seen_y;                                   // the knobs' totals as last applied
    u32 clears_seen, cleared, blank_seen;
    u32 changed;                                          // the board differs from sketch.bin
    u32 to_write, to_remove;                              // a write, a removal wait for the disk mode to close
    u32 writes, waits, failed;
    char title[TITLE_ROOM];
    u32 file[FILE_MOST >> 2];                            // sketch.bin, read and written on a word
    int countdown;
    QuietGate quiet;
} state;

// Offered every key and knob event by the base, on the thread that handles keys: not 0 to take it.
extern "C" int sketch_input(const Op1fInput* e) OP1F_NAMED("sketch_input");
extern "C" int sketch_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2) return 0;
    if (e->what == (u32)OP1F_DOWN_SHIFT && e->key == (u32)KEY_METRONOME) {     // the way in, in any mode
        void* d = document;
        if (d == 0) return 0;                                                  // no page could be made: the keys stay the firmware's
        ++keys;
        if (e->mine) op1f_page_close(&op1f_card_sketch);
        else op1f_page_show(&op1f_card_sketch, blank ? 0 : d, 0);
        return 1;
    }
    if (e->what == (u32)OP1F_UP) return 1;                                     // the up of a key whose down this pack took
    if (!e->mine) return 0;                                                    // the rest only while the page is shown
    if (e->what == (u32)OP1F_TURN) {
        if (e->key >= (u32)KNOBS) return 0;
        if (e->key == (u32)KNOB_BLUE) steps_x = steps_x + (e->turn >> 10);     // 1024 a step; more than 0 to the right
        else if (e->key == (u32)KNOB_SECOND) steps_y = steps_y - (e->turn >> 10);   // more than 0 up
        ++turns;
        return 1;
    }
    if (e->key >= (u32)PRESS0 && e->key < (u32)(PRESS0 + KNOBS)) {            // a knob pressed, with shift or without
        ++presses;
        if (e->key == (u32)(PRESS0 + KNOB_RED)) {
            clears = clears + 1u;
        } else if (e->key == (u32)(PRESS0 + KNOB_THIRD)) {
            u32 other = blank ^ 1u;
            if (op1f_page_show(&op1f_card_sketch, other ? 0 : document, 0)) blank = other;
        }
        return 1;
    }
    if (e->key >= (u32)CLICK0 && e->key < (u32)(CLICK0 + KNOBS)) return 1;     // the click that follows a press
    return 0;
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

static int clamp(int v, int least, int most) {
    return v < least ? least : v > most ? most : v;
}

// One line more on the board, from where the cross was to where it is; the oldest goes when the ring is full.
static void add(int x0, int y0, int x1, int y1) {
    Line* to = &state.lines[(state.first + state.count) & (LINES_MOST - 1)];
    if (state.count == (u32)LINES_MOST) state.first = (state.first + 1u) & (LINES_MOST - 1);
    else ++state.count;
    to->x0 = (u16)x0; to->y0 = (u16)y0; to->x1 = (u16)x1; to->y1 = (u16)y1;
    ++state.drawn;
    state.changed = 1;
    if (state.count >= state.said + (u32)SAID_EVERY) {
        state.said = state.count;
        op1f_log(3, LOG_FILE, 5, "sketch: %u lines on the board", state.count);
    }
}

// The cross moved by so many steps along one knob's way, and the line it leaves.
static void move(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    int x = clamp(state.x + dx * (1 << STEP_SHIFT), X_LEAST, X_MOST), y = clamp(state.y + dy * (1 << STEP_SHIFT), Y_LEAST, Y_MOST);
    if (x == state.x && y == state.y) return;                                  // at the board's edge
    add(state.x, state.y, x, y);
    state.x = x;
    state.y = y;
}

static void empty_board() {
    state.first = 0;
    state.count = 0;
    state.said = 0;
    state.x = X_START;
    state.y = Y_START;
}

static u32 word_at(u32 at) {
    return state.file[at >> 2];
}

static u32 half_at(u32 at) {
    return ((const u16*)state.file)[at >> 1];
}

// sketch.bin read at the first frame, held whole before anything of it is taken.
static void load() {
    int got = op1f_read(&op1f_card_sketch, FOLDER, SAVED, state.file, sizeof state.file);
    if (got < 0) {
        op1f_log(3, LOG_FILE, 11, "sketch: no sketch.bin read at the start (answer %d): the board starts empty", got);
        return;
    }
    u32 n = (u32)got, count = n >= (u32)FILE_HEAD ? word_at(4) : 0u;
    int fits = n >= (u32)FILE_HEAD && word_at(0) == (u32)FILE_MAGIC && count <= (u32)LINES_MOST && n == (u32)FILE_HEAD + count * (u32)FILE_LINE;
    for (u32 at = 8; fits && at < n; at += 4) {
        if (half_at(at) > (u32)X_MOST || half_at(at + 2) > (u32)Y_MOST || half_at(at + 2) < (u32)Y_LEAST || half_at(at) < (u32)X_LEAST) fits = 0;
    }
    if (!fits) {
        op1f_log(2, LOG_FILE, 12, "sketch: sketch.bin of %u bytes is not a board of this pack's: the board starts empty", n);
        return;
    }
    state.x = (int)half_at(8);
    state.y = (int)half_at(10);
    for (u32 i = 0; i < count; ++i) {
        u32 at = (u32)FILE_HEAD + i * (u32)FILE_LINE;
        Line* to = &state.lines[i];
        to->x0 = (u16)half_at(at); to->y0 = (u16)half_at(at + 2); to->x1 = (u16)half_at(at + 4); to->y1 = (u16)half_at(at + 6);
    }
    state.count = count;
    state.said = count;
    op1f_log(3, LOG_FILE, 10, "sketch: sketch.bin read at the start: %u lines on the board", count);
}

static void save() {
    u16* half = (u16*)state.file;
    state.file[0] = (u32)FILE_MAGIC;
    state.file[1] = state.count;
    half[4] = (u16)state.x;
    half[5] = (u16)state.y;
    for (u32 i = 0; i < state.count; ++i) {
        const Line* from = &state.lines[(state.first + i) & (LINES_MOST - 1)];
        u16* to = half + ((u32)FILE_HEAD >> 1) + i * ((u32)FILE_LINE >> 1);
        to[0] = from->x0; to[1] = from->y0; to[2] = from->x1; to[3] = from->y1;
    }
    u32 bytes = (u32)FILE_HEAD + state.count * (u32)FILE_LINE;
    int answer = op1f_write(&op1f_card_sketch, FOLDER, SAVED, state.file, bytes);
    if (answer == (int)bytes) {
        ++state.writes;
        state.changed = 0;
        state.to_write = 0;
        op1f_log(3, LOG_FILE, 8, "sketch: sketch.bin written: %u lines in %u bytes", state.count, bytes);
    } else if (answer == OP1F_NOT_NOW) {
        ++state.waits;                                                         // written when it may be
    } else {
        ++state.failed;
        state.to_write = 0;                                                    // the base's line says why; tried again at the next change
        op1f_log(2, LOG_FILE, 9, "sketch: sketch.bin could not be written (answer %d)", answer);
    }
}

static void remove_saved() {
    int answer = op1f_remove(&op1f_card_sketch, FOLDER, SAVED);
    if (answer == OP1F_NOT_NOW) {
        ++state.waits;
        return;
    }
    state.to_remove = 0;
    op1f_log(3, LOG_FILE, 7, "sketch: sketch.bin removed with the board: %s (answer %d)",
             answer == 0 ? "it is gone" : answer == OP1F_NO_FILE ? "there was none" : "it could NOT be removed", answer);
}

static const char* picture_said() {
    if (!state.asked) return "not asked for yet";
    return document != 0 ? "made" : "not made";
}

static const char* view_said(u32 on_blank) {
    return on_blank ? "the base's blank page" : "its own picture";
}

// The board painted onto this frame's picture: the lines, the bar under the title, the cross.
static void paint(void* canvas, u32 on_blank) {
    for (u32 i = 0; i < state.count; ++i) {
        const Line* l = &state.lines[(state.first + i) & (LINES_MOST - 1)];
        op1f_paint_line(canvas, l->x0, l->y0, l->x1, l->y1, PEN, LIGHT_GREY);
    }
    if (!on_blank && document != 0) {
        int wide = op1f_text_width(document, TITLE);
        if (wide > 0) op1f_paint_box(canvas, TITLE_MIDDLE - (wide >> 1), BAR_Y, wide, BAR_TALL, OCHRE);
    }
    op1f_paint_mask(canvas, state.x - CROSS_HALF, state.y - CROSS_HALF, CROSS, CROSS, CROSS_COVER, CROSS, LIGHT_BLUE);
}

// Once a frame, on the display thread.
extern "C" void sketch_frame(const Op1fFrame* now) OP1F_NAMED("sketch_frame");
extern "C" void sketch_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    if (!state.asked) {                                                        // the first frame: the page's document and the board, once
        state.asked = 1;
        empty_board();
        document = op1f_picture(&op1f_card_sketch, "sketchpad.svg");
        load();
    }
    u32 shown = now->page == &op1f_card_sketch ? 1u : 0u;
    u32 on_blank = blank;
    if (shown && !state.shown) {
        ++state.opened;
        state.blank_seen = on_blank;
        op1f_log(3, LOG_FILE, 3, "sketch: its page is shown, on %s; %u lines on the board", view_said(on_blank), state.count);
    }
    if (shown && on_blank != state.blank_seen) {
        state.blank_seen = on_blank;
        op1f_log(3, LOG_FILE, 6, "sketch: the board is shown on %s", view_said(on_blank));
    }
    u32 c = clears;
    if (c != state.clears_seen) {                                              // the red knob: the board cleared and its file removed
        state.clears_seen = c;
        ++state.cleared;
        op1f_log(3, LOG_FILE, 4, "sketch: the board cleared, %u lines taken off", state.count);
        empty_board();
        state.changed = 0;
        state.to_write = 0;
        state.to_remove = 1;
    }
    int dx = steps_x - state.seen_x, dy = steps_y - state.seen_y;
    state.seen_x += dx;
    state.seen_y += dy;
    move(dx, 0);
    move(0, dy);
    if (!shown && state.shown) {                                               // no longer shown: the file, if the board changed
        ++state.closed;
        if (state.changed) state.to_write = 1;
        op1f_log(3, LOG_FILE, 2, "sketch: its page is no longer shown; %u lines on the board, %s", state.count,
                 state.changed ? "sketch.bin to be written" : "sketch.bin as it was");
    }
    state.shown = shown;
    if (!now->disk) {                                                          // the computer owns the folder while the disk mode is open
        if (state.to_remove) remove_saved();
        if (state.to_write) save();
    }
    if (shown) {
        void* d = document;
        if (d != 0) {
            u32 at = put(state.title, 0, TITLE_ROOM, "sketch ");
            put_number(state.title, at, TITLE_ROOM, state.count);
            op1f_text_set(d, TITLE, state.title, 0, OP1F_CENTRE);
        }
        if (now->canvas != 0) paint(now->canvas, on_blank);
    }
    int why = quiet_popup(&state.quiet, now->popup != 0);
    int period = --state.countdown <= 0;
    if (!period && !why) return;
    if (period) {
        state.countdown = PERIOD_FRAMES;
        u32 s = quiet_mix(quiet_start(), state.opened);
        s = quiet_mix(s, state.closed);
        s = quiet_mix(s, state.count);
        s = quiet_mix(s, state.drawn);
        s = quiet_mix(s, state.cleared);
        s = quiet_mix(s, keys + turns + presses);
        s = quiet_mix(s, state.writes + state.waits + state.failed);
        why |= quiet_period(&state.quiet, s, shown);
    }
    if (!why) return;
    op1f_log(3, LOG_FILE, 1, "sketch: picture %s; shown %u times; %u lines on the board, %u drawn, %u clears", picture_said(), state.opened, state.count,
             state.drawn, state.cleared);
    op1f_log(3, LOG_FILE, 14, "sketch: sketch.bin written %u times, %u waits, %u failed; %u keys, %u turns, %u presses", state.writes, state.waits, state.failed,
             keys, turns, presses);
    if (state.quiet.silent_before)
        op1f_log(3, LOG_FILE, 13, "sketch quiet: these lines after %u periods held (%s); %u printed, %u held", state.quiet.silent_before, quiet_why(why),
                 state.quiet.spoken, state.quiet.skipped);
}

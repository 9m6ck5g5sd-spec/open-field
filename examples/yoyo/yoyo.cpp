// Step M2, the second example pack, yoyo (docs/base.md, level 2): a page of its own, with keys
// and a knob, through level 2 of the base's interface (module/build_m2_card.h). It changes no place of the firmware
// and names no address of it.
//
// Its picture, yoyo.svg, is its own: a grey string hanging from a short bar at the top of the screen's middle, five
// dim marks along it, and a blue yoyo on it, the blue knob's colour (made on the unit by examples/yoyo/yoyo.svg.make). At its
// first frame the pack makes it into a document (op1f_picture) that no screen of the firmware holds, and finds the
// yoyo in it by its name (op1f_node, by its way for such a document).
//
// The keys:
//   shift and the metronome key, in any mode: the page opens, or closes when it is the one shown. The metronome key
//     alone stays the firmware's;
//   while the page is shown: the blue knob moves the yoyo up and down, four pixels a step, 56 at most either way;
//     pressing the blue knob brings it back to the middle (the click the firmware makes when a pressed knob comes up
//     unturned; the press itself is taken too, so the firmware sees nothing of the blue knob while the page shows).
// Every other key and knob goes on to the firmware as it always did. Changing mode closes the page (the base does).
//
// The frame function moves the yoyo toward where the knob put it at 240 pixels a second, on the base's clock
// (op1f_ms), and writes its line on the log through the gate of build_q_quiet.h, with a second line for how long the
// gate held it when it did. Where its picture could not be made, shift and the metronome key stay the firmware's.
// The input function runs on the
// thread that handles keys: it writes three words the frame function reads (where the yoyo should be, and two
// counts), calls the base's op1f_page_open and op1f_page_close, and writes no line. Nothing is divided.
//
// Compiled for the PC as it is, with OP1F_HOST defined, for tests this tree does not hold.

#include "build_m2_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;

enum { KEY_METRONOME = 1, KEY_CLICK_BLUE = 54, KEY_PRESS_BLUE = 61, KNOB_BLUE = 0 };
enum { STEP_PX = 4, REACH_PX = 56, SPEED_Q10_PER_MS = 246, Q = 1024, PERIOD_FRAMES = 256 };   // 240 pixels a second is 245.76 1024ths a millisecond
enum { FRAME_LEVEL_1 = 28, FRAME_LEVEL_2 = 32, INPUT_LEVEL_2 = 36 };

static const char LOG_FILE[] = "op1f/yoyo.cpp";           // the log shows "yoyo"

// The pack's own card, written by tools/pack_build.py from examples/yoyo/recipe.json (examples/yoyo/yoyo_card.s).
extern "C" const Op1fCard op1f_card_yoyo OP1F_NAMED("op1f_card_yoyo");

// Written by the input function, read by the frame function: single words.
static volatile int target;                               // where the yoyo should be, in pixels from the middle, down is more
static volatile u32 turns, recentres, keys;               // the blue knob's turns taken, the clicks that brought it back, the shift-metronome presses taken
static void* volatile document;                           // the page's document, once the frame function made it

// The frame function's own.
static struct {
    u32 asked;                                            // the picture was asked for (once, whatever came of it)
    void* yoyo;                                           // the yoyo's part of the document
    int at;                                               // where the yoyo is, in 1024ths of a pixel from the middle
    int moved;                                            // where the yoyo was put last, so that it is moved only when that changes
    u32 last_ms;
    u32 shown, frames, opened, closed;                    // the page shown at the last frame; frames it was shown; its openings and closings
    int countdown;
    QuietGate quiet;
} state;

static int clamp(int px) {
    if (px > (int)REACH_PX) return REACH_PX;
    if (px < -(int)REACH_PX) return -(int)REACH_PX;
    return px;
}

// Offered every key and knob event by the base, on the thread that handles keys: not 0 to take it.
extern "C" int yoyo_input(const Op1fInput* e) OP1F_NAMED("yoyo_input");
extern "C" int yoyo_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2) return 0;
    if (e->what == (u32)OP1F_DOWN_SHIFT && e->key == (u32)KEY_METRONOME) {     // the page's own keys, in any mode
        if (document == 0) return 0;                                           // no page could be made: the keys stay the firmware's
        ++keys;
        if (e->mine) op1f_page_close(&op1f_card_yoyo);
        else if (document != 0) op1f_page_open(&op1f_card_yoyo, document);
        return 1;
    }
    if (e->what == (u32)OP1F_UP) return 1;                                     // the up of a key whose down this pack took
    if (!e->mine) return 0;                                                    // the rest only while the page is shown
    if (e->what == (u32)OP1F_TURN && e->key == (u32)KNOB_BLUE) {
        target = clamp(target + (e->turn >> 10) * (int)STEP_PX);              // 1024 a step of the knob
        ++turns;
        return 1;
    }
    if ((e->what == (u32)OP1F_DOWN || e->what == (u32)OP1F_DOWN_SHIFT) && (e->key == (u32)KEY_PRESS_BLUE || e->key == (u32)KEY_CLICK_BLUE)) {
        if (e->key == (u32)KEY_CLICK_BLUE) {
            target = 0;
            ++recentres;
        }
        return 1;
    }
    return 0;
}

// The yoyo moved toward where the knob put it, by at most what the time since the last frame allows.
static void move_yoyo(u32 elapsed) {
    int want = target * (int)Q;
    int reach = (int)(elapsed > 1000u ? 1000u : elapsed) * (int)SPEED_Q10_PER_MS;
    if (state.at < want) state.at = state.at + reach > want ? want : state.at + reach;
    else if (state.at > want) state.at = state.at - reach < want ? want : state.at - reach;
    if (state.yoyo == 0 || state.at == state.moved) return;
    int matrix[9] = { Q, 0, 0, 0, Q, 0, 0, state.at, Q };                     // a move down by state.at 1024ths of a pixel
    op1f_move(state.yoyo, matrix);
    state.moved = state.at;
}

static const char* picture_said() {
    if (!state.asked) return "not asked for yet";
    if (document == 0) return "not made";
    return state.yoyo != 0 ? "made, the yoyo found" : "made, the yoyo NOT found";
}

// Once a frame, on the display thread.
extern "C" void yoyo_frame(const Op1fFrame* now) OP1F_NAMED("yoyo_frame");
extern "C" void yoyo_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_1) return;
    u32 ms = op1f_ms();                                                        // the base's clock, as any pack may ask it (the frame's ms is the same number)
    if (!state.asked) {                                                        // the first frame: the page's document, once
        state.asked = 1;
        void* made = op1f_picture(&op1f_card_yoyo, "yoyo.svg");
        if (made != 0) state.yoyo = op1f_node(made, "yoyo_body");
        state.moved = 0;
        state.last_ms = ms;
        document = made;
    }
    u32 shown = now->bytes >= (u32)FRAME_LEVEL_2 && now->page == &op1f_card_yoyo ? 1u : 0u;
    if (shown && !state.shown) ++state.opened;
    if (!shown && state.shown) ++state.closed;
    state.shown = shown;
    if (shown) ++state.frames;
    move_yoyo(ms - state.last_ms);
    state.last_ms = ms;
    int why = quiet_popup(&state.quiet, now->popup != 0);
    int period = --state.countdown <= 0;
    if (!period && !why) return;
    if (period) {
        state.countdown = PERIOD_FRAMES;
        u32 s = quiet_mix(quiet_start(), state.opened);
        s = quiet_mix(s, state.closed);
        s = quiet_mix(s, turns);
        s = quiet_mix(s, recentres);
        s = quiet_mix(s, keys);
        s = quiet_mix(s, (u32)target);
        why |= quiet_period(&state.quiet, s, shown);
    }
    if (!why) return;
    op1f_log(3, LOG_FILE, 1, "yoyo: picture %s; opened %u times, closed %u; %u frames shown; %u keys, %u turns, %u back to the middle, at %d px",
             picture_said(), state.opened, state.closed, state.frames, keys, turns, recentres, target);
    if (state.quiet.silent_before)                                             // how long the gate held the line, as the base's lines say it
        op1f_log(3, LOG_FILE, 2, "yoyo quiet: this line after %u periods held (%s); %u printed, %u held", state.quiet.silent_before, quiet_why(why),
                 state.quiet.spoken, state.quiet.skipped);
}

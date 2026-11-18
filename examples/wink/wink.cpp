// Step M1, the example pack `wink` (docs/first-pack.md walks through it): the first pack that uses the interface the
// base offers packs by other people. It changes no place of the firmware and names no address of it.
//
// What it does. Its picture is the tempo screen's (screen 42, `tempo.svg`, the metronome key), made on the unit from
// the unit's own picture with one small red dot of ours at the top of the middle column, under LINK, named
// `wink_dot` (examples/wink/tempo.svg.make). The base serves that picture to the screen from the
// pack's card, after the four directories of our own features, none of which carries `tempo.svg`. Once a frame the
// base calls this pack's frame function through its card, and the function blinks the dot while the tempo screen is
// the one shown: half a second shown, half a second hidden, by the clock the base hands it. On every other screen
// it does nothing. It finds the dot once, by its name, in the tempo screen's document, and keeps it.
//
// What it uses: the frame the base hands it (Op1fFrame: the screen shown, the clock, the power popup's flag) and
// four of the base's six helpers: op1f_screen (the document of a screen of the firmware, by its number),
// op1f_node (a part of a document, by its name), op1f_show (a part shown or hidden) and op1f_log. Nothing else of
// the firmware or of the base; module/build_m1_card.h declares them.
//
// What it says on the log: one line, through the gate of build_q_quiet.h (a period is 256 frames): whether the dot
// was found, how often it was looked for, how often it came on in the period and since the start. The line is
// printed for the first three periods of a start, for a period in which the tempo screen was shown (and the two
// after it), when the power popup comes up, and otherwise once in 75 periods. Its tag is `wink`: the logger takes
// the name between the last slash and the dot of the file name it is handed.
//
// Runs on the display thread, called by the base once a frame. Nothing is divided (the module links no runtime):
// the half second is counted by subtraction. Compiled for the PC with OP1F_HOST, as it is, for tests this tree does
// not hold.

#include "build_m1_card.h"
#include "build_q_quiet.h"

static const char LOG_FILE[] = "op1f/wink.cpp";
static const char DOT[] = "wink_dot";                  // the dot's id in the picture
enum { TEMPO_SCREEN = 42, HALF_A_SECOND = 500, PERIOD_FRAMES = 256 };
enum { NOT_YET = 0, FOUND = 1, MISSING = 2 };           // what was found of the dot

static struct {
    void* document;                                     // the tempo screen's document the dot was looked for in
    void* dot;                                          // the dot, once found
    unsigned looks;                                     // how often it was looked for
    int showing;                                        // the tempo screen was the one shown at the frame before
    int on;                                             // the dot is shown
    unsigned toggled;                                   // the clock when the dot last came on or went off
    unsigned blinks, blinks_all;                        // how often the dot came on: in this period, since the start
    unsigned frames;                                    // the frames of this period
    QuietGate quiet;
} wink;

static int found() {
    if (wink.dot) return FOUND;
    return wink.document ? MISSING : NOT_YET;
}

// The dot of the tempo screen's document: looked for once in each document the screen has, and never again in the
// same one. The screen has no document until the firmware has made its screens; the firmware makes it once.
static void look() {
    void* document = op1f_screen(TEMPO_SCREEN);
    if (document == wink.document) return;              // looked through already: the dot is kept, or is not there
    wink.document = document;                           // another document, or none: what was found goes with the old
    wink.dot = 0;
    wink.showing = 0;
    if (!document) return;                              // no screen, or no document yet: nothing to look in
    ++wink.looks;
    wink.dot = op1f_node(document, DOT);
}

static void turn(int on, unsigned ms) {
    op1f_show(wink.dot, on);
    wink.on = on;
    wink.toggled = ms;
    if (on) {
        ++wink.blinks;
        ++wink.blinks_all;
    }
}

// While the tempo screen is shown: on as it comes up, then half a second hidden, half a second shown.
static void blink(const Op1fFrame* now) {
    if (now->screen != (unsigned)TEMPO_SCREEN || !wink.dot) {
        wink.showing = 0;                               // another screen: the dot is left as it is
        return;
    }
    if (!wink.showing) {
        wink.showing = 1;
        turn(1, now->ms);
        return;
    }
    unsigned since = now->ms - wink.toggled;
    if (since < (unsigned)HALF_A_SECOND) return;
    unsigned at = wink.toggled + (unsigned)HALF_A_SECOND;
    if (since >= 2u * (unsigned)HALF_A_SECOND) at = now->ms;    // a frame that took long: the rhythm starts anew
    turn(!wink.on, at);
}

static QUIET_OUT_OF_LINE void say(int why) {
    // The logger's line is 260 characters with its own prefix (some forty): at its longest, every number at ten
    // digits, this one is 244. The verdict stands at its head.
    static const char* const WHAT[] = { "not looked for yet", "found in tempo.svg", "not in tempo.svg" };
    op1f_log(3, LOG_FILE, 1, "wink: dot %s; looked for %u times; on %u times in %u frames, %u in all; after %u periods held (%s); %u printed, %u held",
             WHAT[found()], wink.looks, wink.blinks, wink.frames, wink.blinks_all,
             wink.quiet.silent_before, quiet_why(why), wink.quiet.spoken, wink.quiet.skipped);
}

extern "C" void wink_frame(const Op1fFrame* now) OP1F_NAMED("wink_frame");
extern "C" void wink_frame(const Op1fFrame* now) {
    if (!now || now->bytes < (unsigned)sizeof(Op1fFrame)) return;      // a frame of an older base: nothing to go by
    look();
    blink(now);
    ++wink.frames;
    int why = quiet_popup(&wink.quiet, now->popup != 0);
    if (why) say(why);                                  // the state before the unit is off, whatever the period
    if (wink.frames < (unsigned)PERIOD_FRAMES) return;
    unsigned signature = quiet_mix(quiet_start(), (unsigned)found());   // what the line states; its counts are activity
    why = quiet_period(&wink.quiet, signature, wink.blinks != 0);
    if (why) say(why);
    wink.frames = 0;
    wink.blinks = 0;
}

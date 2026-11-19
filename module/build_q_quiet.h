// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Build Q, packs Q1: a gate for a unit's periodic log lines (analysis/quiet_log_study.md).
//
// Since H0 the units of the effect train have printed their summary lines every 232 frames, four seconds, whatever
// the unit did: ten lines, 421 bytes a second, 78 % of the log of a unit that stands switched on and untouched
// (measured on the device on 2026-11-14: a new log file of 512 KiB every sixteen minutes). That costs more than
// room: each full log file is twelve index nodes of the volume's filesystem, and the install of a plain os.ldr
// needs the volume's count of index nodes to stay where it was measured (analysis/os_ldr_install_study.md 12.14).
//
// The gate keeps the four-second cadence while there is something to say and slows the lines down when there is
// not. At the end of a period (quiet_period) the lines are spoken when
//   - the period is one of the first QUIET_FIRST of the start (the state at the start and its costs);
//   - what the lines describe changed since the period before (the caller's signature of its state and of its
//     counts of events), or the caller saw activity in it (an input, voices sounding);
//   - one of the QUIET_HOLD periods after such a period (what a change leaves behind: a build, a save half a
//     second later, its cost);
//   - QUIET_FLOOR periods have passed with nothing spoken (five minutes), so that a long session still shows its
//     costs and a reader sees the unit was alive.
// And every frame (quiet_popup) answers once for each time the power popup comes onto the screen, so that the last
// state before the unit is off is on the log whatever the period: the popup stands for well under four seconds.
//
// The caller collects and clears its statistics of the period at every period, as before; the gate only says
// whether the lines are printed. A signature holds what a line states (which effect on which wagon, a count of
// picks), never what it measures (cycles a block, frames a second): a measure differs at every period by itself.
// The rule it follows: a summary that slows down also prints when what it describes changes, and at the power
// popup.
//
// Plain integer code with nothing called and nothing divided, so that it runs as it is on the PC:
// tools/tests/test_quiet_gate.py compiles this file and walks it through sessions.

#ifndef OP1F_BUILD_Q_QUIET_H
#define OP1F_BUILD_Q_QUIET_H

// The two functions a unit calls are kept out of line in the module: inlined into a large frame function, the
// compiler held their counts in an index register and added to it with a modify register (I3 += M0), which counts
// straight on only while the length register is zero. It is zero there, as at every call of compiled code, but a
// small function of its own does not raise the question (a lesson of step P1).
#ifdef __GNUC__
#define QUIET_OUT_OF_LINE __attribute__((noinline))
#else
#define QUIET_OUT_OF_LINE
#endif

enum { QUIET_FIRST = 3, QUIET_HOLD = 2, QUIET_FLOOR = 75 };
enum { QUIET_WHY_FIRST = 1, QUIET_WHY_CHANGE = 2, QUIET_WHY_ACTIVITY = 4, QUIET_WHY_HOLD = 8, QUIET_WHY_POPUP = 16, QUIET_WHY_FLOOR = 32 };

struct QuietGate {
    unsigned int signature;             // what the lines described at the period before
    unsigned int periods;               // periods seen since the start
    unsigned int spoken, skipped;       // periods whose lines were printed, and periods whose lines were not
    unsigned int silent_before;         // how many periods in a row were not printed before the last printed one
    int hold;                           // printed periods still owed after the last change or activity
    int silent;                         // periods in a row not printed, up to now
    int popup_seen;                     // the popup that is on the screen has had its lines
};

// One word more into a signature (FNV-1a over 32-bit words; the multiply is the processor's own).
static inline unsigned int quiet_mix(unsigned int signature, unsigned int word) {
    return (signature ^ word) * 16777619u;
}

static inline unsigned int quiet_start() { return 2166136261u; }

// The gate counts a printed period, whatever made it so.
static inline void quiet_spoke(QuietGate* g) {
    g->silent_before = (unsigned int)g->silent;
    g->silent = 0;
    ++g->spoken;
}

// At a period's end: the reasons to print this period's lines (QUIET_WHY_ values together), or 0 for none.
static QUIET_OUT_OF_LINE int quiet_period(QuietGate* g, unsigned int signature, int activity) {
    int why = 0;
    if (g->periods < (unsigned int)QUIET_FIRST) why |= QUIET_WHY_FIRST;
    if (g->periods != 0 && signature != g->signature) why |= QUIET_WHY_CHANGE;
    if (activity) why |= QUIET_WHY_ACTIVITY;
    g->signature = signature;
    ++g->periods;
    if (why & (QUIET_WHY_CHANGE | QUIET_WHY_ACTIVITY)) g->hold = QUIET_HOLD;
    else if (g->hold > 0) { --g->hold; why |= QUIET_WHY_HOLD; }
    if (why == 0 && g->silent + 1 >= (int)QUIET_FLOOR) why = QUIET_WHY_FLOOR;
    if (why) quiet_spoke(g);
    else { ++g->silent; ++g->skipped; }
    return why;
}

// At every frame: QUIET_WHY_POPUP once for each time the power popup comes up, else 0. The caller prints its lines
// then and there; the period that is running goes on.
static QUIET_OUT_OF_LINE int quiet_popup(QuietGate* g, int shown) {
    if (!shown) { g->popup_seen = 0; return 0; }
    if (g->popup_seen) return 0;
    g->popup_seen = 1;
    quiet_spoke(g);
    return QUIET_WHY_POPUP;
}

// The reasons as a word for the log, the first that holds.
static inline const char* quiet_why(int why) {
    if (why & QUIET_WHY_POPUP) return "the power popup";
    if (why & QUIET_WHY_CHANGE) return "a change";
    if (why & QUIET_WHY_ACTIVITY) return "activity";
    if (why & QUIET_WHY_FIRST) return "the start";
    if (why & QUIET_WHY_HOLD) return "after a change";
    if (why & QUIET_WHY_FLOOR) return "five minutes";
    return "nothing";
}

#endif

// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R6e, the release's version of the example pack fold: base 30's source of fold (version 2, which stays as it
// was) including the header of level 10, module/build_r6e_card.h, which declares op1f_param_heard and its mark, in
// place of declaring the helper by hand, and the words on where the unit's engine list shows it mended: among
// the unit's own engines, by name.
// Its code is version 2's. What version 2 said follows and holds, but where these words say otherwise.
//
// Step R6d (base 30), the example pack fold, version 2: version 1 (the source of fold 1, which stays as it was)
// with its page drawing each value as the sound plays it (level 10's op1f_param_heard: an LFO aimed at a value moves
// its number on the page as it moves the sound), its cost declared as the unit measured it (25,000 cycles a voice:
// at most 22,235 at base 29's try on a unit, and a ninth more), and two sounds in place of four (hollow, buzz). What version
// 1 said follows and holds, but where these words say otherwise.
//
// Step R6, the example pack fold, version 1: a wave folder, a synth engine of level 9 that renders two channels (OS
// 1.7.0+0 and 1.7.3+0 alike: it names no address of the unit's and changes no place of the firmware).
//
// What the owner meets: `fold` in the unit's engine list (shift and T1 on a synth sound), among the unit's own
// engines by name (after fm), and a folder `fold` of its sounds in the sound list. Picked, the synth sound plays a
// hollow, buzzy tone that grows brighter the harder the note and the higher `fold`. T1 shows its page, the four
// values: the four knobs turn fold, symmetry, sub and drive. The unit's own envelope, play mode, glide, effect and
// LFOs work on it as on any engine.
//
// How it sounds (one voice): a sine at the note's pitch, made louder than full scale by `fold` (and a little by the
// note's velocity) and folded back each time it passes the top or the bottom, so that one cycle turns over itself
// again and again; `symmetry` shifts the fold one way on the left and the other way on the right, which widens the
// sound between the two channels; `sub` mixes a sine an octave down; `drive` pushes the sum into a soft limit. The sine
// is a parabola of the phase (no table, no division); the fold is a triangle of the value, one mask and one
// subtraction.
//
// It asks no working memory: a voice's two phases are words of its own data, one pair for each of the six voices
// (one engine renders one sound at a time, and the voices one after the other), started again when the voice is
// handed over or plucked. `render` runs on the audio thread: no call, no division, no line on the log; it writes all
// 256 words of `out`, left and right in turn.
//
// The page: the base's blank page that stands in for the engine page (T1), on which the frame function paints the four
// values with their names, read through op1f_param_heard for the sound whose page is up, as the sound plays them. It
// needs op1f_level_10, which its card names, and the helpers of level 8 the base offers for an engine's values.
// Compiled for the PC as it is, with OP1F_HOST defined.

#include "build_r6e_card.h"

typedef unsigned int u32;

extern "C" const Op1fCard op1f_card_fold OP1F_NAMED("op1f_card_fold");

enum { VALUES = 4, FRAMES = 128, VOICES = 6, ASK = 0, FRAME_LEVEL_4 = 40, BLOCK_LEVEL_9 = 52 };
enum { FOLD = 0, SYMMETRY = 1, SUB = 2, DRIVE = 3 };

static volatile u32 phases[VOICES], subs[VOICES];

static int clamp15(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }

// A sine of the phase, a whole cycle being 2^32: 4x(1 - |x|) of the phase taken as -1 to 1, -32768 to 32767.
static int sine(u32 phase) {
    int x = (int)phase >> 16;                            // -32768 to 32767
    int a = x < 0 ? -x : x;
    int y = (x * (32768 - a)) >> 13;
    return y > 32767 ? 32767 : y;
}

// The value folded back into -1 to 1, in 16.16: a triangle of the value, period four.
static int folded(int u) {
    int v = (u + 65536) & 0x3ffff;
    int d = v - 131072;
    return 65536 - (d < 0 ? -d : d);
}

// A soft limit of a value in Q15, held to -1 to 1 first: 1.5c - 0.5c^3.
static int soft(int c) {
    c = clamp15(c);
    int c3 = (((c * c) >> 15) * c) >> 15;
    return clamp15(c + (c >> 1) - (c3 >> 1));
}

extern "C" void fold_render(const Op1fVoiceBlock* now) OP1F_NAMED("fold_render");
extern "C" void fold_render(const Op1fVoiceBlock* now) {
    int* out = now->out;
    u32 voice = now->voice < (u32)VOICES ? now->voice : 0u;
    if (now->bytes < (u32)BLOCK_LEVEL_9) {
        for (u32 k = 0; k < 2u * (u32)FRAMES; ++k) out[k] = 0;
        return;
    }
    if (now->events & ((u32)OP1F_VOICE_FRESH | (u32)OP1F_VOICE_BEGINS)) {   // handed anew or plucked: the phases from the top
        phases[voice] = 0;
        subs[voice] = 0;
    }
    const int* value = now->values;
    int velocity = now->velocity >> 16;                  // 0 to 32512
    int gain = 256 + value[FOLD] * 24 + (velocity >> 9); // 1 to about 10.4, Q8
    int shift = value[SYMMETRY] * 328;                   // -16,400 to 16,400, Q15
    int sub = value[SUB] * 327;                          // Q15
    int drive = 32768 + value[DRIVE] * 984;              // 1 to 4, Q15
    u32 step = now->step, phase = phases[voice], low = subs[voice];
    for (u32 k = 0; k < (u32)FRAMES; ++k) {
        int x = sine(phase), s = sine(low);
        int u = (x * gain) >> 8;                         // Q15, up to about 340,000
        int left = folded((u + shift) << 1) >> 1;        // back in Q15
        int right = folded((u - shift) << 1) >> 1;
        int below = (s * sub) >> 15;
        left = soft((((left + below) >> 1) * (drive >> 4)) >> 11);
        right = soft((((right + below) >> 1) * (drive >> 4)) >> 11);
        out[2u * k] = (int)((u32)left << 16);
        out[2u * k + 1u] = (int)((u32)right << 16);
        phase += step;
        low += step >> 1;
    }
    phases[voice] = phase;
    subs[voice] = low;
}

// ---------------------------------------------------------------- the page

static const unsigned COLOURS[4] = {OP1F_COLOUR(0x3c, 0x60, 0x87), OP1F_COLOUR(0x8a, 0x65, 0x3c), OP1F_COLOUR(0xa2, 0xa4, 0xa6), OP1F_COLOUR(0xe5, 0x28, 0x17)};
static const unsigned NAME_GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);
static const unsigned DARK_GREY = OP1F_COLOUR(0x37, 0x3d, 0x42);
static const int MIDDLES[4] = {60, 180, 300, 420};
static const char NAMES[VALUES][12] = {"fold", "symmetry", "sub", "drive"};
enum { VALUE_Y = 30, VALUE_SCALE = 3, NAME_Y = 60, RULE_Y = 82, RULE_X = 16, RULE_WIDE = 448 };

static volatile u32 shown_where, shown_how;
static struct { u32 stood, refusal_said; } page;

extern "C" void fold_shown(unsigned where, unsigned how) OP1F_NAMED("fold_shown");
extern "C" void fold_shown(unsigned where, unsigned how) {
    shown_where = where;
    shown_how = how;
}

static void put_number(char* out, int n) {
    u32 at = 0;
    if (n < 0) { out[at++] = '-'; n = -n; }
    char digits[12];
    u32 d = 0, u = (u32)n;
    do {                                                  // by subtraction: nothing is divided
        u32 tens = 0;
        while (u >= 10u) { u -= 10u; ++tens; }
        digits[d++] = (char)('0' + u);
        u = tens;
    } while (u && d < 11u);
    while (d) out[at++] = digits[--d];
    out[at] = 0;
}

static void centred(void* canvas, int middle, int y, const char* text, unsigned colour, int scale) {
    int n = 0;
    while (text[n]) ++n;
    op1f_paint_text(canvas, middle - (((6 * n - 1) * scale) >> 1), y, text, colour, scale);
}

static void paint(void* canvas, u32 where, u32 how) {
    char text[12];
    for (u32 i = 0; i < (u32)VALUES; ++i) {
        int value = 0;
        if (op1f_param_heard(&op1f_card_fold, ASK, where, i, &value) == 0) put_number(text, value);   // R6d: as the sound plays it
        else { text[0] = '-'; text[1] = 0; }
        centred(canvas, MIDDLES[i], VALUE_Y, text, COLOURS[i], VALUE_SCALE);
        centred(canvas, MIDDLES[i], NAME_Y, NAMES[i], NAME_GREY, 1);
    }
    op1f_paint_box(canvas, RULE_X, RULE_Y, RULE_WIDE, 1, DARK_GREY);
    if (how & (u32)OP1F_SHOWN_OFF) centred(canvas, 240, 110, "switched off", COLOURS[3], 1);
}

extern "C" void fold_frame(const Op1fFrame* now) OP1F_NAMED("fold_frame");
extern "C" void fold_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    if (!page.stood) {                                    // the base's blank page, to paint on
        if (op1f_page_stand_in(&op1f_card_fold, ASK, 0)) page.stood = 1;
        else if (!page.refusal_said) {
            page.refusal_said = 1;
            op1f_log(2, "../src/main/fold.cpp", 1, "fold: the base did not take its page; T1 shows the base's blank page with its name");
        }
    }
    u32 how = shown_how, where = shown_where;
    if (now->page == &op1f_card_fold && now->canvas != 0 && (how & (u32)OP1F_SHOWN_UP)) paint(now->canvas, where, how);
}

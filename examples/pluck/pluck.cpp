// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R6e, the release's version of the example pack pluck: base 30's source of pluck (version 2, which stays as it
// was) including the header of level 10, module/build_r6e_card.h, which declares op1f_param_heard and its mark, in
// place of declaring the helper by hand, and the words on where the unit's engine list shows it mended: among
// the unit's own engines, by name.
// Its code is version 2's. What version 2 said follows and holds, but where these words say otherwise.
//
// Step R6d (base 30), the example pack pluck, version 2: version 1 (the source of pluck 1, which stays as it was)
// with its page drawing each value as the sound plays it (level 10's op1f_param_heard: an LFO aimed at a value moves
// its number on the page as it moves the sound), its cost declared as the unit measured it (33,000 cycles a voice:
// at most 29,415 at base 29's try on a unit, and a ninth more), and two sounds in place of six (nylon, harp). What version 1
// said follows and holds, but where these words say otherwise.
//
// Step R6, the example pack pluck, version 1: a plucked string, a synth engine of level 9 (OS 1.7.0+0 and 1.7.3+0 alike:
// it names no address of the unit's and changes no place of the firmware).
//
// What the owner meets: `pluck` in the unit's engine list (shift and T1 on a synth sound), among the unit's own
// engines by name (after phase), and a folder `pluck` of its sounds in the sound list. Picked, the synth sound plays
// a plucked string on every key: harder notes louder and brighter, softer ones duller. T1 shows its page, the eight
// values in two rows: the four knobs turn the top row (tone, decay, damping, pick), and with shift the bottom row
// (body, drift, noise, level). The unit's own envelope, play mode, glide, effect and LFOs work on it as on any
// engine.
//
// How it sounds (one voice): a delay line one period of the note long (Karplus and Strong's string). A note plucks
// it: for one period a burst of noise goes into the line, louder and brighter the harder the note and the higher
// `tone`; the line feeds back through a gentle low-pass (`damping`, darker as it rises) and a gain a little under one
// (`decay`, longer as it rises). What is heard is read off the line at the note's pitch, with a comb at the pick's
// place (`pick`: from the middle of the string towards its end), a little warmth (`body`), a slow wander of the pitch
// (`drift`), a breath of noise that keeps the string moving (`noise`), and a level. The pitch is the step the unit
// hands, glide and bend applied: the line's length is 2^32 / step frames, worked out once a block by shifts and
// subtractions, with a linear read between two frames for the fraction.
//
// Its working memory is 16 KiB a voice: a short header and 8,160 frames of the line, enough for 10.8 Hz at 88,200
// frames a second. The base clears it before it is handed, and `pluck_reset` clears it again when a voice is handed to
// the engine anew, on the display thread, so `render` never clears the line. `render` runs on the audio thread: no
// call, no division, no line on the log; it writes all 128 words of `out`.
//
// The page: the base's blank page that stands in for the engine page (T1), on which the frame function paints the
// eight values with their names, read through op1f_param_heard for the sound whose page is up, as the sound plays
// them. It needs op1f_level_10, which its card names, and the helpers of level 8 the base offers for an engine's
// values. Compiled for the PC as it is, with OP1F_HOST defined.

#include "build_r6e_card.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern "C" const Op1fCard op1f_card_pluck OP1F_NAMED("op1f_card_pluck");

enum { VALUES = 8, FRAMES = 128, LINE = 8160, HEADER = 64, VOICE_BYTES = 16384, ASK = 0, FRAME_LEVEL_4 = 40, BLOCK_LEVEL_9 = 52 };
enum { TONE = 0, DECAY = 1, DAMPING = 2, PICK = 3, BODY = 4, DRIFT = 5, NOISE = 6, LEVEL = 7 };

// One voice's memory: the header, then the line. 16,384 bytes in all.
struct Voice {
    u32 at;                     // where the next frame of the line is written
    u32 burst;                  // frames of the pluck's burst still to go into the line
    int burst_amp;              // the burst's loudness, Q15
    int burst_bright;           // the burst's low-pass, Q15 (32767: as it comes)
    int burst_lp;               // the burst's low-pass, its state
    int loop_lp;                // the line's low-pass, its state
    int body_lp;                // the warmth's low-pass, its state
    u32 noise;                  // the noise's state
    u32 wander;                 // the drift's phase
    u32 sounding;               // a note was plucked since the voice was handed over
    u32 spare[6];
    short line[LINE];
};
static_assert(sizeof(Voice) == VOICE_BYTES, "a voice's memory is 16 KiB, as the ask says");
static_assert((u32)sizeof(Voice) - (u32)LINE * 2u == (u32)HEADER, "the header is 64 bytes");

static int clamp16(int v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }

// 2^48 / step: the line's length in frames, in 16.16, by shifts and subtractions (no division: the module links no
// runtime). Held to the line's room.
static u32 length16(u32 step) {
    if (step < 65536u) return (u32)(LINE - 2) << 16;
    if (step >= 0x80000000u) return 2u << 16;                // above half the rate: the shortest line
    u32 q = 0, r = 1;                                     // r holds the numerator's top bit, 2^48 as it is brought down
    for (int bit = 47; bit >= 0; --bit) {
        r <<= 1;
        if (r >= step) {
            r -= step;
            if (bit < 32) q |= 1u << bit;
            else return (u32)(LINE - 2) << 16;           // longer than the line can hold
        }
    }
    if (q > ((u32)(LINE - 2) << 16)) q = (u32)(LINE - 2) << 16;
    if (q < (2u << 16)) q = 2u << 16;
    return q;
}

// A triangle of the phase, -32768 to 32767.
static int triangle(u32 phase) {
    int t = (int)(phase >> 15);                          // 0 to 131071
    return t < 65536 ? t - 32768 : 98303 - t;
}

static u32 random_next(u32* s) {
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

// The line's frame `back` frames before the one to be written, `back` 1 to LINE - 1.
static int line_at(const Voice* v, u32 back) {
    u32 i = v->at >= back ? v->at - back : v->at + (u32)LINE - back;
    return v->line[i];
}

// What `render` is handed holds the values in the pack's own numbers, each 0 to 100.
extern "C" void pluck_render(const Op1fVoiceBlock* now) OP1F_NAMED("pluck_render");
extern "C" void pluck_render(const Op1fVoiceBlock* now) {
    int* out = now->out;
    Voice* v = (Voice*)now->state;
    if (now->bytes < (u32)BLOCK_LEVEL_9 || v == 0) {
        for (u32 k = 0; k < (u32)FRAMES; ++k) out[k] = 0;
        return;
    }
    const int* value = now->values;
    if (now->events & (u32)OP1F_VOICE_FRESH) {           // handed anew: what it held is not this note's (the reset cleared the line)
        v->burst = 0;
        v->sounding = 0;
        v->loop_lp = v->burst_lp = v->body_lp = 0;
        v->noise = 0x9e3779b9u + now->voice;
        v->wander = now->voice * 0x2aaaaaaau;
        if (v->at >= (u32)LINE) v->at = 0;
    }
    u32 length = length16(now->step);
    if (now->events & (u32)OP1F_VOICE_BEGINS) {          // a pluck: one period of burst, as loud and bright as the note
        int velocity = now->velocity >> 16;               // 0 to 32512
        if (velocity < 2048) velocity = 2048;
        v->burst = (length >> 16) + 1u;
        v->burst_amp = (velocity * 26000) >> 15;
        int bright = 2000 + value[TONE] * 300;            // 2,000 to 32,000
        v->burst_bright = (bright * (16384 + (velocity >> 1))) >> 15;
        v->sounding = 1;
    }
    if (!v->sounding) {                                   // nothing plucked yet: silence, and the line left as it is
        for (u32 k = 0; k < (u32)FRAMES; ++k) out[k] = 0;
        return;
    }
    v->wander += 9000u * (u32)FRAMES;                     // about a third of a hertz
    int wobble = (triangle(v->wander) * value[DRIFT]) >> 13;   // up to about 400: 1.2 % of the length
    u32 here = length + (u32)((int)(((int)(length >> 8)) * wobble) >> 7);
    if (here > ((u32)(LINE - 2) << 16)) here = (u32)(LINE - 2) << 16;
    if (here < (2u << 16)) here = 2u << 16;
    u32 whole = here >> 16, part = here & 0xffffu;
    u32 pick = (whole * (u32)(5 + value[PICK] * 45)) >> 13; // from a two-thousandth to about half of the string
    if (pick < 1u) pick = 1u;
    int gain = 62915 + value[DECAY] * 26;                 // 0.96 to 0.9997, Q16
    int damp = 32767 - value[DAMPING] * 267;              // the line's low-pass, Q15
    int body = value[BODY] * 300;                         // the warmth's share, Q15
    int breath = value[NOISE] * 40;
    int level = value[LEVEL] * 327;                       // Q15
    for (u32 k = 0; k < (u32)FRAMES; ++k) {
        int a = line_at(v, whole), b = line_at(v, whole + 1u);
        int y = a + (((b - a) * (int)(part >> 1)) >> 15);
        v->loop_lp += ((y - v->loop_lp) * damp) >> 15;
        int next = (v->loop_lp * gain) >> 16;
        int noise = (int)(random_next(&v->noise) >> 17) - 16384;
        if (v->burst) {
            v->burst_lp += ((noise - v->burst_lp) * v->burst_bright) >> 15;
            next += (v->burst_lp * v->burst_amp) >> 14;
            --v->burst;
        }
        if (breath) next += (noise * breath) >> 15;
        v->line[v->at] = (short)clamp16(next);
        v->at = v->at + 1u >= (u32)LINE ? 0u : v->at + 1u;
        int heard = y - (line_at(v, pick) >> 1);          // the comb of the pick's place
        v->body_lp += ((heard - v->body_lp) * 3000) >> 15;
        heard = clamp16(heard + ((v->body_lp * body) >> 14));
        out[k] = (int)((u32)clamp16((heard * level) >> 15) << 16);
    }
}

// On the display thread at the frame after a voice was handed to the engine: its line cleared.
extern "C" void pluck_reset(unsigned slot, unsigned voice, void* state) OP1F_NAMED("pluck_reset");
extern "C" void pluck_reset(unsigned slot, unsigned voice, void* state) {
    (void)slot;
    (void)voice;
    if (state == 0) return;
    u32* words = (u32*)state;
    for (u32 i = 0; i < (u32)VOICE_BYTES / 4u; ++i) words[i] = 0;
}

// ---------------------------------------------------------------- the page

static const unsigned COLOURS[4] = {OP1F_COLOUR(0x3c, 0x60, 0x87), OP1F_COLOUR(0x8a, 0x65, 0x3c), OP1F_COLOUR(0xa2, 0xa4, 0xa6), OP1F_COLOUR(0xe5, 0x28, 0x17)};
static const unsigned NAME_GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);
static const unsigned DARK_GREY = OP1F_COLOUR(0x37, 0x3d, 0x42);
static const int MIDDLES[4] = {60, 180, 300, 420};
static const char NAMES[VALUES][8] = {"tone", "decay", "damping", "pick", "body", "drift", "noise", "level"};
enum { TOP_Y = 24, TOP_SCALE = 3, TOP_NAME_Y = 52, RULE_Y = 74, LOW_Y = 96, LOW_SCALE = 2, LOW_NAME_Y = 118, RULE_X = 16, RULE_WIDE = 448 };

static volatile u32 shown_where, shown_how;
static struct { u32 stood, refusal_said; } page;

extern "C" void pluck_shown(unsigned where, unsigned how) OP1F_NAMED("pluck_shown");
extern "C" void pluck_shown(unsigned where, unsigned how) {
    shown_where = where;
    shown_how = how;
}

static u32 put_number(char* out, int n) {
    u32 at = 0;
    if (n < 0) { out[at++] = '-'; n = -n; }
    char digits[12];
    u32 d = 0;
    u32 u = (u32)n;
    do {                                                  // by subtraction: nothing is divided
        u32 tens = 0;
        while (u >= 10u) { u -= 10u; ++tens; }
        digits[d++] = (char)('0' + u);
        u = tens;
    } while (u && d < 11u);
    while (d) out[at++] = digits[--d];
    out[at] = 0;
    return at;
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
        int got = op1f_param_heard(&op1f_card_pluck, ASK, where, i, &value);         // R6d: as the sound plays it
        if (got == 0) put_number(text, value);
        else { text[0] = '-'; text[1] = 0; }
        u32 top = i < 4u;
        int middle = MIDDLES[i & 3u];
        centred(canvas, middle, top ? TOP_Y : LOW_Y, text, COLOURS[i & 3u], top ? TOP_SCALE : LOW_SCALE);
        centred(canvas, middle, top ? TOP_NAME_Y : LOW_NAME_Y, NAMES[i], NAME_GREY, 1);
    }
    op1f_paint_box(canvas, RULE_X, RULE_Y, RULE_WIDE, 1, DARK_GREY);
    if (how & (u32)OP1F_SHOWN_OFF) centred(canvas, 240, 150, "switched off", COLOURS[3], 1);
}

extern "C" void pluck_frame(const Op1fFrame* now) OP1F_NAMED("pluck_frame");
extern "C" void pluck_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    if (!page.stood) {                                    // the base's blank page, to paint on
        if (op1f_page_stand_in(&op1f_card_pluck, ASK, 0)) page.stood = 1;
        else if (!page.refusal_said) {
            page.refusal_said = 1;
            op1f_log(2, "../src/main/pluck.cpp", 1, "pluck: the base did not take its page; T1 shows the base's blank page with its name");
        }
    }
    u32 how = shown_how, where = shown_where;
    if (now->page == &op1f_card_pluck && now->canvas != 0 && (how & (u32)OP1F_SHOWN_UP)) paint(now->canvas, where, how);
}

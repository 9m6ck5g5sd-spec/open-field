// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R6e, the release's version of the example pack crush: base 30's source of crush (version 2, which stays as it
// was) including the header of level 10, module/build_r6e_card.h, which declares op1f_param_heard and its mark, in
// place of declaring the helper by hand, and the words on where the unit's lists of effects show it mended:
// among the unit's own effects, by name.
// Its code is version 2's. What version 2 said follows and holds, but where these words say otherwise.
//
// Step R6d (base 30), the example pack `crush`, version 2: version 1 (the source of crush 1, which stays as it was)
// with its page drawing each value as the sound plays it (level 10's op1f_param_heard: an LFO aimed at a value moves
// its number on the page as it moves the sound). What version 1 said follows and holds.
//
// Step R5, the example pack `crush`: a sound made rougher, with fewer bits and a lower sample rate, through level 8 of
// the base's interface (module/build_r5_card.h). It changes no place of the firmware and names no address of it.
//
// What the owner meets:
//   `crush` in the unit's own lists of effects, among the unit's own effects by name (before cwo): a sound's (shift
//   and T3) and the master's (on the mixer).
//   Picked, it plays where the unit plays its own effect, and the unit's own key switches it on and off. It stands on
//   the unit's `punch`: a sound saved with it keeps punch's name and four knobs, switched off, so on a unit without the
//   pack, and on the original firmware, that sound plays dry.
//   Its four values, on the unit's four knobs (and on its LFOs, which the LFO pages name by these names):
//     bits   the blue knob: how many bits the sound keeps, 16 (nearly clean) down to 2 (three steps a side). 8 when
//            picked
//     rate   the second knob: the sample rate, halved at each step: 0 is the unit's own 44,100 a second, 1 is 22,050,
//            and so on down to 6, 689 a second. 2
//     drive  the third knob: how hard the sound is pushed into the steps, twice as loud at each step, 0 to 4 (x1 to
//            x16), held at full scale. 0
//     mix    the red knob: how much of what comes out is crushed, from 0 (none: the sound as it came) to 100. 100
//   T3 shows its page (the mixer's T3 for the master): the base's blank page with the name `crush`, on which the pack
//   paints the four values in the knobs' colours, and `off` while the effect is switched off. The page takes no key
//   and no knob: the unit turns the values, and the page shows where they are.
//
// The crush, for each side, left and right alike:
//   the rate   the side is cut into runs of 2^rate frames, and every frame of a run takes the run's first frame. A
//              block's 64 frames hold a whole number of runs at every rate, so a run never crosses from one block to
//              the next: nothing is kept between blocks
//   the drive  that frame times 2^drive, held at full scale
//   the bits   its size cut down to a step of 2^(32 - bits), toward 0: silence stays silence, and at drive 0 nothing
//              comes out louder than the loudest of what went in
//   out        each frame x (1 - mix) + the run's crushed frame x mix, each product rounded toward 0, the sum held at
//              full scale. At mix 0 the block is not touched: the sound goes out bit for bit as it came.
// It works on the two sides apart (no OP1F_EFFECT_MONO): with that flag the base would hand it the mean of the two
// sides and copy it to both, so a sound would leave it in the middle, at mix 0 too; the work is small either way.
//
// It keeps nothing from one block to the next, so it asks no working memory and has no `reset`, and `fresh` (the
// effect made anew) has nothing to clear. On the audio thread `process` allocates nothing, writes no line, calls
// nothing and divides nothing; it reads the values once a block. The module links no runtime: nothing in this file
// divides by a number that is not a power of two, and nothing multiplies in 64 bits.

#include "build_r6e_card.h"

typedef unsigned int u32;

enum { ASK = 0, FRAMES = 64, BLOCK_LEVEL_8 = 44, FRAME_LEVEL_4 = 40, VALUES = 4, SAID_MOST = 16 };
enum { BITS = 0, RATE = 1, DRIVE = 2, MIX = 3 };
enum { BITS_LEAST = 2, BITS_MOST = 16, RATE_MOST = 6, DRIVE_MOST = 4, PERCENT = 100, UNITY = 65536 };
static_assert((FRAMES & ((1 << RATE_MOST) - 1)) == 0, "a block holds a whole number of runs at every rate");

// The pack's own card, written by tools/pack_build.py from its recipe.
extern "C" const Op1fCard op1f_card_crush OP1F_NAMED("op1f_card_crush");

// ---------------------------------------------------------------- the crush, on the audio thread

// a + b, held at full scale. Written with no branch: the loop then costs the same whatever the sound.
static inline int add_held(int a, int b) {
    u32 s = (u32)a + (u32)b;
    u32 over = 0u - ((~((u32)a ^ (u32)b) & ((u32)a ^ s)) >> 31);   // all ones when the sum went past full scale
    u32 most = ((u32)a >> 31) + 0x7fffffffu;                        // full scale on a's side: 2^31 - 1, or -2^31
    return (int)((s & ~over) | (most & over));
}

// x times g / 65536, g from 0 to 65536, rounded toward 0: never larger than x. The size is scaled and the sign put
// back, with no branch.
static inline int scaled(int x, u32 g) {
    u32 sign = 0u - ((u32)x >> 31);                       // all ones for a negative x
    u32 m = ((u32)x ^ sign) - sign;                       // the size, up to 2^31
    u32 p = (m >> 16) * g + (((m & 0xffffu) * g) >> 16);   // at most m: no overflow
    return (int)((p ^ sign) - sign);
}

static inline u32 held(int v, int least, int most) {
    return (u32)(v < least ? least : v > most ? most : v);
}

// A value of 0 to 100 as a gain in 65536ths: 100 is 65536 exactly (671,089 / 1024 is 655.36 and a hair).
static inline u32 gain(u32 hundredths) {
    return (hundredths * 671089u) >> 10;
}

// One frame crushed: louder by 2^drive, held at full scale, then its size cut to the steps, toward 0.
static inline int rough(int x, u32 drive, u32 steps) {
    int d = (int)((u32)x << drive);
    if ((d >> drive) != x) d = x < 0 ? (int)0x80000000u : 0x7fffffff;   // past full scale: held there
    u32 sign = 0u - ((u32)d >> 31);
    u32 m = (((u32)d ^ sign) - sign) & steps;             // the size, cut to the steps
    return (int)((m ^ sign) - sign);
}

static void crush_side(int* side, u32 frames, u32 run, u32 drive, u32 steps, u32 keep, u32 mix) {
    for (u32 i = 0; i < frames; i += run) {
        int crushed = scaled(rough(side[i], drive, steps), mix);
        u32 end = i + run > frames ? frames : i + run;
        for (u32 k = i; k < end; ++k) side[k] = add_held(scaled(side[k], keep), crushed);
    }
}

extern "C" void crush_process(const Op1fEffectBlock* now) OP1F_NAMED("crush_process");
extern "C" void crush_process(const Op1fEffectBlock* now) {
    if (now->bytes < (u32)BLOCK_LEVEL_8) return;
    const int* v = now->values;
    u32 mix = gain(held(v[MIX], 0, PERCENT));
    if (mix == 0) return;                                 // the sound goes out bit for bit as it came
    u32 bits = held(v[BITS], BITS_LEAST, BITS_MOST);
    u32 run = 1u << held(v[RATE], 0, RATE_MOST);
    u32 drive = held(v[DRIVE], 0, DRIVE_MOST);
    u32 steps = 0xffffffffu << (32u - bits);              // the bits a frame keeps
    u32 frames = now->frames > (u32)FRAMES ? (u32)FRAMES : now->frames;
    crush_side(now->left, frames, run, drive, steps, UNITY - mix, mix);
    crush_side(now->right, frames, run, drive, steps, UNITY - mix, mix);
}

// ---------------------------------------------------------------- the page, on the display thread

// What `shown` was told last: single words, written by `shown`, read by the frame function.
static volatile u32 shown_where, shown_how;

extern "C" void crush_shown(unsigned where, unsigned how) OP1F_NAMED("crush_shown");
extern "C" void crush_shown(unsigned where, unsigned how) {
    shown_where = where;
    shown_how = how;
}

static const char LOG_FILE[] = "op1f/crush.cpp";           // the log shows "crush"
static const unsigned KNOB[VALUES] = { OP1F_COLOUR(0x3c, 0x60, 0x87), OP1F_COLOUR(0x8a, 0x65, 0x3c),   // the field palette: blue, ochre,
                                       OP1F_COLOUR(0xa2, 0xa4, 0xa6), OP1F_COLOUR(0xe5, 0x28, 0x17) };  // grey and red, the four knobs
static const unsigned LIGHT_GREY = OP1F_COLOUR(0xcd, 0xd0, 0xc2);
static const unsigned GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);
enum { COLUMN_FIRST = 60, COLUMN_PITCH = 120, VALUE_Y = 86, VALUE_SCALE = 3, NAME_Y = 122, NAME_SCALE = 2, OFF_Y = 176, OFF_SCALE = 2 };
static const char RATES[RATE_MOST + 1][8] = { "44100", "22050", "11025", "5513", "2756", "1378", "689" };
static const char DRIVES[DRIVE_MOST + 1][4] = { "x1", "x2", "x4", "x8", "x16" };

static struct {
    u32 said, lines;                                      // what the last line about the page said; how many such lines
    char text[8];
} page;

// A whole number of 0 to 999 as text, with no division.
static void number_text(char* out, u32 n) {
    u32 at = 0, hundreds = 0, tens = 0;
    while (n >= 100u) { n -= 100u; ++hundreds; }
    while (n >= 10u) { n -= 10u; ++tens; }
    if (hundreds) out[at++] = (char)('0' + hundreds);
    if (hundreds || tens) out[at++] = (char)('0' + tens);
    out[at++] = (char)('0' + n);
    out[at] = 0;
}

// A text in the base's font, centred on `middle`: five pixels and a space a character.
static void paint_centred(void* canvas, int middle, int y, const char* text, unsigned colour, int scale) {
    int characters = 0;
    while (text[characters]) ++characters;
    op1f_paint_text(canvas, middle - (((6 * characters - 1) * scale) >> 1), y, text, colour, scale);
}

static const char* place_said(u32 where, char* out) {
    if (where >= (u32)OP1F_PLACE_MASTER) return "the master";
    const char* host = where >= (u32)OP1F_PLACE_DRUM ? "drum " : "synth ";
    u32 at = 0;
    while (host[at]) { out[at] = host[at]; ++at; }
    number_text(out + at, (where & 7u) + 1u);
    return out;
}

// A line when the page comes onto the screen, goes, or the effect is switched on or off under it: the first sixteen.
static void say_page(u32 how, u32 where) {
    u32 up = how & (u32)OP1F_SHOWN_UP, off = how & (u32)OP1F_SHOWN_OFF;
    u32 said = up ? (up | off | ((where & 31u) << 4)) : 0u;
    if (said == page.said) return;
    page.said = said;
    if (++page.lines > (u32)SAID_MOST) return;
    char place[16];
    if (up) op1f_log(3, LOG_FILE, 1, "crush: its page is on the screen for %s, switched %s", place_said(where, place), off ? "off" : "on");
    else op1f_log(3, LOG_FILE, 2, "crush: its page is no longer on the screen");
}

// The four values as the base hands them for the place whose page is up, each in its knob's colour over its name.
static void paint(void* canvas, u32 where, u32 how) {
    const Op1fEffect* effect = (const Op1fEffect*)op1f_card_crush.asks[ASK].what;
    for (u32 i = 0; i < (u32)VALUES; ++i) {
        int middle = COLUMN_FIRST + (int)i * COLUMN_PITCH;
        int value = 0;
        const char* text = "-";
        if (op1f_param_heard(&op1f_card_crush, ASK, where, i, &value) == 0) {      // R6d: as the sound plays it
            if (i == (u32)RATE) {
                text = RATES[held(value, 0, RATE_MOST)];
            } else if (i == (u32)DRIVE) {
                text = DRIVES[held(value, 0, DRIVE_MOST)];
            } else {
                number_text(page.text, held(value, 0, PERCENT));
                text = page.text;
            }
        }
        paint_centred(canvas, middle, VALUE_Y, text, KNOB[i], VALUE_SCALE);
        paint_centred(canvas, middle, NAME_Y, effect->params[i].name, LIGHT_GREY, NAME_SCALE);
    }
    if (how & (u32)OP1F_SHOWN_OFF) paint_centred(canvas, 240, OFF_Y, "off", GREY, OFF_SCALE);
}

extern "C" void crush_frame(const Op1fFrame* now) OP1F_NAMED("crush_frame");
extern "C" void crush_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    u32 how = shown_how, where = shown_where;
    say_page(how, where);
    if (now->page == &op1f_card_crush && now->canvas != 0 && (how & (u32)OP1F_SHOWN_UP)) paint(now->canvas, where, how);
}

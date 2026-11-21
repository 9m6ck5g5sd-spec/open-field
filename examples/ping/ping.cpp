// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R6e, the release's version of the example pack ping: base 30's source of ping (version 2, which stays as it
// was) including the header of level 10, module/build_r6e_card.h, which declares op1f_param_heard and its mark, in
// place of declaring the helper by hand, and the words on where the unit's lists of effects show it mended:
// among the unit's own effects, by name.
// Its code is version 2's. What version 2 said follows and holds, but where these words say otherwise.
//
// Step R6d (base 30), the example pack `ping`, version 2: version 1 (the source of ping 1, which stays as it was)
// with its page drawing each value as the sound plays it (level 10's op1f_param_heard: an LFO aimed at a value moves
// its number on the page as it moves the sound). What version 1 said follows and holds.
//
// Step R5, the example pack `ping`: an echo in time with the tempo that bounces left and right, through level 8 of the
// base's interface (module/build_r5_card.h). It changes no place of the firmware and names no address of it.
//
// What the owner meets:
//   `ping` in the unit's own lists of effects, among the unit's own effects by name (between phone and punch): a
//   sound's (shift and T3) and the master's (on the mixer).
//   Picked, it plays where the unit plays its own effect, and the unit's own key switches it on and off. It stands on
//   the unit's `delay`: a sound saved with it keeps delay's name and four knobs, switched off, so on a unit without
//   the pack, and on the original firmware, that sound plays dry.
//   Its four values, on the unit's four knobs (and on its LFOs, which the LFO pages name by these names):
//     time      the blue knob: how far behind the echo comes, 1 to 4 sixteenth notes (a sixteenth, an eighth, a dotted
//               eighth, a quarter), in time with the unit's tempo. 3 when picked
//     feedback  the second knob: how much of each echo comes back as the next one, 0 to 90 hundredths. 40
//     spread    the third knob: how far the echoes bounce, from 0 (both sides alike, in the middle) to 100 (left,
//               right, left, all the way). 100
//     mix       the red knob: how much of what comes out is echo, from 0 (none: the sound as it came) to 100 (echo
//               only). 30
//   T3 shows its page (the mixer's T3 for the master): the base's blank page with the name `ping`, on which the pack
//   paints the four values in the knobs' colours, and `off` while the effect is switched off. The page takes no key
//   and no knob: the unit turns the values, and the page shows where they are.
//   Switched off, its echoes ring out (the flag OP1F_EFFECT_TAILS): what is in the lines keeps coming back, softer
//   each time, and nothing new goes in. Once the lines are silent it does nothing at all.
//
// The echo, for each frame (a and b are what the left and the right line hold `delay` frames back):
//   in               the mean of the two sides; nothing while the effect is switched off
//   the left line    gets in + feedback x b    (held at full scale): the right's echo comes back on the left
//   the right line   gets      feedback x a    the left's echo goes over to the right
//   So a sound comes back left, then right, then left, each echo softer than the one before by the feedback.
//   wet left         a - far x (a - b), wet right b + far x (a - b), far = (1 - spread) / 2: at full spread each side
//                    hears its own line, at none both hear the mean of the two (within the smallest step)
//   out              each side x (1 - mix) + its wet x mix; switched off, each side as it came + its wet x mix
// Every product is rounded toward 0, so that a tail dies away to exact silence and nothing comes out louder than went
// in; feedback is at most 0.9, so each echo is softer than the one before; every sum is held at full scale, so that
// nothing wraps round (a wrap is a loud crack). At mix 0 the sound goes out bit for bit as it came.
//
// The time comes from the block's `period`, the length of a 96th of a beat in frames times 2^18, with no division: a
// sixteenth is (period >> 6) x 24 >> 12 frames. The lines live in the working memory the base gives each place the
// effect plays at (one megabyte, `state_bytes`): a head of four words, then two lines of 131,070 frames, about three
// seconds, a frame of the left line and the same frame of the right side by side. Clearing a megabyte takes far longer
// than a block may, so it is done where time is cheap: the base calls `reset` on the display thread before the first
// block after the place's effect was made anew, and `reset` clears the whole memory. A pack that runs `ping` inside its own sound work may say `fresh` with no `reset` before it (a chain
// rebuilt): then the block only forgets the lines, by starting them anew and reading nothing older than what it has
// written since.
//
// On the audio thread `process` allocates nothing, writes no line, calls nothing and divides nothing; it reads the
// values once a block. The module links no runtime: nothing in this file divides by a number that is not a power of
// two, and nothing multiplies in 64 bits.

#include "build_r6e_card.h"

typedef unsigned int u32;

enum { ASK = 0, FRAMES = 64, BLOCK_LEVEL_8 = 44, FRAME_LEVEL_4 = 40, VALUES = 4, SAID_MOST = 16 };
enum { TIME = 0, FEEDBACK = 1, SPREAD = 2, MIX = 3 };
enum { TIME_LEAST = 1, TIME_MOST = 4, FEEDBACK_MOST = 90, PERCENT = 100, SIXTEENTH = 24, UNITY = 65536 };
// The working memory: a head of four words, then the two lines, a megabyte in all.
enum { STATE_BYTES = 1 << 20, HEAD_WORDS = 4, LINE = ((STATE_BYTES >> 2) - HEAD_WORDS) >> 1 };
static_assert(LINE == 131070, "two lines of 131,070 frames behind the head");
// The longest period the time is worked out for in 32 bits: 2^32 - 1 over the most sixteenths' 96 96ths, written out.
static const u32 PERIOD_MOST = 44739242u;
static_assert(PERIOD_MOST * 96ull <= 0xffffffffull && (PERIOD_MOST + 1ull) * 96ull > 0xffffffffull, "the longest period that fits");

// One frame of the two lines, the left's and the right's side by side.
struct Echoes { int left, right; };
struct Lines {
    u32 at;                                               // where the lines are written next
    u32 filled;                                           // frames written since the lines were started anew: nothing older is read
    u32 quiet;                                            // frames since the last block that wrote anything but silence, at most LINE
    u32 spare;
    Echoes line[LINE];
};
static_assert(sizeof(Lines) == STATE_BYTES, "the head and the two lines are the megabyte asked");

// The pack's own card, written by tools/pack_build.py from its recipe.
extern "C" const Op1fCard op1f_card_ping OP1F_NAMED("op1f_card_ping");

// ---------------------------------------------------------------- the echo, on the audio thread

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

// The delay in frames for so many sixteenths at the block's period, held to the lines.
static inline u32 delay_of(u32 period, u32 sixteenths) {
    u32 p = period >> 6;                                  // a 96th of a beat, in frames times 2^12
    if (p > PERIOD_MOST) return LINE - 1;                // slower than the lines can hold anyway
    u32 d = (p * (sixteenths * (u32)SIXTEENTH)) >> 12;
    if (d < 1u) return 1u;
    return d > (u32)(LINE - 1) ? (u32)(LINE - 1) : d;
}

extern "C" void ping_process(const Op1fEffectBlock* now) OP1F_NAMED("ping_process");
extern "C" void ping_process(const Op1fEffectBlock* now) {
    if (now->bytes < (u32)BLOCK_LEVEL_8 || now->state == 0) return;
    Lines* lines = (Lines*)now->state;
    if (now->fresh) {                                     // started anew: the lines forgotten, not cleared
        lines->at = 0;
        lines->filled = 0;
        lines->quiet = LINE;
    }
    u32 on = now->on;
    if (!on && lines->quiet >= (u32)LINE) return;         // switched off, and the lines hold nothing: the sound goes by
    const int* v = now->values;
    u32 delay = delay_of(now->period, held(v[TIME], TIME_LEAST, TIME_MOST));
    u32 back = gain(held(v[FEEDBACK], 0, FEEDBACK_MOST));
    u32 far = (UNITY - gain(held(v[SPREAD], 0, PERCENT))) >> 1;   // (1 - spread) / 2: what each side gives the other
    u32 mix = gain(held(v[MIX], 0, PERCENT));
    u32 keep = on ? UNITY - mix : UNITY;                  // switched off, the sound itself passes whole
    u32 in_mask = on ? 0xffffffffu : 0u;                  // switched off, nothing goes into the lines
    u32 frames = now->frames > (u32)FRAMES ? (u32)FRAMES : now->frames;
    int* left = now->left;
    int* right = now->right;
    Echoes* line = lines->line;
    u32 w = lines->at, filled = lines->filled, written = 0;
    for (u32 i = 0; i < frames; ++i) {
        int a = 0, b = 0;
        if (delay <= filled + i) {                        // older than the lines' start is silence
            const Echoes* back_then = &line[w >= delay ? w - delay : w + (u32)LINE - delay];
            a = back_then->left;
            b = back_then->right;
        }
        int dry_left = left[i], dry_right = right[i];
        int in = (int)((u32)((dry_left >> 1) + (dry_right >> 1)) & in_mask);
        int to_left = add_held(in, scaled(b, back));
        int to_right = scaled(a, back);
        line[w].left = to_left;
        line[w].right = to_right;
        written |= (u32)to_left | (u32)to_right;
        if (++w == (u32)LINE) w = 0;
        if (mix != 0) {                                   // at mix 0 the buffers are not written: the sound as it came
            int apart = scaled((a >> 1) - (b >> 1), far << 1);   // far x (a - b), from halves so that nothing overflows
            int wet_left = add_held(a, -apart), wet_right = add_held(b, apart);
            left[i] = add_held(scaled(dry_left, keep), scaled(wet_left, mix));
            right[i] = add_held(scaled(dry_right, keep), scaled(wet_right, mix));
        }
    }
    lines->at = w;
    lines->filled = filled + frames > (u32)LINE ? (u32)LINE : filled + frames;
    u32 quiet = lines->quiet + frames;                    // counted a block at a time: from the end of the last block that wrote
    lines->quiet = written != 0 ? 0u : quiet > (u32)LINE ? (u32)LINE : quiet;
}

// The display thread, before the first block after a place's effect was made anew: the place's memory cleared whole.
extern "C" void ping_reset(unsigned place, void* state) OP1F_NAMED("ping_reset");
extern "C" void ping_reset(unsigned place, void* state) {
    (void)place;
    if (state == 0) return;
    u32* word = (u32*)state;
    for (u32 i = 0; i < (u32)(STATE_BYTES >> 2); ++i) word[i] = 0;
    ((Lines*)state)->quiet = LINE;                        // nothing in the lines
}

// ---------------------------------------------------------------- the page, on the display thread

// What `shown` was told last: single words, written by `shown`, read by the frame function.
static volatile u32 shown_where, shown_how;

extern "C" void ping_shown(unsigned where, unsigned how) OP1F_NAMED("ping_shown");
extern "C" void ping_shown(unsigned where, unsigned how) {
    shown_where = where;
    shown_how = how;
}

static const char LOG_FILE[] = "op1f/ping.cpp";            // the log shows "ping"
static const unsigned KNOB[VALUES] = { OP1F_COLOUR(0x3c, 0x60, 0x87), OP1F_COLOUR(0x8a, 0x65, 0x3c),   // the field palette: blue, ochre,
                                       OP1F_COLOUR(0xa2, 0xa4, 0xa6), OP1F_COLOUR(0xe5, 0x28, 0x17) };  // grey and red, the four knobs
static const unsigned LIGHT_GREY = OP1F_COLOUR(0xcd, 0xd0, 0xc2);
static const unsigned GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);
enum { COLUMN_FIRST = 60, COLUMN_PITCH = 120, VALUE_Y = 86, VALUE_SCALE = 3, NAME_Y = 122, NAME_SCALE = 2, OFF_Y = 176, OFF_SCALE = 2 };
static const char TIMES[TIME_MOST][8] = { "1/16", "1/8", "3/16", "1/4" };

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
    if (up) op1f_log(3, LOG_FILE, 1, "ping: its page is on the screen for %s, switched %s", place_said(where, place), off ? "off" : "on");
    else op1f_log(3, LOG_FILE, 2, "ping: its page is no longer on the screen");
}

// The four values as the base hands them for the place whose page is up, each in its knob's colour over its name.
static void paint(void* canvas, u32 where, u32 how) {
    const Op1fEffect* effect = (const Op1fEffect*)op1f_card_ping.asks[ASK].what;
    for (u32 i = 0; i < (u32)VALUES; ++i) {
        int middle = COLUMN_FIRST + (int)i * COLUMN_PITCH;
        int value = 0;
        const char* text = "-";
        if (op1f_param_heard(&op1f_card_ping, ASK, where, i, &value) == 0) {       // R6d: as the sound plays it
            if (i == (u32)TIME) {
                text = TIMES[held(value, TIME_LEAST, TIME_MOST) - 1u];
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

extern "C" void ping_frame(const Op1fFrame* now) OP1F_NAMED("ping_frame");
extern "C" void ping_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    u32 how = shown_how, where = shown_where;
    say_page(how, where);
    if (now->page == &op1f_card_ping && now->canvas != 0 && (how & (u32)OP1F_SHOWN_UP)) paint(now->canvas, where, how);
}

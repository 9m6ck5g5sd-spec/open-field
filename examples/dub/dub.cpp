// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R4c, the example pack `dub`, version 3: dub as version 2 was (the source of dub 2, which stays as it was),
// with two changes and nothing else:
//   "Not now" is not "no". The base answers OP1F_NOT_NOW where it cannot take or give a value at this moment (from base
//   27 on, a tape's value while a tape load waits to be told to the packs; or another thread in the way): the value is
//   asked again at the next frame, as the other packs do, and nothing is said of it: it is no refusal. Version 2 took
//   any refusal as "the base does not keep dub's values with the tape" and went back to dub.bin until the next tape was
//   loaded. Any other refusal is still that, said as before.
//   The words "no memory for the echo" wait until the base must have handed the memory over. The base takes the memory
//   at its first frame and clears it over the frames that follow, a quarter of a megabyte a frame, the asks of every pack
//   together at most OP1F_MEMORY_MOST; so by dub's HANDED_FRAMES-th frame every block it gives is handed over, and the
//   stage that waits for it runs. The wait is also at least READY_MS, counted from dub's second frame: the first frame
//   of a start can last seconds, and its length says nothing of the base's clearing. Version 2 counted READY_MS from
//   its first frame, and at a start whose first frame lasted nearly three seconds it said the words for one frame
//   while the base was still clearing the memory.
//
// What follows is version 2's description, which holds.
//
// Step R4b, the example pack `dub`, version 2: dub as version 1 was (the source of dub 1, which stays as it was),
// and one thing more: its three values go with each tape, through level 7 of the base (module/build_r4b_card.h).
//
// Its card asks the base, beside the memory and the sound stage, to keep one member in the tape's file: `dub_echo`, the
// three values as a JSON object, {"back":3,"level":0,"time":6,"version":1} (the `version` is the form's: a later dub
// that keeps another form says so, and this one then takes the tape as having none of its values). The base reads it
// when a tape is loaded and writes it when the unit writes the tape; dub reads and sets it from its frame function.
//
//   A tape loaded, switched to or just made (the base says so at the next frame, through dub_changed: OP1F_KEPT_LOADED
//   or OP1F_KEPT_MADE): dub takes that tape's values; a copy of a tape holds the original's, so it keeps its echo as it
//   keeps its sound. A new tape (just made, with none of dub's) takes dub's first values, time 3, back 3, level 0: dry,
//   as the unit sets its own for a new tape. A tape switched to that has none of dub's (one from before) takes the
//   values of dub.bin, dub's values where no tape holds them (dub 1's, or the first ones where there is no dub.bin).
//   Either keeps them from then on.
//   A knob turned while a tape is loaded: the tape's values change with it, written into the tape's file when the unit
//   next writes the tape. Load another tape, then this one again: its values come back.
//   No tape told of yet, or a base that does not keep the values for dub: dub.bin, as version 1 kept them, read at the
//   first frame and written when the page closes and something changed.
// dub.bin is read as version 1 wrote it, and written only while no tape keeps the values. The echo, the page, the
// keys and the knobs are version 1's, line for line.
//
// What follows is version 1's description, which holds.
//
// Step R4, the example pack `dub`: an echo on everything the unit plays, through level 6 of the base's interface
// (base 24's header for pack authors). It changes no place of the firmware and names no address of it.
//
// What the owner meets:
//   every note, the tape and anything else the unit plays, with an echo behind it, in time with the unit's tempo;
//   shift and the metronome key, in any mode: its page, the base's blank page painted by the pack, or closed when it
//     is the one shown. On it the word `dub` and three bars with their numbers:
//       time    the blue knob: the echo comes 1 to 8 sixteenth notes after the sound (3 at first)
//       back    the second knob: how much of the echo comes back again, 0.0 to 0.6 (0.3 at first)
//       level   the third knob: how loud the echo is, 0 to 10; 0 is off, and the sound goes out as it came in
//     While the page is shown it takes the four knobs' turns, presses and clicks, so that nothing moves unseen.
//   At its first start the level is 0: a unit that gets the pack sounds as before until the third knob is turned.
//   Its three values are kept in dub.bin, in a folder of its own that the computer does not see: read at the first
//   frame, written when the page closes and something changed (while the disk mode is open, at the first frame after).
//   If the base could not give it its memory, or switched its stage off, the page says so and the pack does nothing.
//
// What it asks of the base (its recipe, recipe.json; version 2 added the member): working memory of 1 MiB for its two delay lines (ask 0),
// and one sound stage at the unit's sum after the master effect, that changes the sound and waits for that memory
// (ask 1). The base calls the stage once a block of 64 frames on the audio thread.
//
// The stage, on the audio thread: it allocates nothing, writes no line, calls nothing of the base's but op1f_memory,
// and divides nothing. For each frame, left and right alike:
//   delayed = the line, `delay` frames back
//   line    = input + back x delayed           (saturated: never past full scale)
//   output  = input + level x delayed          (saturated), only when the level is not 0
// `back` is at most 0.6 and `level` under 1, so what comes back is always under full scale and every echo is softer
// than the one before. The products are rounded toward 0, so a tail dies away to exact silence. The delay is the
// time in sixteenth notes from the block's `period` (a sixteenth is period x 24 >> 18 frames), with no division,
// held to the line's length (131,072 frames, about three seconds). The frame function hands it three single words
// (the time, back and level), which it reads once a block; it counts its blocks and the frames it held at full scale
// in words only it writes, which the frame function reads and says on the log.
//
// The buffers are 32-bit signed words (Q1.31). Nothing is divided anywhere in the pack: the module links no runtime.

#include "build_r4b_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;
typedef unsigned char u8;

enum { KEY_METRONOME = 1, KNOBS = 4, KNOB_TIME = 0, KNOB_BACK = 1, KNOB_LEVEL = 2, PRESS0 = 61, CLICK0 = 54 };
enum { FOLDER = 0, ASK_MEMORY = 0, ASK_STAGE = 1, FRAME_LEVEL_4 = 40, INPUT_LEVEL_2 = 36, BLOCK_LEVEL_6 = 36, FRAMES = 64, PERIOD_FRAMES = 256 };
enum { TIME_LEAST = 1, TIME_MOST = 8, TIME_FIRST = 3, BACK_MOST = 6, BACK_FIRST = 3, LEVEL_MOST = 10, LEVEL_FIRST = 0 };
enum { LINE = 1 << 17, LINE_MASK = LINE - 1, SIXTEENTH = 24, READY_MS = 3000 };
// Version 3: the frames by which the base has handed every block of working memory over: the first takes it, each
// one after clears CLEAR_A_FRAME of all packs' asks together (at most OP1F_MEMORY_MOST), and the stage that waits for
// a block runs from the frame the block is handed over; one frame more to be sure. Worked out by the compiler.
enum { CLEAR_A_FRAME = 256 << 10, HANDED_FRAMES = 1 + (int)OP1F_MEMORY_MOST / CLEAR_A_FRAME + 1 };
static_assert(HANDED_FRAMES == 66, "the first frame, sixty-four of clearing, one more");
enum { FILE_MAGIC = 0x31425544u /* "DUB1" */, FILE_BYTES = 16 };
enum { TITLE_Y = 10, TITLE_SCALE = 4, STATUS_Y = 44, VALUE_Y = 58, VALUE_SCALE = 3, LABEL_Y = 84, BAR_TOP = 96, BAR_TALL = 120, BAR_WIDE = 24 };
// The bars' steps: each value's share of BAR_TALL, written out (nothing divided).
enum { TIME_STEP = 15, BACK_STEP = 20, LEVEL_STEP = 12 };
static_assert(TIME_STEP * TIME_MOST == BAR_TALL && BACK_STEP * BACK_MOST == BAR_TALL && LEVEL_STEP * LEVEL_MOST == BAR_TALL, "a full bar is the bar's height");
// The longest period the delay is worked out for in 32 bits: 2^32 - 1 over the most sixteenths' 192 96ths, written out.
static const unsigned PERIOD_MOST = 22369621u;
static_assert(PERIOD_MOST * 192ull <= 0xffffffffull && (PERIOD_MOST + 1ull) * 192ull > 0xffffffffull, "the longest period that fits");
enum { STARTING = 0, RUNNING = 1, NO_MEMORY = 2, SWITCHED_OFF = 3 };

static const char LOG_FILE[] = "op1f/dub.cpp";             // the log shows "dub"
static const char SAVED[] = "dub.bin";
static const unsigned BLUE = OP1F_COLOUR(0x3c, 0x60, 0x87);         // the field palette: the blue knob, the second (ochre),
static const unsigned OCHRE = OP1F_COLOUR(0x8a, 0x65, 0x3c);        // the third (grey), the red knob's red, light grey
static const unsigned GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);         // and the darker grey
static const unsigned RED = OP1F_COLOUR(0xe5, 0x28, 0x17);
static const unsigned LIGHT_GREY = OP1F_COLOUR(0xcd, 0xd0, 0xc2);
static const unsigned DARK_GREY = OP1F_COLOUR(0x37, 0x3d, 0x42);

// The gains, in 32768ths: back 0.0 to 0.6 by tenths, level 0 to 10 by tenths, the top under 1.
static const u32 BACK_GAIN[BACK_MOST + 1] = { 0, 3277, 6554, 9830, 13107, 16384, 19661 };
static const u32 LEVEL_GAIN[LEVEL_MOST + 1] = { 0, 3276, 6552, 9828, 13104, 16380, 19656, 22932, 26208, 29484, 32760 };
static const int COLUMN[3] = { 96, 240, 384 };

// The pack's own card, written by tools/pack_build.py from its recipe (examples/dub/dub_card.s for version 3).
extern "C" const Op1fCard op1f_card_dub OP1F_NAMED("op1f_card_dub");

// Written by the frame function, read by the stage once a block: single words.
static volatile u32 time_steps = TIME_FIRST, back_gain = 0, level_gain = 0;
// Written by the stage only, read by the frame function: single words.
static int* volatile lines;                               // the memory, once op1f_memory answered it
static volatile u32 blocks, held, unready, delay_said;    // blocks done, frames held at full scale, blocks with no memory, the last delay
// Written by the input function, read by the frame function: single words.
static volatile u32 keys;
static volatile int turned[3];                            // the three knobs' steps since the start, totals only the input function writes

// ---------------------------------------------------------------- the stage, on the audio thread

static u32 at;                                            // where the lines are written next: the stage's alone

// a + b, held at full scale.
static inline int add_held(int a, int b) {
    u32 s = (u32)a + (u32)b;
    if ((int)(~((u32)a ^ (u32)b) & ((u32)a ^ s)) < 0) return a < 0 ? (int)0x80000000u : 0x7fffffff;
    return (int)s;
}

// x times g / 32768, g from 0 to 32767, rounded toward 0: the magnitude scaled, so that a small value goes to 0.
static inline int scaled(int x, u32 g) {
    u32 m = x < 0 ? 0u - (u32)x : (u32)x;                 // up to 2^31
    u32 p = (((m >> 16) * g) << 1) + (((m & 0xffffu) * g) >> 15);   // at most 2^31 - 2: no overflow
    return x < 0 ? (int)(0u - p) : (int)p;
}

// The delay in frames for so many sixteenths at the block's period, held to the lines.
static inline u32 delay_of(u32 period, u32 steps) {
    u32 p = period >> 6;                                  // a 96th of a beat, in frames times 2^12
    if (p > PERIOD_MOST) return LINE - 1;                // slower than the lines can hold anyway
    u32 d = (p * (steps * (u32)SIXTEENTH)) >> 12;
    if (d < 1u) return 1u;
    return d > (u32)(LINE - 1) ? (u32)(LINE - 1) : d;
}

extern "C" void dub_process(const Op1fBlock* now) OP1F_NAMED("dub_process");
extern "C" void dub_process(const Op1fBlock* now) {
    if (now->bytes < (u32)BLOCK_LEVEL_6) return;
    int* line = lines;
    if (line == 0) {
        line = (int*)op1f_memory(&op1f_card_dub, ASK_MEMORY);
        if (line == 0) {                                  // the base calls no stage before its memory: never, but kept safe
            ++unready;
            return;
        }
        lines = line;
    }
    u32 steps = time_steps;
    if (steps < (u32)TIME_LEAST || steps > (u32)TIME_MOST) steps = TIME_FIRST;
    u32 delay = delay_of(now->period, steps), back = back_gain, level = level_gain;
    u32 frames = now->frames > (u32)FRAMES ? (u32)FRAMES : now->frames;
    int* left = now->left;
    int* right = now->right;
    int* line_left = line;
    int* line_right = line + LINE;
    u32 w = at, full = 0;
    for (u32 i = 0; i < frames; ++i) {
        u32 r = (w - delay) & (u32)LINE_MASK;
        int dl = line_left[r], dr = line_right[r];
        int xl = left[i], xr = right[i];
        line_left[w] = add_held(xl, scaled(dl, back));
        line_right[w] = add_held(xr, scaled(dr, back));
        if (level != 0) {
            int yl = add_held(xl, scaled(dl, level)), yr = add_held(xr, scaled(dr, level));
            full += (yl == 0x7fffffff || yl == (int)0x80000000u || yr == 0x7fffffff || yr == (int)0x80000000u) ? 1u : 0u;
            left[i] = yl;
            right[i] = yr;
        }
        w = (w + 1u) & (u32)LINE_MASK;
    }
    at = w;
    held = held + full;
    delay_said = delay;
    blocks = blocks + 1u;
}

// ---------------------------------------------------------------- the knobs and the key, on the thread that handles keys

extern "C" int dub_input(const Op1fInput* e) OP1F_NAMED("dub_input");
extern "C" int dub_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2) return 0;
    if (e->what == (u32)OP1F_DOWN_SHIFT && e->key == (u32)KEY_METRONOME) {     // the way in, in any mode
        ++keys;
        if (e->mine) op1f_page_close(&op1f_card_dub);
        else op1f_page_show(&op1f_card_dub, 0, 0);                             // the base's blank page, painted each frame
        return 1;
    }
    if (e->what == (u32)OP1F_UP) return 1;                                     // the up of a key whose down this pack took
    if (!e->mine) return 0;                                                    // the rest only while the page is shown
    if (e->what == (u32)OP1F_TURN) {
        if (e->key >= (u32)KNOBS) return 0;
        if (e->key < 3u) turned[e->key] = turned[e->key] + (e->turn >> 10);   // 1024 a step; more than 0 is up
        return 1;                                                              // the red knob too: nothing moves unseen
    }
    if (e->key >= (u32)PRESS0 && e->key < (u32)(PRESS0 + KNOBS)) return 1;
    if (e->key >= (u32)CLICK0 && e->key < (u32)(CLICK0 + KNOBS)) return 1;
    return 0;
}

// ---------------------------------------------------------------- the frame function, on the display thread

static struct {
    u32 started, start_ms;                                // version 3: start_ms is the second frame's
    u32 frames;                                           // version 3: dub's frames since the start
    u32 time, back, level;                                // the values as the owner set them
    int seen[3];                                          // the knobs' totals as last applied
    u32 shown, changed, to_write;
    u32 state, said_state;
    u32 writes, waits, failed, blocks_seen, held_seen;
    u32 file[FILE_BYTES >> 2];
    char text[16];
    int countdown;
    QuietGate quiet;
} state;

static void hand_over() {
    time_steps = state.time;
    back_gain = BACK_GAIN[state.back];
    level_gain = LEVEL_GAIN[state.level];
}

static void load() {
    int got = op1f_read(&op1f_card_dub, FOLDER, SAVED, state.file, sizeof state.file);
    if (got < 0) {
        op1f_log(3, LOG_FILE, 1, "dub: no dub.bin read at the start (answer %d): time %u, back %u, level %u", got, state.time, state.back, state.level);
        return;
    }
    u32* f = state.file;
    if (got != (int)FILE_BYTES || f[0] != (u32)FILE_MAGIC || f[1] < (u32)TIME_LEAST || f[1] > (u32)TIME_MOST || f[2] > (u32)BACK_MOST || f[3] > (u32)LEVEL_MOST) {
        op1f_log(2, LOG_FILE, 2, "dub: dub.bin of %d bytes is not this pack's: the values it starts with are kept", got);
        return;
    }
    state.time = f[1];
    state.back = f[2];
    state.level = f[3];
    op1f_log(3, LOG_FILE, 3, "dub: dub.bin read at the start: time %u, back %u, level %u", state.time, state.back, state.level);
}

static void save() {
    state.file[0] = (u32)FILE_MAGIC;
    state.file[1] = state.time;
    state.file[2] = state.back;
    state.file[3] = state.level;
    int answer = op1f_write(&op1f_card_dub, FOLDER, SAVED, state.file, FILE_BYTES);
    if (answer == (int)FILE_BYTES) {
        ++state.writes;
        state.to_write = 0;
        op1f_log(3, LOG_FILE, 4, "dub: dub.bin written: time %u, back %u, level %u", state.time, state.back, state.level);
    } else if (answer == OP1F_NOT_NOW) {
        ++state.waits;                                    // written when it may be
    } else {
        ++state.failed;
        state.to_write = 0;                               // tried again at the next change
        op1f_log(2, LOG_FILE, 5, "dub: dub.bin could not be written (answer %d)", answer);
    }
}

static u32 moved(u32 value, int steps, u32 least, u32 most) {
    int v = (int)value + steps;
    return v < (int)least ? least : v > (int)most ? most : (u32)v;
}

static u32 put(char* out, u32 at_, const char* text) {
    while (*text && at_ + 1 < sizeof state.text) out[at_++] = *text++;
    out[at_] = 0;
    return at_;
}

static u32 put_digit(char* out, u32 at_, u32 n) {        // 0 to 10
    if (n >= 10u) { at_ = put(out, at_, "1"); n -= 10u; }
    char d[2] = { (char)('0' + n), 0 };
    return put(out, at_, d);
}

static void paint_value(void* canvas, int middle, int y, const char* text, unsigned colour, int scale) {
    int characters = 0;
    while (text[characters]) ++characters;
    int wide = (6 * characters - 1) * scale;                                   // the base's font: five pixels and a space
    op1f_paint_text(canvas, middle - (wide >> 1), y, text, colour, scale);
}

static void paint_bar(void* canvas, int middle, u32 tall, unsigned colour) {
    int x = middle - (BAR_WIDE >> 1);
    op1f_paint_box(canvas, x, BAR_TOP, BAR_WIDE, BAR_TALL, DARK_GREY);
    if (tall != 0) op1f_paint_box(canvas, x, BAR_TOP + BAR_TALL - (int)tall, BAR_WIDE, (int)tall, colour);
}

static const char* status_words(u32 s) {
    return s == NO_MEMORY ? "no memory for the echo: dub is off" : s == SWITCHED_OFF ? "the base switched dub off" : "dub is getting ready";
}

static void paint(void* canvas) {
    paint_value(canvas, 240, TITLE_Y, "dub", LIGHT_GREY, TITLE_SCALE);
    u32 n = put_digit(state.text, 0, state.time);
    put(state.text, n, "/16");
    paint_value(canvas, COLUMN[0], VALUE_Y, state.text, BLUE, VALUE_SCALE);
    n = put(state.text, 0, "0.");
    put_digit(state.text, n, state.back);
    paint_value(canvas, COLUMN[1], VALUE_Y, state.text, OCHRE, VALUE_SCALE);
    put_digit(state.text, 0, state.level);
    paint_value(canvas, COLUMN[2], VALUE_Y, state.text, GREY, VALUE_SCALE);
    paint_value(canvas, COLUMN[0], LABEL_Y, "time", GREY, 1);
    paint_value(canvas, COLUMN[1], LABEL_Y, "back", GREY, 1);
    paint_value(canvas, COLUMN[2], LABEL_Y, "level", GREY, 1);
    paint_bar(canvas, COLUMN[0], state.time * TIME_STEP, BLUE);
    paint_bar(canvas, COLUMN[1], state.back * BACK_STEP, OCHRE);
    paint_bar(canvas, COLUMN[2], state.level * LEVEL_STEP, GREY);
    if (state.state != (u32)RUNNING) paint_value(canvas, 240, STATUS_Y, status_words(state.state), RED, 1);
}

// ---------------------------------------------------------------- version 2: the values with the tape

static const char KEPT[] = "dub_echo";                   // the member of the tape's file, the card's ask of kind 6
enum { KEPT_VERSION = 1, KEPT_ROOM = 96, TAPE_LINES = 16 };

// Written by dub_changed (the display thread, outside the frame function), taken by the frame function: single words.
static volatile u32 told;
static volatile u32 last_load;                            // the later of the tape's loads told: OP1F_KEPT_LOADED or OP1F_KEPT_MADE

static struct {
    u32 tape;                                             // a tape was told, and the base keeps dub's values with it
    u32 own_time, own_back, own_level;                    // dub.bin's values (or the first ones): a tape switched to with none takes them
    u32 held, held_time, held_back, held_level;           // what the base holds for the tape, as last read or set (held 0: unknown)
    u32 lines;
    u32 load_waits;                                       // version 3: a load told and not taken yet, the base said not now: 1 switched to, 2 just made
    char text[KEPT_ROOM];
} keeps;

extern "C" void dub_changed(unsigned where, unsigned which, unsigned how) OP1F_NAMED("dub_changed");
extern "C" void dub_changed(unsigned where, unsigned which, unsigned how) {
    if (where != (unsigned)OP1F_KEEP_TAPE || which != 0 || how > 31u) return;
    told |= 1u << how;
    if (how == (unsigned)OP1F_KEPT_LOADED || how == (unsigned)OP1F_KEPT_MADE) last_load = how;   // of two loads in one frame, the later counts
}

static const char* skip(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    return p;
}

// The three values out of the member's text: an object of whole numbers by name, in any order. 1 when it holds a
// time, a back and a level within their ranges and its version is 1 (or it has none); 0 otherwise, nothing taken.
static int read_kept(const char* t, u32* time, u32* back, u32* level) {
    int got[4] = { -1, -1, -1, KEPT_VERSION };            // time, back, level, version
    const char* p = skip(t);
    if (*p != '{') return 0;
    p = skip(p + 1);
    if (*p == '}') return 0;
    for (;;) {
        if (*p != '"') return 0;
        const char* name = ++p;
        while (*p && *p != '"') ++p;
        if (*p != '"') return 0;
        u32 n = (u32)(p - name);
        p = skip(p + 1);
        if (*p != ':') return 0;
        p = skip(p + 1);
        int negative = *p == '-';
        if (negative) ++p;
        if (*p < '0' || *p > '9') return 0;              // a whole number, or the value is not dub's
        int v = 0;
        while (*p >= '0' && *p <= '9') {
            if (v > 100000) return 0;
            v = v * 10 + (*p - '0');
            ++p;
        }
        if (*p == '.' || *p == 'e' || *p == 'E') return 0;
        if (negative) v = -v;
        int k = n == 4 && name[0] == 't' && name[1] == 'i' && name[2] == 'm' && name[3] == 'e' ? 0
              : n == 4 && name[0] == 'b' && name[1] == 'a' && name[2] == 'c' && name[3] == 'k' ? 1
              : n == 5 && name[0] == 'l' && name[1] == 'e' && name[2] == 'v' && name[3] == 'e' && name[4] == 'l' ? 2
              : n == 7 && name[0] == 'v' && name[1] == 'e' && name[2] == 'r' && name[3] == 's' && name[4] == 'i' && name[5] == 'o' && name[6] == 'n' ? 3
              : -1;
        if (k >= 0) got[k] = v;                           // a member of another name is left alone
        p = skip(p);
        if (*p == ',') { p = skip(p + 1); continue; }
        if (*p != '}' || *skip(p + 1) != 0) return 0;
        break;
    }
    if (got[3] != KEPT_VERSION || got[0] < TIME_LEAST || got[0] > TIME_MOST || got[1] < 0 || got[1] > BACK_MOST || got[2] < 0 || got[2] > LEVEL_MOST) return 0;
    *time = (u32)got[0];
    *back = (u32)got[1];
    *level = (u32)got[2];
    return 1;
}

static u32 put_small(char* out, u32 at, u32 n) {          // 0 to 10
    if (n >= 10u) { out[at++] = '1'; n -= 10u; }
    out[at++] = (char)('0' + n);
    return at;
}

static u32 put_text(char* out, u32 at, const char* text) {
    while (*text) out[at++] = *text++;
    return at;
}

// The member's text for the three values, its members in the order of their names, as the base hands it back.
static void text_kept(u32 time, u32 back, u32 level) {
    char* t = keeps.text;
    u32 at = put_text(t, 0, "{\"back\":");
    at = put_small(t, at, back);
    at = put_text(t, at, ",\"level\":");
    at = put_small(t, at, level);
    at = put_text(t, at, ",\"time\":");
    at = put_small(t, at, time);
    at = put_text(t, at, ",\"version\":");
    at = put_small(t, at, KEPT_VERSION);
    t[at++] = '}';
    t[at] = 0;
}

// Back to dub.bin: the base does not keep the values for dub (its answer), said once.
static void keeps_off(int answer) {
    keeps.tape = 0;
    keeps.held = 0;
    if (keeps.lines < (u32)TAPE_LINES) {
        ++keeps.lines;
        op1f_log(2, LOG_FILE, 15, "dub: the base does not keep dub's values with the tape (answer %d): dub.bin keeps them", answer);
    }
}

// A tape was loaded: its values; with none of dub's, the first values for a tape just made (`made`), dub.bin's for a
// tape switched to. Version 3: while the base says not now, the load waits and is taken at a later frame.
static void tape_loaded(int made) {
    int got = op1f_value_get(&op1f_card_dub, OP1F_KEEP_TAPE, 0, KEPT, keeps.text, sizeof keeps.text);
    if (got == OP1F_NOT_NOW) return;                      // version 3: not now: asked again at the next frame, nothing said
    keeps.load_waits = 0;
    if (got < 0 && got != OP1F_NO_FILE && got != OP1F_TOO_LARGE) {
        keeps_off(got);
        return;
    }
    keeps.tape = 1;
    u32 time = 0, back = 0, level = 0;
    if (got >= 0 && read_kept(keeps.text, &time, &back, &level)) {
        state.time = time;
        state.back = back;
        state.level = level;
        keeps.held = 1;
        keeps.held_time = time;
        keeps.held_back = back;
        keeps.held_level = level;
        if (keeps.lines < (u32)TAPE_LINES) {
            ++keeps.lines;
            op1f_log(3, LOG_FILE, 12, "dub: the tape's values: time %u, back %u, level %u", time, back, level);
        }
    } else {
        state.time = made ? (u32)TIME_FIRST : keeps.own_time;
        state.back = made ? (u32)BACK_FIRST : keeps.own_back;
        state.level = made ? (u32)LEVEL_FIRST : keeps.own_level;
        keeps.held = 0;
        if (keeps.lines < (u32)TAPE_LINES) {
            ++keeps.lines;
            op1f_log(3, LOG_FILE, 13, "dub: the tape has none of dub's values (answer %d): %s taken: time %u, back %u, level %u", got,
                     made ? "a new tape: dub's first values" : "dub.bin's", state.time, state.back, state.level);
        }
    }
}

// Once a frame, after the knobs: the tape's values made what dub's are, when the base holds something else.
static void tape_keep() {
    if (!keeps.tape || keeps.load_waits) return;              // version 3: nothing set over a tape whose load is still to be taken
    if (keeps.held && keeps.held_time == state.time && keeps.held_back == state.back && keeps.held_level == state.level) return;
    text_kept(state.time, state.back, state.level);
    int answer = op1f_value_set(&op1f_card_dub, OP1F_KEEP_TAPE, 0, KEPT, keeps.text);
    if (answer == OP1F_NOT_NOW) return;                   // version 3: not now: set at the next frame, nothing said
    if (answer != 0) {
        keeps_off(answer);
        state.changed = 1;                                // dub.bin keeps them now: written at the page's close
        return;
    }
    keeps.held = 1;
    keeps.held_time = state.time;
    keeps.held_back = state.back;
    keeps.held_level = state.level;
}

// What the base does with the pack's memory and stage now.
static u32 state_now(u32 ms) {
    if (op1f_stage(&op1f_card_dub, ASK_STAGE)) return RUNNING;
    if (op1f_memory(&op1f_card_dub, ASK_MEMORY) != 0) return SWITCHED_OFF;
    if (state.frames < (u32)HANDED_FRAMES || ms - state.start_ms < (u32)READY_MS) return STARTING;   // version 3: the base may be clearing it still
    return NO_MEMORY;
}

extern "C" void dub_frame(const Op1fFrame* now) OP1F_NAMED("dub_frame");
extern "C" void dub_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    if (state.frames < (u32)HANDED_FRAMES) ++state.frames;                    // version 3: counted up to where it matters
    if (state.frames == 2u) state.start_ms = now->ms;                          // version 3: the wait for the memory counts from the second frame
    if (!state.started) {                                                      // the first frame: the values, once
        state.started = 1;
        state.time = TIME_FIRST;
        state.back = BACK_FIRST;
        state.level = LEVEL_FIRST;
        state.countdown = PERIOD_FRAMES;
        load();
        keeps.own_time = state.time;                                          // version 2: what a tape with none of dub's takes
        keeps.own_back = state.back;
        keeps.own_level = state.level;
        hand_over();
    }
    u32 bits = told;                                                           // version 2: a tape loaded, or written
    told = 0;
    if (bits & ((1u << OP1F_KEPT_LOADED) | (1u << OP1F_KEPT_MADE))) keeps.load_waits = last_load == (u32)OP1F_KEPT_MADE ? 2u : 1u;
    if (keeps.load_waits) tape_loaded(keeps.load_waits == 2u);                 // version 3: taken now, or at a later frame while the base says not now
    if ((bits & (1u << OP1F_KEPT_SAVED)) && keeps.tape && keeps.lines < (u32)TAPE_LINES) {
        ++keeps.lines;
        op1f_log(3, LOG_FILE, 14, "dub: its values written into the tape: time %u, back %u, level %u", keeps.held_time, keeps.held_back, keeps.held_level);
    }
    u32 s = state_now(now->ms);
    if (s != state.state || !state.said_state) {
        state.state = s;
        if (s != (u32)STARTING) {
            state.said_state = 1;
            if (s == (u32)RUNNING) op1f_log(3, LOG_FILE, 6, "dub: the echo runs: time %u, back %u, level %u", state.time, state.back, state.level);
            else op1f_log(2, LOG_FILE, 7, "dub: %s", status_words(s));
        }
    }
    u32 values[3] = { state.time, state.back, state.level };
    static const u32 LEAST[3] = { TIME_LEAST, 0, 0 }, MOST[3] = { TIME_MOST, BACK_MOST, LEVEL_MOST };
    for (u32 k = 0; k < 3u; ++k) {
        int total = turned[k], steps = total - state.seen[k];
        state.seen[k] = total;
        u32 v = moved(values[k], steps, LEAST[k], MOST[k]);
        if (v != values[k]) {
            values[k] = v;
            state.changed = 1;
        }
    }
    state.time = values[0];
    state.back = values[1];
    state.level = values[2];
    hand_over();
    tape_keep();                                                               // version 2
    u32 shown = now->page == &op1f_card_dub ? 1u : 0u;
    if (shown && !state.shown) op1f_log(3, LOG_FILE, 8, "dub: its page is shown");
    if (!shown && state.shown) {
        if (state.changed) {
            state.changed = 0;
            if (!keeps.tape) state.to_write = 1;                               // version 2: with a tape, the tape keeps them
        }
        op1f_log(3, LOG_FILE, 9, "dub: its page is no longer shown: time %u, back %u, level %u, %s", state.time, state.back, state.level,
                 keeps.tape ? "kept with the tape" : state.to_write ? "dub.bin to be written" : "dub.bin as it was");
    }
    state.shown = shown;
    if (state.to_write && !now->disk) save();                                  // the computer owns the folder while the disk mode is open
    if (shown && now->canvas != 0) paint(now->canvas);
    int why = quiet_popup(&state.quiet, now->popup != 0);
    int period = --state.countdown <= 0;
    if (!period && !why) return;
    u32 b = blocks, h = held;
    if (period) {
        state.countdown = PERIOD_FRAMES;
        u32 sig = quiet_mix(quiet_start(), state.state);
        sig = quiet_mix(sig, state.time);
        sig = quiet_mix(sig, state.back);
        sig = quiet_mix(sig, state.level);
        sig = quiet_mix(sig, h != state.held_seen ? 1u : 0u);
        sig = quiet_mix(sig, keys + state.writes + state.waits + state.failed);
        why |= quiet_period(&state.quiet, sig, shown);
    }
    if (!why) return;
    op1f_log(3, LOG_FILE, 10, "dub: %u blocks since the period before, delay %u frames, %u frames held at full scale, %u without memory; time %u, back %u, level %u",
             b - state.blocks_seen, delay_said, h - state.held_seen, unready, state.time, state.back, state.level);
    state.blocks_seen = b;
    state.held_seen = h;
    if (state.quiet.silent_before)
        op1f_log(3, LOG_FILE, 11, "dub quiet: these lines after %u periods held (%s); %u printed, %u held", state.quiet.silent_before, quiet_why(why),
                 state.quiet.spoken, state.quiet.skipped);
}

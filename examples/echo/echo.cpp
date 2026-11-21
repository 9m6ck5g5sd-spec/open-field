// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R4c, the example pack `echo`, version 2: version 1 (the source of echo 1, which
// stays as it was for version 1) with one change and nothing else. Its page is painted as usual while the unit's own
// "sequencer off" notice lies over it (the ask's `shown` says OP1F_SHOWN_COVERED): from base 27 on the base darkens
// the frame after every frame function and draws the unit's notice over it, as the unit does over its own sequencers,
// so the owner sees echo's page dark under the notice. Version 1 painted nothing then, which under base 27 leaves the
// notice on a black page. It still needs op1f_level_5 alone. What version 1's text said follows and holds, but for
// the painting under the notice.
//
// Step R3, the example pack `echo`: a sequencer of its own, through level 5 of the base's interface
// (module/build_r3_card.h). It changes no place of the firmware and names no address of it.
//
// What the owner meets:
//   `echo` at the end of the list of sequencers, for the synth and for the drum. Chosen and switched on, every key
//     played sounds, begun by the pack at the velocity played and ended when the key comes up, and then repeats in
//     time, each repeat softer than the one before: repeat k comes k
//     steps after the key, a step being a quarter, an eighth, a sixteenth or a thirty-second note; each repeat's
//     velocity is the one before less a quarter of it, never under 1; each repeat lasts half a step;
//   its page, in place of the sequencer's screen: the base's blank page, on which it paints the two values at the
//     top, how many repeats in the blue knob's colour and the step (1/4 to 1/32) in the second knob's, and under them
//     a row for each note that is echoing: the note's name, a dot for the key and for each repeat, a dot that has
//     sounded drawn as bright as it was loud, so that the row fades as the echo does, and a small grey point for each
//     repeat still to come;
//   the blue knob sets the repeats, 0 to 7 (3 at first); the second knob the step (an eighth at first; turned up,
//     the echoes come closer). Only while its page is the one on the screen: it takes those two knobs' turns and
//     lets everything else go, so the keys of the keyboard reach its `key`;
//   a key held is never cut by an echo: a repeat that falls due while its note still sounds (the key held, or another
//     repeat of it) is left out, as the base answers, and shown on the page as a short dash; and a key played again
//     ends the pack's own sounding note of that number first, so that the key sounds afresh;
//   MIDI notes beyond the keyboard's two octaves echo too (the ask's `all_midi`);
//   switched off, another sequencer chosen, or the unit changing instrument: the echoes stop at once;
//   its two values kept in echo.bin, in a folder of its own that the computer does not see.
//
// The threads. `key` comes on whatever thread brings the key (the audio thread among them), `tick` on the
// sequencer thread, `run` on the key thread or a MIDI handler's: none of them writes a line. `key` takes every key:
// the firmware tells a sequencer a key's up only when it took the key's down, so the pack plays the key's note
// itself, begun at the down and ended at the up. It keeps which keys are held and the note each began, puts a key
// down into a small queue, one for each instrument, and ends the pack's own sounding note of that number before it
// begins the key's; `tick` takes the queue in, plays what is due with op1f_note_on and op1f_note_off, and keeps the
// echoes; `run` empties the queue and the echoes at a stop and at a start, and says so to `key` by a count, so that
// `key` forgets the keys it held (their ups may have come while the sequencer was off). Everything the three share,
// and what the frame function reads of the echoes to paint them, is changed and read only inside op1f_mask
// sections, and no call of the base's is made inside one: each section decides, and the call is made after it.
// The input function counts the knobs' steps in single words; the frame function does the rest: the settings'
// file, the lines on the log, the painting.
//
// The queue's limit: eight echoing notes for each instrument. A ninth key takes the place of the oldest echo,
// whose sounding repeat is ended and whose repeats still to come are dropped. The keys waiting for the next tick
// are sixteen at most; a key past them (sixteen keys within one pulse of the clock) still sounds, with no echo,
// and is counted.
//
// echo.bin: a word "ECHO", the repeats (0 to 7), the step (0 a quarter to 3 a thirty-second), three words in the
// unit's own byte order, twelve bytes.
//
// Nothing is divided, nothing of a runtime is called: the steps are a small table and "less a quarter" is a shift.

#include "build_r3_card.h"
#include "build_q_quiet.h"

typedef unsigned int u32;
typedef unsigned char u8;

enum { HOSTS = 2, ECHOES = 8, WAITING = 16, REPEATS_MOST = 7, REPEATS_FIRST = 3, STEPS = 4, STEP_FIRST = 1 };
enum { KNOB_BLUE = 0, KNOB_SECOND = 1, FOLDER = 0, ASK = 0, SETTLE_MS = 1000, PERIOD_FRAMES = 256, SAID_MOST = 16 };
enum { FRAME_LEVEL_4 = 40, INPUT_LEVEL_2 = 36, TICK_LEVEL_5 = 32, KEY_LEVEL_5 = 32 };
enum { NOTES = 128, PLACES = 128, PLACE_OF_KEY = 53, SOURCES = 2, TOTALS = 6 };   // a key's place: its number + 53, a MIDI note's own number
enum { FILE_MAGIC = 0x4f484345u /* "ECHO" */, FILE_BYTES = 12 };
enum { VALUE_Y = 14, VALUE_SCALE = 3, BLUE_MIDDLE = 60, SECOND_MIDDLE = 180, LABEL_Y = 42, RULE_X = 16, RULE_Y = 56, RULE_WIDE = 448 };
enum { ROW_FIRST = 76, ROW_PITCH = 18, NAME_X = 16, NAME_SCALE = 2, NAME_HALF = 7, DOT_FIRST = 96, DOT_PITCH = 44, DOT = 9, DOT_HALF = 4, POINT = 3,
       DASH = 7 };

static const char LOG_FILE[] = "op1f/echo.cpp";            // the log shows "echo"
static const char SAVED[] = "echo.bin";
static const unsigned BLUE = OP1F_COLOUR(0x3c, 0x60, 0x87);         // the field palette: the blue knob, the second (ochre),
static const unsigned OCHRE = OP1F_COLOUR(0x8a, 0x65, 0x3c);        // grey, light grey and the darker grey
static const unsigned GREY = OP1F_COLOUR(0xa2, 0xa4, 0xa6);
static const unsigned LIGHT_GREY = OP1F_COLOUR(0xcd, 0xd0, 0xc2);
static const unsigned DARK_GREY = OP1F_COLOUR(0x37, 0x3d, 0x42);

static const u32 STEP_PULSES[STEPS] = { 96, 48, 24, 12 };          // a quarter, an eighth, a sixteenth, a thirty-second note
static const u32 STEP_NOTE[STEPS] = { 4, 8, 16, 32 };              // what the page and the log show behind "1/"
static const char NOTE_NAMES[12][3] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// A dot: a disc with soft edges, as cover for op1f_paint_mask.
static const u8 DOT_COVER[DOT * DOT] = {
      0,   4, 108, 199, 223, 199, 108,   4,   0,
      4, 171, 255, 255, 255, 255, 255, 171,   4,
    108, 255, 255, 255, 255, 255, 255, 255, 108,
    199, 255, 255, 255, 255, 255, 255, 255, 199,
    223, 255, 255, 255, 255, 255, 255, 255, 223,
    199, 255, 255, 255, 255, 255, 255, 255, 199,
    108, 255, 255, 255, 255, 255, 255, 255, 108,
      4, 171, 255, 255, 255, 255, 255, 171,   4,
      0,   4, 108, 199, 223, 199, 108,   4,   0,
};

// The pack's own card, written by tools/pack_build.py from its recipe.
extern "C" const Op1fCard op1f_card_echo OP1F_NAMED("op1f_card_echo");

// A key heard by `key`, waiting for the next tick: its note, its velocity, and the repeats and the step it was
// played with.
struct Waiting { u8 note, velocity, repeats, step; };

// A note echoing. `start` is the tick's count at which it was taken in; repeat k begins at start + k steps and ends
// half a step later. `played` is how many repeats have come (begun, or left out under a held key: bit k of `left`);
// `sounding` is 1 while the last one sounds.
struct Echo {
    u32 start, serial;
    u8 note, first, velocity, repeats, step, played, sounding, used, left;
};

// One instrument's queue and echoes: changed and read only inside op1f_mask sections.
struct Host {
    Waiting waiting[WAITING];
    u32 head, tail;                                       // the waiting keys: `key` writes at the tail, `tick` reads at the head
    Echo echoes[ECHOES];
    u8 held[SOURCES][PLACES];                             // `key`'s: for each key held, by the keyboard or MIDI and its place, its note + 1
    u8 held_notes[NOTES];                                 // `key`'s: how many keys of each note are held
    u32 stops, stops_seen;                                // `run` counts its calls; `key` forgets the held keys when it sees a new count
};

static Host hosts[HOSTS];
static u32 serials;                                       // the echoes' serial numbers, oldest the lowest (tick's, masked)

// Totals since the start, each written by one side only (or inside a section), read by the frame function.
static volatile u32 heard, lost;                          // keys down heard; keys that found the waiting queue full
static volatile u32 played, refused, cut, left_out;       // repeats the base took; notes it refused; echoes cut short for room; repeats left out under a held key
static volatile u32 turned_repeats, turned_step, turns;   // the knobs' steps since the start, totals only the input function writes
static volatile u32 repeats_now, step_now;                // the values `key` reads: written by the frame function only
static volatile u32 shown_where, shown_how;               // what `shown` was told last

// The frame function's own.
static struct {
    u32 started;                                          // the first frame has read the file
    u32 stood, refusal_said;                              // the base took the stand-in; its refusal said
    u32 repeats, step;                                    // the two values
    u32 seen_repeats, seen_step;                          // the knobs' totals as last applied
    u32 tried_repeats, tried_step;                        // the values echo.bin holds, or was last tried with
    u32 last_repeats, last_step, changed_at;              // the values at the last change, and when
    u32 on_page;                                          // the page was ours at the last frame
    u32 said_how, shows_said;                             // what the last line about the page said; how many such lines
    u32 writes, failed;
    u32 printed[TOTALS];                                  // the totals at the last period line
    u32 file[FILE_BYTES >> 2];
    char text[8];
    Echo rows[ECHOES];                                    // a copy of the shown instrument's echoes, for the painting
    int countdown;
    QuietGate quiet;
} state;

// ---------------------------------------------------------------- the sequencer's calls

// The next repeat's velocity: the one before less a quarter of it. It never comes under 1: a quarter of 1, 2 or 3
// is nothing, so those stay as they are.
static u32 softer(u32 v) {
    return v - (v >> 2);
}

static void end_note(u32 note) {
    if (!op1f_note_off(&op1f_card_echo, note, 0)) ++refused;
}

// A key or a MIDI note, before the instrument plays it. The pack takes it (answers 1) and plays the note itself, so
// that its up comes here too: a sequencer is told the up only of a down it took. A down begins the note at the
// velocity played, after ending the pack's own sounding note of that number (a repeat, or the same note under
// another key), so that the key sounds afresh, and is put into its instrument's queue with the values of the moment,
// for the next tick to take in. An up, matched to its down by the key's number (the octave may have moved between
// the two), ends the note that down began, unless another key still holds the same note: the last of them ends it.
// A key whose number lies outside the list of keys held is left to the instrument, with no echo: the pack could not
// match its up.
extern "C" int echo_key(const Op1fKey* e) OP1F_NAMED("echo_key");
extern "C" int echo_key(const Op1fKey* e) {
    if (e->bytes < (u32)KEY_LEVEL_5 || e->host >= (u32)HOSTS || e->note > 127u) return 0;
    u32 down = e->what == (u32)OP1F_DOWN ? 1u : 0u;
    if (!down && e->what != (u32)OP1F_UP) return 0;
    if (down && (e->velocity == 0 || e->velocity > 127u)) return 0;
    int place = e->key + PLACE_OF_KEY;                    // a MIDI key may lie anywhere: held only within the list's bounds
    if (place < 0 || place >= (int)PLACES) return 0;
    u32 source = e->from == (u32)OP1F_FROM_MIDI ? 1u : 0u;
    u32 r = repeats_now, s = step_now, note = e->note, ending = 0, ended = 0;
    Host* h = &hosts[e->host];
    u32 was = op1f_mask();
    if (h->stops_seen != h->stops) {                      // a stop or a start since: the keys held before are forgotten
        h->stops_seen = h->stops;
        for (u32 i = 0; i < (u32)PLACES; ++i) { h->held[0][i] = 0; h->held[1][i] = 0; }
        for (u32 i = 0; i < (u32)NOTES; ++i) h->held_notes[i] = 0;
    }
    u8* held = &h->held[source][place];
    if (*held) {                                          // the up, or a down whose up never came: that key's note let go
        ended = *held;
        *held = 0;
        if (--h->held_notes[ended - 1u]) ended = 0;       // another key still holds that note
    }
    if (!down) {
        op1f_unmask(was);
        if (ended) end_note(ended - 1u);
        return 1;
    }
    if (ended == note + 1u) ended = 0;                    // the same note: ended below, before it begins again
    if (h->held_notes[note]) ending = 1;                  // another key holds this note: begun afresh
    *held = (u8)(note + 1u);
    ++h->held_notes[note];
    ++heard;
    for (u32 i = 0; i < (u32)ECHOES; ++i) {
        Echo* o = &h->echoes[i];
        if (!o->used || !o->sounding || o->note != note) continue;
        o->sounding = 0;
        if (o->played >= o->repeats) o->used = 0;
        ending = 1;
    }
    if (r != 0) {
        if (h->tail - h->head >= (u32)WAITING) {
            ++lost;
        } else {
            Waiting* w = &h->waiting[h->tail & (WAITING - 1)];
            w->note = (u8)e->note;
            w->velocity = (u8)e->velocity;
            w->repeats = (u8)r;
            w->step = (u8)s;
            ++h->tail;
        }
    }
    op1f_unmask(was);
    if (ended) end_note(ended - 1u);
    if (ending) op1f_note_off(&op1f_card_echo, note, 0);  // 0 only where a stop ended it meanwhile: nothing to count
    if (!op1f_note_on(&op1f_card_echo, note, e->velocity, 0)) ++refused;
    return 1;
}

// Switched on, or to stop sounding: the queue and the echoes emptied. At a stop the base ends every note this pack
// left sounding once this call has come back.
extern "C" void echo_run(unsigned host, unsigned how) OP1F_NAMED("echo_run");
extern "C" void echo_run(unsigned host, unsigned how) {
    (void)how;
    if (host >= (u32)HOSTS) return;
    Host* h = &hosts[host];
    u32 was = op1f_mask();
    h->head = h->tail;
    for (u32 i = 0; i < (u32)ECHOES; ++i) h->echoes[i].used = 0;
    ++h->stops;
    op1f_unmask(was);
}

// The keys waiting taken in, one a section: each into a free place, or into the oldest echo's, whose sounding
// repeat is then ended.
static void take_in(Host* h, u32 now) {
    for (;;) {
        u32 ending = 0, note = 0;
        u32 was = op1f_mask();
        if (h->head == h->tail) {
            op1f_unmask(was);
            return;
        }
        Waiting w = h->waiting[h->head & (WAITING - 1)];
        ++h->head;
        Echo* to = 0;
        for (u32 i = 0; i < (u32)ECHOES && to == 0; ++i) {
            if (!h->echoes[i].used) to = &h->echoes[i];
        }
        if (to == 0) {                                    // full: the oldest gives way
            to = &h->echoes[0];
            for (u32 i = 1; i < (u32)ECHOES; ++i) {
                if (h->echoes[i].serial < to->serial) to = &h->echoes[i];
            }
            ending = to->sounding;
            note = to->note;
            ++cut;
        }
        to->start = now;
        to->serial = ++serials;
        to->note = w.note;
        to->first = w.velocity;
        to->velocity = w.velocity;
        to->repeats = w.repeats;
        to->step = w.step;
        to->played = 0;
        to->sounding = 0;
        to->left = 0;
        to->used = 1;
        op1f_unmask(was);
        if (ending) end_note(note);
    }
}

// The repeats whose half step has passed ended, one a section.
static void end_due(Host* h, u32 now) {
    for (;;) {
        u32 note = 0, found = 0;
        u32 was = op1f_mask();
        for (u32 i = 0; i < (u32)ECHOES; ++i) {
            Echo* e = &h->echoes[i];
            if (!e->used || !e->sounding || now < e->start) continue;
            u32 pulses = STEP_PULSES[e->step];
            if (now - e->start < e->played * pulses + (pulses >> 1)) continue;
            e->sounding = 0;
            if (e->played >= e->repeats) e->used = 0;
            note = e->note;
            found = 1;
            break;
        }
        op1f_unmask(was);
        if (!found) return;
        end_note(note);
    }
}

// The repeats that are due begun, one a section. A note this pack already has sounding as another echo's repeat is
// ended before it is begun again. A repeat the base does not begin is left out: where a key still holds its note,
// the base answers 0 (the pack has that note sounding), so that no echo ever cuts a held key.
static void begin_due(Host* h, u32 now) {
    for (;;) {
        u32 note = 0, velocity = 0, serial = 0, found = 0, again = 0;
        Echo* e = 0;
        u32 was = op1f_mask();
        for (u32 i = 0; i < (u32)ECHOES; ++i) {
            e = &h->echoes[i];
            if (!e->used || e->sounding || e->played >= e->repeats || now < e->start) continue;
            if (now - e->start < (e->played + 1u) * STEP_PULSES[e->step]) continue;
            ++e->played;
            e->velocity = (u8)softer(e->velocity);
            e->sounding = 1;
            note = e->note;
            velocity = e->velocity;
            serial = e->serial;
            found = 1;
            for (u32 j = 0; j < (u32)ECHOES; ++j) {
                Echo* o = &h->echoes[j];
                if (o == e || !o->used || !o->sounding || o->note != note) continue;
                o->sounding = 0;
                if (o->played >= o->repeats) o->used = 0;
                again = 1;
            }
            break;
        }
        op1f_unmask(was);
        if (!found) return;
        if (again) end_note(note);
        if (op1f_note_on(&op1f_card_echo, note, velocity, 0)) {
            ++played;
            continue;
        }
        was = op1f_mask();                                // left out, if the echo is still the same
        if (h->held_notes[note]) ++left_out;              // under a held key: what the base's 0 meant
        else ++refused;
        if (e->used && e->serial == serial) {
            e->sounding = 0;
            e->left = (u8)(e->left | (1u << e->played));
            if (e->played >= e->repeats) e->used = 0;
        }
        op1f_unmask(was);
    }
}

// Each pulse of the clock, on the sequencer thread, for the instrument the unit is on.
extern "C" void echo_tick(const Op1fTick* now) OP1F_NAMED("echo_tick");
extern "C" void echo_tick(const Op1fTick* now) {
    if (now->bytes < (u32)TICK_LEVEL_5 || now->host >= (u32)HOSTS) return;
    Host* h = &hosts[now->host];
    take_in(h, now->count);
    end_due(h, now->count);
    begin_due(h, now->count);
}

// Whether its page is on the screen, and how: kept, nothing more.
extern "C" void echo_shown(unsigned where, unsigned how) OP1F_NAMED("echo_shown");
extern "C" void echo_shown(unsigned where, unsigned how) {
    shown_where = where;
    shown_how = how;
}

// ---------------------------------------------------------------- the knobs

// Offered every key and knob event by the base, on the thread that handles keys: the blue and the second knob's
// turns are taken while its page is the one on the screen; everything else goes on.
extern "C" int echo_input(const Op1fInput* e) OP1F_NAMED("echo_input");
extern "C" int echo_input(const Op1fInput* e) {
    if (e->bytes < (u32)INPUT_LEVEL_2 || !e->mine || e->what != (u32)OP1F_TURN) return 0;
    if (e->key == (u32)KNOB_BLUE) turned_repeats = turned_repeats + (u32)(e->turn >> 10);     // 1024 a step; more than 0 up
    else if (e->key == (u32)KNOB_SECOND) turned_step = turned_step + (u32)(e->turn >> 10);
    else return 0;
    ++turns;
    return 1;
}

// ---------------------------------------------------------------- the frame function's work

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

static u32 clamp(int v, int most) {
    return v < 0 ? 0u : v > most ? (u32)most : (u32)v;
}

static const char* host_said(u32 where) {
    return where == 0 ? "synth" : "drum";
}

static void publish() {
    repeats_now = state.repeats;
    step_now = state.step;
}

// echo.bin read at the first frame, held whole before anything of it is taken.
static void load() {
    int got = op1f_read(&op1f_card_echo, FOLDER, SAVED, state.file, sizeof state.file);
    if (got < 0) {
        op1f_log(3, LOG_FILE, 3, "echo: no echo.bin read at the start (answer %d): %u repeats, a step of 1/%u", got, state.repeats,
                 STEP_NOTE[state.step]);
    } else if (got != (int)FILE_BYTES || state.file[0] != (u32)FILE_MAGIC || state.file[1] > (u32)REPEATS_MOST || state.file[2] >= (u32)STEPS) {
        op1f_log(2, LOG_FILE, 4, "echo: echo.bin of %u bytes is not this pack's settings: %u repeats, a step of 1/%u", (u32)got, state.repeats,
                 STEP_NOTE[state.step]);
    } else {
        state.repeats = state.file[1];
        state.step = state.file[2];
        op1f_log(3, LOG_FILE, 2, "echo: echo.bin read at the start: %u repeats, a step of 1/%u", state.repeats, STEP_NOTE[state.step]);
    }
}

static void save() {
    state.file[0] = (u32)FILE_MAGIC;
    state.file[1] = state.repeats;
    state.file[2] = state.step;
    state.tried_repeats = state.repeats;
    state.tried_step = state.step;
    int answer = op1f_write(&op1f_card_echo, FOLDER, SAVED, state.file, FILE_BYTES);
    if (answer == (int)FILE_BYTES) {
        ++state.writes;
        op1f_log(3, LOG_FILE, 5, "echo: echo.bin written: %u repeats, a step of 1/%u", state.repeats, STEP_NOTE[state.step]);
    } else {
        ++state.failed;                                                        // the base's line says why; tried again at the next change
        op1f_log(2, LOG_FILE, 6, "echo: echo.bin could not be written (answer %d)", answer);
    }
}

// A value at the top, centred on its knob's column.
static void paint_value(void* canvas, int middle, const char* text, unsigned colour, int scale, int y) {
    u32 n = 0;
    while (text[n]) ++n;
    int wide = (6 * (int)n - 1) * scale;
    op1f_paint_text(canvas, middle - (wide >> 1), y, text, colour, scale);
}

// The cover of a dot as bright as a velocity: a weight of 24 to 128 in 128ths.
static void paint_dot(void* canvas, int x, int y, u32 velocity) {
    u8 cover[DOT * DOT];
    u32 weight = 24u + ((velocity * 105u) >> 7);
    for (u32 i = 0; i < (u32)(DOT * DOT); ++i) cover[i] = (u8)(((u32)DOT_COVER[i] * weight) >> 7);
    op1f_paint_mask(canvas, x - DOT_HALF, y - DOT_HALF, DOT, DOT, cover, DOT, LIGHT_GREY);
}

static void paint_row(void* canvas, const Echo* e, int y) {
    u32 n = e->note, octave = 0;
    while (n >= 12u) { n -= 12u; ++octave; }
    u32 at = put(state.text, 0, sizeof state.text, NOTE_NAMES[n]);
    if (octave == 0) put(state.text, at, sizeof state.text, "-1");
    else put_number(state.text, at, sizeof state.text, octave - 1u);
    op1f_paint_text(canvas, NAME_X, y - NAME_HALF, state.text, GREY, NAME_SCALE);
    u32 v = e->first;
    for (u32 k = 0; k <= e->repeats; ++k) {
        int x = DOT_FIRST + (int)k * DOT_PITCH;
        if (k > e->played) {
            op1f_paint_box(canvas, x - (POINT >> 1), y - (POINT >> 1), POINT, POINT, DARK_GREY);
        } else if ((e->left >> k) & 1u) {                 // left out under a held key
            op1f_paint_box(canvas, x - (DASH >> 1), y, DASH, 1, GREY);
        } else {
            paint_dot(canvas, x, y, v);
        }
        v = softer(v);
    }
}

// The page: the two values, their names, a rule, and a row for each note echoing on the instrument shown, the
// oldest at the top.
static void paint(void* canvas, u32 where) {
    put_number(state.text, 0, sizeof state.text, state.repeats);
    paint_value(canvas, BLUE_MIDDLE, state.text, BLUE, VALUE_SCALE, VALUE_Y);
    put_number(state.text, put(state.text, 0, sizeof state.text, "1/"), sizeof state.text, STEP_NOTE[state.step]);
    paint_value(canvas, SECOND_MIDDLE, state.text, OCHRE, VALUE_SCALE, VALUE_Y);
    paint_value(canvas, BLUE_MIDDLE, "repeats", GREY, 1, LABEL_Y);
    paint_value(canvas, SECOND_MIDDLE, "step", GREY, 1, LABEL_Y);
    op1f_paint_box(canvas, RULE_X, RULE_Y, RULE_WIDE, 1, DARK_GREY);
    Host* h = &hosts[where & 1u];
    u32 was = op1f_mask();
    for (u32 i = 0; i < (u32)ECHOES; ++i) state.rows[i] = h->echoes[i];
    op1f_unmask(was);
    u32 last = 0;                                         // the rows in the order of their serials, oldest first
    for (u32 row = 0; row < (u32)ECHOES; ++row) {
        const Echo* next = 0;
        for (u32 i = 0; i < (u32)ECHOES; ++i) {
            const Echo* e = &state.rows[i];
            if (e->used && e->serial > last && (next == 0 || e->serial < next->serial)) next = e;
        }
        if (next == 0) break;
        last = next->serial;
        paint_row(canvas, next, ROW_FIRST + (int)row * ROW_PITCH);
    }
}

// A line when the page comes onto the screen, goes, or its sequencer is switched on or off under it: the first
// sixteen.
static void say_page(u32 how, u32 where) {
    u32 up = how & (u32)OP1F_SHOWN_UP, off = how & (u32)OP1F_SHOWN_OFF;
    u32 said = up ? up | off | ((where & 1u) << 4) : 0u;
    if (said == state.said_how) return;
    state.said_how = said;
    if (++state.shows_said > (u32)SAID_MOST) return;
    if (up) op1f_log(3, LOG_FILE, 9, "echo: its page is on the screen for the %s, its sequencer %s", host_said(where), off ? "off" : "on");
    else op1f_log(3, LOG_FILE, 10, "echo: its page is no longer on the screen");
}

static void say_period(const u32* now) {
    op1f_log(3, LOG_FILE, 1, "echo: %u key(s), %u echo(es) played, %u left out under a held key, %u refused, %u cut short for room, %u without an echo; %u repeats, a step of 1/%u",
             now[0] - state.printed[0], now[1] - state.printed[1], now[2] - state.printed[2], now[3] - state.printed[3], now[4] - state.printed[4],
             now[5] - state.printed[5], state.repeats, STEP_NOTE[state.step]);
    for (u32 i = 0; i < (u32)TOTALS; ++i) state.printed[i] = now[i];
}

// Once a frame, on the display thread.
extern "C" void echo_frame(const Op1fFrame* now) OP1F_NAMED("echo_frame");
extern "C" void echo_frame(const Op1fFrame* now) {
    if (now->bytes < (u32)FRAME_LEVEL_4) return;
    if (!state.started) {                                                      // the first frame: the settings, once
        state.started = 1;
        state.repeats = REPEATS_FIRST;
        state.step = STEP_FIRST;
        load();
        state.tried_repeats = state.last_repeats = state.repeats;
        state.tried_step = state.last_step = state.step;
        state.seen_repeats = turned_repeats;
        state.seen_step = turned_step;
        publish();
    }
    if (!state.stood) {                                                        // the blank page, to paint on
        if (op1f_page_stand_in(&op1f_card_echo, ASK, 0)) {
            state.stood = 1;
            op1f_log(3, LOG_FILE, 7, "echo: its page stands in for the sequencer's screen: the base's blank page, painted by the pack");
        } else if (!state.refusal_said) {
            state.refusal_said = 1;
            op1f_log(2, LOG_FILE, 8, "echo: the base did not take its page; the sequencer's screen shows the base's blank page with its name");
        }
    }
    u32 r = turned_repeats, s = turned_step;                                   // the knobs, as far as the limits let them
    int dr = (int)(r - state.seen_repeats), ds = (int)(s - state.seen_step);
    state.seen_repeats = r;
    state.seen_step = s;
    state.repeats = clamp((int)state.repeats + dr, REPEATS_MOST);
    state.step = clamp((int)state.step + ds, STEPS - 1);
    publish();
    if (state.repeats != state.last_repeats || state.step != state.last_step) {
        state.last_repeats = state.repeats;
        state.last_step = state.step;
        state.changed_at = now->ms;
    }
    u32 how = shown_how, where = shown_where;
    say_page(how, where);
    u32 on_page = now->page == &op1f_card_echo ? 1u : 0u;
    if ((state.repeats != state.tried_repeats || state.step != state.tried_step) && !now->disk
        && (now->ms - state.changed_at >= (u32)SETTLE_MS || !on_page)) save();
    state.on_page = on_page;
    if (on_page && now->canvas != 0) paint(now->canvas, where);              // version 2: under the unit's notice too
    u32 totals[TOTALS] = { heard, played, left_out, refused, cut, lost };
    u32 moved = 0;
    for (u32 i = 0; i < (u32)TOTALS; ++i) moved |= totals[i] ^ state.printed[i];
    if (quiet_popup(&state.quiet, now->popup != 0) && moved) say_period(totals);
    if (--state.countdown > 0) return;
    state.countdown = PERIOD_FRAMES;
    u32 sign = quiet_start();
    for (u32 i = 0; i < (u32)TOTALS; ++i) sign = quiet_mix(sign, totals[i]);
    sign = quiet_mix(quiet_mix(sign, state.repeats), state.step);
    if (quiet_period(&state.quiet, sign, moved != 0) && moved) say_period(totals);
}

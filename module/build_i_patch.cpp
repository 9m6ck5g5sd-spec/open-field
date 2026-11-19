// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Build I image I9: an instrument carries its train and its LFOs (analysis/lfo_train_study.md section 22).
//
// What was wanted after the pairs of presets: a change of instrument empties the train and the LFOs and puts the
// instrument's own in their place, and a saved instrument keeps the effects and the LFOs that were put on it.
// Until I8 the train and the three added LFOs belonged to the sound key: a sound loaded onto the key
// changed the wagon under the lamp and the LFO under the searchlight, and the rest stayed.
//
// What stock does (BgFile.cpp, read for this image):
//   * A patch chosen in the list is loaded by a job: the synth's 0x8d4e9cb0 reads the file (0x8c0828e0 at
//     0x8d4ea880), runs the schema pass 0x8d4f787c over it (a fresh document of the known keys: anything else in the
//     file is gone after it), and puts the json on the slot's record through 0x8d4f2a00(record, json); the synth
//     sampler's and the drum's jobs read their file with the plain reader 0x8c0534d0 (at 0x8d4ec7a2 and 0x8d4ebb6c),
//     copy the audio, and end in the same 0x8d4f2a00. A change of engine (the synth's and the drum's type jobs) ends
//     there too, with the old document's effect and LFO kept. Then the job tells the GUI, which applies the record.
//   * The save of an instrument, 0x8c0cd4dc(slot), copies the record's document, names it and hands it to
//     0x8d4fef30 (synth) or 0x8d4ff060 (drum), which write the file with the json file writer.
//   * The boot, the save itself and the random patch put a document on a record by another way (0x8d4f5b1c), so
//     none of them passes 0x8d4f2a00.
//
// What this unit does, behind six CALL.a retargets (the group build-i9; the stubs are build_i_patch.s):
//   * At the three file reads it calls stock's reader and then looks in the json just read for the module's key,
//     "op1f", an array of integers: the slot's integers exactly as user/op1f.json holds them for a slot (the chain,
//     the LFOs' effect wagons, the three added LFOs with the searchlight's place and the order). It keeps them, with
//     the kind of load, until the job puts the patch on a record.
//   * At 0x8d4f2a00's assign it tells the job by its return address. For a load from a file it publishes, for the
//     record's slot, "a new instrument, with these integers" or "a new instrument, with none", after stock's assign
//     has returned. A change of engine and anything else is counted and left alone.
//   * The chain unit asks op1f_patch_pending at every reconcile (stock's apply of the record, on the GUI thread):
//     with integers it reads them as it reads its file at the boot; with none it forgets the slot, and its reconcile
//     then makes the chain of a slot seen for the first time: the instrument's own effect on the blue wagon, the
//     three others empty; the LFO page gets the instrument's own LFO on the blue UFO and nothing else. An instrument
//     stock did not apply within half a second is taken from the frame instead.
//   * At the save it adds the slot's integers to the json stock is about to write (nothing for a slot that is as
//     stock alone has it), so the file itself carries them: stock firmware drops the key at its schema pass.
//
// The jobs run on stock's background file thread [inferred from the unit's name; not read], so the reads and the
// assign hook touch only this unit's own memory and the job's own json, and what they publish is read by the GUI
// thread under a sequence count (odd while the writer is in). The save hook runs where stock's save runs, on the
// GUI thread, and is the only place this unit asks the chain unit for a slot's integers.
//
// Not covered, and said to the owner: a patch copied onto the unit through the disk mode passes the schema pass on
// its way in (0x8fcbf426), which drops the key; the random patch keeps the train as before.

#include "build_i_patch.h"

typedef unsigned int u32;
typedef unsigned char u8;
typedef int i32;

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef int (*JsonSizeFn)(void* json);                               // 0x8c084c22
typedef void* (*JsonLookupFn)(void* json, const char* key);          // 0x8c085e84: the member, inserted when missing
typedef int (*JsonHasFn)(void* json, const char* key);               // 0x8c085068
typedef void* (*JsonGetFn)(void* json, const char* key);             // 0x8c084f74
typedef void* (*JsonAtFn)(void* json, int index);                    // 0x8c084ee0
typedef int (*JsonIntFn)(void* json);                                // 0x8c084840
typedef void (*JsonCtorFn)(void* json, int kind);                    // 0x8c085ba8: 0 is null, which the first push turns into an array
typedef void (*JsonFromIntFn)(void* json, int value);                // 0x8c083b1c
typedef void (*JsonPushFn)(void* array, void* value);                // 0x8c085d54
typedef u32 (*JsonAssignFn)(void* target, void* value);              // 0x8c0845b8; what it leaves in R0 is handed back to 0x8d4f2a00, which returns it
typedef void (*JsonDtorFn)(void* json);                              // 0x8c0844e0
typedef int (*JsonReadFileFn)(const char* path, void* json);         // 0x8c0534d0: the file parsed into the json, non-zero on success
typedef int (*JsonReadRawFn)(void* a, void* path_string, void* json, int flag);   // 0x8c0828e0: the reader under it, as the synth's patch job calls it (R0 to R2 and one word on the stack, as 0x8c0534d0 calls it at 0x8c05356c); non-zero on success

#define STOCK_LOG ((LogFn)0x8c053a20u)
#define STOCK_JSON_SIZE ((JsonSizeFn)0x8c084c22u)
#define STOCK_JSON_LOOKUP ((JsonLookupFn)0x8c085e84u)
#define STOCK_JSON_HAS ((JsonHasFn)0x8c085068u)
#define STOCK_JSON_GET ((JsonGetFn)0x8c084f74u)
#define STOCK_JSON_AT ((JsonAtFn)0x8c084ee0u)
#define STOCK_JSON_INT ((JsonIntFn)0x8c084840u)
#define STOCK_JSON_CTOR ((JsonCtorFn)0x8c085ba8u)
#define STOCK_JSON_FROM_INT ((JsonFromIntFn)0x8c083b1cu)
#define STOCK_JSON_PUSH ((JsonPushFn)0x8c085d54u)
#define STOCK_JSON_ASSIGN ((JsonAssignFn)0x8c0845b8u)
#define STOCK_JSON_DTOR ((JsonDtorFn)0x8c0844e0u)
#define STOCK_JSON_READ_FILE ((JsonReadFileFn)0x8c0534d0u)
#define STOCK_JSON_READ_RAW ((JsonReadRawFn)0x8c0828e0u)
#define STOCK_SYNTH_RECORDS 0x080235dcu                              // eight records of 52 bytes, the document at +8
#define STOCK_DRUM_RECORDS 0x0802377cu
#define STOCK_SYNTH_SLOT (*(volatile u32*)0x11904704u)               // the GUI's current sound slot, by host
#define STOCK_DRUM_SLOT (*(volatile u32*)0x11904708u)

extern "C" u32 op1f_chain_ms() asm("op1f_chain_ms");                // build_h_chain.cpp: milliseconds since the first frame

static const char LOG_FILE[] = "../src/main/op1f_patch.cpp";
static const char KEY[] = "op1f";

struct Json { u32 words[4]; };                                       // a stock json value is 12 bytes; 16 for alignment

enum { HOSTS = 2, SLOTS = 8, INTS = OP1F_PATCH_INTS, RECORD_BYTES = 52, JSON_ARRAY_KIND = 6, JSON_OBJECT_KIND = 7 };
enum { KIND_NONE = 0, KIND_SYNTH_PRESET = 1, KIND_DRUM = 2, KIND_SYNTH_SAMPLER = 3, KINDS = 4 };
enum { LATE_MS = 500, EVENT_LINES = 60, COUNT_LINES = 40 };
// The return addresses into the five jobs that call 0x8d4f2a00 (each a 4-byte CALL, so the site + 4).
static const u32 CALLER_SYNTH_PRESET = 0x8d4ea95au, CALLER_DRUM = 0x8d4ebf9au, CALLER_SYNTH_SAMPLER = 0x8d4ec968u;
static const u32 CALLER_SYNTH_TYPE = 0x8d4ec6f0u, CALLER_DRUM_TYPE = 0x8d4eba60u;

// What the last patch file read carried for the module. Written and read by the reads and the assign hook only (the
// thread that runs stock's patch jobs).
struct Stash { int kind; int count; int values[INTS]; };
static Stash stash;

// A new instrument on a slot, for the GUI thread. seq is odd while the writer is in and even once it is whole; the
// reader takes a copy only when seq stood still and even around it.
struct Pending { volatile u32 seq; int kind; int count; u32 ms; int values[INTS]; };
static Pending pending[HOSTS][SLOTS];
static u32 taken_seq[HOSTS][SLOTS];                                  // GUI thread: the seq last taken

static struct {
    u32 loads[KINDS], with_own, without, engine_changes, other, mismatched;   // the job thread's counts
    u32 taken, late, torn, saves, saves_with, saves_cleared, saves_bad_slot;  // the GUI thread's
    u32 printed, event_lines, count_lines;
} pt;

static int is_kind(void* json, int kind) { return json != 0 && *((u8*)json + 4) == kind; }

// The module's integers out of a patch's json: the count, 0 when the json has none (or something that is not ours).
static int values_of(void* json, int* out) {
    if (!is_kind(json, JSON_OBJECT_KIND) || !STOCK_JSON_HAS(json, KEY)) return 0;
    void* entry = STOCK_JSON_GET(json, KEY);
    if (!is_kind(entry, JSON_ARRAY_KIND)) return 0;
    int n = STOCK_JSON_SIZE(entry);
    if (n < 1 || n > INTS) return 0;
    for (int i = 0; i < n; ++i) {
        void* e = STOCK_JSON_AT(entry, i);
        out[i] = e ? STOCK_JSON_INT(e) : 0;
    }
    return n;
}

static void stash_take(int kind, void* json) {
    stash.kind = kind;
    stash.count = json ? values_of(json, stash.values) : 0;
}

// ---------------------------------------------------------------- the three file reads (the job thread)

// Replaces the call of 0x8c0828e0 at 0x8d4ea880: the synth's patch file, read before the schema pass.
extern "C" int op1f_patch_read_preset(void* a, void* path_string, void* json, int flag) asm("op1f_patch_read_preset");
extern "C" int op1f_patch_read_preset(void* a, void* path_string, void* json, int flag) {
    int ok = STOCK_JSON_READ_RAW(a, path_string, json, flag);
    stash_take(KIND_SYNTH_PRESET, ok ? json : 0);
    return ok;
}

// Replaces the call of 0x8c0534d0 at 0x8d4ec7a2: the synth sampler's patch file.
extern "C" int op1f_patch_read_sampler(const char* path, void* json) asm("op1f_patch_read_sampler");
extern "C" int op1f_patch_read_sampler(const char* path, void* json) {
    int ok = STOCK_JSON_READ_FILE(path, json);
    stash_take(KIND_SYNTH_SAMPLER, ok ? json : 0);
    return ok;
}

// Replaces the call of 0x8c0534d0 at 0x8d4ebb6c: the drum's patch file.
extern "C" int op1f_patch_read_drum(const char* path, void* json) asm("op1f_patch_read_drum");
extern "C" int op1f_patch_read_drum(const char* path, void* json) {
    int ok = STOCK_JSON_READ_FILE(path, json);
    stash_take(KIND_DRUM, ok ? json : 0);
    return ok;
}

// ---------------------------------------------------------------- the patch put on a record (the job thread)

static int record_slot(u32 record, int* host) {
    for (int h = 0; h < HOSTS; ++h) {
        u32 base = h ? STOCK_DRUM_RECORDS : STOCK_SYNTH_RECORDS;
        for (int s = 0; s < SLOTS; ++s) if (record == base + (u32)RECORD_BYTES * (u32)s) { *host = h; return s; }
    }
    return -1;
}

// From the stub at 0x8d4f3b26: stock's own call first, then the word for the GUI thread.
extern "C" u32 op1f_patch_assign(void* document, void* json, u32 caller) asm("op1f_patch_assign");
extern "C" u32 op1f_patch_assign(void* document, void* json, u32 caller) {
    int kind = caller == CALLER_SYNTH_PRESET ? KIND_SYNTH_PRESET : (caller == CALLER_DRUM ? KIND_DRUM : (caller == CALLER_SYNTH_SAMPLER ? KIND_SYNTH_SAMPLER : KIND_NONE));
    int host = 0;
    int slot = record_slot((u32)document - 8u, &host);
    if (kind != KIND_NONE && stash.kind != kind) stash_take(kind, json);   // no read of this kind came before: what the json itself still carries
    u32 result = STOCK_JSON_ASSIGN(document, json);
    if (kind == KIND_NONE) {
        if (caller == CALLER_SYNTH_TYPE || caller == CALLER_DRUM_TYPE) ++pt.engine_changes; else ++pt.other;
    } else if (slot < 0 || host != (kind == KIND_DRUM ? 1 : 0)) {
        ++pt.mismatched;
    } else {
        Pending* p = &pending[host][slot];
        p->seq = p->seq + 1u;                                           // odd: the writer is in
        p->kind = kind;
        p->count = stash.count;
        for (int i = 0; i < stash.count; ++i) p->values[i] = stash.values[i];
        p->ms = op1f_chain_ms();
        p->seq = p->seq + 1u;                                           // even: whole
        ++pt.loads[kind];
        if (stash.count) ++pt.with_own; else ++pt.without;
    }
    stash.kind = KIND_NONE;
    stash.count = 0;
    return result;
}

// ---------------------------------------------------------------- the GUI thread

extern "C" int op1f_patch_pending(int host, int slot, int* out) {
    if ((host != 0 && host != 1) || slot < 0 || slot >= SLOTS) return -1;
    Pending* p = &pending[host][slot];
    u32 seq = p->seq;
    if (seq == taken_seq[host][slot]) return -1;
    if (seq & 1u) { ++pt.torn; return -1; }
    int count = p->count;
    if (count < 0 || count > INTS) count = 0;
    for (int i = 0; i < count; ++i) out[i] = p->values[i];
    if (p->seq != seq) { ++pt.torn; return -1; }
    taken_seq[host][slot] = seq;
    ++pt.taken;
    return count;
}

// From the stubs at 0x8c0ce1cc and 0x8c0ce21a: the json stock is about to write as the instrument's file.
extern "C" void op1f_patch_saving(int host, int slot, void* json) asm("op1f_patch_saving");
extern "C" void op1f_patch_saving(int host, int slot, void* json) {
    static int values[INTS];
    ++pt.saves;
    if (slot < 0 || slot >= SLOTS) {                                    // not where the save function was read to keep it: the sound on the screen
        ++pt.saves_bad_slot;
        slot = (int)((host ? STOCK_DRUM_SLOT : STOCK_SYNTH_SLOT) & 7u);
    }
    if (!is_kind(json, JSON_OBJECT_KIND)) return;
    int n = op1f_chain_slot_values(host, slot, values);
    if (n < 0 || n > INTS) n = 0;
    if (n == 0 && !STOCK_JSON_HAS(json, KEY)) return;                   // as stock alone has it, and nothing stale to clear: the file as stock writes it
    Json array, item;
    STOCK_JSON_CTOR(&array, 0);
    for (int i = 0; i < n; ++i) {
        STOCK_JSON_FROM_INT(&item, values[i]);
        STOCK_JSON_PUSH(&array, &item);
        STOCK_JSON_DTOR(&item);
    }
    STOCK_JSON_ASSIGN(STOCK_JSON_LOOKUP(json, KEY), &array);            // with none: a null in place of what a loaded patch's document still carried
    STOCK_JSON_DTOR(&array);
    if (n) ++pt.saves_with; else ++pt.saves_cleared;
    if (pt.event_lines < EVENT_LINES) {
        ++pt.event_lines;
        STOCK_LOG(3, LOG_FILE, 3, "patch: %s s%d saved: %d integers of its train and LFOs added to the file", host ? "drum" : "synth", slot, n);
    }
}

extern "C" void op1f_patch_frame() {
    u32 now = op1f_chain_ms();
    for (int h = 0; h < HOSTS; ++h) {
        for (int s = 0; s < SLOTS; ++s) {
            Pending* p = &pending[h][s];
            u32 seq = p->seq;
            if (seq == taken_seq[h][s] || (seq & 1u) || (i32)(now - p->ms) < LATE_MS) continue;
            ++pt.late;                                                  // stock did not apply the record: taken from here
            op1f_chain_instrument(h, s);
        }
    }
    u32 sum = pt.loads[1] + pt.loads[2] + pt.loads[3] + pt.engine_changes + pt.other + pt.mismatched + pt.taken + pt.late + pt.torn + pt.saves;
    if (sum != pt.printed && pt.count_lines < COUNT_LINES) {
        pt.printed = sum;
        ++pt.count_lines;
        STOCK_LOG(3, LOG_FILE, 4, "patch: loads %u synth, %u synth sampler, %u drum (%u with a train of their own, %u without); %u taken, %u late, %u torn",
                  pt.loads[KIND_SYNTH_PRESET], pt.loads[KIND_SYNTH_SAMPLER], pt.loads[KIND_DRUM], pt.with_own, pt.without, pt.taken, pt.late, pt.torn);
        STOCK_LOG(3, LOG_FILE, 5, "patch: %u engine changes left alone, %u other, %u mismatched; %u saves (%u with their train, %u cleared, %u slot guessed)",
                  pt.engine_changes, pt.other, pt.mismatched, pt.saves, pt.saves_with, pt.saves_cleared, pt.saves_bad_slot);
    }
}

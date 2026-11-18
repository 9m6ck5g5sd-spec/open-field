// Build J packs J5g: the sidechain without the pack `8 tracks` (OS 1.7.0+0; analysis/pack_loader_study.md
// section 22).
//
// The duck (build_g_duck.cpp) is driven from four places of stock, and since G1 and G2 the tape's units have had
// those four sites and called the duck from their own functions there, through weak references:
//
//   the tape driver's call of read_block, 0x11a0822e (L1)   op1f_read_block_ext (build_b_play.cpp): the duck's gains
//                                                           before stock's read_block, the detector after it
//   the signal flow screen's call for its VU vector,        op1f_signalflow_vu (build_e_screens): stock's vector, the
//   0x8fc33bb8                                              meters of tracks 5 to 8, then the duck's page
//   the tape json's writer, 0x8c0c1408                      op1f_tape_json_set_mixer (build_b_mixer.cpp): stock's set,
//                                                           the tape units' own keys, then the duck's settings
//   the tape json's loader, 0x8c0c2adc                      op1f_tape_json_load / op1f_tape_json_loaded: after stock's
//                                                           loader the tape units' keys, and the duck's settings
//
// Where the tape's units are away (the pack `sidechain` on a unit without the pack `8 tracks`), this unit has the
// same four sites: the sidechain's table names the functions below there, each entry marked "only when the pack
// 8 tracks is away", so that the base makes them when that pack does not run and passes them over when it does
// (module/build_j5c_base.cpp, left_to_another). Each function does what the tape's function at that site does for
// the duck, and nothing else: there are no tracks 5 to 8 here.
//
// The picture: the pack carries the signal flow screen's picture twice, with the duck's page for eight tracks
// (signalflow.svg, on F3's drawing of eight meters) and for four (signalflow4.svg, on stock's own drawing).
// op1f_art_pick_sidechain is asked by the picture loader of `kit` (build_j5g_art.cpp) which of the pack's pictures
// a name stands for, and answers the page for four when the tape's units are away.
//
// Threads: op1f_duck_read_block on the audio thread (no logging); the rest on the GUI's.
// Freestanding like the other units.

typedef unsigned int u32;
typedef unsigned char u8;

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef int (*ReadBlockFn)(int speed, void* state, int rolling);   // 0x8f01d144: fills the four window buffers, answers the window's first frame
typedef void* (*VuVectorFn)();                                     // 0x8d515ad0: ReadInterface's four-float VU vector, in dB
typedef void (*DbSetFn)(int db, const char* key, void* value);     // 0x8d4f13bc
typedef void* (*JsonRootFn)();                                     // 0x8d4f1a60: the tape db's root object

#define STOCK_LOG ((LogFn)0x8c0539f0u)
#define STOCK_READ_BLOCK ((ReadBlockFn)0x8f01d144u)
#define STOCK_VU_VALUES ((VuVectorFn)0x8d515ad0u)
#define STOCK_DB_SET ((DbSetFn)0x8d4f13bcu)
#define STOCK_JSON_ROOT ((JsonRootFn)0x8d4f1a60u)

static const char LOG_FILE[] = "../src/gui/op1f_duck_alone.cpp";
enum { LOG_BUDGET = 4 };

// build_g_duck.cpp
extern "C" void op1f_duck_block_gains() asm("op1f_duck_block_gains");
extern "C" void op1f_duck_detect(int speed, int w0) asm("op1f_duck_detect");
extern "C" void op1f_duck_screen(u8* screen, const u8* vector, const u32* ext_db) asm("op1f_duck_screen");
extern "C" void op1f_duck_json_write() asm("op1f_duck_json_write");
extern "C" void op1f_duck_json_read(void* root) asm("op1f_duck_json_read");
// The tape's units, when they are there (build_b_cache.cpp).
extern "C" u32 __attribute__((weak)) op1f_cache_tracks asm("op1f_cache_tracks");

// The loader trampoline's state (build_j5g_duck_alone.s): the caller's RETS, the loader's flag, its result.
extern "C" u32 op1f_duck_load_state[3] asm("op1f_duck_load_state");
u32 op1f_duck_load_state[3];

// Counters, for the log.
extern "C" u32 op1f_duck_alone_blocks asm("op1f_duck_alone_blocks");     // blocks the sidechain's own hook saw
extern "C" u32 op1f_duck_alone_updates asm("op1f_duck_alone_updates");   // updates of the signal flow screen through its own hook
extern "C" u32 op1f_duck_alone_saves asm("op1f_duck_alone_saves");
extern "C" u32 op1f_duck_alone_loads asm("op1f_duck_alone_loads");
u32 op1f_duck_alone_blocks, op1f_duck_alone_updates, op1f_duck_alone_saves, op1f_duck_alone_loads;
static u32 load_log_budget = LOG_BUDGET;
static u32 save_log_budget = LOG_BUDGET;

// In place of the driver's call of 0x8f01d144 at 0x11a0822e: this block's duck gains into the hold-to-solo words
// before stock reads them, stock's read_block, the detector over this block's frames. What op1f_read_block_ext does
// for the duck, in its order.
extern "C" int op1f_duck_read_block(int speed, void* state, int rolling) asm("op1f_duck_read_block");
extern "C" int op1f_duck_read_block(int speed, void* state, int rolling) {
    op1f_duck_block_gains();
    int w0 = STOCK_READ_BLOCK(speed, state, rolling);
    op1f_duck_detect(speed, w0);
    ++op1f_duck_alone_blocks;
    return w0;
}

// From op1f_duck_vu (the stub at the signal flow screen's call of 0x8d515ad0, 0x8fc33bb8): stock's vector for
// stock's own loop, and the duck's page drawn from it. There are no levels of tracks 5 to 8 to hand over.
extern "C" void* op1f_duck_vu_ext(u8* screen) asm("op1f_duck_vu_ext");
extern "C" void* op1f_duck_vu_ext(u8* screen) {
    void* vector = STOCK_VU_VALUES();
    op1f_duck_screen(screen, (const u8*)vector, 0);
    ++op1f_duck_alone_updates;
    return vector;
}

// In place of the writer's call of 0x8d4f13bc at 0x8c0c1408 (db 2, "mixer_params", the eight-float array): stock's
// own set, then the duck's settings as "op1f_duck".
extern "C" void op1f_duck_json_set(int db, const char* key, void* array) asm("op1f_duck_json_set");
extern "C" void op1f_duck_json_set(int db, const char* key, void* array) {
    STOCK_DB_SET(db, key, array);
    op1f_duck_json_write();
    ++op1f_duck_alone_saves;
    if (save_log_budget) {
        --save_log_budget;
        STOCK_LOG(3, LOG_FILE, 2, "duck alone: the duck's settings written with the tape's json by the sidechain's own hook (%u)", op1f_duck_alone_saves);
    }
}

// After stock's loader 0x8c0c2adc(this, flag) returned (the trampoline op1f_duck_json_load comes here with the flag
// it saw at entry): the tape's ducks, or the defaults; flag != 0 is stock's defaults path.
extern "C" void op1f_duck_json_loaded(int flag) asm("op1f_duck_json_loaded");
extern "C" void op1f_duck_json_loaded(int flag) {
    op1f_duck_json_read(flag != 0 ? 0 : STOCK_JSON_ROOT());
    ++op1f_duck_alone_loads;
    if (load_log_budget) {
        --load_log_budget;
        STOCK_LOG(3, LOG_FILE, 1, "duck alone: the pack 8 tracks is away: the sidechain's own hooks are in, the tape has four tracks (load %u, flag %d)",
                  op1f_duck_alone_loads, flag);
    }
}

static int same(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

// Asked by the picture loader of `kit` for every name, before it looks into this pack's pictures: the name of the
// picture to take for it. The signal flow screen's picture is the page for four tracks on stock's own drawing when
// the tape's units are away, and the page for eight on F3's drawing when they are there.
extern "C" const char* op1f_art_pick_sidechain(const char* name) asm("op1f_art_pick_sidechain");
extern "C" const char* op1f_art_pick_sidechain(const char* name) {
    if (&op1f_cache_tracks == 0 && same(name, "signalflow.svg")) return "signalflow4.svg";
    return name;
}

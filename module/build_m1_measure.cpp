// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step M1, the base pack: the measuring unit, whose frame function calls the frame functions of the packs that
// carry a card (OS 1.7.0+0; docs/base.md). This is the measuring unit of the pack `kit` that went before, with one
// declaration and one statement added and nothing else changed: once a frame it calls op1f_cards_frame
// (module/build_m3_cards.cpp),
// which calls each card's frame function in the order of the packs' file names and keeps the base's clock. The call
// stands after the frame work of our own features (the duck, the effect stage's measure, the chain's or the LFOs'
// own) and right before the two shared screens are dressed, since those show what a frame's work has made. That
// unit is one of this pack, so the call is made outright, where every other unit's is made through a weak name.
//
// What follows is the elder's text.
//
// M2 measurement image, OS 1.7.0+0: what the frame hook shows and logs.
//
// Runs on the gfx thread once per frame (entered from m2_measure.s). Every second of CYCLES it
// computes the audio handler's share of the core clock from the counters the audio wrapper keeps,
// the longest block as a share of the block period, the block rate, the free space of the two
// stock C heaps, and which megabytes of DDR changed since the first frame; it draws three text lines
// with the stock bitmap font and, every 16th second, logs the numbers through the stock logger.
//
// Freestanding: compiled by tools/module_build.py with open-xy's flags, no library. No division
// operator (libgcc), no memset/memcpy, no static constructors, no floating point.
// How such addresses are found: docs/methods.md.

typedef unsigned int u32;
typedef unsigned short u16;

struct AudioCounters { u32 busy; u32 count; u32 peak; };

// Written by op1f_audio_handler (m2_measure.s) on the SPORT thread, read here. 32-bit accesses.
extern "C" volatile AudioCounters op1f_audio asm("op1f_audio");
volatile AudioCounters op1f_audio;
extern "C" u32 op1f_frame_counter asm("op1f_frame_counter");
u32 op1f_frame_counter;

// Build B step B1 (module/build_b_play.cpp) counts the blocks in which tracks 5-6 were resampled; weak, so
// images without that unit (every image before B1) link unchanged and the line below stays silent.
extern "C" u32 __attribute__((weak)) op1f_ext_play_blocks asm("op1f_ext_play_blocks");
extern "C" u32 __attribute__((weak)) op1f_ext_play_active asm("op1f_ext_play_active");
extern "C" int __attribute__((weak)) op1f_ext_play_w0 asm("op1f_ext_play_w0");
// Build B step B2 (module/build_b_record.cpp, with B1b in build_b_play.cpp): the key-combination counters and
// the label of the selected extension track; weak for the same reason.
extern "C" u32 __attribute__((weak)) op1f_ext_play_skipped asm("op1f_ext_play_skipped");
extern "C" u32 __attribute__((weak)) op1f_ext_track_selects asm("op1f_ext_track_selects");
extern "C" u32 __attribute__((weak)) op1f_ext_keys_blocked asm("op1f_ext_keys_blocked");
extern "C" u32 __attribute__((weak)) op1f_ext_monitor_blocks asm("op1f_ext_monitor_blocks");
extern "C" const char* __attribute__((weak)) op1f_record_track_label() asm("op1f_record_track_label");
// Build B step B3a (module/build_b_mix.cpp): the VU words of tracks 5-8, read and cleared here once a second.
extern "C" unsigned short __attribute__((weak)) op1f_ext_vu[4] asm("op1f_ext_vu");
extern "C" u32 __attribute__((weak)) op1f_ext_mix_changes asm("op1f_ext_mix_changes");
// Build B step B3b (module/build_b_xfade.cpp and build_b_play.cpp): the loop crossfade and clip-edge counters of tracks 5-6.
extern "C" u32 __attribute__((weak)) op1f_ext_xfade_computes asm("op1f_ext_xfade_computes");
extern "C" u32 __attribute__((weak)) op1f_ext_xfade_fades asm("op1f_ext_xfade_fades");
// Build D (module/build_d_clip.cpp, build_b_edit.cpp, build_b_mix.cpp): lift-alls and drop-alls over six tracks, renders of a
// clipboard into a sample, MIDI mixer changes on tracks 5-6; weak for the same reason.
extern "C" u32 __attribute__((weak)) op1f_ext_lift_alls asm("op1f_ext_lift_alls");
extern "C" u32 __attribute__((weak)) op1f_ext_drop_alls asm("op1f_ext_drop_alls");
extern "C" u32 __attribute__((weak)) op1f_ext_renders asm("op1f_ext_renders");
extern "C" u32 __attribute__((weak)) op1f_midi_ext_changes asm("op1f_midi_ext_changes");
extern "C" u32 __attribute__((weak)) op1f_mtp_extra_lookups asm("op1f_mtp_extra_lookups");   // D2: track_5/6.wav opened in disk mode
extern "C" u32 __attribute__((weak)) op1f_ext_xfade_waits asm("op1f_ext_xfade_waits");
extern "C" u32 __attribute__((weak)) op1f_ext_loop_blocks asm("op1f_ext_loop_blocks");
extern "C" u32 __attribute__((weak)) op1f_ext_edge_fades asm("op1f_ext_edge_fades");
// Build B step B3c (module/build_b_edit.cpp): lifts, drops and range clears on tracks 5-6, and the refusals.
extern "C" u32 __attribute__((weak)) op1f_ext_lifts asm("op1f_ext_lifts");
extern "C" u32 __attribute__((weak)) op1f_ext_drops asm("op1f_ext_drops");
extern "C" u32 __attribute__((weak)) op1f_ext_clears asm("op1f_ext_clears");
extern "C" u32 __attribute__((weak)) op1f_ext_edit_refusals asm("op1f_ext_edit_refusals");
// Build C (module/build_b_storage.cpp): snapshots taken and dropped with the extension records carried along.
extern "C" u32 __attribute__((weak)) op1f_snapshots_taken asm("op1f_snapshots_taken");
extern "C" u32 __attribute__((weak)) op1f_snapshots_dropped asm("op1f_snapshots_dropped");
// Build B step B3d (module/build_b_mixer.cpp, build_b_mix.cpp): the mixer page flag, cleared here when the mode leaves
// the mixer, and the page and persistence counters.
extern "C" volatile unsigned char __attribute__((weak)) op1f_mixer_page asm("op1f_mixer_page");
extern "C" u32 __attribute__((weak)) op1f_ext_page_holds asm("op1f_ext_page_holds");
extern "C" u32 __attribute__((weak)) op1f_ext_page_knob_moves asm("op1f_ext_page_knob_moves");
extern "C" u32 __attribute__((weak)) op1f_ext_params_saves asm("op1f_ext_params_saves");
extern "C" u32 __attribute__((weak)) op1f_ext_params_loads asm("op1f_ext_params_loads");
// Build B step B3e (build_b_mixer.cpp, build_b_art.cpp): the selected extension track put back after a load, the artworks
// parsed from the module's own copies.
extern "C" u32 __attribute__((weak)) op1f_ext_track_restores asm("op1f_ext_track_restores");
extern "C" u32 __attribute__((weak)) op1f_svg_served asm("op1f_svg_served");
// Build E (build_e_screens.cpp): the help screen's updates with track 5/6 selected, the signal-flow screen's level
// reads of track 5/6; weak for the same reason.
extern "C" u32 __attribute__((weak)) op1f_help_ext_frames asm("op1f_help_ext_frames");
extern "C" u32 __attribute__((weak)) op1f_signalflow_ext_reads asm("op1f_signalflow_ext_reads");
extern "C" u32 __attribute__((weak)) op1f_signalflow_meter_frames asm("op1f_signalflow_meter_frames");   // E4: updates that reshaped the meters of 5-6
// Build F, image F1 (build_b_storage.cpp, build_b_play.cpp): the eMMC measurement, the free chunks of the live tape, the
// track-blocks the assembly filter ran; weak for the same reason.
extern "C" u32 __attribute__((weak)) op1f_io_reloads asm("op1f_io_reloads");
extern "C" u32 __attribute__((weak)) op1f_io_writebacks asm("op1f_io_writebacks");
extern "C" u32 __attribute__((weak)) op1f_io_read_chunks asm("op1f_io_read_chunks");
extern "C" u32 __attribute__((weak)) op1f_io_read_cycles asm("op1f_io_read_cycles");
extern "C" u32 __attribute__((weak)) op1f_io_reload_cycles asm("op1f_io_reload_cycles");
extern "C" u32 __attribute__((weak)) op1f_tape_free_chunks asm("op1f_tape_free_chunks");
extern "C" u32 __attribute__((weak)) op1f_ext_play_tracks asm("op1f_ext_play_tracks");
// Build G (G1, build_g_duck.cpp): the duck's frame call and its counters; weak for the same reason.
extern "C" void __attribute__((weak)) op1f_duck_frame() asm("op1f_duck_frame");
extern "C" const char* __attribute__((weak)) op1f_duck_label() asm("op1f_duck_label");
extern "C" u32 __attribute__((weak)) op1f_duck_blocks asm("op1f_duck_blocks");
extern "C" u32 __attribute__((weak)) op1f_duck_hits asm("op1f_duck_hits");
extern "C" u32 __attribute__((weak)) op1f_duck_missed asm("op1f_duck_missed");
extern "C" u32 __attribute__((weak)) op1f_duck_frames asm("op1f_duck_frames");
extern "C" u32 __attribute__((weak)) op1f_duck_knob_moves asm("op1f_duck_knob_moves");
extern "C" u32 __attribute__((weak)) op1f_duck_face_toggles asm("op1f_duck_face_toggles");
extern "C" u32 __attribute__((weak)) op1f_duck_guarded asm("op1f_duck_guarded");            // G2: hits the re-trigger guard swallowed
extern "C" u32 __attribute__((weak)) op1f_duck_key_selects asm("op1f_duck_key_selects");    // G2: track selections from the page
extern "C" u32 __attribute__((weak)) op1f_duck_json_saves asm("op1f_duck_json_saves");      // G2: op1f_duck written to the tape json
extern "C" u32 __attribute__((weak)) op1f_duck_json_loads asm("op1f_duck_json_loads");      // G2: op1f_duck read after the loader
extern "C" u32 __attribute__((weak)) op1f_duck_stops asm("op1f_duck_stops");                // G2e: envelopes put to rest because the tape stopped inside a dive
extern "C" void __attribute__((weak)) op1f_fx_frame() asm("op1f_fx_frame");            // Build H step H0: the effect stage measurement, once per frame
extern "C" void __attribute__((weak)) op1f_chain_frame(u16* fb) asm("op1f_chain_frame");   // Build H image H1: the chain's frame work, the saves
extern "C" void __attribute__((weak)) op1f_lfo_alone_frame() asm("op1f_lfo_alone_frame");  // Build J packs J5h: the LFOs' frame work where the chain's unit, which calls it otherwise, is away
extern "C" void __attribute__((weak)) op1f_train_frame() asm("op1f_train_frame");          // Build H image H2: the train screen dressed from the chain, the strip
extern "C" void __attribute__((weak)) op1f_presets_frame() asm("op1f_presets_frame");      // Build H image H5: the presets list screen dressed from the chain
extern "C" void __attribute__((weak)) op1f_xmap_frame() asm("op1f_xmap_frame");            // Build K pack K2: the id of the tape's extension file into the tape's json
extern "C" void __attribute__((weak)) op1f_sums_frame() asm("op1f_sums_frame");            // Build K pack K5: the sums of the chunks of tracks 5 to 8
// Pictures step P3b (module/build_p3b_cost.cpp): what a frame costs. Two time stamps, at this function's first statement and at
// its last, beside the two that unit takes around stock's work before the draw and around the draw; weak, so that a kit
// without that unit links as before.
extern "C" void __attribute__((weak)) op1f_cost_frame_in() asm("op1f_cost_frame_in");
extern "C" void __attribute__((weak)) op1f_cost_frame_out() asm("op1f_cost_frame_out");
extern "C" void op1f_cards_frame() asm("op1f_cards_frame");                                // Step M1 (module/build_m3_cards.cpp): the frame functions of the packs that carry a card, and the base's clock
#define CURRENT_MODE (*(volatile int*)0x8400f978u)     // the mode switch 0x8c0bce20 keeps it: 0 synth, 1 drum, 2 tape, 3 mixer
#define MODE_MIXER 3

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef void (*DrawCharFn)(u16* framebuffer, int character);
typedef u32 (*HeapFreeFn)(int heap_index);
#define STOCK_LOG ((LogFn)0x8c053a20u)              // log(level, file, line, fmt, ...), level 3 = I
#define STOCK_DRAW_CHAR ((DrawCharFn)0x8c0dd2ccu)   // bitmapfont draw_char(framebuffer, c) at the cursor
#define STOCK_HEAP_FREE ((HeapFreeFn)0x8fcc51fc)    // ADI runtime heap_space_unused(index): heap 0 at 0x80000000, 1 at 0x82000000
// J5i: the OS's own heap (TE's op1_heap): one dlmalloc space of 20 MiB, made at 0x8d4e36cc with its base at
// 0x8c0e36cc, from which every new and malloc of OS code comes; when it is empty the OS stops with an error
// line. The module's reservation is its top 2 MiB (profiles/op1f-1.7.0.json), and the pages' parsed documents,
// labels and json values of ours live in what is left. [0x080226ec] is the space's state once it is made (the byte
// 0x080226f0 says so). In the state: +0xc the size of the top chunk (what is free at the space's end), +0x18 that
// chunk's address, +0x1b0 the space's size: stock's getter 0x8c05967a reads the size there, which places the fields
// before it as dlmalloc's malloc_state has them [the two others inferred from that until a read-out shows them].
// Read here, written by nobody of ours.
#define OS_HEAP_MADE (*(volatile const unsigned char*)0x080226f0u)
#define OS_HEAP_STATE (*(volatile const u32*)0x080226ecu)
#define OS_HEAP_WORD(offset) (*(volatile const u32*)(OS_HEAP_STATE + (offset)))
enum { OS_HEAP_TOP_SIZE = 0xc, OS_HEAP_TOP = 0x18, OS_HEAP_SIZE = 0x1b0 };
#define CURSOR_X (*(volatile int*)0x840148b0u)
#define CURSOR_Y (*(volatile int*)0x840148b4u)
#define CORE_CLOCK_OVERRIDE (*(volatile const int*)0x8491bd38u)   // -1 = use the default below
static const u32 CORE_CLOCK_DEFAULT = 400000000u;   // [0x8490e038]: stock's cycles-per-second default
static const char LOG_FILE[] = "../src/main/op1f_cfw.cpp";

static const u32 DDR_BASE = 0x80000000u;
enum { DDR_MEGABYTES = 256, SAMPLE_WORDS = 256 };   // the first KiB of every MiB is sampled

static struct {
    u32 started;
    u32 window_start;                 // CYCLES at the start of the current one-second window
    u32 busy_at_start;
    u32 count_at_start;
    u32 windows;                      // completed windows
    u32 load_permille;                // audio handler cycles per 1000 cycles of wall clock
    u32 peak_permille;                // longest block per 1000 cycles of block period
    u32 block_cycles;                 // measured cycles per block
    u32 blocks_per_second;
    u32 heap0_kib;
    u32 heap1_kib;
    u32 os_top_least;                 // J5i: the least the OS heap's top chunk was at any frame, in bytes
    u32 vu5, vu6, vu7, vu8;          // the last second's peaks of tracks 5-8 (B3a; 7-8 since F2), 16-bit like stock's VU words
    u32 io_chunks_at_log, io_cycles_at_log, io_reloads_at_log, io_reload_cycles_at_log;   // F1: the eMMC counters at the last log line
    u32 baseline[DDR_MEGABYTES];      // sample sums at the first frame
    u32 changed[DDR_MEGABYTES / 32];  // bit per MiB whose sample differs from the baseline
    char line1[48];
    char line2[48];
    char line3[32];                   // B3b: the selected extension track's strip, on its own line
} state;

static u32 cycles() {
    u32 c;
    asm volatile("%0 = CYCLES;" : "=d"(c));
    return c;
}

// Restoring shift-subtract division, so no libgcc routine is needed. d == 0 yields 0.
static u32 udiv(u32 n, u32 d) {
    if (d == 0) return 0;
    u32 q = 0, r = 0;
    for (int i = 31; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) { r -= d; q |= 1u << i; }
    }
    return q;
}

static char* put_text(char* p, const char* s) {
    while (*s) *p++ = *s++;
    return p;
}

static char* put_u32(char* p, u32 v) {
    char digits[10];
    int n = 0;
    do {
        u32 q = udiv(v, 10);
        digits[n++] = (char)('0' + (v - q * 10));
        v = q;
    } while (v);
    while (n) *p++ = digits[--n];
    return p;
}

static char* put_permille(char* p, u32 permille) {   // 123 -> "12.3"
    u32 whole = udiv(permille, 10);
    p = put_u32(p, whole);
    *p++ = '.';
    *p++ = (char)('0' + (permille - whole * 10));
    return p;
}

static void draw_text(u16* fb, int x, int y, const char* s) {
    int saved_x = CURSOR_X, saved_y = CURSOR_Y;
    CURSOR_X = x;
    CURSOR_Y = y;
    for (; *s; ++s) STOCK_DRAW_CHAR(fb, *s);
    CURSOR_X = saved_x;
    CURSOR_Y = saved_y;
}

// Sum of the first KiB of a megabyte of DDR. Every DDR address is covered by the kernel's four
// 64 MiB data CPLB pages (table at 0x8491b2e0), so a read cannot fault and has no side effect.
static u32 sample_sum(u32 megabyte) {
    const volatile u32* p = (const volatile u32*)(DDR_BASE + (megabyte << 20));
    u32 s = 0;
    for (int i = 0; i < SAMPLE_WORDS; ++i) s += p[i];
    return s;
}

static void format_lines() {
    char* p = put_text(state.line1, "cfw ");
    p = put_u32(p, op1f_frame_counter);
    p = put_text(p, " aud ");
    p = put_permille(p, state.load_permille);
    p = put_text(p, "% pk ");
    p = put_permille(p, state.peak_permille);
    *p++ = '%';
    *p = 0;
    p = state.line3;                                                 // B3b: " T5 v100 pL20 M" on its own line (the first ran past column 480)
    if (&op1f_record_track_label != 0) p = put_text(p, op1f_record_track_label());
    *p = 0;
    p = put_text(state.line2, "blk ");
    p = put_u32(p, state.blocks_per_second);
    p = put_text(p, "/s heap ");
    p = put_u32(p, state.heap0_kib);
    p = put_text(p, "k ");
    p = put_u32(p, state.heap1_kib);
    *p++ = 'k';
    *p = 0;
}

static void end_of_window(u32 now, u32 elapsed) {
    u32 busy = op1f_audio.busy;
    u32 count = op1f_audio.count;
    u32 peak = op1f_audio.peak;
    op1f_audio.peak = 0;
    u32 dbusy = busy - state.busy_at_start;
    u32 dcount = count - state.count_at_start;
    u32 ms = udiv(elapsed, CORE_CLOCK_DEFAULT / 1000);
    state.load_permille = udiv(dbusy, udiv(elapsed, 1000));
    state.block_cycles = udiv(elapsed, dcount);
    state.peak_permille = udiv(peak, udiv(state.block_cycles, 1000));
    state.blocks_per_second = udiv(dcount * 1000u, ms);
    state.heap0_kib = STOCK_HEAP_FREE(0) >> 10;
    state.heap1_kib = STOCK_HEAP_FREE(1) >> 10;
    if (op1f_ext_vu != 0) {
        state.vu5 = op1f_ext_vu[0];
        state.vu6 = op1f_ext_vu[1];
        state.vu7 = op1f_ext_vu[2];
        state.vu8 = op1f_ext_vu[3];
        op1f_ext_vu[0] = 0;
        op1f_ext_vu[1] = 0;
        op1f_ext_vu[2] = 0;
        op1f_ext_vu[3] = 0;
    }
    for (u32 mb = 0; mb < DDR_MEGABYTES; ++mb) {
        if (sample_sum(mb) != state.baseline[mb]) state.changed[mb >> 5] |= 1u << (mb & 31);
    }
    state.window_start = now;
    state.busy_at_start = busy;
    state.count_at_start = count;
    ++state.windows;
    format_lines();
    if ((state.windows & 15u) == 1u) {
        STOCK_LOG(3, LOG_FILE, 2, "aud %u.%u%% pk %u.%u%% blk %u/s %u cyc heap0 %u KiB heap1 %u KiB frame %u",
                  udiv(state.load_permille, 10), state.load_permille - udiv(state.load_permille, 10) * 10,
                  udiv(state.peak_permille, 10), state.peak_permille - udiv(state.peak_permille, 10) * 10,
                  state.blocks_per_second, state.block_cycles, state.heap0_kib, state.heap1_kib,
                  op1f_frame_counter);
        STOCK_LOG(3, LOG_FILE, 3, "ddr MiB changed since frame 1, bit per MiB from 0x80000000: %08x %08x %08x %08x %08x %08x %08x %08x",
                  state.changed[0], state.changed[1], state.changed[2], state.changed[3],
                  state.changed[4], state.changed[5], state.changed[6], state.changed[7]);
        if (OS_HEAP_MADE)                                            // J5i: the heap every new of the OS comes from, and how near its end it has been
            STOCK_LOG(3, LOG_FILE, 13, "os heap: %u KiB in all, %u KiB free at its top, the least seen %u KiB; state at %08x, top chunk at %08x",
                      OS_HEAP_WORD(OS_HEAP_SIZE) >> 10, OS_HEAP_WORD(OS_HEAP_TOP_SIZE) >> 10, state.os_top_least >> 10,
                      OS_HEAP_STATE, OS_HEAP_WORD(OS_HEAP_TOP));
        if (&op1f_ext_play_blocks != 0 && op1f_ext_play_blocks != 0)
            STOCK_LOG(3, LOG_FILE, 4, "ext play: %u blocks, %u with signal, %u skipped as silent, %u track-blocks filtered, window from frame %d",
                      op1f_ext_play_blocks, op1f_ext_play_active,
                      &op1f_ext_play_skipped != 0 ? op1f_ext_play_skipped : 0u,
                      &op1f_ext_play_tracks != 0 ? op1f_ext_play_tracks : 0u, op1f_ext_play_w0);
        if (&op1f_io_reloads != 0) {                                 // F1: the eMMC rate since the last line, and the free tape
            u32 chunks = op1f_io_read_chunks - state.io_chunks_at_log;
            u32 cyc = op1f_io_read_cycles - state.io_cycles_at_log;
            u32 reloads = op1f_io_reloads - state.io_reloads_at_log;
            u32 reload_cyc = op1f_io_reload_cycles - state.io_reload_cycles_at_log;
            state.io_chunks_at_log = op1f_io_read_chunks;
            state.io_cycles_at_log = op1f_io_read_cycles;
            state.io_reloads_at_log = op1f_io_reloads;
            state.io_reload_cycles_at_log = op1f_io_reload_cycles;
            u32 read_ms = udiv(cyc, CORE_CLOCK_DEFAULT / 1000);
            u32 kib_per_s = read_ms ? udiv(chunks * 128u * 1000u, read_ms) : 0u;
            u32 us_per_chunk = chunks ? udiv(udiv(cyc, chunks), CORE_CLOCK_DEFAULT / 1000000) : 0u;
            u32 us_per_reload = reloads ? udiv(udiv(reload_cyc, reloads), CORE_CLOCK_DEFAULT / 1000000) : 0u;
            u32 free_seconds = udiv(op1f_tape_free_chunks * 16384u, 44100u);
            STOCK_LOG(3, LOG_FILE, 10, "ext io: %u chunks read in %u ms since the last line (%u KiB/s, %u us per chunk), %u slot reloads (%u us each), %u write-backs; tape free %u chunks = %u:%02u",
                      chunks, read_ms, kib_per_s, us_per_chunk, reloads, us_per_reload, op1f_io_writebacks,
                      op1f_tape_free_chunks, udiv(free_seconds, 60u), free_seconds - udiv(free_seconds, 60u) * 60u);
        }
        if (&op1f_ext_track_selects != 0)
            STOCK_LOG(3, LOG_FILE, 5, "ext rec: %u track 5-8 selections, %u clip keys blocked, %u monitor blocks diverted%s",
                      op1f_ext_track_selects, op1f_ext_keys_blocked, op1f_ext_monitor_blocks,
                      &op1f_record_track_label != 0 ? op1f_record_track_label() : "");
        if (op1f_ext_vu != 0)
            STOCK_LOG(3, LOG_FILE, 6, "ext mix: %u knob or mute changes on tracks 5-8, last second's peaks %u/%u/%u/%u",
                      &op1f_ext_mix_changes != 0 ? op1f_ext_mix_changes : 0u, state.vu5, state.vu6, state.vu7, state.vu8);
        if (&op1f_ext_xfade_computes != 0)
            STOCK_LOG(3, LOG_FILE, 7, "ext xfade: %u loop computes, %u junction fades, %u waits for a chunk, %u blocks from the loop buffers, %u clip edge fades",
                      op1f_ext_xfade_computes, op1f_ext_xfade_fades, op1f_ext_xfade_waits,
                      &op1f_ext_loop_blocks != 0 ? op1f_ext_loop_blocks : 0u, op1f_ext_edge_fades);
        if (&op1f_ext_lifts != 0)
            STOCK_LOG(3, LOG_FILE, 8, "ext edit: %u lifts, %u drops, %u clears on tracks 5-8, %u refusals, %u snapshots taken, %u dropped, %u lift-alls, %u drop-alls, %u renders, %u midi, %u disk files",
                      op1f_ext_lifts, op1f_ext_drops, op1f_ext_clears, op1f_ext_edit_refusals,
                      &op1f_snapshots_taken != 0 ? op1f_snapshots_taken : 0u, &op1f_snapshots_dropped != 0 ? op1f_snapshots_dropped : 0u,
                      &op1f_ext_lift_alls != 0 ? op1f_ext_lift_alls : 0u, &op1f_ext_drop_alls != 0 ? op1f_ext_drop_alls : 0u,
                      &op1f_ext_renders != 0 ? op1f_ext_renders : 0u, &op1f_midi_ext_changes != 0 ? op1f_midi_ext_changes : 0u,
                      &op1f_mtp_extra_lookups != 0 ? op1f_mtp_extra_lookups : 0u);
        if (&op1f_ext_page_holds != 0)
            STOCK_LOG(3, LOG_FILE, 9, "ext mixer: %u page holds, %u knob moves on the page, %u saves, %u loads of tracks 5-8's strip, %u track restores, %u artworks from the module, %u help frames on 5/6, %u signal-flow reads, %u meter frames",
                      op1f_ext_page_holds, op1f_ext_page_knob_moves, op1f_ext_params_saves, op1f_ext_params_loads,
                      &op1f_ext_track_restores != 0 ? op1f_ext_track_restores : 0u, &op1f_svg_served != 0 ? op1f_svg_served : 0u,
                      &op1f_help_ext_frames != 0 ? op1f_help_ext_frames : 0u, &op1f_signalflow_ext_reads != 0 ? op1f_signalflow_ext_reads : 0u,
                      &op1f_signalflow_meter_frames != 0 ? op1f_signalflow_meter_frames : 0u);
        if (&op1f_duck_blocks != 0)
        {                                                            // H1a: two lines, since the device cuts a log line at 260 characters (the G2e field was lost)
            STOCK_LOG(3, LOG_FILE, 11, "duck: %s", &op1f_duck_label != 0 ? op1f_duck_label() : "");
            STOCK_LOG(3, LOG_FILE, 12, "duck counts: %u blocks, %u hits, %u guarded, %u missed slot frames, %u page frames, %u knob moves, %u face toggles, %u key selects, %u saves, %u loads, %u tape stops",
                      op1f_duck_blocks, op1f_duck_hits,
                      &op1f_duck_guarded != 0 ? op1f_duck_guarded : 0u, op1f_duck_missed,
                      op1f_duck_frames, op1f_duck_knob_moves, op1f_duck_face_toggles,
                      &op1f_duck_key_selects != 0 ? op1f_duck_key_selects : 0u,
                      &op1f_duck_json_saves != 0 ? op1f_duck_json_saves : 0u, &op1f_duck_json_loads != 0 ? op1f_duck_json_loads : 0u,
                      &op1f_duck_stops != 0 ? op1f_duck_stops : 0u);
        }
    }
}

#define SHOW_MEASUREMENT_TEXT 0                       // Build E3: the three on-screen lines are not drawn any more

extern "C" void op1f_on_frame(u16* fb) asm("op1f_on_frame");
extern "C" void op1f_on_frame(u16* fb) {
    if (&op1f_cost_frame_in != 0) op1f_cost_frame_in();           // P3b: the stamp at the start of our own frame work
    ++op1f_frame_counter;
    u32 now = cycles();
    if (!state.started) {
        state.started = 1;
        state.window_start = now;
        state.busy_at_start = op1f_audio.busy;
        state.count_at_start = op1f_audio.count;
        for (u32 mb = 0; mb < DDR_MEGABYTES; ++mb) state.baseline[mb] = sample_sum(mb);
        *put_text(state.line1, "cfw measuring") = 0;
        state.line2[0] = 0;
        state.line3[0] = 0;
        state.os_top_least = 0xffffffffu;
        STOCK_LOG(3, LOG_FILE, 1, "m2 measure alive: core clock override %d (default %u), audio blocks before frame 1: %u",
                  CORE_CLOCK_OVERRIDE, CORE_CLOCK_DEFAULT, op1f_audio.count);
    }
    if (OS_HEAP_MADE) {                              // J5i: one word read a frame; a dip between two frames is not seen
        u32 top = OS_HEAP_WORD(OS_HEAP_TOP_SIZE);
        if (top < state.os_top_least) state.os_top_least = top;
    }
    u32 elapsed = now - state.window_start;
    if (elapsed >= CORE_CLOCK_DEFAULT) end_of_window(now, elapsed);
    if (&op1f_mixer_page != 0 && op1f_mixer_page && CURRENT_MODE != MODE_MIXER) op1f_mixer_page = 0;   // B3d: the key's release went elsewhere
    if (&op1f_duck_frame != 0) op1f_duck_frame();                 // Build G: the duck page's face and card timer
    if (&op1f_fx_frame != 0) op1f_fx_frame();                     // Build H step H0: stock's stage profiler and the effect hosts' state, logged every four seconds
    if (&op1f_chain_frame != 0) op1f_chain_frame(fb);             // Build H image H1: the chain's frame work (H2: it paints nothing)
    else if (&op1f_lfo_alone_frame != 0) op1f_lfo_alone_frame();  // Build J packs J5h: the LFOs' own frame work, before the page is dressed, where no chain unit calls it
    op1f_cards_frame();                                           // Step M1: each card's frame function, in the order of the packs' file names, before the shared screens are dressed
    if (&op1f_train_frame != 0) op1f_train_frame();               // Build H image H2: the train screen's nodes and the effect pages' strip
    if (&op1f_presets_frame != 0) op1f_presets_frame();           // Build H image H5: the presets list's labels and nodes
    if (&op1f_sums_frame != 0) op1f_sums_frame();                 // Build K pack K5: sums learnt by reading on their way to the file, the sums' log lines
    if (&op1f_xmap_frame != 0) op1f_xmap_frame();                 // Build K pack K2: binds the mounted tape's extension file to its json
    if (SHOW_MEASUREMENT_TEXT) {                     // Build E3: off; the numbers stay in the log every 16th second
        draw_text(fb, 236, 2, state.line1);
        draw_text(fb, 236, 18, state.line2);
        draw_text(fb, 236, 34, state.line3);
    }
    if (&op1f_cost_frame_out != 0) op1f_cost_frame_out();         // P3b: the stamp at its end, and the line for a full period
}

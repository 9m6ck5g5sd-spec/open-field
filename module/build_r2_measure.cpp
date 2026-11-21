// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R2, the base pack (base.pack 21): the frame hook unit of a base that knows no pack by name. A sibling of
// M1's measuring unit, which stays as it was for the bases before. Its frame function keeps the cost stamps,
// the frame counter, the one-second window with the base's own lines (the first frame's, aud, ddr, os heap), the
// watch of the OS heap and the call of the cards' frame functions; every name of a feature it asked for is gone,
// with the features' frame calls, their counters, the meters' read and clear and the nine lines it printed from
// them: a feature does that work in its own pack, from its card's frame function.
//
// The rest is the elder's, unchanged; its description follows and holds, but where it names the features' calls,
// counters and lines, and the third line of text on the screen, which showed a feature's strip.
//
// Step M1, the base pack: the measuring unit, whose frame function calls the frame functions of the packs that
// carry a card (OS 1.7.0+0). This is the first frame hook's measuring unit, which is kept as it is for the pack `kit`, with one declaration and one
// statement added and nothing else changed: once a frame it calls op1f_cards_frame (M1's cards unit),
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

typedef unsigned int u32;
typedef unsigned short u16;

struct AudioCounters { u32 busy; u32 count; u32 peak; };

// Written by op1f_audio_handler (m2_measure.s) on the SPORT thread, read here. 32-bit accesses.
extern "C" volatile AudioCounters op1f_audio asm("op1f_audio");
volatile AudioCounters op1f_audio;
extern "C" u32 op1f_frame_counter asm("op1f_frame_counter");
u32 op1f_frame_counter;

// Pictures step P3b (P3b's cost unit): what a frame costs. Two time stamps, at this function's first statement and at
// its last, beside the two that unit takes around stock's work before the draw and around the draw; weak, so that a kit
// without that unit links as before.
extern "C" void __attribute__((weak)) op1f_cost_frame_in() asm("op1f_cost_frame_in");
extern "C" void __attribute__((weak)) op1f_cost_frame_out() asm("op1f_cost_frame_out");
extern "C" void op1f_cards_frame() asm("op1f_cards_frame");                                // Step M1 (M3's cards unit): the frame functions of the packs that carry a card, and the base's clock

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
typedef void (*DrawCharFn)(u16* framebuffer, int character);
typedef u32 (*HeapFreeFn)(int heap_index);
#define STOCK_LOG ((LogFn)0x8c053a20u)              // log(level, file, line, fmt, ...), level 3 = I
#define STOCK_DRAW_CHAR ((DrawCharFn)0x8c0dd2ccu)   // bitmapfont draw_char(framebuffer, c) at the cursor
#define STOCK_HEAP_FREE ((HeapFreeFn)0x8fcc51fc)    // ADI runtime heap_space_unused(index): heap 0 at 0x80000000, 1 at 0x82000000
// J5i: the OS's own heap (TE's op1_heap): one dlmalloc space of 20 MiB, made at 0x8d4e36cc with its base at
// 0x8c0e36cc, from which every new and malloc of OS code comes; when it is empty the OS stops with an error
// line. The module's reservation is its top 2 MiB, and the pages' parsed documents,
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
    u32 baseline[DDR_MEGABYTES];      // sample sums at the first frame
    u32 changed[DDR_MEGABYTES / 32];  // bit per MiB whose sample differs from the baseline
    char line1[48];
    char line2[48];
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
    op1f_cards_frame();                                           // Step M1: each card's frame function, in the order of the packs' file names
    if (SHOW_MEASUREMENT_TEXT) {                     // Build E3: off; the numbers stay in the log every 16th second
        draw_text(fb, 236, 2, state.line1);
        draw_text(fb, 236, 18, state.line2);
    }
    if (&op1f_cost_frame_out != 0) op1f_cost_frame_out();         // P3b: the stamp at its end, and the line for a full period
}

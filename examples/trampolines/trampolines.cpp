// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// The example pack `trampolines` (docs/first-pack.md): the one line it writes into the unit's log.
//
// The pack's two routines (trampolines.s) stand at the first instructions of two functions of the firmware that the
// main thread calls once each at every start, one right after the other: the function that draws the version text
// on the start screen, then a small function with one argument. The first routine counts. The second calls the
// function below, which prints one line: how often each routine was passed, and a checksum of each room as it
// stands. A room holds what the loader copied there from the running firmware and a jump back. If either room were
// wrong, the start would not get past it: the line, and the start going on behind it, show that both work.
//
// The line's tag is a file name of our own choosing, "op1f/trampolines.cpp". The firmware's logger prints what
// stands between the last slash and the dot, so the line begins `trampolines[1]:`.
//
// Called on the main thread. One function of the firmware is used, the logger at 0x8c053a20: a level (3 for a line
// of information, 2 for a warning, 1 for an error), a file name for the tag, a number that is printed after the
// tag, a format as printf has it, and the values. It is not to be called from the audio thread.

typedef unsigned int u32;
typedef unsigned char u8;

typedef void (*LogFn)(int level, const char* file, int line, const char* format, ...);
#define STOCK_LOG ((LogFn)0x8c053a20u)

static const char LOG_FILE[] = "op1f/trampolines.cpp";
enum { DRAW_ROOM_BYTES = 16, FLAG_ROOM_BYTES = 20, LINES = 4 };

extern "C" u32 op1f_tramp_draws asm("op1f_tramp_draws");                  // how often the first routine was passed
extern "C" u32 op1f_tramp_flags asm("op1f_tramp_flags");                  // and the second
extern "C" const u8 op1f_tramp_draw_room[] asm("op1f_tramp_draw_room");
extern "C" const u8 op1f_tramp_flag_room[] asm("op1f_tramp_flag_room");
extern "C" void op1f_tramp_seen() asm("op1f_tramp_seen");

u32 op1f_tramp_draws;
u32 op1f_tramp_flags;

// CRC-32 as zlib computes it, bit by bit: thirty-six bytes a start need no table.
static u32 sum_of(const u8* bytes, u32 count) {
    u32 crc = 0xffffffffu;
    for (u32 i = 0; i < count; ++i) {
        crc ^= bytes[i];
        for (int k = 0; k < 8; ++k) crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320u) : (crc >> 1);
    }
    return crc ^ 0xffffffffu;
}

void op1f_tramp_seen() {
    ++op1f_tramp_flags;
    if (op1f_tramp_flags > LINES) return;
    STOCK_LOG(3, LOG_FILE, 1, "trampolines: the draw of the version text came through its trampoline %u time(s), and this is call %u through the second; "
              "the rooms' sums are %08x and %08x",
              op1f_tramp_draws, op1f_tramp_flags, sum_of(op1f_tramp_draw_room, DRAW_ROOM_BYTES), sum_of(op1f_tramp_flag_room, FLAG_ROOM_BYTES));
}

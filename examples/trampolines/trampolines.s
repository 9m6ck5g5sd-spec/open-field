/* The example pack `trampolines` (docs/first-pack.md): two small routines of ours, each entered where a function
 * of the firmware begins. Neither holds an instruction of the firmware.
 *
 * The loader writes a jump over the first eight bytes of each of the two functions: the pack's table says where
 * (trampolines_table.s, made from profile.json). The jump lands in one of the routines below. A routine does its
 * own work and then lets the firmware's function run as if nothing had happened. For that it ends in a jump to its
 * room: some bytes of this pack's zeroed data into which the loader has copied, from the running firmware, the
 * instructions the jump took the place of, with a jump back behind them. That is a trampoline.
 *
 * Both functions are called once at every start, by the main thread, shortly after the loader has run:
 *
 *   0x8c0dc920  draws a text with the bitmap font: here, the version text on the start screen. The instructions
 *               under the jump end eight bytes in, so the trampoline copies eight.
 *   0x8c053a6c  a small function with one argument, called right after that draw. The instructions under the
 *               jump end twelve bytes in: the trampoline copies twelve and comes back four bytes behind the jump.
 *
 * At a function's first instruction the registers a call may change hold nothing but the arguments (R0 to R2),
 * and RETS holds the address to return to. So the first routine may use R3 and P1 as it likes. The second calls
 * compiled code: it keeps RETS and R0 on the stack around the call and gives the call the twelve free bytes that
 * every call must have above the return address (tools/stub_calls.py holds every hand-written call to that).
 */
.text
.align 2

/* Entered by the jump the loader writes at 0x8c0dc920. Counts, and goes on through the room. */
.global op1f_tramp_draw
.type op1f_tramp_draw, STT_FUNC
op1f_tramp_draw:
    P1.H = op1f_tramp_draws;
    P1.L = op1f_tramp_draws;
    R3 = [P1];
    R3 += 1;
    [P1] = R3;
    P1.H = op1f_tramp_draw_room;
    P1.L = op1f_tramp_draw_room;
    JUMP (P1);                          /* the displaced instructions, then 0x8c0dc928 */
.size op1f_tramp_draw, .-op1f_tramp_draw

/* Entered by the jump the loader writes at 0x8c053a6c. Says in the log what came through, and goes on through the room. */
.global op1f_tramp_flag
.type op1f_tramp_flag, STT_FUNC
op1f_tramp_flag:
    [--SP] = RETS;
    [--SP] = R0;                        /* the function's one argument */
    SP += -12;                          /* the argument space of the call */
    CALL op1f_tramp_seen;
    SP += 12;
    R0 = [SP++];
    RETS = [SP++];
    P1.H = op1f_tramp_flag_room;
    P1.L = op1f_tramp_flag_room;
    JUMP (P1);                          /* the displaced instructions, then 0x8c053a78 */
.size op1f_tramp_flag, .-op1f_tramp_flag

/* The two rooms: zeroed data, which the loader clears when it loads the pack and fills when it applies the table. Each
 * is the bytes its trampoline copies and the eight of the jump back. */
.section .bss
.balign 4
.global op1f_tramp_draw_room
.type op1f_tramp_draw_room, STT_OBJECT
op1f_tramp_draw_room:
    .space 16
.size op1f_tramp_draw_room, .-op1f_tramp_draw_room
.global op1f_tramp_flag_room
.type op1f_tramp_flag_room, STT_OBJECT
op1f_tramp_flag_room:
    .space 20
.size op1f_tramp_flag_room, .-op1f_tramp_flag_room

/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Build J image J1: stock code changed by the module itself at start-up.
 * Profile group "build-j1": one CALL.a stub. The rest of this file is what the patcher (Build J's patcher) calls to
 * write eight bytes of code, in DDR and in L1, and the small function its self-test rewrites.
 *
 * The site, 0x8c0b4358, is the 8-byte load of 0x8c0b2b84 into R0 in the main thread's start-up, right after the step-3
 * line `[init filesystem (3)]` is logged: the disk is mounted, the log runs, and nothing of the database, the tape,
 * the sound engine, the screens or the sound slots has been set up. The next stock instruction is a CALL that takes
 * R0, and the site is reached both straight on and from a cold path that has just made calls, so nothing is live in
 * the registers a call may change; the main thread's function keeps its own RETS in its frame.
 *
 * Writing code. In DDR a store goes into the data cache, which writes back, and the instruction may be in the
 * instruction cache: the bytes are stored, their data cache line is written out (FLUSH) and the instruction cache's
 * copy dropped (IFLUSH), for the first and the last byte's line. In L1 nothing is cached, and the memory takes plain
 * stores only while bit 16 of the data memory control register 0x1fc00004 is set (Analog Devices' ENX): te-boot loads
 * the firmware's own L1 code this way (its helper at 0x0803e5d8: interrupts off, the bit set, a core sync, the stores,
 * the register back, a core sync, interrupts on), and stock's start-up routine 0x11a09d48 reads and writes all of L1 code under the same bit. The
 * routines here follow te-boot's order. A site is only 2-byte aligned, so the eight bytes go as four half-words (a
 * 32-bit store to an address that is not a multiple of four is a fault); te-boot's helper writes its last bytes one
 * by one under the bit, so the width is free.
 *
 * Every routine is a leaf called from compiled code: it uses R0 to R3 and P0 to P2, and the two L1 routines R4 to R7
 * as well, pushed and popped; each masks interrupts for the length of the write and puts back what it changed.
 */
.text
.align 2

/* Replaces the load of 0x8c0b2b84 into R0 at 0x8c0b4358. The patcher is compiled code: it gets the twelve bytes of argument space. */
.global op1f_boot_stub
.type op1f_boot_stub, STT_FUNC
op1f_boot_stub:
    [--SP] = RETS;
    SP += -12;
    CALL op1f_boot_apply;
    SP += 12;
    RETS = [SP++];
    R0.H = 0x8c0b;
    R0.L = 0x2b84;                      /* the displaced load */
    RTS;
.size op1f_boot_stub, .-op1f_boot_stub

/* u32 op1f_boot_control(): the data memory control register. */
.global op1f_boot_control
.type op1f_boot_control, STT_FUNC
op1f_boot_control:
    P1.H = 0x1fc0;
    P1.L = 0x0004;
    R0 = [P1];
    RTS;
.size op1f_boot_control, .-op1f_boot_control

/* void op1f_boot_write_ddr(u32 site, u32 lo, u32 hi): eight bytes of code in DDR, then both caches. */
.global op1f_boot_write_ddr
.type op1f_boot_write_ddr, STT_FUNC
op1f_boot_write_ddr:
    P0 = R0;
    P1 = R0;
    P1 += 6;                            /* the last half-word: another line when the site crosses a 32-byte boundary */
    CLI R3;
    W[P0] = R1;
    R1 >>= 16;
    W[P0 + 2] = R1;
    W[P0 + 4] = R2;
    R2 >>= 16;
    W[P0 + 6] = R2;
    FLUSH [P0];                         /* the data cache line out to memory */
    FLUSH [P1];
    IFLUSH [P0];                        /* the instruction cache's copy dropped */
    IFLUSH [P1];
    SSYNC;
    STI R3;
    RTS;
.size op1f_boot_write_ddr, .-op1f_boot_write_ddr

/* The two L1 routines run their window twice. te-boot's helper runs from L2 and stock's routine from L1 with the
 * caches not yet on; this code runs from DDR through the instruction cache, and what a cache fill does while the bit
 * is set has not been read. So the first pass is a dry one: the same instructions, with nothing added to the control
 * register and a scratch word pair in the module's own data as the target. After it every instruction between CLI
 * and STI is in the instruction cache, and the second pass, with the bit and the real site, fetches nothing. Inside
 * the window of the second pass the only memory touched is the site in L1 and the control register. */

/* u32 op1f_boot_write_l1(u32 site, u32 lo, u32 hi): eight bytes of code in L1 under bit 16 of the control register.
 * Returns the register as it was. */
.global op1f_boot_write_l1
.type op1f_boot_write_l1, STT_FUNC
op1f_boot_write_l1:
    [--SP] = (R7:4);
    P1.H = 0x1fc0;
    P1.L = 0x0004;
    P0.H = op1f_boot_scratch;
    P0.L = op1f_boot_scratch;           /* the dry pass writes here */
    R7 = R0;                            /* the site, for the second pass */
    R4 = R1;
    R4 >>= 16;                          /* the four half-words: R1, R4, R2, R5 */
    R5 = R2;
    R5 >>= 16;
    R6 = 0;                             /* what is added to the control register: nothing in the dry pass */
.Lwrite_l1_pass:
    CLI R3;
    R0 = [P1];
    P2 = R0;                            /* the register as it was */
    R0 = R0 | R6;
    [P1] = R0;
    CSYNC;
    W[P0] = R1;
    W[P0 + 2] = R4;
    W[P0 + 4] = R2;
    W[P0 + 6] = R5;
    R0 = P2;
    [P1] = R0;
    CSYNC;
    STI R3;
    CC = R6 == 0;
    IF !CC JUMP .Lwrite_l1_done;
    BITSET (R6, 16);                    /* the second pass: bit 16, the site */
    P0 = R7;
    JUMP .Lwrite_l1_pass;
.Lwrite_l1_done:
    R0 = P2;
    (R7:4) = [SP++];
    RTS;
.size op1f_boot_write_l1, .-op1f_boot_write_l1

/* void op1f_boot_read_l1(u32 site, u32* out): the eight bytes at an L1 code site as two words, read under the same bit
 * (stock's start-up routine reads L1 code this way). The words are stored after the bit is back and interrupts are on. */
.global op1f_boot_read_l1
.type op1f_boot_read_l1, STT_FUNC
op1f_boot_read_l1:
    [--SP] = (R7:4);
    [--SP] = R1;                        /* out */
    P1.H = 0x1fc0;
    P1.L = 0x0004;
    P0.H = op1f_boot_scratch;
    P0.L = op1f_boot_scratch;           /* the dry pass reads here */
    R7 = R0;                            /* the site, for the second pass */
    R6 = 0;
.Lread_l1_pass:
    CLI R3;
    R2 = [P1];
    P2 = R2;                            /* the register as it was */
    R2 = R2 | R6;
    [P1] = R2;
    CSYNC;
    R0 = W[P0] (Z);
    R4 = W[P0 + 2] (Z);
    R1 = W[P0 + 4] (Z);
    R5 = W[P0 + 6] (Z);
    R2 = P2;
    [P1] = R2;
    CSYNC;
    STI R3;
    CC = R6 == 0;
    IF !CC JUMP .Lread_l1_done;
    BITSET (R6, 16);                    /* the second pass: bit 16, the site */
    P0 = R7;
    JUMP .Lread_l1_pass;
.Lread_l1_done:
    R4 <<= 16;
    R0 = R0 | R4;
    R5 <<= 16;
    R1 = R1 | R5;
    P2 = [SP++];
    [P2] = R0;
    [P2 + 4] = R1;
    (R7:4) = [SP++];
    RTS;
.size op1f_boot_read_l1, .-op1f_boot_read_l1

/* The self-test's function: it answers 1 until the patcher writes the eight bytes of op1f_boot_probe_alt over it, then
 * 2. It has just run when it is rewritten, so its line is in the instruction cache. No alignment above four here: an
 * input section aligned to 32 moves the start of the module's .text off the reservation's first byte, and the image
 * would be loaded at another address than it was linked for (tools/module_build.py now refuses that). */
.balign 4
.global op1f_boot_probe
.type op1f_boot_probe, STT_FUNC
op1f_boot_probe:
    R0 = 1 (X);
    RTS;
    NOP;
    NOP;
.size op1f_boot_probe, .-op1f_boot_probe
.balign 4
.global op1f_boot_probe_alt
.type op1f_boot_probe_alt, STT_OBJECT
op1f_boot_probe_alt:
    R0 = 2 (X);
    RTS;
    NOP;
    NOP;
.size op1f_boot_probe_alt, .-op1f_boot_probe_alt

/* The dry pass's target: eight bytes of the module's own data. */
.data
.balign 4
.global op1f_boot_scratch
.type op1f_boot_scratch, STT_OBJECT
op1f_boot_scratch:
    .long 0, 0
.size op1f_boot_scratch, .-op1f_boot_scratch

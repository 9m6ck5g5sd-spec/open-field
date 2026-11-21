/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Build J image J4: the base's cache routine for code it has loaded.
 *
 * The base reads a pack's bytes into DDR with ordinary stores, so they stand in the data cache, which writes back
 * only when a line is pushed out; instructions are fetched through the instruction cache, which fills from memory.
 * Before the pack's code is reached its lines are written out (FLUSH) and whatever the instruction cache holds for
 * the range is dropped (IFLUSH), a 32-byte line at a time, as build_j_boot.s does for the eight bytes of a code
 * write (proven since J1). A line that is in neither cache costs nothing.
 *
 * A leaf called from compiled code: R0 to R2, P0 and P1, CC. Interrupts stay on: nothing else touches the range.
 */
.text
.align 2

/* void op1f_base_flush(u32 start, u32 end): the cache lines of [start, end). */
.global op1f_base_flush
.type op1f_base_flush, STT_FUNC
op1f_base_flush:
    R2 = -32;
    R0 = R0 & R2;                       /* the first byte's line */
    P0 = R0;
    P1 = R1;
.Lflush_line:
    CC = P0 < P1 (IU);
    IF !CC JUMP .Lflush_done;
    FLUSH [P0];
    IFLUSH [P0];
    P0 += 32;
    JUMP .Lflush_line;
.Lflush_done:
    SSYNC;
    RTS;
.size op1f_base_flush, .-op1f_base_flush

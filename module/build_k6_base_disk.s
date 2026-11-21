/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Build K image K6: the base's stub in the disk mode's tree builder.
 *
 * The same site and the same shape as the pack's stub (Build H's disk stub, proven since J3b): the eight bytes at
 * 0x8fcafc0a in the disk mode's tree builder 0x8fcadd64 are the call that makes the mixdown folder, with the move of
 * its argument before it (R7 the root folder, a callee-saved register), and the first instruction of the epilogue.
 * The base writes a CALL.a to this stub there only when no pack runs (image K6's base unit, image K6's table);
 * a pack that runs writes its own stub to the same site. The stub makes the mixdown folder as stock did, then the
 * base's "packs" folder (op1f_base_populate in image K6's disk unit), restores RETS and does the displaced
 * step that gives twelve bytes of stack back before returning to 0x8fcaddf2.
 *
 * The twelve bytes of argument space are made before the first call and dropped after the second: a callee may
 * write the words at [SP + 0] to [SP + 11] of its caller, and stock's add-file function 0x8fcaed68, which
 * op1f_base_populate may reach by a tail call, keeps a word at [SP + 0x30], the first word above its own frame
 * (the J3 freeze; tools/stub_calls.py and its test hold every hand-written call to this).
 */

.set STOCK_MIXDOWN_FOLDER, 0x8fcaf830

.section .text
.align 2

.global op1f_base_populate_stub
.type op1f_base_populate_stub, @function
op1f_base_populate_stub:
    [--SP] = RETS;
    SP += -12;                              /* the argument space for both calls: a callee may write it */
    R0 = R7;                                /* the root folder */
    P2.H = STOCK_MIXDOWN_FOLDER;
    P2.L = STOCK_MIXDOWN_FOLDER;
    CALL (P2);                              /* stock's mixdown folder, as the displaced call did */
    R0 = R7;
    CALL op1f_base_populate;                /* the base's packs folder and its two files (R7 is callee-saved) */
    SP += 12;
    RETS = [SP++];
    SP += 0xc;                              /* the displaced first instruction of the epilogue */
    RTS;
.size op1f_base_populate_stub, .-op1f_base_populate_stub

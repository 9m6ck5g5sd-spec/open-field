/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Base 29's mend M3 (2026-11-21): the four stubs at the unit's two list jobs, the synth's and the drum's, for the values
 * packs keep with a sound (module/build_r6b_keeps.cpp, OS 1.7.0+0; 1.7.3 by the map). The elder stubs of the same unit
 * stay in module/build_r4_keeps.s. Each place is one entry of the base's table, a CALL.a over eight bytes:
 *
 *   L1  0x8d4ecb42  the call of 0x8c0534d0, the synth's list job's read of the sound picked (path, document)
 *                                                                                    -> op1f_keeps_read_list_synth_stub
 *   L3  0x8d4ec1f4  the call of 0x8c0534d0, the drum's list job's read of the kit picked (path, document)
 *                                                                                    -> op1f_keeps_read_list_drum_stub
 *       As F2 and F3 (module/build_r4_keeps.s): the stub only jumps, so that the C function, which makes the
 *       firmware's read itself, returns to the job with the job's stack.
 *
 *   L2  0x8d4ecc2c  the synth's list job's call of 0x8d4f6c3c, its put of the document on the slot's record (record,
 *                   slot, document, and on the job's stack a word 0), with the move of its third argument before it
 *                   and of the job's document after it                               -> op1f_keeps_put_synth_at
 *   L4  0x8d4ec2c6  the drum's list job's call of 0x8d4f6748, its put, the same arguments, with a store of the job's
 *                   own to its stack before it
 *                                                                                    -> op1f_keeps_put_drum_at
 *       Four-byte calls, so the eight bytes taken are the call and the instructions beside it, each whole, none
 *       naming a place by its distance but the call, nothing in either image branching into bytes 2 to 7 (as W6
 *       and W7, module/build_r5_effects.s). The stub does what the displaced instructions other than the call did
 *       before it, then jumps to the C function, which makes the firmware's put itself and returns where the
 *       firmware's call would have, from the same stack: it finds the put's fourth argument at [SP + 0xc], where
 *       the job wrote it, and the job's twelve bytes of argument space above it. At L2 the C function answers the
 *       document, which is R6 (the job's R2 = R6 before the call): so the job's R0 = R6 after the call holds. At L4
 *       the job sets R0 itself after the call. At L4 the CALL.a does not move SP, so [SP + 0x44] is the job's word.
 *
 * P1 at the puts, P2 at the reads, are the registers the stubs change before the C function runs: each is one a called
 * function may change, and the job holds nothing in either across its call.
 */
.section .text
.align 2

.global op1f_keeps_read_list_synth_stub
.type op1f_keeps_read_list_synth_stub, @function
op1f_keeps_read_list_synth_stub:
    P2.H = op1f_keeps_read_list_synth;
    P2.L = op1f_keeps_read_list_synth;
    JUMP (P2);                              /* (path, document): it returns to the job */
.size op1f_keeps_read_list_synth_stub, .-op1f_keeps_read_list_synth_stub

.global op1f_keeps_read_list_drum_stub
.type op1f_keeps_read_list_drum_stub, @function
op1f_keeps_read_list_drum_stub:
    P2.H = op1f_keeps_read_list_drum;
    P2.L = op1f_keeps_read_list_drum;
    JUMP (P2);                              /* (path, document) */
.size op1f_keeps_read_list_drum_stub, .-op1f_keeps_read_list_drum_stub

.global op1f_keeps_put_synth_at
.type op1f_keeps_put_synth_at, @function
op1f_keeps_put_synth_at:
    R2 = R6;                                /* the document, as the job's own instruction before its call */
    P1.H = op1f_keeps_put_synth;
    P1.L = op1f_keeps_put_synth;
    JUMP (P1);                              /* (record, slot, document, [SP + 0xc]): answers the document, the job's R0 = R6 */
.size op1f_keeps_put_synth_at, .-op1f_keeps_put_synth_at

.global op1f_keeps_put_drum_at
.type op1f_keeps_put_drum_at, @function
op1f_keeps_put_drum_at:
    [SP + 0x44] = R7;                       /* the job's own instruction before its call */
    P1.H = op1f_keeps_put_drum;
    P1.L = op1f_keeps_put_drum;
    JUMP (P1);                              /* (record, slot, document, [SP + 0xc]) */
.size op1f_keeps_put_drum_at, .-op1f_keeps_put_drum_at

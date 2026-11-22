/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R4, the base pack (base.pack 25): the eight stubs at the places of the unit's files, for the values packs keep
 * there (base 24's values unit, OS 1.7.0+0). Each place is one entry of the base's table: a CALL.a retargeted to a
 * stub (seven of them), or the first eight bytes of a function replaced by a JUMP.a to a stub (the tape's load). The
 * forms are the ones the features have run on the unit at the same places since images I9 and B3d, moved here.
 *
 * Six of a sound's file:
 *
 *   0x8d4ea880  the call of 0x8c0828e0, the synth's preset read (R0, R1, R2 and a word on the stack)  -> op1f_keeps_read_preset_stub
 *   0x8d4ec7a2  the call of 0x8c0534d0, the synth sampler's preset read (path, document)              -> op1f_keeps_read_sampler_stub
 *   0x8d4ebb6c  the call of 0x8c0534d0, the drum's preset read (path, document)                       -> op1f_keeps_read_drum_stub
 *       The three go on to the C function, which takes the firmware's own arguments, makes the firmware's call
 *       itself and returns its answer: the stub only jumps, so that the return address and the stack are the
 *       caller's.
 *
 *   0x8d4f3b26  the call of 0x8c0845b8, a sound put on a slot's record, inside the short function that does it, a
 *               frame of its own and the call alone in it, (document, json)                      -> op1f_keeps_assign_stub
 *       [FP + 4] is that function's return address, into the job that called it: it tells a load from a file
 *       from a change of the engine. The stub hands it to the C function as its third argument and jumps there; the
 *       C function makes the firmware's assign itself and returns its answer.
 *
 *   0x8c0ce1cc  the call of 0x8d4fef30, the synth's preset about to be written (out, name, document)  -> op1f_keeps_save_synth_stub
 *   0x8c0ce21a  the call of 0x8d4ff060, the drum's, the same                                          -> op1f_keeps_save_drum_stub
 *       Inside the save function, which keeps the slot at [FP + 0xc]. The stub keeps the three arguments and RETS,
 *       calls op1f_keeps_saving(host, slot, document) with the twelve bytes of argument space, puts everything back
 *       and jumps to the firmware's writer, which returns to the save function as if called from there. 28 bytes of
 *       stack on the way.
 *
 * Two of a tape's:
 *
 *   0x8c0c1db8  the call of 0x8d4f24dc, the tape's save, its last set of a member (2, key, value)     -> op1f_keeps_tape_save_stub
 *       Goes on to the C function, as the three reads do.
 *
 *   0x8c0c348c  the tape's load (this, flag), its first eight bytes, as the profile holds them
 *                                                                                                     -> op1f_keeps_tape_load
 *       The stub keeps the caller's RETS and the flag, points RETS at its own continuation and goes into the
 *       firmware through its room (the eight bytes the entry displaced, copied there by the base with a jump back
 *       behind them); the firmware's own return comes back to the continuation with the stack as it was at the
 *       entry, which calls op1f_keeps_tape_loaded(flag) with the twelve bytes of argument space and returns the
 *       firmware's answer to the caller. 12 bytes of stack on the way after the firmware's own.
 *
 * P2, and at the tape's load R2 and P1, are the registers the stubs change before the firmware or the C function
 * runs: each is one a called function may change.
 */
.set STOCK_ADD_SYNTH_PATCH, 0x8d4fef30
.set STOCK_ADD_DRUM_PATCH, 0x8d4ff060

.section .text
.align 2

.global op1f_keeps_read_preset_stub
.type op1f_keeps_read_preset_stub, @function
op1f_keeps_read_preset_stub:
    P2.H = op1f_keeps_read_preset;
    P2.L = op1f_keeps_read_preset;
    JUMP (P2);                              /* (a, path, document, the word on the stack): it returns to the read's caller */
.size op1f_keeps_read_preset_stub, .-op1f_keeps_read_preset_stub

.global op1f_keeps_read_sampler_stub
.type op1f_keeps_read_sampler_stub, @function
op1f_keeps_read_sampler_stub:
    P2.H = op1f_keeps_read_sampler;
    P2.L = op1f_keeps_read_sampler;
    JUMP (P2);                              /* (path, document) */
.size op1f_keeps_read_sampler_stub, .-op1f_keeps_read_sampler_stub

.global op1f_keeps_read_drum_stub
.type op1f_keeps_read_drum_stub, @function
op1f_keeps_read_drum_stub:
    P2.H = op1f_keeps_read_drum;
    P2.L = op1f_keeps_read_drum;
    JUMP (P2);                              /* (path, document) */
.size op1f_keeps_read_drum_stub, .-op1f_keeps_read_drum_stub

.global op1f_keeps_assign_stub
.type op1f_keeps_assign_stub, @function
op1f_keeps_assign_stub:
    R2 = [FP + 4];                          /* the return address into the job that put the sound on the record */
    P2.H = op1f_keeps_assign;
    P2.L = op1f_keeps_assign;
    JUMP (P2);                              /* (document, json, caller): it returns behind the call it stands for */
.size op1f_keeps_assign_stub, .-op1f_keeps_assign_stub

.global op1f_keeps_save_synth_stub
.type op1f_keeps_save_synth_stub, @function
op1f_keeps_save_synth_stub:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    R0 = 0;                                 /* the synth */
    R1 = [FP + 0xc];                        /* the slot, where the save function keeps it */
    SP += -12;
    CALL op1f_keeps_saving;                 /* (host, slot, document): R2 is still the document */
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    P2.H = STOCK_ADD_SYNTH_PATCH;
    P2.L = STOCK_ADD_SYNTH_PATCH;
    JUMP (P2);
.size op1f_keeps_save_synth_stub, .-op1f_keeps_save_synth_stub

.global op1f_keeps_save_drum_stub
.type op1f_keeps_save_drum_stub, @function
op1f_keeps_save_drum_stub:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    R0 = 1;                                 /* the drum */
    R1 = [FP + 0xc];
    SP += -12;
    CALL op1f_keeps_saving;
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    P2.H = STOCK_ADD_DRUM_PATCH;
    P2.L = STOCK_ADD_DRUM_PATCH;
    JUMP (P2);
.size op1f_keeps_save_drum_stub, .-op1f_keeps_save_drum_stub

.global op1f_keeps_tape_save_stub
.type op1f_keeps_tape_save_stub, @function
op1f_keeps_tape_save_stub:
    P2.H = op1f_keeps_tape_saving;
    P2.L = op1f_keeps_tape_saving;
    JUMP (P2);                              /* (2, key, value): it returns to the tape's save */
.size op1f_keeps_tape_save_stub, .-op1f_keeps_tape_save_stub

.global op1f_keeps_tape_load
.type op1f_keeps_tape_load, @function
op1f_keeps_tape_load:
    P2.H = op1f_keeps_load_state;
    P2.L = op1f_keeps_load_state;
    R2 = RETS;
    [P2] = R2;                              /* the caller's return */
    [P2 + 0x4] = R1;                        /* the flag */
    P1.H = op1f_keeps_tape_load_after;
    P1.L = op1f_keeps_tape_load_after;
    R2 = P1;
    RETS = R2;                              /* the firmware's UNLINK puts this back, its RTS comes here */
    P2.H = op1f_keeps_tape_load_room;       /* the eight bytes the entry displaced, then a jump back behind them */
    P2.L = op1f_keeps_tape_load_room;
    JUMP (P2);
op1f_keeps_tape_load_after:
    P2.H = op1f_keeps_load_state;
    P2.L = op1f_keeps_load_state;
    [P2 + 0x8] = R0;                        /* the firmware's answer, kept */
    R0 = [P2 + 0x4];
    SP += -12;
    CALL op1f_keeps_tape_loaded;            /* void (int flag) */
    SP += 12;
    P2.H = op1f_keeps_load_state;
    P2.L = op1f_keeps_load_state;
    R0 = [P2 + 0x8];
    R2 = [P2];
    RETS = R2;
    RTS;
.size op1f_keeps_tape_load, .-op1f_keeps_tape_load

/* The tape load's words (the caller's return, the flag, the firmware's answer) and its room: zeroed data, which the
 * base clears when it loads the pack and fills when it applies its table: the eight bytes copied and the eight of the
 * jump back. */
.section .bss
.balign 4
.global op1f_keeps_load_state
.type op1f_keeps_load_state, STT_OBJECT
op1f_keeps_load_state:
    .space 12
.size op1f_keeps_load_state, .-op1f_keeps_load_state
.global op1f_keeps_tape_load_room
.type op1f_keeps_tape_load_room, STT_OBJECT
op1f_keeps_tape_load_room:
    .space 16
.size op1f_keeps_tape_load_room, .-op1f_keeps_tape_load_room

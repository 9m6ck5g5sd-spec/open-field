/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Build I image I9: an instrument carries its train and its LFOs (analysis/lfo_train_study.md section 22).
 * Profile group build-i9: six CALL.a retargets, three of them through the stubs below (the three file reads go
 * straight to C functions of build_i_patch.cpp, which take stock's own arguments).
 *
 * 1. The moment a new patch is put on a sound slot. BgFile.cpp's jobs end in 0x8d4f2a00(record, json):
 *
 *      It is a short function: it makes a frame, steps from the record to the record's document, calls the json
 *      assign 0x8c0845b8 at 0x8d4f3b26, which copies the json into the document,         <- the hook
 *      and returns.
 *
 *    Five jobs call it; [FP + 4] is the return address into the one that did, which tells a patch loaded from a
 *    file (0x8d4ea95a the synth's, 0x8d4ec968 the synth sampler's, 0x8d4ebf9a the drum's) from a change of engine
 *    (0x8d4ec6f0, 0x8d4eba60), where stock keeps the sound's effect and LFO. The stub hands that address to the C
 *    function as its third argument and jumps to it; the C function makes stock's call itself and returns what it
 *    returned, to 0x8d4f2a0e. R2 and P2 are free here: the displaced instruction is a call.
 *
 * 2. The save of an instrument, 0x8c0cd4dc(slot) (the slot is kept in its frame, at FP + 0xc). Its two calls of the
 *    functions that write the patch's file:
 *
 *      at 0x8c0ce1cc the call of 0x8d4fef30 (out, the name, the json to write), the synth's      <- the hook
 *      at 0x8c0ce21a the call of 0x8d4ff060, the same, the drum's                                <- the hook
 *
 *    The stub keeps stock's three arguments and RETS, calls op1f_patch_saving(host, slot, json), which adds the
 *    slot's integers to the json under the module's key, puts everything back and jumps to stock's function, which
 *    returns to the save function as if called from there. Both stock functions take their three arguments in R0
 *    to R2 and read nothing else at entry (LINK 0x28; [--SP] = (R7:4); R7 = R1; [FP + 8] = R0; R4 = R2). The C
 *    function gets the twelve bytes of argument space (the I5 lesson).
 */

.set STOCK_ADD_SYNTH_PATCH, 0x8d4fef30
.set STOCK_ADD_DRUM_PATCH, 0x8d4ff060

.section .text
.align 2

/* Replaces the call of 0x8c0845b8 at 0x8d4f3b26. */
.global op1f_patch_assign_stub
.type op1f_patch_assign_stub, @function
op1f_patch_assign_stub:
    R2 = [FP + 4];                          /* the return address into the job that called 0x8d4f2a00 */
    P2.H = op1f_patch_assign;
    P2.L = op1f_patch_assign;
    JUMP (P2);                              /* (document, json, caller); it returns to 0x8d4f2a0e */
.size op1f_patch_assign_stub, .-op1f_patch_assign_stub

/* Replaces the call of 0x8d4fef30 at 0x8c0ce1cc. */
.global op1f_patch_save_synth_stub
.type op1f_patch_save_synth_stub, @function
op1f_patch_save_synth_stub:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    R0 = 0;                                 /* the synth */
    R1 = [FP + 0xc];                        /* the slot, where the save function keeps it */
    SP += -12;
    CALL op1f_patch_saving;                 /* (host, slot, json): R2 is still the json */
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    P2.H = STOCK_ADD_SYNTH_PATCH;
    P2.L = STOCK_ADD_SYNTH_PATCH;
    JUMP (P2);
.size op1f_patch_save_synth_stub, .-op1f_patch_save_synth_stub

/* Replaces the call of 0x8d4ff060 at 0x8c0ce21a. */
.global op1f_patch_save_drum_stub
.type op1f_patch_save_drum_stub, @function
op1f_patch_save_drum_stub:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    R0 = 1;                                 /* the drum */
    R1 = [FP + 0xc];
    SP += -12;
    CALL op1f_patch_saving;
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    P2.H = STOCK_ADD_DRUM_PATCH;
    P2.L = STOCK_ADD_DRUM_PATCH;
    JUMP (P2);
.size op1f_patch_save_drum_stub, .-op1f_patch_save_drum_stub

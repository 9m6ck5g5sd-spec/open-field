/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R5, the base pack 28: the entries at three functions of the firmware for the effects (OS 1.7.0+0). The logic is
 * in base 28's effects unit. The base's table puts a JUMP.a to each entry over the first eight bytes of its function,
 * with a trampoline the loader makes in the room named beside it: those eight bytes copied from the running firmware,
 * and a jump back behind them. A jump to the room is the firmware's function as it always was.
 *
 *   op1f_fx_pick_entry     W2, the instruments' pick, 0x8d4f398c; R0 the slot's record, R1 the id the owner picked.
 *                          With no effect registered, the room at once; else op1f_fx_pick_do(record, id), which calls
 *                          the room itself, and back to the pick's caller.
 *   op1f_fx_names_entry    W4, the knob names of the LFO pages, 0x8d51750c; R0 the kind, R1 the index. With no effect
 *                          registered and no unit offering op1f_engines_names (a weak name of a later level: its
 *                          address 0), the room at once; else op1f_fx_names_do(kind, index) answers a name, or 0 for
 *                          the room with R0 and R1 as they came.
 *   op1f_fx_active_entry   W5, the setter of a record's fx_active, 0x8d4f3c24; R0 the record, R1 the value. With no
 *                          effect registered, the room at once; else op1f_fx_active_do(record, value) answers the value
 *                          handed on, and the room with R0 as it came and that value in R1.
 *   op1f_fx_apply_synth_at, op1f_fx_apply_drum_at   W6 and W7 as eight-byte places (below).
 *
 * The registers. Each entry is a function's first instruction, reached by a call of compiled code: R0 to R2 and the
 * caller's stack words are the function's arguments, and R3, P0 to P2 and ASTAT hold nothing the caller keeps. So an
 * entry decides with R3, P0 and CC and calls nothing before it has decided; it leaves R0 to R2, RETS, SP and the
 * caller's argument space as they came on the way to the room (but for R1 at W5, which is the point). P1 carries the
 * jump to the room. On the way to the base's function it pushes what it gives back and gives the compiled code its
 * twelve bytes of argument space (SP += -12 after the pushes), so that nothing of the caller's is written. The stack an
 * entry takes at its deepest: 0 bytes on the way straight to the room; 28 on the way through the base's function
 * (RETS, R0, R1, R2 and the twelve bytes), below which the compiled code takes its own.
 */
.section .text
.align 2

.global op1f_fx_pick_entry
.type op1f_fx_pick_entry, STT_FUNC
op1f_fx_pick_entry:
    P0.H = op1f_effects_registered;
    P0.L = op1f_effects_registered;
    R3 = [P0];
    CC = R3 == 0;
    IF CC JUMP .Lpick_firmware;
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;
    CALL op1f_fx_pick_do;                   /* (record, id), as they came: it calls the room itself */
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    RTS;
.Lpick_firmware:
    P1.H = op1f_fx_pick_room;
    P1.L = op1f_fx_pick_room;
    JUMP (P1);
.size op1f_fx_pick_entry, .-op1f_fx_pick_entry

.weak op1f_engines_names                    /* a later level's: 0 while no unit offers it */
.global op1f_fx_names_entry
.type op1f_fx_names_entry, STT_FUNC
op1f_fx_names_entry:
    P0.H = op1f_effects_registered;
    P0.L = op1f_effects_registered;
    R3 = [P0];
    CC = R3 == 0;
    IF !CC JUMP .Lnames_base;
    R3.H = op1f_engines_names;
    R3.L = op1f_engines_names;
    CC = R3 == 0;
    IF CC JUMP .Lnames_firmware;            /* no effect registered and no engines' names: the firmware's at once */
.Lnames_base:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;
    CALL op1f_fx_names_do;                  /* (kind, index), as they came */
    SP += 12;
    CC = R0 == 0;
    R3 = R0;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Lnames_firmware;
    R0 = R3;                                /* the pack's name of the value */
    RTS;
.Lnames_firmware:
    P1.H = op1f_fx_names_room;
    P1.L = op1f_fx_names_room;
    JUMP (P1);
.size op1f_fx_names_entry, .-op1f_fx_names_entry

.global op1f_fx_active_entry
.type op1f_fx_active_entry, STT_FUNC
op1f_fx_active_entry:
    P0.H = op1f_effects_registered;
    P0.L = op1f_effects_registered;
    R3 = [P0];
    CC = R3 == 0;
    IF CC JUMP .Lactive_firmware;
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;
    CALL op1f_fx_active_do;                 /* (record, value), as they came: answers the value handed on */
    SP += 12;
    R3 = R0;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    R1 = R3;
.Lactive_firmware:
    P1.H = op1f_fx_active_room;
    P1.L = op1f_fx_active_room;
    JUMP (P1);
.size op1f_fx_active_entry, .-op1f_fx_active_entry

/* W6 and W7 as eight-byte places (offered to the lead: the base's table takes eight-byte hooks and words, and the call
 * of the on setter is four bytes). In both records' applies, and in both firmwares, the call stands behind two
 * instructions of two bytes that hand it its arguments, and nothing branches into the three:
 *   synth 0x8d4f6468 (1.7.3 0x8d4f6468), drum 0x8d4f4f38 (1.7.3 0x8d4f4f38): the two moves, then the call of the on setter
 * A CALL.a over the eight bytes comes here; the stub does what the two did and goes on to the base's C function with
 * a jump, so that the function returns where the firmware's call would have, from the same stack: it has the apply's
 * own twelve bytes of argument space above it, as the firmware's setter had (the apply keeps its RETS in its frame,
 * which the CALL.a writes, as the setter's CALL did). P1 carries the jump: the setter itself writes it. */
.section .text
.align 2
.global op1f_fx_apply_synth_at
.type op1f_fx_apply_synth_at, STT_FUNC
op1f_fx_apply_synth_at:
    R1 = R0;                                /* the value the apply read from the file */
    R0 = R6;                                /* the slot */
    P1.H = op1f_fx_apply_synth;
    P1.L = op1f_fx_apply_synth;
    JUMP (P1);                              /* (slot, value): it calls the setter itself */
.size op1f_fx_apply_synth_at, .-op1f_fx_apply_synth_at

.global op1f_fx_apply_drum_at
.type op1f_fx_apply_drum_at, STT_FUNC
op1f_fx_apply_drum_at:
    R1 = R0;
    R0 = R6;
    P1.H = op1f_fx_apply_drum;
    P1.L = op1f_fx_apply_drum;
    JUMP (P1);
.size op1f_fx_apply_drum_at, .-op1f_fx_apply_drum_at

/* The trampolines' rooms, in the pack's zeroed data: the eight bytes each copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_fx_pick_room
.type op1f_fx_pick_room, STT_OBJECT
op1f_fx_pick_room:
    .space 16
.size op1f_fx_pick_room, .-op1f_fx_pick_room
.balign 4
.global op1f_fx_names_room
.type op1f_fx_names_room, STT_OBJECT
op1f_fx_names_room:
    .space 16
.size op1f_fx_names_room, .-op1f_fx_names_room
.balign 4
.global op1f_fx_active_room
.type op1f_fx_active_room, STT_OBJECT
op1f_fx_active_room:
    .space 16
.size op1f_fx_active_room, .-op1f_fx_active_room

/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R3, the base pack 20: the notes heard, the two entries (OS 1.7.0+0). Logic in module/build_r3_notes.cpp.
 *
 * The firmware's note on is handed the note in R0, the velocity in R1, a third word in R2 and four more words on
 * its caller's stack, the last of them "no MIDI" (not 0: the note is not sent out over MIDI); its note off the same
 * three registers and three words, the last "no MIDI" as well. At the function's first instruction, with SP as the
 * caller left it (its twelve bytes of argument space at [SP + 0] to [SP + 11]), "no MIDI" is the word at
 * [SP + 0x18] for a note on and at [SP + 0x14] for a note off.
 *
 * The base's table puts a JUMP.a over the first eight bytes of each function, with a trampoline the loader makes in
 * the room named beside it: those eight bytes copied from the running firmware (whole instructions, none relative
 * to its own place), and a jump back behind them.
 *
 *   op1f_notes_on_entry    the note on's entry; its room op1f_notes_on_room
 *   op1f_notes_off_entry   the note off's entry; its room op1f_notes_off_room
 *
 * An entry runs before anything of the function, on whatever thread asks for the note, and may itself be
 * interrupted by another note on another thread. It keeps everything as it came: every data, pointer, index,
 * modify, base and length register, both accumulators, the loop registers, ASTAT and RETS (entered by a jump, RETS
 * still holds the caller's return), SP, and the caller's stack words, which it only reads. Then it goes on in the
 * room, so that the firmware's function runs as it always did.
 *
 * With no pack listening (op1f_notes_listening is 0, as it is before the base's first frame) it decides in a load
 * and a test, with P0 and ASTAT pushed and put back, and nothing else touched. With packs listening it pushes
 * everything, hands the C function the note, the velocity and the "no MIDI" word, and gives it the twelve bytes of
 * argument space a caller owes compiled code (SP += -12 below its own pushes). The C function calls the packs, which
 * are compiled code too and may use any register the convention lets a function change, the loop registers among
 * them: all of them are put back before the room. The jump to a room is by distance: room and entry are in the same
 * pack, and no register is free to carry it.
 *
 * The stack it takes at its deepest: 8 bytes on the way with no pack listening; 180 bytes on the way with packs
 * listening (8, then RETS, R0 to R7, P1 to P5, the four accumulator parts, the six loop registers, the sixteen index,
 * modify, base and length registers, and the twelve bytes of argument space), below which the C function and the
 * packs' functions take their own.
 */
.section .text
.align 2

.global op1f_notes_on_entry
.type op1f_notes_on_entry, STT_FUNC
op1f_notes_on_entry:
    [--SP] = ASTAT;
    [--SP] = P0;
    P0.H = op1f_notes_listening;
    P0.L = op1f_notes_listening;
    P0 = [P0];                              /* how many packs listen */
    CC = P0 == 0;
    IF !CC JUMP .Lon_heard;
    P0 = [SP++];
    ASTAT = [SP++];
    JUMP.L op1f_notes_on_room;              /* nobody listens: the firmware's own way at once */
.Lon_heard:
    [--SP] = RETS;
    [--SP] = (R7:0, P5:1);                  /* P0 is the second word pushed */
    [--SP] = A0.X;
    [--SP] = A0.W;
    [--SP] = A1.X;
    [--SP] = A1.W;
    [--SP] = LC0;
    [--SP] = LT0;
    [--SP] = LB0;
    [--SP] = LC1;
    [--SP] = LT1;
    [--SP] = LB1;
    [--SP] = I0;
    [--SP] = I1;
    [--SP] = I2;
    [--SP] = I3;
    [--SP] = M0;
    [--SP] = M1;
    [--SP] = M2;
    [--SP] = M3;
    [--SP] = B0;
    [--SP] = B1;
    [--SP] = B2;
    [--SP] = B3;
    [--SP] = L0;
    [--SP] = L1;
    [--SP] = L2;
    [--SP] = L3;
    R2 = [SP + 0xc0];                       /* "no MIDI": 0x18 above the entry's SP, which lies 0xa8 above here */
    SP += -12;                              /* the argument space of the call */
    CALL op1f_notes_heard_on;               /* (note R0, velocity R1, no MIDI R2), as they came */
    SP += 12;
    L3 = [SP++];
    L2 = [SP++];
    L1 = [SP++];
    L0 = [SP++];
    B3 = [SP++];
    B2 = [SP++];
    B1 = [SP++];
    B0 = [SP++];
    M3 = [SP++];
    M2 = [SP++];
    M1 = [SP++];
    M0 = [SP++];
    I3 = [SP++];
    I2 = [SP++];
    I1 = [SP++];
    I0 = [SP++];
    LB1 = [SP++];
    LT1 = [SP++];
    LC1 = [SP++];
    LB0 = [SP++];
    LT0 = [SP++];
    LC0 = [SP++];
    A1.W = [SP++];
    A1.X = [SP++];
    A0.W = [SP++];
    A0.X = [SP++];
    (R7:0, P5:1) = [SP++];
    RETS = [SP++];
    P0 = [SP++];
    ASTAT = [SP++];
    JUMP.L op1f_notes_on_room;              /* the firmware's own way, from the instructions under the jump */
.size op1f_notes_on_entry, .-op1f_notes_on_entry

.global op1f_notes_off_entry
.type op1f_notes_off_entry, STT_FUNC
op1f_notes_off_entry:
    [--SP] = ASTAT;
    [--SP] = P0;
    P0.H = op1f_notes_listening;
    P0.L = op1f_notes_listening;
    P0 = [P0];
    CC = P0 == 0;
    IF !CC JUMP .Loff_heard;
    P0 = [SP++];
    ASTAT = [SP++];
    JUMP.L op1f_notes_off_room;
.Loff_heard:
    [--SP] = RETS;
    [--SP] = (R7:0, P5:1);
    [--SP] = A0.X;
    [--SP] = A0.W;
    [--SP] = A1.X;
    [--SP] = A1.W;
    [--SP] = LC0;
    [--SP] = LT0;
    [--SP] = LB0;
    [--SP] = LC1;
    [--SP] = LT1;
    [--SP] = LB1;
    [--SP] = I0;
    [--SP] = I1;
    [--SP] = I2;
    [--SP] = I3;
    [--SP] = M0;
    [--SP] = M1;
    [--SP] = M2;
    [--SP] = M3;
    [--SP] = B0;
    [--SP] = B1;
    [--SP] = B2;
    [--SP] = B3;
    [--SP] = L0;
    [--SP] = L1;
    [--SP] = L2;
    [--SP] = L3;
    R2 = [SP + 0xbc];                       /* "no MIDI": 0x14 above the entry's SP, which lies 0xa8 above here */
    SP += -12;
    CALL op1f_notes_heard_off;              /* (note R0, velocity R1, no MIDI R2), as they came */
    SP += 12;
    L3 = [SP++];
    L2 = [SP++];
    L1 = [SP++];
    L0 = [SP++];
    B3 = [SP++];
    B2 = [SP++];
    B1 = [SP++];
    B0 = [SP++];
    M3 = [SP++];
    M2 = [SP++];
    M1 = [SP++];
    M0 = [SP++];
    I3 = [SP++];
    I2 = [SP++];
    I1 = [SP++];
    I0 = [SP++];
    LB1 = [SP++];
    LT1 = [SP++];
    LC1 = [SP++];
    LB0 = [SP++];
    LT0 = [SP++];
    LC0 = [SP++];
    A1.W = [SP++];
    A1.X = [SP++];
    A0.W = [SP++];
    A0.X = [SP++];
    (R7:0, P5:1) = [SP++];
    RETS = [SP++];
    P0 = [SP++];
    ASTAT = [SP++];
    JUMP.L op1f_notes_off_room;
.size op1f_notes_off_entry, .-op1f_notes_off_entry

/* The trampolines' rooms, in the pack's zeroed data: the eight bytes each copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_notes_on_room
.type op1f_notes_on_room, STT_OBJECT
op1f_notes_on_room:
    .space 16
.size op1f_notes_on_room, .-op1f_notes_on_room
.global op1f_notes_off_room
.type op1f_notes_off_room, STT_OBJECT
op1f_notes_off_room:
    .space 16
.size op1f_notes_off_room, .-op1f_notes_off_room

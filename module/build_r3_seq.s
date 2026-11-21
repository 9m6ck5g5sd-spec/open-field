/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R3, the base pack 20: the entries at the firmware's sequencer functions (OS 1.7.0+0), and the mask a pack's
 * sequencer may use. The logic is in base 20's sequencer unit. The base's table puts a JUMP.a to each entry over the first
 * bytes of its function, with a trampoline the loader makes in the room named beside it: those bytes copied from the
 * running firmware, and a jump back behind them. A jump to the room is the firmware's function as it always was.
 *
 *   op1f_seq_switch_entry   the switch to a sequencer's type, 0x8fc97914; R0 the type. Counts the switch first, then:
 *                           a type of the firmware's, or one nobody registered, goes to the room; one of ours to
 *                           op1f_seq_switch(type). The room copies 8 bytes.
 *   op1f_seq_name_entry     a name to a type, 0x8fca1d84; R0 the firmware's string object. With no sequencer
 *                           registered, the room at once; else op1f_seq_type_of(name) answers the type, or -1 for the
 *                           room. The room copies 8 bytes.
 *   op1f_seq_type_entry     a type to its name, 0x8fca1d00; R0 the type. One of ours: op1f_seq_name_of(type); any
 *                           other, the room. The room copies 14 bytes.
 *   op1f_seq_ctl_entry,     the two getters of a sequencer's controller, 0x8c0c109c and 0x8c0c1460; R0 the manager.
 *   op1f_seq_ctl2_entry     While the type of the instrument the unit is on is 7 or more (one of ours), the base's
 *                           controller (op1f_seq_controller, never 0); else the room. Each room copies 14 bytes.
 *
 * The registers. Each entry is a function's first instruction, reached by a call of compiled code: R0 to R2 and the
 * caller's stack words are the function's arguments, and R3, P0 to P2 and ASTAT hold nothing the caller keeps. So an
 * entry decides with R3, P0 and CC and calls nothing before it has decided; it leaves R0 to R2, RETS, SP and the
 * caller's argument space as they came on the way to the room. P1 carries the jump to the room. On the way to the
 * base's function it pushes what it gives back and gives the compiled code its twelve bytes of argument space
 * (SP += -12 after the pushes), so that nothing of the caller's is written.
 */
.set STOCK_INSTRUMENT, 0x84d21088           /* 0 the synth, 1 the drum */
.set STOCK_TYPES, 0x84d1cccc                /* the synth's sequencer type, the drum's behind it */

.section .text
.align 2

.global op1f_seq_switch_entry
.type op1f_seq_switch_entry, STT_FUNC
op1f_seq_switch_entry:
    P0.H = op1f_seq_switches;
    P0.L = op1f_seq_switches;
    R3 = [P0];
    R3 += 1;
    [P0] = R3;                              /* every switch counted: the frame keeps the owner's choice */
    CC = R0 < 7 (IU);                       /* the types of the packs' sequencers begin at 7 */
    IF CC JUMP .Lswitch_firmware;
    P0.H = op1f_seq_registered;
    P0.L = op1f_seq_registered;
    R3 = [P0];
    R3 += 7;
    CC = R0 < R3 (IU);
    IF !CC JUMP .Lswitch_firmware;
    [--SP] = RETS;
    SP += -12;
    CALL op1f_seq_switch;                   /* R0, the type, as it came */
    SP += 12;
    RETS = [SP++];
    RTS;
.Lswitch_firmware:
    P1.H = op1f_seq_switch_room;
    P1.L = op1f_seq_switch_room;
    JUMP (P1);
.size op1f_seq_switch_entry, .-op1f_seq_switch_entry

.global op1f_seq_name_entry
.type op1f_seq_name_entry, STT_FUNC
op1f_seq_name_entry:
    P0.H = op1f_seq_registered;
    P0.L = op1f_seq_registered;
    R3 = [P0];
    CC = R3 == 0;
    IF CC JUMP .Lname_firmware;
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;
    CALL op1f_seq_type_of;                  /* R0, the string object, as it came */
    SP += 12;
    CC = R0 < 0;
    R3 = R0;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Lname_firmware;
    R0 = R3;                                /* the type of one of ours */
    RTS;
.Lname_firmware:
    P1.H = op1f_seq_name_room;
    P1.L = op1f_seq_name_room;
    JUMP (P1);
.size op1f_seq_name_entry, .-op1f_seq_name_entry

.global op1f_seq_type_entry
.type op1f_seq_type_entry, STT_FUNC
op1f_seq_type_entry:
    CC = R0 < 7 (IU);                       /* the types of the packs' sequencers begin at 7 */
    IF CC JUMP .Ltype_firmware;
    P0.H = op1f_seq_registered;
    P0.L = op1f_seq_registered;
    R3 = [P0];
    R3 += 7;
    CC = R0 < R3 (IU);
    IF !CC JUMP .Ltype_firmware;
    [--SP] = RETS;
    SP += -12;
    CALL op1f_seq_name_of;                  /* R0, the type, as it came */
    SP += 12;
    RETS = [SP++];
    RTS;
.Ltype_firmware:
    P1.H = op1f_seq_type_room;
    P1.L = op1f_seq_type_room;
    JUMP (P1);
.size op1f_seq_type_entry, .-op1f_seq_type_entry

.global op1f_seq_ctl_entry
.type op1f_seq_ctl_entry, STT_FUNC
op1f_seq_ctl_entry:
    P0.H = STOCK_INSTRUMENT;
    P0.L = STOCK_INSTRUMENT;
    R3 = [P0];
    CC = R3 == 0;
    P0.H = STOCK_TYPES;
    P0.L = STOCK_TYPES;
    IF CC JUMP .Lctl_host;
    P0 += 4;                                /* any instrument but the synth: the drum's type, as the firmware takes it */
.Lctl_host:
    R3 = [P0];
    CC = R3 < 7 (IU);
    IF CC JUMP .Lctl_firmware;
    [--SP] = RETS;
    SP += -12;
    CALL op1f_seq_controller;
    SP += 12;
    RETS = [SP++];
    RTS;
.Lctl_firmware:
    P1.H = op1f_seq_ctl_room;
    P1.L = op1f_seq_ctl_room;
    JUMP (P1);
.size op1f_seq_ctl_entry, .-op1f_seq_ctl_entry

.global op1f_seq_ctl2_entry
.type op1f_seq_ctl2_entry, STT_FUNC
op1f_seq_ctl2_entry:
    P0.H = STOCK_INSTRUMENT;
    P0.L = STOCK_INSTRUMENT;
    R3 = [P0];
    CC = R3 == 0;
    P0.H = STOCK_TYPES;
    P0.L = STOCK_TYPES;
    IF CC JUMP .Lctl2_host;
    P0 += 4;
.Lctl2_host:
    R3 = [P0];
    CC = R3 < 7 (IU);
    IF CC JUMP .Lctl2_firmware;
    [--SP] = RETS;
    SP += -12;
    CALL op1f_seq_controller;
    SP += 12;
    RETS = [SP++];
    RTS;
.Lctl2_firmware:
    P1.H = op1f_seq_ctl2_room;
    P1.L = op1f_seq_ctl2_room;
    JUMP (P1);
.size op1f_seq_ctl2_entry, .-op1f_seq_ctl2_entry

/* The mask for a pack's short sections: the interrupts off, the old mask answered, and given back. From any thread. */
.global op1f_mask
.type op1f_mask, STT_FUNC
op1f_mask:
    CLI R0;
    RTS;
.size op1f_mask, .-op1f_mask

.global op1f_unmask
.type op1f_unmask, STT_FUNC
op1f_unmask:
    STI R0;
    RTS;
.size op1f_unmask, .-op1f_unmask

/* The trampolines' rooms, in the pack's zeroed data: the bytes each copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_seq_switch_room
.type op1f_seq_switch_room, STT_OBJECT
op1f_seq_switch_room:
    .space 16
.size op1f_seq_switch_room, .-op1f_seq_switch_room
.balign 4
.global op1f_seq_name_room
.type op1f_seq_name_room, STT_OBJECT
op1f_seq_name_room:
    .space 16
.size op1f_seq_name_room, .-op1f_seq_name_room
.balign 4
.global op1f_seq_type_room
.type op1f_seq_type_room, STT_OBJECT
op1f_seq_type_room:
    .space 22
.size op1f_seq_type_room, .-op1f_seq_type_room
.balign 4
.global op1f_seq_ctl_room
.type op1f_seq_ctl_room, STT_OBJECT
op1f_seq_ctl_room:
    .space 22
.size op1f_seq_ctl_room, .-op1f_seq_ctl_room
.balign 4
.global op1f_seq_ctl2_room
.type op1f_seq_ctl2_room, STT_OBJECT
op1f_seq_ctl2_room:
    .space 22
.size op1f_seq_ctl2_room, .-op1f_seq_ctl2_room

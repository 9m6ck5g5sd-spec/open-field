/* Step M2, the base pack: our calls at the five places of the firmware level 2 stands at (OS 1.7.0+0;
 * module/build_m3_cards.cpp, what they ask). The base's table (module/build_m2_base_table.s) puts a jump to each
 * of the first four at its place, with a
 * trampoline the loader makes in the room named beside it: the stock instructions the jump stands on, copied from
 * the running firmware, and a jump back behind them. The fifth is a call in place of one 8-byte load.
 *
 *   op1f_input_down_entry   the key-down function's entry, 0x8c0b1320; the trampoline copies 8 bytes. R0 is the
 *                           firmware's record of the key; RETS the caller's return.
 *   op1f_input_shift_entry  the shifted key-down function's entry, 0x8c0b1390; the trampoline copies 12 bytes. The
 *                           same registers.
 *   op1f_input_up_entry     the key-up function's entry, 0x8c0b13c4; the trampoline copies 12 bytes. R0 is the
 *                           key's number: the firmware hands an up on as a number, not as a record.
 *   op1f_input_turn_place   0x8c0b1438 inside the turn function, after the firmware's own bookkeeping (the keys are on,
 *                           the knob's byte for the click is cleared); the trampoline copies 8 bytes. The function
 *                           has made its frame; R0 and R7 hold the knob, R6 the turn, R5 the knob + 4.
 *   op1f_page_screen        0x8c0b1db0 in the screen request, where the firmware loads the word named below: this call
 *                           makes that load. R0 holds the screen the current controller answered, R6 and R7 the
 *                           function's own words.
 *
 * Each of the four asks the base's function (op1f_input_down, op1f_input_up, op1f_input_turn), which answers 1 when
 * a pack took the event. Taken: the firmware's function is left at once, as if it had run: back to its caller from
 * an entry (none of the ten calls of the four uses what they answer), or through the turn function's
 * own way out, named below, which takes its frame down and returns. Not taken: the trampoline, so the firmware's function
 * runs as it always did, from the instructions the jump stands on, with every register it was handed.
 *
 * The registers. The base's functions are compiled code, which keeps R4 to R7, P3 to P5, FP and SP and may change
 * the rest. An entry has R0 to hand on (and nothing else: the four functions read no other argument register before
 * they write it) and RETS to return by; both are pushed around the call. The turn place has nothing to keep that
 * compiled code changes but R0, and RETS was saved by the function's LINK. Each call is given its twelve bytes of
 * argument space (SP += -12 after the pushes; docs/methods.md, section 9). P1 carries the jump, a register no
 * argument is in. Hardware loops: none of the five places lies in one, and compiled code is called at every place the
 * firmware calls its own compiled handlers.
 *
 * op1f_page_screen makes the firmware's load itself, last, and changes R0 to 77 only while a pack's page stands
 * under 77 in the mode the unit is in: the base writes op1f_page_screen_mode (0, or that mode + 1) on the display
 * thread after the document is in place. It reads two words and keeps P0 and R2, which it uses, as it came: nothing
 * it touches is live in the request but R0 and R1, and CC, which the request sets again at once. It calls nothing,
 * so it runs as it is inside a request made with interrupts masked (J5i).
 */
.set STOCK_MODE, 0x8400f7d4                 /* the mode: 0 synth, 1 drum, 2 tape, 3 mixer */
.set STOCK_SLOT_WORD, 0x84d20bc4            /* the word the screen request loads at 0x8c0b1db0 */
.set STOCK_TURN_END, 0x8c0b145c             /* the turn function's own way out: its frame taken down, back to its caller */
.set PAGE_SCREEN, 77

.section .text
.align 2

.global op1f_input_down_entry
.type op1f_input_down_entry, STT_FUNC
op1f_input_down_entry:
    [--SP] = RETS;
    [--SP] = R0;
    SP += -12;
    R1 = 0;                                 /* not through the shifted function */
    CALL op1f_input_down;
    SP += 12;
    CC = R0 == 0;
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Ldown_on;
    RTS;                                    /* a pack took the key: the firmware never sees it */
.Ldown_on:
    P1.H = op1f_input_down_room;
    P1.L = op1f_input_down_room;
    JUMP (P1);                              /* the firmware's own way, from the instructions under the jump */
.size op1f_input_down_entry, .-op1f_input_down_entry

.global op1f_input_shift_entry
.type op1f_input_shift_entry, STT_FUNC
op1f_input_shift_entry:
    [--SP] = RETS;
    [--SP] = R0;
    SP += -12;
    R1 = 1;                                 /* through the shifted function */
    CALL op1f_input_down;
    SP += 12;
    CC = R0 == 0;
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Lshift_on;
    RTS;
.Lshift_on:
    P1.H = op1f_input_shift_room;
    P1.L = op1f_input_shift_room;
    JUMP (P1);
.size op1f_input_shift_entry, .-op1f_input_shift_entry

.global op1f_input_up_entry
.type op1f_input_up_entry, STT_FUNC
op1f_input_up_entry:
    [--SP] = RETS;
    [--SP] = R0;
    SP += -12;
    CALL op1f_input_up;                     /* R0, the key's number, is its argument as it came */
    SP += 12;
    CC = R0 == 0;
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Lup_on;
    RTS;
.Lup_on:
    P1.H = op1f_input_up_room;
    P1.L = op1f_input_up_room;
    JUMP (P1);
.size op1f_input_up_entry, .-op1f_input_up_entry

.global op1f_input_turn_place
.type op1f_input_turn_place, STT_FUNC
op1f_input_turn_place:
    [--SP] = R0;
    SP += -12;
    R1 = R6;                                /* the turn; R0, the knob, is the first argument as it came */
    CALL op1f_input_turn;
    SP += 12;
    CC = R0 == 0;
    R0 = [SP++];
    IF CC JUMP .Lturn_on;
    P1.H = STOCK_TURN_END;                  /* a pack took the turn: the turn function's own end */
    P1.L = STOCK_TURN_END;
    JUMP (P1);
.Lturn_on:
    P1.H = op1f_input_turn_room;
    P1.L = op1f_input_turn_room;
    JUMP (P1);
.size op1f_input_turn_place, .-op1f_input_turn_place

.global op1f_page_screen
.type op1f_page_screen, STT_FUNC
op1f_page_screen:
    [--SP] = P0;
    [--SP] = R2;
    P0.H = op1f_page_screen_mode;
    P0.L = op1f_page_screen_mode;
    R2 = [P0];                              /* 0, or the mode + 1 in which a pack's page stands under 77 */
    CC = R2 == 0;
    IF CC JUMP .Lscreen_on;
    P0.H = STOCK_MODE;
    P0.L = STOCK_MODE;
    R1 = [P0];
    R1 += 1;
    CC = R1 == R2;
    IF !CC JUMP .Lscreen_on;
    R0 = PAGE_SCREEN (X);                   /* the page, whatever the controller answered */
.Lscreen_on:
    P0.H = STOCK_SLOT_WORD;
    P0.L = STOCK_SLOT_WORD;
    R1 = [P0];                              /* the firmware's own load, which this call stands in place of */
    R2 = [SP++];
    P0 = [SP++];
    RTS;
.size op1f_page_screen, .-op1f_page_screen

/* The trampolines' rooms, in the pack's zeroed data: the bytes each copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_input_down_room
.type op1f_input_down_room, STT_OBJECT
op1f_input_down_room:
    .space 16
.size op1f_input_down_room, .-op1f_input_down_room
.global op1f_input_shift_room
.type op1f_input_shift_room, STT_OBJECT
op1f_input_shift_room:
    .space 20
.size op1f_input_shift_room, .-op1f_input_shift_room
.global op1f_input_up_room
.type op1f_input_up_room, STT_OBJECT
op1f_input_up_room:
    .space 20
.size op1f_input_up_room, .-op1f_input_up_room
.global op1f_input_turn_room
.type op1f_input_turn_room, STT_OBJECT
op1f_input_turn_room:
    .space 16
.size op1f_input_turn_room, .-op1f_input_turn_room

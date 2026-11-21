/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R5, the base pack (base.pack 28): base 27's screen stub, which stays as it was for base 27, with two changes
 * in op1f_page_screen, and one word of its own:
 *
 *   It keeps the screen the current controller answered (R0 as the request hands it in, before anything here changes
 *   it) in the word op1f_screen_asked, at every request, on the thread that makes it: the effects unit
 *   (base 28's effects unit) reads it from the display thread to tell whether a pack's effect page stands in for
 *   that screen now. The word is defined here, in the pack's zeroed data, beside the rooms.
 *
 *   It answers 77 also while the mode the unit is in + 1 and that screen are one of the four pairs of the table
 *   op1f_stand_screens ({mode + 1, screen}, {0, 0} for none; the effects unit defines it, module/build_r5_base.h): entry
 *   0 the instruments' effect page, entry 1 the master's, entries 2 and 3 for later levels. The cards unit
 *   (base 28's cards unit) writes a pair (the screen, then the mode + 1) on the display thread after the page's
 *   document is under 77, and takes it back (the mode first) before the document of before goes back; the effects unit
 *   writes one at a pick, before it hands the pick on. So, unlike the two words, a pair answers 77 for one screen of a
 *   mode, not for the whole mode: T1 on the synth goes on to the synth's own first page while the effect's page stands
 *   in for T3's.
 *
 * The two words are read first, as base 27 reads them; the table after them. The four entries, the rooms and every
 * other instruction are M2's. What base 27's text said follows and holds; where it speaks of op1f_page_screen reading
 * two words, it reads two words and the table, and writes one word.
 *
 * Step R4c, the base pack (base.pack 27): M2's input stub, which stays as it was for the bases before, with
 * one change in op1f_page_screen: it reads a second word, op1f_page_aside_mode, which the cards unit
 * (base 27's cards unit) writes while a page that stays with its mode is set aside (0, or that page's mode + 1),
 * and answers 77 when either word is the mode the unit is in + 1. A return straight from the page that took 77 to
 * the page set aside's mode is then answered 77 at the key, and the base's next frame puts that page back under 77.
 * The four entries, the rooms and every other instruction are M2's. What M2's text said follows and holds; where it
 * speaks of op1f_page_screen reading one word, it reads two.
 *
 * Step M2, the base pack: our calls at the five places of the firmware level 2 stands at (OS 1.7.0+0;
 * M2's cards unit, what they
 * ask). The base's table (M2's table of the base) puts a jump to each of the first four at its place, with a
 * trampoline the loader makes in the room named beside it: the stock instructions the jump stands on, copied from
 * the running firmware, and a jump back behind them. The fifth is a call in place of one 8-byte load.
 *
 *   op1f_input_down_entry   the key-down function's entry, 0x8c0b1ae8; the trampoline copies 8 bytes. R0 is the
 *                           firmware's record of the key; RETS the caller's return.
 *   op1f_input_shift_entry  the shifted key-down function's entry, 0x8c0b1b58; the trampoline copies 12 bytes. The
 *                           same registers.
 *   op1f_input_up_entry     the key-up function's entry, 0x8c0b1b8c; the trampoline copies 12 bytes. R0 is the
 *                           key's number: the firmware hands an up on as a number, not as a record.
 *   op1f_input_turn_place   0x8c0b1c00 inside the turn function, after the firmware's own bookkeeping (the keys are on,
 *                           the knob's byte for the click is cleared); the trampoline copies 8 bytes. The function
 *                           has made its frame; R0 and R7 hold the knob, R6 the turn, R5 the knob + 4.
 *   op1f_page_screen        0x8c0b2578 in the screen request, where the firmware loads the word named below: this call
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
 * argument space (SP += -12 after the pushes). P1 carries the jump, a register no
 * argument is in. Hardware loops: none of the five places lies in one, and compiled code is called at every place the
 * firmware calls its own compiled handlers.
 *
 * op1f_page_screen makes the firmware's load itself, last, and changes R0 to 77 only while a pack's page stands
 * under 77 in the mode the unit is in: the base writes op1f_page_screen_mode (0, or that mode + 1) on the display
 * thread after the document is in place (R4c: or while a page set aside waits for that mode, op1f_page_aside_mode;
 * R5: or while the mode + 1 and the screen answered are a pair of op1f_stand_screens). It reads three words (R4c: the
 * mode first, then the two words; R5: then the table's pairs, and it writes op1f_screen_asked) and keeps P0 and R2,
 * which it uses, as it came: nothing it touches is live in the request but R0 and R1, and CC, which the request sets
 * again at once. It calls nothing, so it runs as it is inside a request made with interrupts masked (J5i).
 */
.set STOCK_MODE, 0x8400f978                 /* the mode: 0 synth, 1 drum, 2 tape, 3 mixer */
.set STOCK_SLOT_WORD, 0x84d21088            /* the word the screen request loads at 0x8c0b2578 */
.set STOCK_TURN_END, 0x8c0b1c24             /* the turn function's own way out: its frame taken down, back to its caller */
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
    P0.H = op1f_screen_asked;               /* R5: the screen the controller answered, kept at every request */
    P0.L = op1f_screen_asked;
    [P0] = R0;
    P0.H = STOCK_MODE;
    P0.L = STOCK_MODE;
    R1 = [P0];
    R1 += 1;                                /* the mode the unit is in + 1, never 0: a word of 0 matches no mode */
    P0.H = op1f_page_screen_mode;
    P0.L = op1f_page_screen_mode;
    R2 = [P0];                              /* 0, or the mode + 1 in which a pack's page stands under 77 */
    CC = R1 == R2;
    IF CC JUMP .Lscreen_page;
    P0.H = op1f_page_aside_mode;            /* R4c: 0, or the mode + 1 of the page set aside */
    P0.L = op1f_page_aside_mode;
    R2 = [P0];
    CC = R1 == R2;
    IF CC JUMP .Lscreen_page;
    P0.H = op1f_stand_screens;              /* R5: the four pairs {mode + 1, screen}: 77 for that one screen of that mode */
    P0.L = op1f_stand_screens;
    R2 = [P0];
    CC = R1 == R2;
    IF !CC JUMP .Lpair_1;
    R2 = [P0 + 4];
    CC = R0 == R2;
    IF CC JUMP .Lscreen_page;
.Lpair_1:
    R2 = [P0 + 8];
    CC = R1 == R2;
    IF !CC JUMP .Lpair_2;
    R2 = [P0 + 12];
    CC = R0 == R2;
    IF CC JUMP .Lscreen_page;
.Lpair_2:
    R2 = [P0 + 16];
    CC = R1 == R2;
    IF !CC JUMP .Lpair_3;
    R2 = [P0 + 20];
    CC = R0 == R2;
    IF CC JUMP .Lscreen_page;
.Lpair_3:
    R2 = [P0 + 24];
    CC = R1 == R2;
    IF !CC JUMP .Lscreen_on;
    R2 = [P0 + 28];
    CC = R0 == R2;
    IF !CC JUMP .Lscreen_on;
.Lscreen_page:
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
/* R5: the screen the current controller answered at the last request (module/build_r5_base.h). */
.global op1f_screen_asked
.type op1f_screen_asked, STT_OBJECT
op1f_screen_asked:
    .space 4
.size op1f_screen_asked, .-op1f_screen_asked

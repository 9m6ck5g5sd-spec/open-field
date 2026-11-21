/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Base 30 (R6d): base 29's stub at the place in the firmware's render loop for the unit's own popups over a page at 77
 * (base 29's popups stub, which stays as it was), its code instruction for instruction, its words said without
 * the firmware's own instructions at the place.
 *
 * Base 29, second build (R6b): the place in the firmware's render loop for the unit's own popups over a page at 77
 * (OS 1.7.0+0; 1.7.3 by translation through the map). The logic is op1f_popups_loop and held_draw in
 * module/build_r6d_cards.cpp.
 *
 *   op1f_popups_place  P1, the render loop's call of a screen's render (in 1.7.0 at 0x8fc2688e, in 1.7.3 at
 *                      0x8fc2688e).
 *                      At the place the loop has the screen in R0, the screen's table in P1 and the time it worked
 *                      out for this frame in R1, and its eight bytes there call the screen's render, found in the
 *                      screen's table, with a piece of the loop's own work on R6 before the call (the frame's first
 *                      screen is behind it). The base's table writes a CALL.a to this entry over the eight bytes, so
 *                      that it returns behind them; the entry does that work and finds the render itself, the first
 *                      two lines of its code. While the screen the GUI shows is
 *                      not 77 it jumps to the render with the loop's R0 and R1 and the loop's return address in RETS:
 *                      the call the loop made, from one place
 *                      further. While it is 77 (a page of the base's or of a pack at 77, which the loop draws after
 *                      every screen up to 76 and the packs paint after the loop) it hands the screen, the time and the
 *                      render to op1f_popups_loop, which keeps the render of one of the unit's own popups (60 to 85,
 *                      but 77) for the frame call to make after the pages are painted, once, with that time, and makes
 *                      any other at once.
 *
 * The registers. At the place the loop has just set R0, R1 and P1 for the call; R2, R3, P0 and ASTAT hold nothing it
 * keeps across the call (a render is compiled code, which may change them, and the loop sets each before it reads it
 * again: read in both images), and R4 to R7, P3 to P5 and FP are the loop's, which neither way changes (the compiled
 * function keeps them as the calling convention says). So the entry decides with R2, R3, P0 and CC. On the way to the
 * render it takes no stack; on the way to the base's function it pushes RETS and gives the compiled code its twelve
 * bytes of argument space (SP += -12 after the push): 16 bytes below the loop's own argument space.
 *
 * The instructions while the screen shown is not 77, from the entry to the render: eight (the loop's own two, the two
 * halves of the word's address, its load, 77 into R2, the test, the branch not taken, the jump), where the loop's own
 * way had three.
 */
.set STOCK_CURRENT_SCREEN, 0x848d2540          /* the id of the screen the GUI shows (0x8fc23a1c sets it) */

.section .text
.align 2

.global op1f_popups_place
.type op1f_popups_place, STT_FUNC
op1f_popups_place:
    R6.L = 0;                                   /* the loop's work on R6, done here where the place did it */
    P1 = [P1 + 12];                             /* the screen's render, found in its table */
    P0.H = STOCK_CURRENT_SCREEN;
    P0.L = STOCK_CURRENT_SCREEN;
    R3 = [P0];
    R2 = 77 (X);                                /* a plain number: the assembler takes a .set in a compare as 0 */
    CC = R3 == R2;
    IF CC JUMP .Lpage;
    JUMP (P1);                                  /* the render, with the loop's R0 and R1: it returns into the loop */
.Lpage:
    [--SP] = RETS;
    R2 = P1;                                    /* the render, the third argument */
    SP += -12;
    CALL op1f_popups_loop;                      /* (screen, time, render) */
    SP += 12;
    RETS = [SP++];
    RTS;                                        /* into the loop, behind the eight bytes */
.size op1f_popups_place, .-op1f_popups_place

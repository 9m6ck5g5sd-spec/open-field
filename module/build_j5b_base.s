/* Build J image J5b: the base's stub at stock's draw of the version text (analysis/pack_loader_study.md, section 16).
 *
 * The site, 0x8c0b3b14, is the 8-byte load of the word at 0x0801b7f8 into R0 (the frame buffer) in the main thread's start-up,
 * a few hundred instructions after the load point. Stock has just stored the two stack arguments of its one draw of
 * the version text, 0x8c0dc920(frame buffer, 240, 170, text, centred):
 *
 *     The text ("1.7.0") and the word that says "centred on x" have just been stored as the call's two
 *     arguments on the stack. The site loads the frame buffer's address as the first argument; behind it the
 *     two that are left, x 240 and y 170, are loaded, and the draw is called.
 *
 * The site is reached straight on only (from 0x8c0b3af8, which two jumps reach). After it R2 and R1 are set anew,
 * the call follows, and what stock uses after the call it takes from its frame: nothing is live in the registers a
 * call may change but R0, which this stub sets last. The main thread's function keeps its own RETS in its frame.
 * The base writes the CALL.a at the load point (build_j5b_base.cpp, build_j5b_base_table.s); no image and no pack
 * has the site.
 *
 * The stub hands stock's text to op1f_base_screen_text and puts its answer where the text was: stock's own pointer,
 * or the base's buffer with stock's text and what runs after it. Then the displaced load. The same shape as the
 * load point's stub (build_j_boot.s, proven since J1), with the twelve bytes of argument space a compiled callee
 * may write (tools/stub_calls.py holds every hand-written call to that). With RETS pushed and the twelve bytes
 * made, the caller's [SP + 0xc] is this routine's [SP + 28].
 */
.text
.align 2

.global op1f_base_text_stub
.type op1f_base_text_stub, STT_FUNC
op1f_base_text_stub:
    [--SP] = RETS;
    SP += -12;                          /* the argument space of the call */
    R0 = [SP + 28];                     /* stock's text: the caller's [SP + 0xc] */
    CALL op1f_base_screen_text;
    [SP + 28] = R0;                     /* the text stock's draw is handed */
    SP += 12;
    RETS = [SP++];
    P1.H = 0x0801;
    P1.L = 0xb7f8;
    R0 = [P1];                          /* the displaced load: the frame buffer */
    RTS;
.size op1f_base_text_stub, .-op1f_base_text_stub

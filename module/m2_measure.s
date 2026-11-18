/* M2 measurement image, OS 1.7.0+0: the two entry stubs. The logic is in m2_measure.cpp.
 *
 * op1f_gfx_frame_hook: the display thread's call at 0x8c0b116e, which writes the frame buffer out of the
 * cache before it is shown. Its three arguments are the frame buffer's first address, its last and a zero.
 * Calls the C++ frame function with the frame buffer, then tail-jumps to the firmware's function with the
 * arguments and the return address as found.
 *
 * op1f_audio_handler: installed in place of the stock audio block handler 0x8c0d80dc
 * (Audio.cpp on_handle_buffer) by the profile group "m2-audio", which rewrites the three
 * loads of 0x8c0d80dc into R0 that stock uses to register it (0x8c0d848c at boot, 0x8c0ca0d0 and
 * 0x8c0ca2f8 when the debug loopback controller restores it). The SPORT thread calls the handler
 * once per 64-frame DMA half-buffer with R0 = receive half, R1 = transmit half. This wrapper reads
 * CYCLES, calls the stock handler with the untouched arguments, and adds the cycles spent to the
 * counters op1f_audio {busy, count, peak} (defined in m2_measure.cpp, read on the gfx thread).
 * Nothing else runs on the audio thread: no logging, no drawing, no division.
 */
.set STOCK_FLUSH,         0x11a09d18
.set STOCK_AUDIO_HANDLER, 0x8c0d80dc

.section .text
.align 2
.global op1f_gfx_frame_hook
.type op1f_gfx_frame_hook, STT_FUNC
.global field_module_entry
.set field_module_entry, op1f_gfx_frame_hook
op1f_gfx_frame_hook:
    LINK 0;
    [--SP] = (R7:5);
    SP += -12;
    R5 = R0;
    R6 = R1;
    R7 = R2;
    CALL op1f_on_frame;
    R0 = R5;
    R1 = R6;
    R2 = R7;
    SP += 12;
    (R7:5) = [SP++];
    UNLINK;
    P2.H = STOCK_FLUSH;
    P2.L = STOCK_FLUSH;
    JUMP (P2);
.size op1f_gfx_frame_hook, .-op1f_gfx_frame_hook

.global op1f_audio_handler
.type op1f_audio_handler, STT_FUNC
op1f_audio_handler:
    LINK 0;
    [--SP] = (R7:4, P5:5);
    SP += -12;
    R4 = R0;
    R5 = R1;
    R6 = R2;
    R7 = CYCLES;
    P2.H = STOCK_AUDIO_HANDLER;
    P2.L = STOCK_AUDIO_HANDLER;
    CALL (P2);
    R0 = CYCLES;
    R0 = R0 - R7;                       /* cycles spent in the stock handler for this block */
    P5.H = op1f_audio;
    P5.L = op1f_audio;
    R1 = [P5];
    R1 = R1 + R0;
    [P5] = R1;                          /* busy += cycles */
    R1 = [P5 + 4];
    R1 += 1;
    [P5 + 4] = R1;                      /* count += 1 */
    R1 = [P5 + 8];
    CC = R1 < R0 (IU);
    IF CC R1 = R0;
    [P5 + 8] = R1;                      /* peak = max(peak, cycles) */
    SP += 12;
    (R7:4, P5:5) = [SP++];
    UNLINK;
    RTS;
.size op1f_audio_handler, .-op1f_audio_handler

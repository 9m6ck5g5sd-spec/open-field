/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Build J packs J5e: the entry of the picture loader (OS 1.7.0+0). Logic in J5e's picture loader.
 *
 * This is Build B's picture stub without the pictures: since J5e they stand in the pack's file behind the lists and are read
 * when a screen asks for one. Build B's picture stub and Build B's picture loader are kept
 * as they are for the packs before J5e.
 *
 * svg_loader.cpp's load(name) 0x8fc8ab7c builds "content/display/<name>" (0x848daf5c), opens the file, reads it
 * into a buffer from 0x8fcc7560, parses the buffer with parse_svg_from_ram 0x8c040298 and frees it; the document
 * is its result (0 on any failure, which the caller 0x8fc88e70 turns into a fatal assert). A JUMP.a at its first
 * 8 bytes lands here. The C function answers with a document parsed from a picture a pack carries for that name
 * and with 0 for every other name, and for a picture that cannot be read or does not parse; then the routine goes
 * on through its room and stock continues with the device's file.
 *
 *      load  0x8fc8ab7c  -> its room, then 0x8fc88c6c
 *
 * The room (packs L3): until L3 this routine ran the instructions its hook
 * displaced itself, as a copy. Now it holds none of them and ends in a jump to its room: sixteen bytes of the pack's
 * zeroed data into which the base copies those instructions from the running firmware, with a jump back behind
 * them, before it writes the hook (module/build_m4_loader.cpp, make_trampoline; the table entry names the room). P2,
 * the register it jumps through, is the one it jumped through before.
 *
 * The C function is compiled code that calls stock functions with a fourth argument on the stack, so this routine
 * gives it the twelve bytes of argument space a caller owes (Build B's picture stub gave none, and its callee kept its
 * arguments in registers).
 */
.section .text
.align 2

.global op1f_svg_load
.type op1f_svg_load, STT_FUNC
op1f_svg_load:
    [--SP] = RETS;
    [--SP] = R0;
    SP += -12;                          /* the argument space of the call */
    CALL op1f_svg_embedded;             /* void* (const char* name): a parsed document, or 0 */
    SP += 12;
    R2 = R0;
    R0 = [SP++];
    RETS = [SP++];
    CC = R2 == 0;
    IF CC JUMP .Lsvg_stock;
    R0 = R2;
    RTS;
.Lsvg_stock:
    P2.H = op1f_svg_load_room;          /* the instructions the hook displaced, then 0x8fc88c6c */
    P2.L = op1f_svg_load_room;
    JUMP (P2);
.size op1f_svg_load, .-op1f_svg_load

/* The room: zeroed data, which the base clears when it loads the pack and fills when it applies the table: the
 * eight bytes its trampoline copies and the eight of the jump back. */
.section .bss
.balign 4
.global op1f_svg_load_room
.type op1f_svg_load_room, STT_OBJECT
op1f_svg_load_room:
    .space 16
.size op1f_svg_load_room, .-op1f_svg_load_room

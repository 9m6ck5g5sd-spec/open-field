/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step S4a of piece 11, the base pack: the door's build 5, its two stubs (OS 1.7.0+0). The sibling of the door's stubs of base 23
 * (build 4, proven on a unit), the door's stubs of base 22 (build 3) and the door's stubs of base 18
 * (build 2): the same instructions under the same names, so the base's table and its rooms are the
 * starting base's own (base 24's). Logic in the door's unit of base 25.
 *
 * The firmware's handler of complete SysEx messages 0x8fc8daae(midi_if, buffer, length, a4, a5, a6) is called with
 * every complete SysEx message: from the USB poll in the main loop (0x8fc8c674: the buffer 0x848ea444), from a second
 * parser fed by USB (0x8fc8bf88) and for Bluetooth MIDI (0x8fca1f40, in command_handler_ble.cpp: the BLE link's
 * messages of type 0x23; build 5's reading, where build 4's comment said
 * the keyboard controller's link). The base's table puts a JUMP.a to op1f_door_entry over its first eight bytes, with a trampoline the loader
 * makes in op1f_door_room: those eight bytes copied from the running firmware, and a jump back behind them.
 *
 *      the handler at 0x8fc8daae: its first eight bytes are three whole instructions, none of them relative to its
 *      own place, so the room can run them; the room then jumps back to 0x8fc8dab6.
 *
 * op1f_door_sysex is handed the buffer's address and the message's length and answers 1 for a message the door took
 * (answered, or dropped): this routine then returns to the handler's caller as if the handler had run (what the three
 * callers do with the answer: none uses it, the third's own caller at 0x8fca2814 writes R0
 * at once; R0 is left 0, as the test bench's stub left it). For every other message it puts back the three argument registers
 * and RETS and jumps to the room: the firmware's own instructions, then the handler, with every register and the
 * caller's stack words (a4 to a6, which the handler reads through FP) as they came. P1 carries the jump: the
 * handler's first instruction writes it, so it holds nothing of the caller's. The C function is compiled code, so
 * this routine gives it the twelve bytes of argument space a caller owes.
 */
.section .text
.align 2

.global op1f_door_entry
.type op1f_door_entry, STT_FUNC
op1f_door_entry:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;                          /* the argument space of the call */
    R0 = R1;                            /* the buffer's address */
    R1 = R2;                            /* the message's length */
    CALL op1f_door_sysex;               /* int (u32 buffer, int length): 1 when the door took it */
    SP += 12;
    CC = R0 == 0;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    IF CC JUMP .Ldoor_firmware;
    R0 = 0;
    RTS;                                /* back to the handler's caller */
.Ldoor_firmware:
    P1.H = op1f_door_room;
    P1.L = op1f_door_room;
    JUMP (P1);                          /* the firmware's own way, from the instructions under the jump */
.size op1f_door_entry, .-op1f_door_entry

/* The trampoline's room, in the pack's zeroed data: the eight bytes it copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_door_room
.type op1f_door_room, STT_OBJECT
op1f_door_room:
    .space 16
.size op1f_door_room, .-op1f_door_room

/* Build 2, mend A: the door's own copy of the keyboard controller's map, taken at the map handler's store (OS 1.7.0
 * 0x8fca54dc, 1.7.3 0x8fca54dc). The handler 0x8fca5436 ends,
 * after it has posted a key event for every changed bit, with
 *
 *      R0 = 1 (the answer); [P1 + 0x823c] = R1 (keys 0 to 31); [P1 + 0x8240] = R2 (keys 32 to 63); SP += 12; ...
 *
 * The base's table puts a JUMP.a over the two stores, eight bytes, two whole instructions, nothing relative to their
 * place; no branch of the image lands on either (both images read, and bfin_callers). Their trampoline in
 * op1f_door_keymap_room makes the two stores and jumps back to the SP += 12 behind them. Live there: R0 (the answer),
 * R1, R2 and P1 (the stores'), SP and FP; nothing else the handler reads before it returns.
 *
 * It runs where the handler runs: on the audio thread (the keyboard pump is called from the audio handler) or on a
 * thread waiting in a request. So: no call, no loop, no log; every register it uses and ASTAT are put back as they
 * came; it writes only the door's four words op1f_door_keys: the two map words, P1 (where the firmware stores), and a
 * count of the stores. The jump to the room is by distance: room and stub are in the same pack.
 */
.section .text
.align 2
.global op1f_door_keymap_entry
.type op1f_door_keymap_entry, STT_FUNC
op1f_door_keymap_entry:
    [--SP] = ASTAT;
    [--SP] = P0;
    [--SP] = R3;
    P0.H = op1f_door_keys;
    P0.L = op1f_door_keys;
    [P0] = R1;                          /* keys 0 to 31, as the firmware is about to store them */
    [P0 + 4] = R2;                      /* keys 32 to 63 */
    [P0 + 8] = P1;                      /* the object it stores at: the map at P1 + 0x823c */
    R3 = [P0 + 12];
    R3 += 1;                            /* the count of stores (ASTAT put back below) */
    [P0 + 12] = R3;
    R3 = [SP++];
    P0 = [SP++];
    ASTAT = [SP++];
    JUMP.L op1f_door_keymap_room;       /* the two stores, then back to the handler */
.size op1f_door_keymap_entry, .-op1f_door_keymap_entry

.section .bss
.balign 4
.global op1f_door_keymap_room
.type op1f_door_keymap_room, STT_OBJECT
op1f_door_keymap_room:
    .space 16
.size op1f_door_keymap_room, .-op1f_door_keymap_room
.global op1f_door_keys
.type op1f_door_keys, STT_OBJECT
op1f_door_keys:
    .space 16
.size op1f_door_keys, .-op1f_door_keys

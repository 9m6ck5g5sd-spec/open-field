/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R6, the base pack of level 9: the entries at two functions of the firmware for the synth engines (OS 1.7.0+0).
 * The logic is in base 29's engines unit. The base's table puts a JUMP.a to each entry over the first eight bytes of its
 * function, with a trampoline the loader makes in the room named beside it: those eight bytes copied from the running
 * firmware (whole instructions, none relative to its own place), and a jump back behind them. A jump to the room is
 * the firmware's function as it always was; a call of the room is the same function returning to its caller.
 *
 *   op1f_engine_factory_entry  E1, the engine factory, 0x8f0112e8; R0 the type, R1 where the six objects go, R2 a third
 *                              word (the sampler's arm reads it). With level 9 at rest (op1f_engines_awake 0), the room
 *                              at once. Else the entry calls the room, so that the firmware's factory builds and returns
 *                              here, then calls op1f_engine_built(type, where) and returns to the factory's caller with
 *                              the factory's answer in R0. The factory and every one of its arms read no word of their
 *                              caller's stack (read in both images), so the call from here, with the
 *                              entry's own words above it, builds as the caller's call would have.
 *   op1f_engine_pick_entry     E2, the engine pick, 0x8d4eb3e8; R0 the catalog id picked, R1 the synth slot. With level
 *                              9 at rest, the room at once; else op1f_engine_pick_do(id, slot), which calls the room
 *                              itself, and back to the pick's caller.
 *
 * The registers. Each entry is a function's first instruction, reached by a call of compiled code: R0 to R2 are the
 * function's arguments, and R3, P0 to P2 and ASTAT hold nothing the caller keeps (no caller of either function uses
 * them, nor R0, after the call: the readings' (l) for the factory's four, and the list's and MIDI's calls of the pick).
 * So an entry decides with R3, P0 and CC, and calls nothing before it has decided; on the way at rest it leaves every
 * other register, RETS, SP and the caller's stack as they came. The jump to a room is by distance: room and entry are
 * in the same pack. On the way through the base's function an entry pushes what it gives back and gives the compiled
 * code its twelve bytes of argument space (SP += -12 after the pushes), so that nothing of the caller's is written.
 * The stack an entry takes at its deepest: 0 bytes on the way at rest; 24 on the factory's way (RETS, the type, the
 * place, and the twelve bytes the factory is called with), below which the factory takes its own; 28 on the pick's way
 * (RETS, R0, R1, R2 and the twelve bytes).
 *
 * The instructions on the way at rest, from the entry to the room: six for each (the two halves of the word's address,
 * the load, the test, the branch not taken, the jump).
 */
.section .text
.align 2

.global op1f_engine_factory_entry
.type op1f_engine_factory_entry, STT_FUNC
op1f_engine_factory_entry:
    P0.H = op1f_engines_awake;
    P0.L = op1f_engines_awake;
    R3 = [P0];
    CC = R3 == 0;
    IF !CC JUMP .Lfactory_awake;
    JUMP.L op1f_engine_factory_room;        /* at rest: the firmware's own way at once */
.Lfactory_awake:
    [--SP] = RETS;
    [--SP] = R0;                            /* the type */
    [--SP] = R1;                            /* where the six objects go */
    SP += -12;
    CALL op1f_engine_factory_room;          /* the firmware's factory, R0 to R2 as they came: it returns here */
    SP += 12;
    R1 = [SP++];                            /* where */
    R2 = [SP++];                            /* the type */
    [--SP] = R0;                            /* the factory's answer, kept */
    R0 = R2;
    SP += -12;
    CALL op1f_engine_built;                 /* (type, where) */
    SP += 12;
    R0 = [SP++];
    RETS = [SP++];
    RTS;
.size op1f_engine_factory_entry, .-op1f_engine_factory_entry

.global op1f_engine_pick_entry
.type op1f_engine_pick_entry, STT_FUNC
op1f_engine_pick_entry:
    P0.H = op1f_engines_awake;
    P0.L = op1f_engines_awake;
    R3 = [P0];
    CC = R3 == 0;
    IF !CC JUMP .Lpick_awake;
    JUMP.L op1f_engine_pick_room;           /* at rest: the firmware's own way at once */
.Lpick_awake:
    [--SP] = RETS;
    [--SP] = R0;
    [--SP] = R1;
    [--SP] = R2;
    SP += -12;
    CALL op1f_engine_pick_do;               /* (id, slot), as they came: it calls the room itself */
    SP += 12;
    R2 = [SP++];
    R1 = [SP++];
    R0 = [SP++];
    RETS = [SP++];
    RTS;
.size op1f_engine_pick_entry, .-op1f_engine_pick_entry

/* The trampolines' rooms, in the pack's zeroed data: the eight bytes each copies, and eight for the jump back. */
.section .bss
.balign 4
.global op1f_engine_factory_room
.type op1f_engine_factory_room, STT_OBJECT
op1f_engine_factory_room:
    .space 16
.size op1f_engine_factory_room, .-op1f_engine_factory_room
.balign 4
.global op1f_engine_pick_room
.type op1f_engine_pick_room, STT_OBJECT
op1f_engine_pick_room:
    .space 16
.size op1f_engine_pick_room, .-op1f_engine_pick_room

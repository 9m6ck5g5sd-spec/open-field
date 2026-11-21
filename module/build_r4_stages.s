/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step R4, the base pack 24: the sound stages, the four places in the sound's path (OS 1.7.0+0). Logic in
 * base 24's stages unit.
 *
 * The base's table puts a CALL.a over one instruction of eight bytes at each place, inside a function of the firmware
 * that keeps its own return address in its frame, so that the CALL's write of RETS is harmless there. Each stub does
 * what the instruction under the CALL did, as its last step before it comes back:
 *
 *   op1f_stages_synth_after    place 0, the synth's sound after its effect: the load of its address, 0x84a01554, into P2
 *   op1f_stages_drum_after     place 1, the drum's sound after its effect: the load of its address, 0x84a01564, into P2
 *   op1f_stages_master_before  place 2, the unit's sum before the master effect: the load of the word at 0x84a03c88 into R0
 *   op1f_stages_master_after   place 3, the unit's sum after the master effect: the load of its address, 0x84a015d0, into P0
 *
 * At each place R0 to R3, P1, P2 and CC are written again before they are read, and at the master's two places P0 as
 * well; at the instruments' two places P0 holds a pointer the code after reads. Everything else is kept as it came:
 * on the way with no stage a stub touches only R0, P1 and CC, and puts ASTAT back; on the way with stages it pushes and
 * puts back every data, pointer, index, modify, base and length register, both accumulators, the loop registers, ASTAT
 * and RETS, and hands the C function the twelve bytes of argument space a caller owes compiled code (SP += -12 below
 * its own pushes). Compiled code takes the length registers to be 0: a stub sets them so for the call, and puts back
 * what was there.
 *
 * op1f_stages_at holds how many stages stand at each place, a word each, by the places' numbers; it is 0 until the
 * base's first frame. With none at a place the stub decides in a load and a test, and the sound goes on as it always
 * did. With stages there, op1f_stages_run(place) is called. It runs on the audio thread, calls the stages' functions
 * and nothing of the firmware's, and says no line.
 *
 * The stack a stub takes at its deepest: 4 bytes on the way with no stage; 180 bytes on the way with stages (ASTAT,
 * RETS, R0 to R7, P0 to P5, the four accumulator parts, the six loop registers, the sixteen index, modify, base and
 * length registers, and the twelve bytes of argument space), below which the C function and the stages take their own.
 */
.set SYNTH_AFTER, 0x84a01554
.set DRUM_AFTER, 0x84a01564
.set MASTER_EFFECT, 0x84a03c88
.set MASTER_AFTER, 0x84a015d0

.section .text
.align 2

/* In the place of the load of 0x84a01554 into P2: the synth's sound after its effect. P0 is kept. */
.global op1f_stages_synth_after
.type op1f_stages_synth_after, STT_FUNC
op1f_stages_synth_after:
    [--SP] = ASTAT;
    P1.H = op1f_stages_at;
    P1.L = op1f_stages_at;
    R0 = [P1 + 0];                      /* how many stages stand at place 0 */
    CC = R0 == 0;
    IF CC JUMP .Lsynth_after_none;
    [--SP] = RETS;
    [--SP] = (R7:0, P5:0);
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
    R0 = 0;
    L0 = R0;                            /* compiled code takes them to be 0 */
    L1 = R0;
    L2 = R0;
    L3 = R0;
    R0 = 0;                             /* the place */
    SP += -12;                          /* the argument space of the call */
    CALL op1f_stages_run;               /* (place) */
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
    (R7:0, P5:0) = [SP++];
    RETS = [SP++];
.Lsynth_after_none:
    ASTAT = [SP++];
    P2.H = SYNTH_AFTER;                 /* the instruction the CALL stands on */
    P2.L = SYNTH_AFTER;
    RTS;
.size op1f_stages_synth_after, .-op1f_stages_synth_after

/* In the place of the load of 0x84a01564 into P2: the drum's sound after its effect. P0 is kept. */
.global op1f_stages_drum_after
.type op1f_stages_drum_after, STT_FUNC
op1f_stages_drum_after:
    [--SP] = ASTAT;
    P1.H = op1f_stages_at;
    P1.L = op1f_stages_at;
    R0 = [P1 + 4];                      /* how many stages stand at place 1 */
    CC = R0 == 0;
    IF CC JUMP .Ldrum_after_none;
    [--SP] = RETS;
    [--SP] = (R7:0, P5:0);
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
    R0 = 0;
    L0 = R0;                            /* compiled code takes them to be 0 */
    L1 = R0;
    L2 = R0;
    L3 = R0;
    R0 = 1;                             /* the place */
    SP += -12;                          /* the argument space of the call */
    CALL op1f_stages_run;               /* (place) */
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
    (R7:0, P5:0) = [SP++];
    RETS = [SP++];
.Ldrum_after_none:
    ASTAT = [SP++];
    P2.H = DRUM_AFTER;                  /* the instruction the CALL stands on */
    P2.L = DRUM_AFTER;
    RTS;
.size op1f_stages_drum_after, .-op1f_stages_drum_after

/* In the place of the load of the word at 0x84a03c88 into R0: the unit's sum before the master effect, and the word the firmware tests next. */
.global op1f_stages_master_before
.type op1f_stages_master_before, STT_FUNC
op1f_stages_master_before:
    [--SP] = ASTAT;
    P1.H = op1f_stages_at;
    P1.L = op1f_stages_at;
    R0 = [P1 + 8];                      /* how many stages stand at place 2 */
    CC = R0 == 0;
    IF CC JUMP .Lmaster_before_none;
    [--SP] = RETS;
    [--SP] = (R7:0, P5:0);
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
    R0 = 0;
    L0 = R0;                            /* compiled code takes them to be 0 */
    L1 = R0;
    L2 = R0;
    L3 = R0;
    R0 = 2;                             /* the place */
    SP += -12;                          /* the argument space of the call */
    CALL op1f_stages_run;               /* (place) */
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
    (R7:0, P5:0) = [SP++];
    RETS = [SP++];
.Lmaster_before_none:
    ASTAT = [SP++];
    P1.H = MASTER_EFFECT;               /* the instruction the CALL stands on */
    P1.L = MASTER_EFFECT;
    R0 = [P1];
    RTS;
.size op1f_stages_master_before, .-op1f_stages_master_before

/* In the place of the load of 0x84a015d0 into P0: the unit's sum after the master effect. */
.global op1f_stages_master_after
.type op1f_stages_master_after, STT_FUNC
op1f_stages_master_after:
    [--SP] = ASTAT;
    P1.H = op1f_stages_at;
    P1.L = op1f_stages_at;
    R0 = [P1 + 12];                     /* how many stages stand at place 3 */
    CC = R0 == 0;
    IF CC JUMP .Lmaster_after_none;
    [--SP] = RETS;
    [--SP] = (R7:0, P5:0);
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
    R0 = 0;
    L0 = R0;                            /* compiled code takes them to be 0 */
    L1 = R0;
    L2 = R0;
    L3 = R0;
    R0 = 3;                             /* the place */
    SP += -12;                          /* the argument space of the call */
    CALL op1f_stages_run;               /* (place) */
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
    (R7:0, P5:0) = [SP++];
    RETS = [SP++];
.Lmaster_after_none:
    ASTAT = [SP++];
    P0.H = MASTER_AFTER;                /* the instruction the CALL stands on */
    P0.L = MASTER_AFTER;
    RTS;
.size op1f_stages_master_after, .-op1f_stages_master_after

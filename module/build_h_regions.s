/* Build H image H4: the same effect on several wagons (analysis/fx_chain_study.md, section 20). Profile group
 * "build-h4": three CALL.a stubs, each in place of the 8-byte immediate load that names a producer's fixed state
 * region, so that the chain unit (build_h_chain.cpp) can point a module-built instance at a private region of its own
 * around the instance's process call. Stock's own instances (the mirrored wagon's producer, the master effect's) see
 * stock's bases: the module words hold them whenever the audio thread is not inside a module instance's call.
 *
 * The spring (Rymd) keeps its delay lines at 0x84ab3b68 + 58,272 x index (the index the init stored at this + 0x64),
 * the mother (the reverb) its two sub-blocks at 0x840293a0 + 0x42318 x index (the index at this + 0x84); both compute
 * the base from an immediate inside the process function, so the module cannot hand them another region through the
 * object. The stubs load the base from the module word instead. The sites, all inside LINKed functions (the function's
 * RETS is in its frame, so the CALL's RETS write is harmless):
 *
 *   0x8d516c3e  the load of the regions' base 0x84ab3b68 into P4, in the spring process 0x8d516b28, before the
 *               index times 58272 is added to it. Only P4 is written here. -> op1f_spring_base_p4.
 *   0x8d5172da  the load of the same base into R6, on the same function's wrap path (0x8d5172ac: the region's
 *               tail is cleared when the write position wraps). P0 is live there, so the stub spills P1 to the
 *               stack around its load. -> op1f_spring_base_r6.
 *   0x11a066be  the load of the regions' base 0x840293a0 into P4, in the mother process 0x11a06694 (L1), before
 *               the index times 0x42318 is added to it. Only P4 is written. -> op1f_mother_base_p4 (a reviewed
 *               hook inside the protected L1 range, as H1a's four).
 *
 * The words op1f_spring_base and op1f_mother_base are defined by build_h_chain.cpp (volatile, initialised to the stock
 * bases); the audio thread sets them to an instance's region before its call and restores them after it. No other
 * register, no flag (a load changes neither CC nor ASTAT) and no loop register is touched.
 */
.text
.align 2

/* Replaces the load of 0x84ab3b68 into P4 at 0x8d516c3e. */
.global op1f_spring_base_p4
.type op1f_spring_base_p4, STT_FUNC
op1f_spring_base_p4:
    P4.H = op1f_spring_base;
    P4.L = op1f_spring_base;
    P4 = [P4];
    RTS;
.size op1f_spring_base_p4, .-op1f_spring_base_p4

/* Replaces the load of 0x84ab3b68 into R6 at 0x8d5172da. P1 is spilled: P0 is live at the site. */
.global op1f_spring_base_r6
.type op1f_spring_base_r6, STT_FUNC
op1f_spring_base_r6:
    [--SP] = P1;
    P1.H = op1f_spring_base;
    P1.L = op1f_spring_base;
    R6 = [P1];
    P1 = [SP++];
    RTS;
.size op1f_spring_base_r6, .-op1f_spring_base_r6

/* Replaces the load of 0x840293a0 into P4 at 0x11a066be. */
.global op1f_mother_base_p4
.type op1f_mother_base_p4, STT_FUNC
op1f_mother_base_p4:
    P4.H = op1f_mother_base;
    P4.L = op1f_mother_base;
    P4 = [P4];
    RTS;
.size op1f_mother_base_p4, .-op1f_mother_base_p4

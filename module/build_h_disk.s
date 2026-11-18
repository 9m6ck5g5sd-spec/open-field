/* Build H image H6: the chain presets as files in the disk mode (analysis/fx_chain_study.md section 22).
 *
 * The disk mode's device tree is built by 0x8fcadd64 (basset_device.cpp): the root folder at root + 8, then the
 * "synth", "drum", "tapes" and "mixdown" folders through 0x8fcadd0c, 0x8fcadd38, 0x8fcad7e8 and 0x8fcada10, each
 * called with the root folder in R7. The eight bytes at 0x8fcaddea are the last builder's call, with the move of
 * its argument before it, and the first instruction of the epilogue, which gives twelve bytes of stack back; this
 * stub takes their place (a CALL.a from the hook), makes the mixdown folder as stock did and then the module's
 * "fx" folder (op1f_mtp_populate_fx in build_h_disk.cpp, with R7 still the root: a callee-saved register),
 * restores RETS and gives the twelve bytes back itself before returning to 0x8fcaddf2, where stock goes on.
 *
 * The root behaviour's child maker (its vtable's slot +0x10, 0x8fcab080) is replaced by a `word` edit with
 * op1f_mtp_root_child (build_h_disk.cpp), which answers "fx" with the module's folder behaviour and every other
 * name with stock's, so 0x8fcaccf0(root, "fx") builds the folder node through stock's own code.
 *
 * J3b: the stub gives its callees the twelve bytes of argument space the calling convention asks of a caller
 * (the words at [SP + 0] to [SP + 11] when the CALL is made, which a callee may write). From H6 to J2 it pushed RETS
 * and called at once, so the saved RETS lay in that place; that stood only because neither callee wrote there.
 * In J3 the compiler made the last call of op1f_mtp_populate_fx a tail call of stock's add-file function
 * 0x8fcacf48, which keeps a word at [SP + 0x30], the first word above its own 0x30-byte frame, whenever a folder's
 * list has to grow (0x8fcad04c), and a new folder's first file always makes it grow: the word (4) replaced the
 * saved RETS, the stub returned to address 4 and the unit froze as the disk mode was entered
 * (analysis/build_j3_device_result.md). tools/stub_calls.py and its test hold every hand-written call to this.
 */

.set STOCK_MIXDOWN_FOLDER, 0x8fcada10

.section .text
.align 2

.global op1f_mtp_populate_stub
.type op1f_mtp_populate_stub, @function
op1f_mtp_populate_stub:
    [--SP] = RETS;
    SP += -12;                              /* the argument space for both calls: a callee may write it */
    R0 = R7;                                /* the root folder */
    P2.H = STOCK_MIXDOWN_FOLDER;
    P2.L = STOCK_MIXDOWN_FOLDER;
    CALL (P2);                              /* stock's mixdown folder, as the displaced call did */
    R0 = R7;
    CALL op1f_mtp_populate_fx;              /* the module's folders and their files (R7 is callee-saved) */
    SP += 12;
    RETS = [SP++];
    SP += 0xc;                              /* the displaced first instruction of the epilogue */
    RTS;
.size op1f_mtp_populate_stub, .-op1f_mtp_populate_stub

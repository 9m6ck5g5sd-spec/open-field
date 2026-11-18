/* Step M1, the base pack: op1f_log, the log for a pack with a card (OS 1.7.0+0; module/build_m1_card.h).
 *
 *      void op1f_log(int level, const char* file, int line, const char* format, ...)
 *
 * is stock's own logger, 0x8c0539f0, which every unit of ours calls by its address: the level (1 an error, 2 a
 * warning, 3 for information), a source path of which the log shows the name between the last slash and the dot, a
 * number shown with that name, a format and what it takes. A pack with a card names no address of the firmware, so
 * the base offers the logger under a name.
 *
 * The routine is a jump and nothing else. The caller's first three arguments are in R0 to R2 and the others on the
 * caller's stack, above the twelve bytes of argument space the caller owes; RETS is the caller's. All of that is
 * left as it came, so stock's logger finds what it would find had the pack called it, and returns to the pack. P1
 * is a register a call may change and that carries no argument. Nothing is called here, so no argument space is
 * owed and nothing is saved.
 */
.set STOCK_LOG, 0x8c0539f0

.section .text
.align 2
.global op1f_log
.type op1f_log, STT_FUNC
op1f_log:
    P1.H = STOCK_LOG;
    P1.L = STOCK_LOG;
    JUMP (P1);
.size op1f_log, .-op1f_log

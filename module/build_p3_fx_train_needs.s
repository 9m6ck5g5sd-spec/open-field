/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Pictures step P3 (analysis/picture_maker_study.md, section 11): the pack `fx train` carries its three own pictures as
 * instructions of the second kind, by which the unit puts each together from its own stock pictures and what the
 * pack carries of ours (module/build_p3_art.cpp and build_p2_map.cpp in `kit`). Its five LFO pages are
 * instructions of the first kind since P1. A kit of before P2 does not know the second kind, and one of before P1 knows none.
 * So this pack names the marks of a kit that knows each kind outright: the base leaves a pack out when a name it needs is offered by no pack
 * that runs, and says so on the start screen and on the log. Nothing reads these words.
 */
.section .rodata
.balign 4
.global op1f_fx_train_needs
.type op1f_fx_train_needs, STT_OBJECT
op1f_fx_train_needs:
    .long op1f_art_maker_1
    .long op1f_art_maker_2
.size op1f_fx_train_needs, .-op1f_fx_train_needs

/* Pictures step P3 (analysis/picture_maker_study.md, section 11): the pack `lfo` carries the three pictures its screens need as
 * instructions of the second kind, by which the unit puts each together from its own stock pictures and what the
 * pack carries of ours (module/build_p3_art.cpp and build_p2_map.cpp in `kit`). A kit of before P2 does not know that kind and would hand
 * the instructions to stock's parser as if they were a picture.
 * So this pack names the mark of a kit that knows the second kind outright: the base leaves a pack out when a name it needs is offered by no pack
 * that runs, and says so on the start screen and on the log. Nothing reads this word.
 */
.section .rodata
.balign 4
.global op1f_lfo_needs
.type op1f_lfo_needs, STT_OBJECT
op1f_lfo_needs:
    .long op1f_art_maker_2
.size op1f_lfo_needs, .-op1f_lfo_needs

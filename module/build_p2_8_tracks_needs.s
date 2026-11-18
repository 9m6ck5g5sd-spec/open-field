/* Pictures step P2 (analysis/picture_maker_study.md, section 9): the pack `8 tracks` carries its eight pictures as
 * instructions of the second kind, by which the unit puts each together from its own stock pictures
 * (module/build_p2_art.cpp and build_p2_map.cpp in `kit`). A kit of before P2 does not know that kind: P1's tells a
 * set of instructions by the first kind's first line alone, and an older one knows none, so either would hand these
 * instructions to stock's parser as if they were a picture. So this pack names the mark of a kit that knows the
 * second kind, outright: the base leaves a pack out when a name it needs is offered by no pack that runs, and says
 * so on the start screen and on the log. Nothing reads this word.
 */
.section .rodata
.balign 4
.global op1f_8_tracks_needs
.type op1f_8_tracks_needs, STT_OBJECT
op1f_8_tracks_needs:
    .long op1f_art_maker_2
.size op1f_8_tracks_needs, .-op1f_8_tracks_needs

/* Pictures step P3 (analysis/picture_maker_study.md, section 11): the pack `sidechain` carries its two pages as
 * instructions of the second kind, by which the unit puts each together from its own stock pictures and what the
 * pack carries of ours (module/build_p3_art.cpp and build_p2_map.cpp in `kit`). Each page takes stretches from five stock
 * pictures, and the kit of P2 holds four for one set: it would refuse both pages, and the sidechain would stand on
 * stock's own signal flow picture, in which none of its parts are.
 * So this pack names the mark of a kit that holds eight outright: the base leaves a pack out when a name it needs is offered by no pack
 * that runs, and says so on the start screen and on the log. Nothing reads this word.
 */
.section .rodata
.balign 4
.global op1f_sidechain_needs
.type op1f_sidechain_needs, STT_OBJECT
op1f_sidechain_needs:
    .long op1f_art_maker_3
.size op1f_sidechain_needs, .-op1f_sidechain_needs

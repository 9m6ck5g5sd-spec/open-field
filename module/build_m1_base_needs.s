/* Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's. */
/* Step M1, the base pack: the base serves the packs that carry a card from the list the loader fills (op1f_cards,
 * M3's cards unit). A loader of before M1 hands no cards over: under it this pack would run with an empty
 * list and serve no such pack, with no line that says so. So the pack names the mark of a loader that hands the
 * cards over, outright: the loader of M1 answers for the name op1f_loader_1 itself when it joins the packs, and a
 * loader that does not know the name leaves a pack out when a name it needs is offered by nobody, and says so on
 * the start screen and on the log. Nothing reads this word.
 */
.section .rodata
.balign 4
.global op1f_base_needs
.type op1f_base_needs, STT_OBJECT
op1f_base_needs:
    .long op1f_loader_1
.size op1f_base_needs, .-op1f_base_needs

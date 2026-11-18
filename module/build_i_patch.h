// Build I image I9: an instrument carries its train and its LFOs (analysis/lfo_train_study.md section 22).
// What the patch unit (build_i_patch.cpp, the hooks in stock's patch loads and its save) and the chain unit
// (build_h_chain.cpp, the store) tell each other. Plain integers only.
#ifndef OP1F_BUILD_I_PATCH_H
#define OP1F_BUILD_I_PATCH_H

// A sound slot's integers as the module's file user/op1f.json holds them for a slot: the chain's 48, the two pairs
// of the LFO's effect wagons, then the tag 98 and the LFO unit's 50 (103 in all today). Room for more.
enum { OP1F_PATCH_INTS = 160 };

// The patch unit, asked by the chain unit on the GUI thread. A new instrument was put on the slot since the last
// call: the count of integers its file carried for the module (0: none, an instrument as stock alone knows it),
// copied to out. -1: nothing new. Taken once.
extern "C" int op1f_patch_pending(int host, int slot, int* out) asm("op1f_patch_pending");
// Once a frame, on the GUI thread: an instrument whose record stock did not apply within half a second is taken
// from here; the counters' line.
extern "C" void op1f_patch_frame() asm("op1f_patch_frame");

// The chain unit, asked by the patch unit on the GUI thread. The slot's integers for an instrument's file; 0 for a
// slot the module does not know or one that is as stock alone has it (its one effect on the blue wagon, its one LFO).
extern "C" int op1f_chain_slot_values(int host, int slot, int* out) asm("op1f_chain_slot_values");
// The slot's record against its chain, once more (it takes what op1f_patch_pending holds for the slot).
extern "C" void op1f_chain_instrument(int host, int slot) asm("op1f_chain_instrument");

#endif

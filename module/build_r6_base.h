// Made for OS 1.7.3 from this source's OS 1.7.0 form; every address the code uses is 1.7.3's; an address in a comment that the code does not use may still be 1.7.0's.
// Step R6: what the units of the base share among themselves at level 9 (synth engines). Nothing here is offered to a
// pack: these are names between the base's own units, fixed in one place so that each unit can be written and tested
// by itself. It adds to module/build_r5_base.h (level 8's), which stands as it is, and includes it.
//
// The unit of level 9 and its files:
//   the engines unit   base 29's engines unit, its stubs module/build_r6_engines.s: the registry of engines, the
//                      check of the asks of kind OP1F_ASK_ENGINE, the rows of the unit's engine list, the pick, the
//                      rides, the six voice objects of the base's own that stand in the synth's six words while the
//                      sound that plays rides an engine, the render of a pack's engine on the audio thread, the mapping
//                      of knob words and values, the engines' working memory, the engines' own sounds written into the
//                      unit's sound list, and the mark op1f_level_9
// What level 9 needs of level 8's units (their siblings, the cards, sequencer, effects and
// values units of base 29's first build, each base 28's with what is named here): the catalog's read and write call the engines unit beside the effects unit; level 8's helpers hand
// an ask of kind OP1F_ASK_ENGINE to the engines unit; the knob names' entry (W4) hands kind 0 to it; the screen stub
// gains the engines' stand-in entry; the cards unit knows kind 8 for a card of level 9, calls the engines unit's frame
// work and asks it for a standing; the values unit keeps the base's own member op1f_engine and tells the engines unit
// at a sound's assign.
//
// The places of the firmware level 9 stands at (OS 1.7.0; 1.7.3 beside), named here once for the table and the
// profile (each read in both images: the same eight bytes, whole instructions, none relative to its own place):
//   E1  0x8f0112e8 (0x8f0112e8)  the head of the engine factory (type, the six words, a third word), its 8 bytes: a
//                                jump to op1f_engine_factory_entry, the trampoline in the room op1f_engine_factory_room,
//                                which the entry calls, so that the factory returns into it
//   E2  0x8d4eb3e8 (0x8d4eb3e8)  the head of the engine pick (catalog id, slot), its 8 bytes: a jump to
//                                op1f_engine_pick_entry, the room op1f_engine_pick_room

#ifndef OP1F_BUILD_R6_BASE_H
#define OP1F_BUILD_R6_BASE_H

#include "build_r6_card.h"
#include "build_r5_base.h"

// ---- the words the stubs read ----

// Not 0 while level 9 has work: an engine is registered, or a synth sound names one (registered or away), or the base's
// voice objects stand in the six words. 0 is level 9 at rest: E1 and E2 then go the firmware's own way at once.
extern "C" volatile unsigned op1f_engines_awake OP1F_NAMED("op1f_engines_awake");

// ---- from the engines unit, for the catalog (the sequencer unit's sibling) ----

// At the catalog's read, on the thread of the start, after the sequencers' and the effects' rows: the first call
// checks every ask of kind OP1F_ASK_ENGINE of the cards in use (no line: the lines come at the first frame) and
// registers those in order, ids 32 and on; every call adds a row to `synth_types` for each registered engine that has
// none of its name. Nothing when no engine is registered.
extern "C" void op1f_engines_catalog_read(void* document) OP1F_NAMED("op1f_engines_catalog_read");

// How many engines are registered: 0 before the catalog's read and when none is. One word, from any thread.
extern "C" volatile unsigned op1f_engines_registered OP1F_NAMED("op1f_engines_registered");

// 1 when an element of `synth_types` is a row the engines unit added (its `type` the name of a registered engine), else
// 0: the catalog's write leaves such rows out of the copy it writes (as op1f_effects_row_is for `fx_types`).
extern "C" int op1f_engines_row_is(const void* row) OP1F_NAMED("op1f_engines_row_is");

// ---- from the engines unit, for the cards unit ----

// Once a frame on the display thread, after op1f_keeps_frame and op1f_effects_frame and before the cards' frame
// functions: the engines unit's frame work. With level 9 at rest it tests one word and returns.
extern "C" void op1f_engines_frame() OP1F_NAMED("op1f_engines_frame");

// Does an engine's page stand in now? 1 and `out` filled while the unit is in the synth's mode, the screen the current
// controller asked for (op1f_screen_asked) is the engine page of the unit's own engine the packs' engines stand on
// (T1), and the synth sound the owner sees rides an engine or names one that is away: `out->where` is the slot,
// `out->how` OP1F_SHOWN_UP with the base's bits below; `*mode` and `*screen` the mode and the screen the stand-in
// takes; `*name` the engine's name, for the base's blank page. For an engine that is away `out->slot` is
// OP1F_STAND_NO_CARD (no pack's page can stand in: the base's blank page says the words of OP1F_STAND_AWAY). 0
// otherwise. It says no line and calls nothing of a pack's. The cards unit asks it after op1f_effects_standing.
extern "C" int op1f_engines_standing(Op1fStanding* out, unsigned* mode, unsigned* screen, const char** name) OP1F_NAMED("op1f_engines_standing");
// As op1f_engines_standing, whatever screen the controller asked for: the cards unit keeps the engine's page ready under
// 77 with its entry written while the sound the owner sees rides an engine, so that T1 answers the page at once (level
// 8's op1f_effects_ahead, for the engines).
extern "C" int op1f_engines_ahead(Op1fStanding* out, unsigned* mode, unsigned* screen, const char** name) OP1F_NAMED("op1f_engines_ahead");
// The base's bits of a standing's `how` for an engine, beside level 8's OP1F_STAND_NO_ROOM ("no room") and
// OP1F_STAND_OVER ("switched off"): the sound names an engine whose pack is not on the unit; the blank page shows
// "'<name>' is not on this unit".
enum { OP1F_STAND_AWAY = 0x400 };
static const unsigned OP1F_STAND_NO_CARD = 0xffffffffu;

// The engines' entry of level 8's table of stand-in screens (op1f_stand_screens, module/build_r5_base.h: a table of
// four pairs {mode + 1, screen}, entries 2 and 3 left for later levels): entry 2. Written by the engines unit at a pick
// of an engine (the screen before the mode, under op1f_mask) so that the request the list's close makes already answers
// 77, and cleared at a pick of another engine; at every other moment it is the cards unit's to write and clear, as
// level 8's entries are, from op1f_engines_standing.
enum { OP1F_STAND_ENGINE = 2 };

// ---- from the engines unit, for level 8's helpers ----

// For an ask of kind OP1F_ASK_ENGINE of the card at `card_slot` (op1f_cards' slot): what op1f_param_get,
// op1f_param_set and op1f_riding answer, `place` a synth slot. The helpers check the card and the thread first (as for
// kind 7) and call these; each answers OP1F_ENGINE_NOT_MINE when that ask is no registered engine, and the helper goes
// on as it does for kind 7.
enum { OP1F_ENGINE_NOT_MINE = 1 };
extern "C" int op1f_engines_param_get(unsigned card_slot, unsigned ask, unsigned place, unsigned index, int* out) OP1F_NAMED("op1f_engines_param_get");
extern "C" int op1f_engines_param_set(unsigned card_slot, unsigned ask, unsigned place, unsigned index, int value) OP1F_NAMED("op1f_engines_param_set");
extern "C" int op1f_engines_riding(unsigned card_slot, unsigned ask, unsigned* bits) OP1F_NAMED("op1f_engines_riding");

// ---- from the engines unit, for the knob names' entry (level 8's W4, base 28's effects unit) ----

// The pack's name of value `index` (as the synth's LFO page asks it) for the synth slot `slot` (the one the owner sees)
// when it rides a registered engine with that many values, else 0 (the firmware's own function then answers). Level
// 8's W4 asks it, as a weak name, for the engines' kind on the synth's LFO page. It reads words only.
extern "C" const char* op1f_engines_names(unsigned slot, int index) OP1F_NAMED("op1f_engines_names");

// ---- between the engines unit and the values unit's sibling (level 7) ----

// At a sound put on a synth slot's record (the place F4, op1f_keeps_assign), on the firmware's background worker,
// BEFORE the firmware's own assign: `slot` 0 to 7; `how` OP1F_KEPT_LOADED (a sound loaded from a file) or
// OP1F_KEPT_ENGINE (the engine job: the engine list or MIDI picked an engine); `json` the document about to be
// assigned (for the engine job its `name` is set here to the engine's when the slot rides one); `name` the loaded
// file's member op1f_engine as a name, zero ended (taken by the values unit at the file's read, F1), or 0 when the
// file holds none. With no engine registered and no sound naming one, and no name handed, it returns at once.
extern "C" void op1f_engines_assign(unsigned slot, unsigned how, void* json, const char* name) OP1F_NAMED("op1f_engines_assign");

// The base's own member op1f_engine, kept as a pack's values are (level 7): at the top of a synth sound's preset (read
// where the unit reads it, written where it writes it) and for each synth slot in the base's store. The text is the
// member's JSON text (a JSON string of the engine's name). Get: the bytes written into `out` (ended with a zero), or
// OP1F_NO_FILE (no member), OP1F_TOO_LARGE. Set: `text` 0 takes the member away; answers 0, OP1F_BAD_VALUE,
// OP1F_TOO_LARGE, OP1F_NOT_NOW (asked again at the next frame). Both from the display thread only.
extern "C" int op1f_keeps_engine_get(unsigned slot, char* out, unsigned room) OP1F_NAMED("op1f_keeps_engine_get");
extern "C" int op1f_keeps_engine_set(unsigned slot, const char* text) OP1F_NAMED("op1f_keeps_engine_set");

// What the values unit tells of the member, as it tells a pack's `changed` (OP1F_KEPT_START, OP1F_KEPT_LOADED,
// OP1F_KEPT_DROPPED for `where` OP1F_KEEP_SYNTH): on the display thread, in op1f_keeps_frame.
extern "C" void op1f_engines_changed(unsigned where, unsigned which, unsigned how) OP1F_NAMED("op1f_engines_changed");

#endif

/**
 *    Name:        state.h
 *    Description: a save state: the whole game's state in a file, written and read back by the parts that keep it
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Each part of the game visits what it keeps (simpsons_state_part, then its bytes) the same way to save it and to
 *    load it: the board (simpsons_machine_state: everything simpsons_state_hash covers and the rest of
 *    simpsons_board, the sound board's ports, the EEPROM, the board's time and frame), the 053248 program's runtime
 *    (simpsons_program_state: the interrupt mask and the values the program carries in its registers, game_context),
 *    the video (simpsons_video_state: the chips as the video last took them and its caches), the sound board
 *    (simpsons_sound_state: the Z80's RAM and time, the YM2151, the K053260, the samples made ahead of the board, the
 *    sound program's own variables), and a port's own (game/state.h). A part's pointers are its own run's, never the
 *    file's: a part puts back the ones it has after a load.
 *
 *    The file (little endian): "SMST", u32 the format, u32 0x01020304 in the host's byte order (the parts are the
 *    host's memory), u32 the game's version (game/assets.h's game_assets_version: the program's tables and data, as the
 *    assets pack has it), u32 the layout (FNV-1a of every part's name and of every block's size, in the order they are
 *    visited: a build whose parts differ has another), u64 the frame (the board's), then the parts' bytes. A file of
 *    another format, byte order, version or layout is refused, and so is one whose bytes do not end where the parts
 *    do; nothing is changed before the whole file has been checked.
 */
#ifndef SIMPSONS_STATE_H
#define SIMPSONS_STATE_H

#include "simpsons/script.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct simpsons_state simpsons_state;

/* What a save or a load came to */
typedef enum
{
	SIMPSONS_STATE_DONE,
	SIMPSONS_STATE_NOT_HERE,          /* the game is where no state can be taken or put back (game/state.h) */
	SIMPSONS_STATE_NONE,              /* no file to load */
	SIMPSONS_STATE_NOT_A_STATE,       /* a file that is no saved state */
	SIMPSONS_STATE_OTHER_VERSION,     /* a state of another format, byte order, version or layout of the game */
	SIMPSONS_STATE_DAMAGED,           /* its bytes do not end where its parts do */
	SIMPSONS_STATE_NOT_WRITTEN,       /* the file could not be written (or no memory for it) */
	SIMPSONS_STATE_RESULTS
} simpsons_state_result;

/* The result in words, for a log */
const char *simpsons_state_problem(simpsons_state_result result);

/* A part begins: its name goes in the layout */
void simpsons_state_part(simpsons_state *state, const char *name);

/* 'size' bytes at 'data': saved, or loaded over them (the size goes in the layout) */
void simpsons_state_bytes(simpsons_state *state, void *data, size_t size);

/* A count of what follows, elements of 'element' bytes: saved, or loaded into 'count' (no larger than what the file
   has left for them); its value is no layout's. Then the elements themselves, simpsons_state_more */
void simpsons_state_count(simpsons_state *state, uint64_t *count, size_t element);
void simpsons_state_more(simpsons_state *state, void *data, size_t size);

/* Whether the visit is a load (a part then puts back its pointers and what follows from what it loaded) */
bool simpsons_state_loading(const simpsons_state *state);

/* The state each of 'visit''s parts visits, into 'path' (a file of its own, written whole and then put in the old
   one's place) or back from it; 'version' is the game's. The frame is the board's */
simpsons_state_result simpsons_state_write(const char *path, uint32_t version, void (*visit)(simpsons_state *state));
simpsons_state_result simpsons_state_read(const char *path, uint32_t version, void (*visit)(simpsons_state *state));

/* ---- the parts ---- */

/* The board: simpsons_board (state.c) */
void simpsons_machine_state(simpsons_state *state);
/* The video's chips as it took them, and its caches (video.c) */
void simpsons_video_state(simpsons_state *state);
/* The sound board: the Z80, its chips, its program's variables (sound_board.c, sound_cpu.c) */
void simpsons_sound_state(simpsons_state *state);
/* The decompiled 053248 program's runtime: the mask, the interrupts taken, game_context (game_program.c) */
void simpsons_program_state(simpsons_state *state);
/* A control script's place in itself (script.c; NULL: none, the same bytes), for a check's runs */
void simpsons_script_state(simpsons_script *script, simpsons_state *state);

#ifdef __cplusplus
}
#endif

#endif

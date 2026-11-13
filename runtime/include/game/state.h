/**
 *    Name:        state.h
 *    Description: the game's save states: the whole game's state into a file and back, where the program waits for
 *                 its next frame in its main loop
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The parts (simpsons/state.h): the board, the 053248 program's runtime, the video, the sound board, then the
 *    port's own ('more': The Simpsons' console, the picture it showed). The game's version is the assets pack's
 *    (game_assets_version).
 *
 *    Where: the decompiled program keeps its control flow on the C stack, and the port has the board only where the
 *    program calls it, at each frame (program_frame, from frame_wait). The program waits there in three places: the
 *    boot's two (SIMPSONS_BOOT_WAIT, the sprite DMA's FIRQ at power on; SIMPSONS_SWITCH_WAIT, the test switch held
 *    from power on), each with the boot's call on the C stack, and the main loop's (SIMPSONS_FRAME_WAIT, from the
 *    boot's end on, whatever the game is doing: the attract mode, a stage, a boss, a bonus stage, the continue, a story,
 *    the name entry, the test mode). Every phase of the game runs in the interrupts' handlers and the main loop's jobs,
 *    which the program takes after the wait and which have returned before it waits again, so at the main loop's wait
 *    the C stack is always the same: the boot's call, the loop, the wait, the port's frame; and the loop's only value
 *    past the wait is the job it read, none (the wait is the empty queue's). A state is taken and put back there alone
 *    (game_state_possible): anywhere after the boot.
 */
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "simpsons/state.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Whether a state can be taken or put back now: the program waits for its next frame in its main loop */
bool game_state_possible(void);

/* The game's state, with the port's part 'more' visits (NULL: none), into 'path' or back from it */
simpsons_state_result game_state_save(const char *path, void (*more)(simpsons_state *state));
simpsons_state_result game_state_load(const char *path, void (*more)(simpsons_state *state));

#ifdef __cplusplus
}
#endif

#endif

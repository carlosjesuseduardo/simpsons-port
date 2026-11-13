/**
 *    Name:        game_state.c
 *    Description: the game's save states: its parts, where they can be taken, and its version
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Where a state can be taken and why there, in game/state.h. What each part keeps, in simpsons/state.h.
 */
#include "game/state.h"

#include "game/assets.h"
#include "simpsons/frame.h"
#include "simpsons/program.h"

static void (*g_more)(simpsons_state *state);

static void visit(simpsons_state *state)
{
	simpsons_machine_state(state);
	simpsons_program_state(state);
	simpsons_video_state(state);
	simpsons_sound_state(state);
	if (g_more)
		g_more(state);
}

bool game_state_possible(void)
{
	return program_waiting_at() == SIMPSONS_FRAME_WAIT;
}

simpsons_state_result game_state_save(const char *path, void (*more)(simpsons_state *state))
{
	if (!game_state_possible())
		return SIMPSONS_STATE_NOT_HERE;
	g_more = more;
	return simpsons_state_write(path, game_assets_version(), visit);
}

simpsons_state_result game_state_load(const char *path, void (*more)(simpsons_state *state))
{
	if (!game_state_possible())
		return SIMPSONS_STATE_NOT_HERE;
	g_more = more;
	return simpsons_state_read(path, game_assets_version(), visit);
}

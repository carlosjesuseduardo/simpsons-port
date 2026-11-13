/**
 *    Name:        rom_pack.c
 *    Description: the makers' assets pack (simpsons.assets) out of the player's ROM set, as the computer's
 *                 first run makes it
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The first run's steps, the game's own (ports/sdl2/first_run.c's pack_made): the ROM set read once through the
 *    assets' layout (game_load_rom_set: only what the assets give, no code) and written as the pack the devices'
 *    builds read (game_assets_pack_write, runtime/src/assets_pack.c). As tmnt-port's, with no folder between the set
 *    and the pack: the set's load is the pack's. The game loads its assets once, so a tool does too: the ROM set it
 *    took first is the one it writes packs of.
 */
#include "rom_pack.h"

#include "game/assets.h"
#include "game/game.h"

#include <stdio.h>
#include <string.h>

#define PATH_SIZE     1200

static char loaded_from[PATH_SIZE];    /* the ROM set whose assets the tool holds, or "" */
static int loaded_whole;               /* ... and they loaded */

const char *rom_pack_make(const char *rom, const char *pack)
{
	const char *problem;

	if (loaded_from[0])
	{
		if (strcmp(loaded_from, rom) != 0)
			return "the tool holds another ROM set's assets";
		return loaded_whole ? game_assets_pack_write(pack) : "the ROM set's assets cannot be loaded";
	}
	snprintf(loaded_from, sizeof(loaded_from), "%s", rom);    /* (loaded or not: the game's loader runs once) */
	if ((problem = game_load_rom_set(rom)) != NULL)
		return problem;
	loaded_whole = 1;
	return game_assets_pack_write(pack);
}

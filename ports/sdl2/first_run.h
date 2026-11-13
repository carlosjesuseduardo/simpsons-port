/**
 *    Name:        first_run.h
 *    Description: the first run: the game's assets pack out of the player's ROM set (first_run.c)
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    As tmnt-port's first_run.h.
 */
#ifndef SIMPSONS_SDL2_FIRST_RUN_H
#define SIMPSONS_SDL2_FIRST_RUN_H

#include <stddef.h>

/* On macOS the folder the .app is in (with its separator), where 'base' (SDL_GetBasePath) is its Resources: where a
   player puts the ROM set or the pack beside the game, as beside an executable elsewhere; the folder the player put
   it in when macOS runs it from a copy of its own (App Translocation). 0 elsewhere */
int app_folder(const char *base, char *folder, size_t size);

/* The game's assets pack written to 'pack' (simpsons.assets in the folder of what the game keeps) from the player's
   ROM set: simpsons.zip or a folder rom/ beside the executable ('base', with its separator: the window's letters are
   there too), in 'beside' (the folder the .app is in on macOS; NULL elsewhere) or in the folder of what the game
   keeps ('saves'); else one dropped on a window that asks for it, with 'title' on it. 'stale': what is wrong with
   the pack the game found (another version's: the window says so when there is no ROM set to make it again), or
   NULL. 1 when the pack is written, 0 when the window is closed first */
int first_run(const char *title, const char *base, const char *beside, const char *saves, const char *pack,
	const char *stale);

#endif

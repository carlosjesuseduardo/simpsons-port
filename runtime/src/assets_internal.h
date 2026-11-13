/**
 *    Name:        assets_internal.h
 *    Description: what the assets' loader (assets.c) shares with the pack's (assets_pack.c): a load given span by
 *                 span, as the folder's gives its pieces
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    A load begins (the memories cleared, the programs' pieces and the spans forgotten), gives its bytes a span at a
 *    time (a program's span kept as its piece, the others' into their memories), and ends (the pieces in the order
 *    of their places, the memories' spans merged): the folder's load, the pack's and the ROM set's all go this way,
 *    so they leave the same state when they give the same bytes at the same places.
 */
#ifndef ASSETS_INTERNAL_H
#define ASSETS_INTERNAL_H

#include "game/assets.h"

/* A load begins into 'into': NULL, or what is wrong with it (a memory missing) */
const char *assets_begin(const game_assets_memory into[GAME_ASSETS_REGIONS]);

/* 'length' bytes at 'place' of a region: 1, or 0 (assets_problem says why) */
int assets_give(const game_assets_memory into[GAME_ASSETS_REGIONS], game_assets_region region, uint32_t place,
	const uint8_t *data, size_t length);

/* The load is over: NULL, or what is wrong with what it gave (two pieces at one place) */
const char *assets_end(void);

/* What the last thing that failed said */
const char *assets_problem(void);

/* The bytes of the last load's span 'index' of a region (game_assets_loaded_spans' order): a program's piece, or
   where the load put them in its memory */
const uint8_t *assets_span_bytes(game_assets_region region, size_t index);

#endif

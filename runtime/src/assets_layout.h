/**
 *    Name:        assets_layout.h
 *    Description: the assets' layout (assets_layout.c, which simpsons_assets_check --write-layout writes): where
 *                 the bytes the assets give are, region by region, without a byte of the ROM
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef ASSETS_LAYOUT_H
#define ASSETS_LAYOUT_H

#include "game/assets.h"

#include <stdint.h>

/* A region's spans, as game_assets_loaded_spans gives them after a load of the assets folder */
typedef struct
{
	const game_assets_span *spans;
	uint32_t count;
} assets_layout_region;

extern const assets_layout_region assets_layout[GAME_ASSETS_REGIONS];

#endif

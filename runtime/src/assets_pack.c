/**
 *    Name:        assets_pack.c
 *    Description: the game's assets as one file, the pack (simpsons.assets): written from what a load gave and
 *                 loaded in its place; and the same load straight from the player's ROM set, through the assets'
 *                 layout (game/assets.h)
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    For the devices, which take no assets folder (the Switch's romfs, Android's APK, the Anbernic's folder): the
 *    pack is made on the player's computer, from the player's ROM set or assets, and never shared. Format (little
 *    endian, its parts 4-byte aligned): "SMAP", u32 the pack's format, u32 the version (game_assets_version: the
 *    layout's spans, and the names and places of the tables both programs' C reads by name: a build of another
 *    layout reads other places), then each region in game_assets_region's order: u32 the count of its spans, each
 *    span's place and length (u32 each), and the spans' bytes one after another (then zeros to the next 4). The
 *    spans are the layout's (assets_layout.c): the given stretches of the four memories (the rest of each is 0) and
 *    the programs' data pieces, by the places the programs know them by. A pack is loaded a span at a time as the
 *    folder's pieces are (assets_internal.h), so it leaves the state the folder's load left.
 *    The ROM set's load reads the set (game/extract.h) into game/data.h's regions, makes from them the pack of the
 *    layout's spans in memory and loads it: the state the folder's load leaves, as the assets give the ROM's bytes
 *    (simpsons_assets_check, ports/headless/assets_check.c, compares the two packs byte for byte).
 */
#include "game/assets.h"

#include "assets_internal.h"
#include "assets_layout.h"
#include "game/data.h"
#include "game/extract.h"
#ifndef SIMPSONS_TRANSLATION_SCAFFOLD    /* (the translation scaffold reads the program's data by address alone) */
#include "game/support.h"
/* (the two programs' supports name some of their macros alike: none of them is used here) */
#undef AS8
#undef AS16
#undef AT8
#undef AT16
#undef BOARD_ORIGINAL
#undef IN_PAGE
#undef TABLE8
#undef TABLE16
#endif
#include "sound/support.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACK_MAGIC      "SMAP"
#define PACK_FORMAT     1u
#define PACK_HEADER     12u
#define PROBLEM_SIZE    256

static char g_problem[PROBLEM_SIZE];

/* the ROM set's regions (game/data.h), by game_assets_region */
static uint8_t *const ROM_REGIONS[GAME_ASSETS_REGIONS] = {
	game_program, game_sound, game_tiles, game_sprites, game_samples, game_eeprom };

static size_t aligned(size_t size)
{
	return size + (4u - size % 4u) % 4u;
}

static void put32(uint8_t *at, uint32_t value)
{
	at[0] = (uint8_t)value;
	at[1] = (uint8_t)(value >> 8);
	at[2] = (uint8_t)(value >> 16);
	at[3] = (uint8_t)(value >> 24);
}

static uint32_t get32(const uint8_t *at)
{
	return at[0] | (uint32_t)at[1] << 8 | (uint32_t)at[2] << 16 | (uint32_t)at[3] << 24;
}

/* ---- the version ---- */

static uint32_t fnv(uint32_t hash, const void *bytes, size_t size)
{
	const uint8_t *byte = bytes;

	for (size_t index = 0; index < size; index++)
		hash = (hash ^ byte[index]) * 16777619u;
	return hash;
}

static uint32_t fnv32(uint32_t hash, uint32_t value)
{
	uint8_t bytes[4];

	put32(bytes, value);
	return fnv(hash, bytes, 4);
}

size_t game_assets_layout_spans(game_assets_region region, const game_assets_span **spans)
{
	if (region >= GAME_ASSETS_REGIONS)
	{
		*spans = NULL;
		return 0;
	}
	*spans = assets_layout[region].spans;
	return assets_layout[region].count;
}

/* The layout's spans, region by region, then the names and places of the tables both programs' C reads by name
   (FNV-1a) */
uint32_t game_assets_version(void)
{
	static uint32_t version;
	static int known;
	uint32_t hash = 2166136261u;

	if (known)
		return version;
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		hash = fnv32(hash, assets_layout[region].count);
		for (uint32_t index = 0; index < assets_layout[region].count; index++)
			hash = fnv32(fnv32(hash, assets_layout[region].spans[index].place), assets_layout[region].spans[index].length);
	}
#ifndef SIMPSONS_TRANSLATION_SCAFFOLD
	for (size_t index = 0; index < game_table_count; index++)
		hash = fnv32(fnv(hash, game_tables[index].name, strlen(game_tables[index].name) + 1), game_tables[index].place);
#endif
	for (size_t index = 0; index < sound_table_count; index++)
		hash = fnv32(fnv(hash, sound_tables[index].name, strlen(sound_tables[index].name) + 1), sound_tables[index].address);
	version = hash;
	known = 1;
	return version;
}

/* ---- the pack written ---- */

/* the bytes of a span of the layout ('index' in its region's list), or NULL */
typedef const uint8_t *(*span_bytes)(game_assets_region region, size_t index, const game_assets_span *span);

static const uint8_t *loaded_bytes(game_assets_region region, size_t index, const game_assets_span *span)
{
	(void)span;
	return assets_span_bytes(region, index);
}

static const uint8_t *rom_bytes(game_assets_region region, size_t index, const game_assets_span *span)
{
	(void)index;
	return ROM_REGIONS[region] + span->place;
}

/* The pack of the layout's spans, their bytes from 'bytes_of', in memory (calloc's: its padding is 0): NULL, or
   g_problem says why */
static uint8_t *make_pack(span_bytes bytes_of, size_t *size)
{
	size_t total = PACK_HEADER, at = PACK_HEADER;
	uint8_t *pack;

	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		size_t bytes = 0;

		for (uint32_t index = 0; index < assets_layout[region].count; index++)
		{
			const game_assets_span *span = &assets_layout[region].spans[index];

			if (span->place > game_assets_size(region) || span->length > game_assets_size(region) - span->place)
			{
				snprintf(g_problem, sizeof(g_problem), "the assets' layout is past the %s region's end (%X)",
					game_assets_name(region), (unsigned)span->place);
				return NULL;
			}
			bytes += span->length;
		}
		total += 4u + 8u * assets_layout[region].count + aligned(bytes);
	}
	if (!(pack = calloc(total, 1)))
	{
		snprintf(g_problem, sizeof(g_problem), "no memory for the assets pack");
		return NULL;
	}
	memcpy(pack, PACK_MAGIC, 4);
	put32(pack + 4, PACK_FORMAT);
	put32(pack + 8, game_assets_version());
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const assets_layout_region *layout = &assets_layout[region];

		put32(pack + at, layout->count);
		at += 4;
		for (uint32_t index = 0; index < layout->count; index++, at += 8)
		{
			put32(pack + at, layout->spans[index].place);
			put32(pack + at + 4, layout->spans[index].length);
		}
		for (uint32_t index = 0; index < layout->count; index++)
		{
			const uint8_t *bytes = bytes_of((game_assets_region)region, index, &layout->spans[index]);

			if (!bytes)
			{
				snprintf(g_problem, sizeof(g_problem), "the %s region's bytes at %X are not there to pack",
					game_assets_name(region), (unsigned)layout->spans[index].place);
				free(pack);
				return NULL;
			}
			memcpy(pack + at, bytes, layout->spans[index].length);
			at += layout->spans[index].length;
		}
		at = aligned(at);
	}
	*size = total;
	return pack;
}

/* Whether the last load gave the layout's spans: NULL, or what differs */
static const char *loaded_is_layout(void)
{
	const game_assets_span *spans;
	size_t count = 0;

	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
		count += game_assets_loaded_spans((game_assets_region)region, &spans);
	if (!count)
		return "no assets have been loaded to pack";
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		count = game_assets_loaded_spans((game_assets_region)region, &spans);
		if (count != assets_layout[region].count
			|| (count && memcmp(spans, assets_layout[region].spans, count * sizeof(game_assets_span))))
		{
			snprintf(g_problem, sizeof(g_problem), "the assets loaded do not have this build's layout (their %s "
				"region differs): runtime/src/assets_layout.c is to be written again from them "
				"(simpsons_assets_check --write-layout) and the game built again", game_assets_name(region));
			return g_problem;
		}
	}
	return NULL;
}

const char *game_assets_pack_write(const char *path)
{
	const char *problem = loaded_is_layout();
	uint8_t *pack;
	size_t size = 0;
	FILE *output;
	int written;

	if (problem)
		return problem;
	if (!(pack = make_pack(loaded_bytes, &size)))
		return g_problem;
	if (!(output = fopen(path, "wb")))
	{
		free(pack);
		return "the assets pack cannot be written";
	}
	written = fwrite(pack, 1, size, output) == size;
	free(pack);
	if (fclose(output) != 0 || !written)
	{
		remove(path);
		return "the assets pack cannot be written";
	}
	return NULL;
}

/* ---- the pack loaded ---- */

int game_assets_is_pack(const char *path)
{
	FILE *input = fopen(path, "rb");
	char magic[4];
	int is_pack;

	if (!input)
		return 0;
	is_pack = fread(magic, 1, 4, input) == 4 && !memcmp(magic, PACK_MAGIC, 4);
	fclose(input);
	return is_pack;
}

const char *game_assets_pack_load_bytes_into(const uint8_t *bytes, size_t size,
	const game_assets_memory into[GAME_ASSETS_REGIONS])
{
	const uint8_t *data[GAME_ASSETS_REGIONS];
	size_t at = PACK_HEADER;
	const char *problem;

	if (size < PACK_HEADER || memcmp(bytes, PACK_MAGIC, 4))
		return "the file is not an assets pack";
	if (get32(bytes + 4) != PACK_FORMAT)
	{
		snprintf(g_problem, sizeof(g_problem), "the assets pack is of another format (%u; this game reads %u): it is "
			"to be made again", (unsigned)get32(bytes + 4), PACK_FORMAT);
		return g_problem;
	}
	if (get32(bytes + 8) != game_assets_version())
	{
		snprintf(g_problem, sizeof(g_problem), "the assets pack is of another version of the game (%08X; this one "
			"reads %08X): it is to be made again from the ROM set", (unsigned)get32(bytes + 8),
			(unsigned)game_assets_version());
		return g_problem;
	}
	/* (all of it checked before anything is given) */
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const assets_layout_region *layout = &assets_layout[region];
		size_t length = 0;

		if (size - at < 4u || get32(bytes + at) != layout->count || (size - at - 4u) / 8u < layout->count)
			return "the assets pack does not hold this game's layout";
		at += 4;
		for (uint32_t index = 0; index < layout->count; index++, at += 8)
		{
			if (get32(bytes + at) != layout->spans[index].place || get32(bytes + at + 4) != layout->spans[index].length)
				return "the assets pack does not hold this game's layout";
			length += layout->spans[index].length;
		}
		if (size - at < aligned(length))
			return "the assets pack is cut short";
		data[region] = bytes + at;
		at += aligned(length);
	}
	if (at != size)
		return "the assets pack holds more than this game's layout";
	if ((problem = assets_begin(into)) != NULL)
		return problem;
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const uint8_t *from = data[region];

		for (uint32_t index = 0; index < assets_layout[region].count; index++)
		{
			const game_assets_span *span = &assets_layout[region].spans[index];

			if (!assets_give(into, (game_assets_region)region, span->place, from, span->length))
				return assets_problem();
			from += span->length;
		}
	}
	return assets_end();
}

const char *game_assets_pack_load_into(const char *path, const game_assets_memory into[GAME_ASSETS_REGIONS])
{
	FILE *input = fopen(path, "rb");
	uint8_t *bytes = NULL;
	const char *problem;
	long size = 0;

	if (!input)
		return "the assets pack cannot be read";
	if (fseek(input, 0, SEEK_END) != 0 || (size = ftell(input)) <= 0 || fseek(input, 0, SEEK_SET) != 0
		|| !(bytes = malloc((size_t)size)) || fread(bytes, 1, (size_t)size, input) != (size_t)size)
	{
		fclose(input);
		free(bytes);
		return "the assets pack cannot be read";
	}
	fclose(input);
	problem = game_assets_pack_load_bytes_into(bytes, (size_t)size, into);
	free(bytes);
	return problem;
}

/* ---- the ROM set through the layout ---- */

const char *game_assets_rom_set_load_into(const char *source, const game_assets_memory into[GAME_ASSETS_REGIONS])
{
	const char *problem = game_rom_set_load(source);
	uint8_t *pack;
	size_t size = 0;

	if (problem)
		return problem;
	pack = make_pack(rom_bytes, &size);
	/* (the set's regions keep no more than the assets give: the load clears and fills those it loads into, and
	   the others are cleared here) */
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		int loaded_into = 0;

		for (int other = 0; other < GAME_ASSETS_REGIONS; other++)
			loaded_into |= into[other].memory == ROM_REGIONS[region];
		if (!loaded_into)
			memset(ROM_REGIONS[region], 0, game_assets_size(region));
	}
	if (!pack)
		return g_problem;
	problem = game_assets_pack_load_bytes_into(pack, size, into);
	free(pack);
	return problem;
}

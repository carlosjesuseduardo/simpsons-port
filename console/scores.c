/**
 *    Name:        scores.c
 *    Description: the high scores kept: the game's table in a file of the console's, put back after the boot and
 *                 written when a new score comes in
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The cabinet keeps its high score table in the work RAM alone (high_scores, 4980: ten records of 8 bytes, best
 *    first: three initials, the score in four BCD digits, the character, two bytes the game leaves 0), which its boot
 *    fills from default_high_scores (07893E, BAT HOM LIS MAR MAG INC SIM MAT FOX ...) and loses at power-off: the
 *    EEPROM holds its test pattern and the settings alone. The Simpsons keeps the table in simpsons.scores, in the
 *    folder of what the game keeps (apart from simpsons.ini: the settings are OPTIONS' and written whole whenever they
 *    change, the table is the game's own memory, as The Simpsons Arcade Game's simpsons.nv is the EEPROM's, and a broken
 *    table leaves the settings alone). The file (little endian): "SMHS", u32 the format, the table's 80 bytes as the
 *    game keeps them, u32 FNV-1a of the format and the table; a file that is not that (or a table whose scores are no
 *    BCD, whose characters are none of the four, or not best first) is not taken, and the boot's table stays.
 *
 *    The kept table goes into the game's at the end of the first frame the program waits for in its main loop: the boot
 *    has set its table by then (boot: the queues, the table, the settings, then the loop), and nothing has read it (the
 *    console's presentation runs first). Only insert_high_score writes the table after the boot, at the name entry's
 *    end (name_entry_save: each player who made the table, in one frame): at the end of a frame in which the game's
 *    table has changed, the kept one takes what came in, and the file is written. A save state loaded (states.c) puts
 *    the state's table in the game, perhaps an older one than kept: the kept table goes back into the game once the
 *    game loaded is over (out of the game mode), and what comes into the state's table meanwhile goes into the kept
 *    one by its score, as high_score_rank places it (before the first entry whose score is no higher). The Simpsons
 *    Arcade Game keeps nothing, as the cabinet.
 */
#include "console_internal.h"

#include "game.h"
#include "simpsons/frame.h"
#include "simpsons/machine.h"
#include "simpsons/program.h"

#include <stdio.h>
#include <string.h>

#define SCORES_MAGIC        "SMHS"
#define SCORES_FORMAT       1u
#define ENTRIES             10u
#define ENTRY_SIZE          8u
#define TABLE_SIZE          (ENTRIES * ENTRY_SIZE)    /* high_scores[80] */
#define SCORE_AT            3u                        /* an entry's score, a big-endian word of four BCD digits */
#define CHARACTER_AT        5u
#define FILE_SIZE           (4u + 4u + TABLE_SIZE + 4u)

static char g_path[1100];
static bool g_kept_from_file;         /* the kept table is the file's */
static bool g_restored;               /* the boot is over: the kept table is in the game, or the boot's kept */
static bool g_state_loaded;           /* a save state put its table in the game: the kept one back once out of a game */
static uint8_t g_kept[TABLE_SIZE];    /* the table as kept */
static uint8_t g_seen[TABLE_SIZE];    /* the game's table at the last frame's end */

static uint32_t fnv(uint32_t hash, const uint8_t *bytes, size_t size)
{
	for (size_t index = 0; index < size; index++)
		hash = (hash ^ bytes[index]) * 16777619u;
	return hash;
}

static void put32(uint8_t *at, uint32_t value)
{
	for (int index = 0; index < 4; index++)
		at[index] = (uint8_t)(value >> (8 * index));
}

static uint32_t get32(const uint8_t *at)
{
	return (uint32_t)at[0] | (uint32_t)at[1] << 8 | (uint32_t)at[2] << 16 | (uint32_t)at[3] << 24;
}

static unsigned score_of(const uint8_t *entry)
{
	return (unsigned)entry[SCORE_AT] << 8 | entry[SCORE_AT + 1u];
}

/* A table as the game keeps one: BCD scores best first, characters of the four */
static bool table_valid(const uint8_t *table)
{
	for (unsigned entry = 0; entry < ENTRIES; entry++)
	{
		const uint8_t *record = &table[entry * ENTRY_SIZE];

		for (unsigned digit = 0; digit < 4u; digit++)
			if (((record[SCORE_AT + digit / 2u] >> (digit % 2u ? 0 : 4)) & 0x0Fu) > 9u)
				return false;
		if (record[CHARACTER_AT] >= CHARACTERS
			|| (entry && score_of(record) > score_of(&table[(entry - 1u) * ENTRY_SIZE])))
			return false;
	}
	return true;
}

static void table_of_game(uint8_t *table)
{
	for (unsigned offset = 0; offset < TABLE_SIZE; offset++)
		table[offset] = *simpsons_work_ram_byte((uint16_t)(high_scores_ADDRESS + offset));
}

static void into_game(const uint8_t *table)
{
	for (unsigned offset = 0; offset < TABLE_SIZE; offset++)
		*simpsons_work_ram_byte((uint16_t)(high_scores_ADDRESS + offset)) = table[offset];
}

static void write_file(void)
{
	uint8_t bytes[FILE_SIZE];
	FILE *output;

	memcpy(bytes, SCORES_MAGIC, 4);
	put32(bytes + 4, SCORES_FORMAT);
	memcpy(bytes + 8, g_kept, TABLE_SIZE);
	put32(bytes + 8 + TABLE_SIZE, fnv(2166136261u, bytes + 4, 4u + TABLE_SIZE));
	if ((output = fopen(g_path, "wb")) == NULL || fwrite(bytes, 1, sizeof(bytes), output) != sizeof(bytes))
		fprintf(stderr, "High scores: %s cannot be written\n", g_path);
	else
		fprintf(stderr, "High scores: kept in %s\n", g_path);
	if (output)
		fclose(output);
}

void console_scores_open(const char *path)
{
	uint8_t bytes[FILE_SIZE + 1];
	FILE *input;
	size_t got;

	g_path[0] = 0;
	g_kept_from_file = g_restored = g_state_loaded = false;
	if (!path || !path[0])
		return;
	snprintf(g_path, sizeof(g_path), "%s", path);
	if ((input = fopen(g_path, "rb")) == NULL)
	{
		fprintf(stderr, "High scores: the game's own (no %s)\n", g_path);
		return;
	}
	got = fread(bytes, 1, sizeof(bytes), input);
	fclose(input);
	if (got != FILE_SIZE || memcmp(bytes, SCORES_MAGIC, 4) != 0 || get32(bytes + 4) != SCORES_FORMAT
		|| get32(bytes + 8 + TABLE_SIZE) != fnv(2166136261u, bytes + 4, 4u + TABLE_SIZE) || !table_valid(bytes + 8))
	{
		fprintf(stderr, "High scores: %s is no table of the game's, the game's own taken\n", g_path);
		return;
	}
	memcpy(g_kept, bytes + 8, TABLE_SIZE);
	g_kept_from_file = true;
	fprintf(stderr, "High scores: %s\n", g_path);
}

/* How many of 'table''s entries are 'record' */
static unsigned count_of(const uint8_t *table, const uint8_t *record)
{
	unsigned count = 0;

	for (unsigned entry = 0; entry < ENTRIES; entry++)
		count += memcmp(&table[entry * ENTRY_SIZE], record, ENTRY_SIZE) == 0;
	return count;
}

/* 'record' into the kept table by its score, as high_score_rank places it, the lower ones moved down */
static void keep_entry(const uint8_t *record)
{
	unsigned place = 0;

	while (place < ENTRIES && score_of(&g_kept[place * ENTRY_SIZE]) > score_of(record))
		place++;
	if (place == ENTRIES)
		return;
	memmove(&g_kept[(place + 1u) * ENTRY_SIZE], &g_kept[place * ENTRY_SIZE], (ENTRIES - 1u - place) * ENTRY_SIZE);
	memcpy(&g_kept[place * ENTRY_SIZE], record, ENTRY_SIZE);
}

void console_scores_state_loaded(void)
{
	if (g_path[0] && g_restored)
	{
		g_state_loaded = true;
		table_of_game(g_seen);    /* (the state's: what comes into it from now on is new) */
	}
}

/* A LAN game (lan.c): the boot's table on every device of it (the board reset), none of a device's own, nothing of
   it kept; once it is over and out of a game, the kept table in the game again */
static bool g_lan, g_lan_over;

void console_scores_lan(bool playing)
{
	if (playing)
		g_lan = true;
	else if (g_lan)
	{
		g_lan = false;
		g_lan_over = true;
	}
}

void console_scores_frame(void)
{
	uint8_t table[TABLE_SIZE];

	if (!g_path[0] || g_lan)
		return;
	if (g_lan_over)
	{
		if (game_mode == GAME_MODE_GAME || program_waiting_at() != SIMPSONS_FRAME_WAIT)
			return;
		if (g_restored)
		{
			into_game(g_kept);
			memcpy(g_seen, g_kept, TABLE_SIZE);
		}
		g_lan_over = false;
		return;
	}
	if (!g_restored)
	{
		if (program_waiting_at() != SIMPSONS_FRAME_WAIT)
			return;    /* (the boot: its table to come) */
		if (g_kept_from_file)
			into_game(g_kept);
		else
			table_of_game(g_kept);
		memcpy(g_seen, g_kept, TABLE_SIZE);
		g_restored = true;
		return;
	}
	table_of_game(table);
	if (memcmp(table, g_seen, TABLE_SIZE) != 0)
	{
		/* (scores came in: the game's table is the kept one with them, or, after a save state, they go into it) */
		if (!g_state_loaded)
			memcpy(g_kept, table, TABLE_SIZE);
		else
			for (unsigned entry = 0; entry < ENTRIES; entry++)
			{
				const uint8_t *record = &table[entry * ENTRY_SIZE];
				unsigned earlier = 0;

				for (unsigned other = 0; other < entry; other++)
					earlier += memcmp(&table[other * ENTRY_SIZE], record, ENTRY_SIZE) == 0;
				if (earlier)
					continue;    /* (a record twice in it: its new ones counted at its first) */
				for (unsigned more = count_of(table, record); more > count_of(g_seen, record); more--)
					keep_entry(record);
			}
		memcpy(g_seen, table, TABLE_SIZE);
		write_file();
	}
	if (g_state_loaded && game_mode != GAME_MODE_GAME)
	{
		into_game(g_kept);
		memcpy(g_seen, g_kept, TABLE_SIZE);
		g_state_loaded = false;
	}
}

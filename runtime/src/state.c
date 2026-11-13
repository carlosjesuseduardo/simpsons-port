/**
 *    Name:        state.c
 *    Description: a save state's file, its parts' visit (saved, checked, loaded), and the board's part
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The parts are visited four ways, each the same walk: for the layout alone (no bytes: the names and sizes, the
 *    layout of this build's parts), to save (their bytes into memory, then the file), to check a file (its counts
 *    read, its bytes walked: they must end where the parts do), and to load (the bytes over the parts'). A file is
 *    read whole, its header and layout compared, then checked, and only then loaded: a refused file changes nothing.
 */
#include "simpsons/state.h"

#include "simpsons/machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATE_MAGIC        "SMST"
#define STATE_FORMAT       1u
#define BYTE_ORDER_MARK    0x01020304u
#define HEADER_SIZE        28u          /* the magic, the format, the byte order's mark, the version, the layout, the
                                           frame */
#define LAYOUT_START       2166136261u  /* (FNV-1a's) */
#define FILE_MOST          (64u << 20)  /* a file larger is no state of this game's */

enum
{
	VISIT_LAYOUT,
	VISIT_SAVE,
	VISIT_CHECK,
	VISIT_LOAD
};

struct simpsons_state
{
	int visit;
	uint32_t layout;
	uint8_t *bytes;           /* saving: grown as the parts come; checking and loading: the file's */
	size_t size, capacity;
	size_t at;                /* checking and loading: the next byte */
	bool failed;              /* saving: no memory; checking: the file ended before the parts did */
};

const char *simpsons_state_problem(simpsons_state_result result)
{
	static const char *const problems[SIMPSONS_STATE_RESULTS] = {
		"done",
		"the game is where no state can be taken or put back",
		"there is no saved state",
		"the file is no saved state",
		"the saved state is of another version of the game",
		"the saved state is damaged",
		"the saved state could not be written"
	};

	return result >= 0 && result < SIMPSONS_STATE_RESULTS ? problems[result] : "?";
}

/* ---- the visit ---- */

static uint32_t fnv(uint32_t hash, const void *bytes, size_t size)
{
	const uint8_t *byte = (const uint8_t *)bytes;

	for (size_t index = 0; index < size; index++)
		hash = (hash ^ byte[index]) * 16777619u;
	return hash;
}

static void layout_value(simpsons_state *state, uint64_t value)
{
	uint8_t bytes[8];

	for (int index = 0; index < 8; index++)
		bytes[index] = (uint8_t)(value >> (8 * index));
	state->layout = fnv(state->layout, bytes, sizeof(bytes));
}

static void put(simpsons_state *state, const void *data, size_t size)
{
	if (state->failed)
		return;
	if (size > state->capacity - state->size)
	{
		const size_t capacity = state->size + size + state->size / 2u + 0x10000u;
		uint8_t *bytes = (uint8_t *)realloc(state->bytes, capacity);

		if (!bytes)
		{
			state->failed = true;
			return;
		}
		state->bytes = bytes;
		state->capacity = capacity;
	}
	memcpy(state->bytes + state->size, data, size);
	state->size += size;
}

/* The file's next 'size' bytes into 'data' (NULL: walked over); false, and the visit failed, past its end */
static bool take(simpsons_state *state, void *data, size_t size)
{
	if (state->failed || size > state->size - state->at)
	{
		state->failed = true;
		return false;
	}
	if (data)
		memcpy(data, state->bytes + state->at, size);
	state->at += size;
	return true;
}

static void transfer(simpsons_state *state, void *data, size_t size)
{
	switch (state->visit)
	{
		case VISIT_SAVE:
			put(state, data, size);
			break;
		case VISIT_CHECK:
			take(state, NULL, size);
			break;
		case VISIT_LOAD:
			take(state, data, size);
			break;
		default:
			break;
	}
}

void simpsons_state_part(simpsons_state *state, const char *name)
{
	state->layout = fnv(state->layout, name, strlen(name) + 1u);
}

void simpsons_state_bytes(simpsons_state *state, void *data, size_t size)
{
	layout_value(state, size);
	transfer(state, data, size);
}

void simpsons_state_count(simpsons_state *state, uint64_t *count, size_t element)
{
	uint64_t value;

	layout_value(state, UINT64_MAX - element);    /* (a count of elements of that size: its value is none of the
	                                                 layout's) */
	switch (state->visit)
	{
		case VISIT_SAVE:
			put(state, count, sizeof(*count));
			break;
		case VISIT_CHECK:
		case VISIT_LOAD:
			if (!take(state, &value, sizeof(value)) || (element && value > (state->size - state->at) / element))
			{
				state->failed = true;
				value = 0;
			}
			*count = value;
			break;
		default:
			break;
	}
}

void simpsons_state_more(simpsons_state *state, void *data, size_t size)
{
	transfer(state, data, size);
}

bool simpsons_state_loading(const simpsons_state *state)
{
	return state->visit == VISIT_LOAD;
}

/* ---- the file ---- */

static void put32(uint8_t *at, uint32_t value)
{
	for (int index = 0; index < 4; index++)
		at[index] = (uint8_t)(value >> (8 * index));
}

static uint32_t get32(const uint8_t *at)
{
	return (uint32_t)at[0] | (uint32_t)at[1] << 8 | (uint32_t)at[2] << 16 | (uint32_t)at[3] << 24;
}

/* This build's layout: the parts' names and sizes, as 'visit' walks them */
static uint32_t layout_of(void (*visit)(simpsons_state *state))
{
	simpsons_state state = { VISIT_LAYOUT, LAYOUT_START, NULL, 0, 0, 0, false };

	visit(&state);
	return state.layout;
}

simpsons_state_result simpsons_state_write(const char *path, uint32_t version, void (*visit)(simpsons_state *state))
{
	simpsons_state state = { VISIT_SAVE, LAYOUT_START, NULL, 0, 0, 0, false };
	uint8_t header[HEADER_SIZE] = { 0 };
	char written[1200];
	FILE *output;
	bool good;

	if (!path || !path[0])
		return SIMPSONS_STATE_NOT_WRITTEN;
	put(&state, header, sizeof(header));
	visit(&state);
	if (state.failed)
	{
		free(state.bytes);
		return SIMPSONS_STATE_NOT_WRITTEN;
	}
	memcpy(state.bytes, STATE_MAGIC, 4);
	put32(state.bytes + 4, STATE_FORMAT);
	{
		const uint32_t mark = BYTE_ORDER_MARK;

		memcpy(state.bytes + 8, &mark, 4);    /* (in the host's order) */
	}
	put32(state.bytes + 12, version);
	put32(state.bytes + 16, state.layout);
	put32(state.bytes + 20, (uint32_t)simpsons_board.frame);
	put32(state.bytes + 24, (uint32_t)(simpsons_board.frame >> 32));
	/* (written whole beside it, then in its place: a write that fails leaves the state there was) */
	snprintf(written, sizeof(written), "%s.new", path);
	output = fopen(written, "wb");
	good = output && fwrite(state.bytes, 1, state.size, output) == state.size;
	if (output && fclose(output) != 0)
		good = false;
	free(state.bytes);
	if (!good)
	{
		remove(written);
		return SIMPSONS_STATE_NOT_WRITTEN;
	}
	remove(path);
	if (rename(written, path) != 0)
	{
		remove(written);
		return SIMPSONS_STATE_NOT_WRITTEN;
	}
	return SIMPSONS_STATE_DONE;
}

simpsons_state_result simpsons_state_read(const char *path, uint32_t version, void (*visit)(simpsons_state *state))
{
	simpsons_state state = { VISIT_CHECK, LAYOUT_START, NULL, 0, 0, HEADER_SIZE, false };
	FILE *input = path && path[0] ? fopen(path, "rb") : NULL;
	long length;
	uint32_t mark;

	if (!input)
		return SIMPSONS_STATE_NONE;
	if (fseek(input, 0, SEEK_END) != 0 || (length = ftell(input)) < 0 || fseek(input, 0, SEEK_SET) != 0)
	{
		fclose(input);
		return SIMPSONS_STATE_NOT_A_STATE;
	}
	if ((unsigned long)length < HEADER_SIZE || (unsigned long)length > FILE_MOST
		|| (state.bytes = (uint8_t *)malloc((size_t)length)) == NULL
		|| fread(state.bytes, 1, (size_t)length, input) != (size_t)length)
	{
		fclose(input);
		free(state.bytes);
		return SIMPSONS_STATE_NOT_A_STATE;
	}
	fclose(input);
	state.size = (size_t)length;
	memcpy(&mark, state.bytes + 8, 4);
	if (memcmp(state.bytes, STATE_MAGIC, 4) != 0)
	{
		free(state.bytes);
		return SIMPSONS_STATE_NOT_A_STATE;
	}
	if (get32(state.bytes + 4) != STATE_FORMAT || mark != BYTE_ORDER_MARK || get32(state.bytes + 12) != version
		|| get32(state.bytes + 16) != layout_of(visit))
	{
		free(state.bytes);
		return SIMPSONS_STATE_OTHER_VERSION;
	}
	/* (the whole file walked first, the parts' bytes to its end; then loaded) */
	visit(&state);
	if (state.failed || state.at != state.size)
	{
		free(state.bytes);
		return SIMPSONS_STATE_DAMAGED;
	}
	state.visit = VISIT_LOAD;
	state.at = HEADER_SIZE;
	state.layout = LAYOUT_START;
	visit(&state);
	free(state.bytes);
	return SIMPSONS_STATE_DONE;
}

/* ---- the board's part ---- */

/* Everything of simpsons_board's but its hooks, which are this run's: the memories and registers the state hash covers,
   the bank and the lines, the interrupts and the DMA's times, the ports between the two CPUs, the EEPROM (its cells,
   its serial interface's state and times), the inputs, the board's time and its frame */
void simpsons_machine_state(simpsons_state *state)
{
	static simpsons_machine kept;

	simpsons_state_part(state, "board");
	if (simpsons_state_loading(state))
		kept = simpsons_board;
	simpsons_state_bytes(state, &simpsons_board, sizeof(simpsons_board));
	if (simpsons_state_loading(state))
	{
		simpsons_board.sound_sync = kept.sound_sync;
		simpsons_board.sound_irq = kept.sound_irq;
		simpsons_board.sound_chip_read = kept.sound_chip_read;
		simpsons_board.sound_chip_write = kept.sound_chip_write;
		simpsons_board.sound_nmi_arm = kept.sound_nmi_arm;
		simpsons_board.now = kept.now;
		simpsons_board.write_watch = kept.write_watch;
		simpsons_board.program_at = kept.program_at;
		simpsons_board.frame_timing = kept.frame_timing;
	}
}

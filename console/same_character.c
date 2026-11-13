/**
 *    Name:        same_character.c
 *    Description: characters chosen more than once (the Konami code's REPEAT CHARACTERS): a slot playing another
 *                 character's moves with its looks
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Sebas: "y ahi dejamos repetir" (console/PLAN.md, guideline 9; tmnt-port's console/same_turtle.c). Each slot of the
 *    cabinet is a player (his lives, score, box and its colour, his mark when he comes in, where he comes in, the
 *    points his blows win: shadow_player's bits 4-5) and a character: its object's kind (00 Marge, 01 Homer, 02 Bart,
 *    03 Lisa), which every one of the characters' differences is read by. The sprite engine picks the object's set of
 *    animations by it (draw_object: animation_sets, E591), so the four sets share their animations' numbers and each
 *    draws its own moves in its own colours (the sprites' colours are the set's: the four characters' palettes are
 *    always loaded); and the players' code reads it for the attacks' reach (player_attack_boxes), the voices (a
 *    character's sound is the command plus its kind, player_play_voice), the team-ups (team_moves_by_characters, team_move_animation_offsets, pair_balloon_players), what it carries and how
 *    high (player_set_animation, the carry offsets, Homer's own carry), the balloons over it and the heights it
 *    throws from. All of it in src/players/, run by player_run_all and players_update_second_sprites, besides the
 *    sprite engine.
 *
 *    The program keeps each player's object's kind its slot's (the stories set it so at their end), and reads it as
 *    the player too: where he comes in (player_run: the kind times 32 from the left), whose his blows are in a team-up
 *    (shadow_player = the kind times 16: the owner's, and each one's own at its end), which player a story picks
 *    (story_random_player), whose box flashes when he is hit (player_flash_palette). So the kind stays the slot's,
 *    and a slot played by another character has its object's kind that character's while the players run (player_run_all, players_update_second_sprites: but at the frame its
 *    player comes in, where his place is his slot's) and while the sprite engine draws it (draw_object); the team-ups'
 *    player bits the code wrote from a character go back to the slot of the one it meant (himself, or his partner);
 *    and two of the same character never team up (the cabinet has no team-up of a character with itself:
 *    team_moves_by_characters' entries for it are the bytes around the table). Its picture in its box is the
 *    character's (gauges.c), and its portraits where the game shows a slot's (kind 0A plus the slot: the second bonus
 *    game's players, the bonus games' results) the character's, as his face in the name entry and his icon and default
 *    initials in the high score table, and the mark over him as he comes in names the character under the slot's
 *    number ("1UP BART": same_character_marks); the box that flashes when he is hit is his slot's. The portraits of
 *    the game's start are the characters' boxes: the one a player's slot is not his character's own does not come
 *    alive (the character's box is his).
 */
#include "console_internal.h"

#include "game.h"
#include "game/data.h"

#include <string.h>

#define PLAYER_OBJECT(slot) OBJECT_ADDRESS(2u * (uint32_t)(slot))      /* (4A40 + C0 x the slot) */
#define SECOND_OFFSET       0x60u      /* the player's second sprite (his mark, balloons, what he throws) */
#define OBJECT_STRIDE       0xC0u
#define PORTRAIT_KIND       0x0Au      /* the characters' portraits' sets: 0A Marge ... 0D Lisa */
#define SHADOW_PLAYER_BITS  0x30u      /* shadow_player: the player whose the blows are (bits 4-5) */
#define TEAM_CODE           0x80u      /* a player's attack ready to team up: 80 plus its kind */
#define ENTRY_STATUS(slot)  RAM8(marge_entry_status_ADDRESS + 8u * (uint32_t)(slot))    /* (0: not come in yet) */
#define PHASE_STEPS         1u
#define STEP_PORTRAITS      0u         /* game_step: the game start's portraits, */
#define STEP_BONUS_1        3u         /* the bonus games */
#define STEP_BONUS_2        12u
#define PORTRAIT_OBJECT(slot) OBJECT_ADDRESS(14u + 2u * (uint32_t)(slot))    /* (game_start_run_portraits) */
#define PORTRAIT_WAIT_JOIN  1u         /* a portrait's step: waiting for its slot's credit, */
#define PORTRAIT_WAIT       2u         /* its credit in, */
#define PORTRAIT_ALIVE      3u         /* coming alive */

int console_slot_character[CONSOLE_PLAYERS];

bool console_repeats_allowed(void)
{
	return (console.konami || modes_code_rules()) && console.repeats;    /* (MR. PLOW MODE's games too, the code
		entered on the menu) */
}

void console_characters_reset(void)
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		console_slot_character[slot] = slot;
}

static bool any_remapped(void)
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		if (console_slot_character[slot] != slot)
			return true;
	return false;
}

/* Whether 'slot' is another player's than 'player' (picked on the portraits; in a game, playing or coming in) */
static bool slot_taken(int player, int slot)
{
	if (console.page == PAGE_CHOICE)
	{
		for (int other = 0; other < CONSOLE_PLAYERS; other++)
			if (other != player && console.choice_picked[other] && console_slot_of[other] == slot)
				return true;
		return false;
	}
	return console_slot_plays(slot) || console_slot_joining(slot);
}

/* Whether some player other than 'player' has 'character' (picked, or playing it) */
static bool character_had(int player, int character)
{
	for (int other = 0; other < CONSOLE_PLAYERS; other++)
	{
		const int slot = console_slot_of[other];

		if (other != player && slot >= 0 && console_slot_character[slot] == character && slot_taken(player, slot))
			return true;
	}
	return false;
}

int console_slot_for_character(int player, int character)
{
	int found = -1;

	if (character < 0 || character >= CHARACTERS)
		return -1;
	if (!slot_taken(player, character))
		found = character;    /* (its own) */
	for (int slot = 0; slot < CONSOLE_PLAYERS && found < 0; slot++)
		if (!slot_taken(player, slot) && !character_had(player, slot))
			found = slot;     /* (a slot whose own character nobody has: left for its own while it can be) */
	for (int slot = 0; slot < CONSOLE_PLAYERS && found < 0; slot++)
		if (!slot_taken(player, slot))
			found = slot;
	if (found >= 0)
		console_slot_character[found] = character;
	return found;
}

/* ---- the game's functions in the console's versions ---- */

/* The slot of a player's object or of its second sprite, -1 for another object; whether it is the second sprite */
static int player_slot(uint16_t object, bool *second)
{
	const uint32_t offset = (uint32_t)object - PLAYER_OBJECT(0);

	if (object < PLAYER_OBJECT(0) || offset >= CONSOLE_PLAYERS * OBJECT_STRIDE || offset % SECOND_OFFSET != 0u)
		return -1;
	*second = (offset % OBJECT_STRIDE) != 0u;
	return (int)(offset / OBJECT_STRIDE);
}

/* The slot a team-up's player bits were written for, from the character the code read them from: the object's own
   slot when it is its character, its partner's when the partner's (both: its own) */
static uint8_t player_bits_for(int slot, uint8_t shadow)
{
	const int written = (shadow & SHADOW_PLAYER_BITS) >> 4;
	const uint16_t partner = OBJECT(PLAYER_OBJECT(slot))->player.partner;
	bool second = false;
	int partner_slot;

	if (written == console_slot_character[slot])
		return (uint8_t)((shadow & ~SHADOW_PLAYER_BITS) | (unsigned)slot << 4);
	if ((int8_t)OBJECT(PLAYER_OBJECT(slot))->player.teamed >= 0)
		return shadow;    /* (no team-up: the bits are the slot's own, its player's coming in) */
	partner_slot = player_slot(partner, &second);
	if (partner_slot >= 0 && !second && console_slot_character[partner_slot] == written)
		return (uint8_t)((shadow & ~SHADOW_PLAYER_BITS) | (unsigned)partner_slot << 4);
	return shadow;
}

/* Each slot another character plays with its object's kind that character's (but at the frame its player comes in:
   his place is his slot's), and back */
static void kinds_to_characters(bool swapped[CONSOLE_PLAYERS])
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t object = PLAYER_OBJECT(slot);

		swapped[slot] = console_slot_character[slot] != slot && OBJECT(object)->kind == slot
			&& ENTRY_STATUS(slot) != 0u;
		if (swapped[slot])
			OBJECT(object)->kind = (uint8_t)console_slot_character[slot];
	}
}

static void kinds_to_slots(const bool swapped[CONSOLE_PLAYERS])
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		if (swapped[slot] && OBJECT(PLAYER_OBJECT(slot))->kind == console_slot_character[slot])
			OBJECT(PLAYER_OBJECT(slot))->kind = (uint8_t)slot;
}

/* Two players of the same character never team up: a team-up starts when two players standing by each other touch
   each other with their team codes (the attack 80 plus the kind, which the hits' check leaves in the other's hit:
   player_take_touches), and its move is team_moves_by_characters' by the two kinds; a character with itself is no
   pair of the cabinet's, whose entries are the bytes around the table (the two players held in a move of nothing).
   So a player touched by the team code of his own character has no touch */
static void no_team_with_self(void)
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t object = PLAYER_OBJECT(slot);

		if (OBJECT(object)->hit == TEAM_CODE + (unsigned)console_slot_character[slot])
			OBJECT(object)->hit = 0u;
	}
}

/* The four players run with their characters' kinds (no team-up of a character with itself), the team-ups' player
   bits written from a character back to its slot's */
void player_run_all(void)
{
	bool swapped[CONSOLE_PLAYERS];
	uint8_t shadows[CONSOLE_PLAYERS][2];

	if (!any_remapped())
	{
		original_player_run_all();
		return;
	}
	no_team_with_self();
	kinds_to_characters(swapped);
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		shadows[slot][0] = OBJECT(PLAYER_OBJECT(slot))->shadow_player;
		shadows[slot][1] = OBJECT(PLAYER_OBJECT(slot) + SECOND_OFFSET)->shadow_player;
	}
	original_player_run_all();
	kinds_to_slots(swapped);
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t object = PLAYER_OBJECT(slot);

		if (OBJECT(object)->shadow_player != shadows[slot][0])
			OBJECT(object)->shadow_player = player_bits_for(slot, OBJECT(object)->shadow_player);
		if (OBJECT(object + SECOND_OFFSET)->shadow_player != shadows[slot][1])
			OBJECT(object + SECOND_OFFSET)->shadow_player = player_bits_for(slot,
				OBJECT(object + SECOND_OFFSET)->shadow_player);
	}
}

/* The players' second sprites run (what is over each one's head and how high, the balloons and their words, what
   each throws and from how high) with their characters' kinds */
uint16_t players_update_second_sprites(void)
{
	bool swapped[CONSOLE_PLAYERS];
	uint16_t result;

	if (!any_remapped())
		return original_players_update_second_sprites();
	kinds_to_characters(swapped);
	result = original_players_update_second_sprites();
	kinds_to_slots(swapped);
	return result;
}

/* The kind 'object' is drawn with: a slot's player object of the slot's own kind, its character's; a portrait of a
   slot's (kind 0A plus the slot: on the player's objects in the second bonus game, the bonus games' results),
   its character's portrait */
static uint8_t drawn_kind(uint16_t object, uint8_t kind)
{
	bool second = false;
	const int slot = player_slot(object, &second);

	if (slot >= 0)
	{
		const int character = console_slot_character[slot];

		if (character == slot)
			return kind;
		if (!second && kind == slot)
			return (uint8_t)character;
		if (kind == PORTRAIT_KIND + slot)
			return (uint8_t)(PORTRAIT_KIND + character);
		return kind;
	}
	if (mode_phase == PHASE_STEPS && (game_step == STEP_BONUS_1 || game_step == STEP_BONUS_2)
		&& kind >= PORTRAIT_KIND && kind < PORTRAIT_KIND + CHARACTERS)
		return (uint8_t)(PORTRAIT_KIND + console_slot_character[kind - PORTRAIT_KIND]);
	return kind;
}

/* An object drawn: a slot's character's looks on the slot's objects */
uint16_t draw_object(uint16_t object, uint16_t sprites)
{
	uint8_t kind, drawn;
	uint16_t result;

	if (!any_remapped() || game_mode != GAME_MODE_GAME)
		return original_draw_object(object, sprites);
	kind = OBJECT(object)->kind;
	drawn = drawn_kind(object, kind);
	if (drawn == kind)
		return original_draw_object(object, sprites);
	OBJECT(object)->kind = drawn;
	result = original_draw_object(object, sprites);
	OBJECT(object)->kind = kind;
	return result;
}

/* The hurt flash is the player's box flashing (player_flash_palette: the palette effect 22 plus the kind, which
   animates pen 11 of palettes 20-23: the four boxes' colours, red, blue, green, orange, in the fixed layer), so it is
   the slot's: a slot playing another character flashes its own box, not that character's slot's (two Barts: the
   second hit flashed the first one's green box, and both seemed to flash together) */
uint16_t player_flash_palette(uint16_t y, uint16_t player)
{
	bool second = false;
	const int slot = player_slot(player, &second);
	uint8_t kind;
	uint16_t result;

	if (slot < 0 || second || OBJECT(player)->kind == slot)
		return original_player_flash_palette(y, player);
	kind = OBJECT(player)->kind;
	OBJECT(player)->kind = (uint8_t)slot;
	result = original_player_flash_palette(y, player);
	OBJECT(player)->kind = kind;
	return result;
}

/* ---- what names the slot's character ---- */

/* The join mark over a player coming in, "1UP MARGE" (each set's animation 87: a sprite of two cells, 7828 Marge,
   782C Homer, 782A Bart, 782E Lisa, in its colour's pen 9, 4, A, 8; the slot's number in rows 0-9, the name in rows
   11-15 under it, colour 08 of each). The game has no "1UP BART", so while a slot plays another character the
   slot's mark's name rows are that character's, in the slot's pen, in the sprite data in memory (game_sprites, the
   assets'), and the game's own again out of such a game: the slot's number over the character's name. Nothing else
   reads those cells but the test mode's ROM check (the K053246's), which no game reaches */
#define MARK_CELL_BYTES     128u       /* a 16 x 16 cell, 4 bits a pixel */
#define MARK_ROW_BYTES      8u
#define MARK_NAME_ROW       11u        /* the name's rows: 11 to 15 */
#define MARK_NAME_ROWS      5u
static const uint16_t mark_cells[CHARACTERS] = { 0x7828u, 0x782Cu, 0x782Au, 0x782Eu };
static const uint8_t mark_pens[CHARACTERS] = { 0x9u, 0x4u, 0xAu, 0x8u };
static uint8_t mark_names[CHARACTERS][2][MARK_NAME_ROWS * MARK_ROW_BYTES];    /* (each one's name rows as loaded) */
static bool marks_taken;
static int mark_shown[CHARACTERS];    /* the character each slot's mark names now, plus one (0: its own, as loaded) */

static uint8_t *mark_name_rows(int slot, int cell)
{
	return &game_sprites[(size_t)(mark_cells[slot] + (unsigned)cell) * MARK_CELL_BYTES + MARK_NAME_ROW * MARK_ROW_BYTES];
}

/* 'slot''s mark naming 'character': its name rows, the character's pen the slot's */
static void mark_name(int slot, int character)
{
	for (int cell = 0; cell < 2; cell++)
	{
		uint8_t *rows = mark_name_rows(slot, cell);

		for (unsigned index = 0; index < MARK_NAME_ROWS * MARK_ROW_BYTES; index++)
		{
			const uint8_t pixels = mark_names[character][cell][index];
			const uint8_t high = (uint8_t)(pixels >> 4), low = pixels & 0x0Fu;

			rows[index] = (uint8_t)((high == mark_pens[character] ? mark_pens[slot] : high) << 4
				| (low == mark_pens[character] ? mark_pens[slot] : low));
		}
	}
	mark_shown[slot] = character == slot ? 0 : character + 1;
}

void same_character_marks(void)
{
	const bool playing = game_mode == GAME_MODE_GAME && !console_arcade_testing();

	if (!marks_taken)
	{
		for (int character = 0; character < CHARACTERS; character++)
			for (int cell = 0; cell < 2; cell++)
				memcpy(mark_names[character][cell], mark_name_rows(character, cell), sizeof(mark_names[0][0]));
		marks_taken = true;
	}
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const int character = playing ? console_slot_character[slot] : slot;

		if (mark_shown[slot] != (character == slot ? 0 : character + 1))
			mark_name(slot, character);
	}
}

/* The high score table: a slot played by another character goes in as that character (the ranking's icon by it), and
   the slot's initials a player's last score goes in with as he joins again (player_join: MRG, HMR, BRT, LSA, the
   player records') the character's; initials a player typed go in as typed (the name entry's) */
uint16_t insert_high_score(uint16_t place)
{
	const unsigned slot = new_high_score_player;

	if (slot < CONSOLE_PLAYERS && console_slot_character[slot] != (int)slot)
	{
		const int character = console_slot_character[slot];
		const struct player *own = PLAYER(player_variable_records_ADDRESS + slot * (unsigned)sizeof(struct player));
		const struct player *chosen = PLAYER(player_variable_records_ADDRESS
			+ (unsigned)character * (unsigned)sizeof(struct player));

		if (new_high_score == (uint16_t)(own->initial_1 << 8 | own->initial_2)
			&& new_high_score_initial_3 == own->initial_3)
		{
			new_high_score = (uint16_t)(chosen->initial_1 << 8 | chosen->initial_2);
			new_high_score_initial_3 = chosen->initial_3;
		}
		new_high_score_player = (uint8_t)character;
	}
	return original_insert_high_score(place);
}

/* The name entry's column of a slot: its face (pictures 4-7, Marge to Lisa) the character's the slot plays */
#define NAME_ENTRY_FACES    4u

void name_entry_draw_picture(uint16_t column_and_picture)
{
	const unsigned column = (column_and_picture >> 8) & 0x7Fu, picture = column_and_picture & 0xFFu;

	if (column < CONSOLE_PLAYERS && picture == NAME_ENTRY_FACES + column)
		column_and_picture = (uint16_t)((column_and_picture & 0xFF00u)
			| (NAME_ENTRY_FACES + (unsigned)console_slot_character[column]));
	original_name_entry_draw_picture(column_and_picture);
}

/* ---- the portraits ---- */

void same_character_portraits(void)
{
	if (game_mode != GAME_MODE_GAME || mode_phase != PHASE_STEPS || game_step != STEP_PORTRAITS)
		return;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t portrait = PORTRAIT_OBJECT(slot);
		const uint8_t step = OBJECT(portrait)->portrait.step;

		if (console_slot_character[slot] != slot && (step == PORTRAIT_WAIT || step == PORTRAIT_ALIVE))
			OBJECT(portrait)->portrait.step = PORTRAIT_WAIT_JOIN;    /* (it waits for its credit again: never alive) */
	}
}

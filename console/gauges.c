/**
 *    Name:        gauges.c
 *    Description: no coins at home: the players' boxes show the game's PRESS START where the cabinet's show INSERT COIN;
 *                 a box's picture of its character, the one its slot plays
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    A player's box at the top of the screen (its quarter of the fixed layer, gauge_draw_player) shows one of the
 *    game's pictures by the slot's status: picture 3, INSERT COIN, where nobody plays it (tiles C7-CF over D7-DF), and
 *    picture 5, PRESS START, where a credit waits for its player (E9-EF and EA EB over F9-FF and FA FB: the cabinet
 *    shows it on the portraits after a coin). A START brings a player in at home (console.c), so the console's boxes
 *    show the game's own PRESS START in the place of INSERT COIN, in the box's colour as the game draws it
 *    (console/PLAN.md, guideline 2; tmnt-port's message_insert_coin). A slot playing another character (the Konami
 *    code's REPEAT CHARACTERS, same_character.c) shows that character's picture in its box (pictures 6-9, Marge to
 *    Lisa, gauge_draw_player_picture's: the quarter's number plus 6), in the box's own colour.
 */
#include "console_internal.h"

#include "game.h"

#define PICTURE_INSERT_COIN     3u     /* gauge_pictures: INSERT COIN, */
#define PICTURE_PRESS_START     5u     /* PRESS START, */
#define PICTURE_CHARACTERS      6u     /* and the characters' pictures, 6 Marge to 9 Lisa */
#define SCORE_DIGITS            4u     /* a score's tiles (print_bcd_word) */
#define BLANK_TILE              0x10u  /* the fixed layer's space, the game's '@' (a letter's tile is the letter less 30) */

/* A quarter's picture put out: its tiles in the game's spaces (FRANK GRIMES LIFE's other boxes, where nobody comes
   in: their tab, their line and their INSERT COIN, nothing of them shown, Sebas: "si no hay jugadores, no tendria que
   verse"). The game's own blank (bit 7 of the quarter) only draws the picture in colour 0, which still shows its
   letters: the parts are walked as gauge_draw_picture walks them, each a place, a width, rows, a colour and its tiles,
   until a part ends with a byte not 0 */
static void gauge_blank(unsigned quarter, unsigned picture)
{
	const unsigned column = gauge_player_columns[quarter].column;
	uint32_t cursor = gauge_pictures[picture], end;

	do
	{
		uint32_t tile = (mem_read16(cursor) + (uint32_t)(int8_t)column) & 0xFFFFu;
		const unsigned width = mem_read8(cursor + 2u), rows = mem_read8(cursor + 3u);

		cursor += 5u + width * rows;
		for (unsigned row = 0; row < rows; row++, tile += 0x40u)
			for (unsigned x = 0; x < width; x++)
			{
				mem_write8(tile + x - 0x2000u, 0u);
				mem_write8(tile + x, BLANK_TILE);
			}
		end = cursor++;
	} while (mem_read8(end) == 0u);
}

/* Draws picture B in player A's quarter, in its colour (blank with A bit 7 set): PRESS START for INSERT COIN, the
   character the quarter's slot plays */
void gauge_draw_picture(uint16_t quarter_and_picture)
{
	const unsigned picture = quarter_and_picture & 0xFFu, quarter = (quarter_and_picture >> 8) & 0x7Fu;

	if (console_arcade_testing())
		;    /* (the arcade's own scenarios: the cabinet's INSERT COIN, its tiles in the state they compare) */
	else if (console.mode == MODE_FRANK_GRIMES && quarter < CONSOLE_PLAYERS && (console.chooser < 0
		|| console.chooser >= CONSOLE_PLAYERS || (int)quarter != console_slot_of[console.chooser]))
	{
		/* (FRANK GRIMES LIFE: nobody else comes in, the other boxes empty: their pictures, and the score
		   gauge_print_score wrote just before, its four digits) */
		const uint32_t score_tiles = GAUGE_PLAYER(gauge_player_records[quarter])->score_tiles;

		gauge_blank(quarter, picture);
		for (uint32_t digit = 0; digit < SCORE_DIGITS; digit++)
		{
			mem_write8(score_tiles + digit - 0x2000u, 0u);
			mem_write8(score_tiles + digit, BLANK_TILE);
		}
		return;
	}
	else if (picture == PICTURE_INSERT_COIN)
		quarter_and_picture = (uint16_t)((quarter_and_picture & 0xFF00u) | PICTURE_PRESS_START);
	else if (quarter < CONSOLE_PLAYERS && picture == PICTURE_CHARACTERS + quarter)
		quarter_and_picture = (uint16_t)((quarter_and_picture & 0xFF00u)
			| (PICTURE_CHARACTERS + (unsigned)console_slot_character[quarter]));
	original_gauge_draw_picture(quarter_and_picture);
}

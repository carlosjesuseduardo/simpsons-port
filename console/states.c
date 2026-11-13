/**
 *    Name:        states.c
 *    Description: the pause's SAVE STATE and LOAD STATE: the console's part of a save state, and what the pause says
 *                 a save or a load came to
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    A save state is the whole game's (game/state.h: the board, the program's runtime, the video, the sound board),
 *    taken and put back where the program waits for its next frame in its main loop: the pause's, in a game. The
 *    console's part is what it keeps from frame to frame in a game: its own state (the page, the players' slots and
 *    the characters they play, the character choice and changes, the mode's lives, continues and credits, the Konami
 *    game's settings), the joins under way and the players' last controls, the last pick, WIDE's account of the
 *    layers' rings and its sides, and the mode under way (MR. PLOW MODE's). What it works out again by itself is not
 *    kept: the join marks in the sprite data (same_character_marks writes them for the characters the slots play at
 *    the game's next frame), Bart's shirt (the video's recolouring, from the settings), the menus' pages, which a game
 *    leaves for the TV again. The high scores kept are the player's whatever the state: they go back into the game's
 *    table once the game loaded is over (scores.c).
 */
#include "console_internal.h"

#include "simpsons/state.h"

#define MESSAGE_Y           186          /* under the pause's lines (menu.c's PAUSE_LINES_Y, PAUSE_LINE_STEP) */
#define MESSAGE_COLOUR      0xFFFFFFu

void console_state_part(simpsons_state *state)
{
	simpsons_state_part(state, "console");
	simpsons_state_bytes(state, &console, sizeof(console));
	simpsons_state_bytes(state, console_slot_of, sizeof(console_slot_of));
	simpsons_state_bytes(state, console_slot_character, sizeof(console_slot_character));
	console_joins_state(state);
	characters_state(state);
	wide_state(state);
	modes_state(state);
	if (simpsons_state_loading(state))
		console_scores_state_loaded();
}

/* In the game's letters (capitals, digits and . , - / ( ) ' ? +) */
const char *console_state_message(simpsons_state_result result, bool loading)
{
	switch (result)
	{
		case SIMPSONS_STATE_DONE:
			return loading ? "STATE LOADED" : "STATE SAVED";
		case SIMPSONS_STATE_NONE:
			return "NO STATE SAVED YET";
		case SIMPSONS_STATE_OTHER_VERSION:
			return "THE STATE IS OF ANOTHER VERSION";
		case SIMPSONS_STATE_NOT_A_STATE:
		case SIMPSONS_STATE_DAMAGED:
			return "THE STATE CANNOT BE READ";
		case SIMPSONS_STATE_NOT_WRITTEN:
			return "THE STATE CANNOT BE WRITTEN";
		case SIMPSONS_STATE_NOT_HERE:
			return loading ? "NO STATE CAN BE LOADED HERE" : "NO STATE CAN BE SAVED HERE";
		default:
			return NULL;
	}
}

void console_pause_message(console_picture *frame, const char *message)
{
	if (message)
		draw_game_letters_centred(frame, MESSAGE_Y, 1, MESSAGE_COLOUR, message);
}

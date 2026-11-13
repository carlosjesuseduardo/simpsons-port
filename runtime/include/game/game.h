/**
 *    Name:        game.h
 *    Description: the game as the ports run it: the board with the assets in place of the ROM, the program on
 *                 it, its frames drawn and its sound made
 *    Version:     0.1.0
 *    Created:     06-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The ports (ports/sdl2, the game's headless) load the assets, reset the game and run it (simpsons/program.h's
 *    program_run); when the program waits for its next frame the board's VBLANK comes (the frame is drawn, the
 *    board's events), and then the port's game_frame_done, where it applies the controls, closes the frame's
 *    sound (game_end_frame), shows the frame and takes its hashes. The program's data is the assets' (game/assets.h):
 *    the game opens no ROM, and a read of the program's code as data stops it.
 */
#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "simpsons/video.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Loads the assets into the board's memories from 'assets', their folder or their pack (simpsons.assets, as
   game_assets_is_pack tells): NULL, or what failed (a pack of another version of the game is refused) */
const char *game_load(const char *assets);

/* Loads the same from the player's ROM set at 'source' (a .zip or a folder), through this build's layout: only what
   the assets give (the first run's, which then writes the pack: game_assets_pack_write). NULL, or what failed */
const char *game_load_rom_set(const char *source);

/* The board after its reset (the factory EEPROM from the assets), its sound board at power on, the program at its
   reset; the test switch on if 'service'. With 'draw', every frame is drawn (game_frame). NULL, or what failed */
const char *game_reset(bool service, bool draw);

/* Whether the next frames are drawn (game_reset's 'draw'; the board's state the same either way): a port that runs
   frames it does not show draws none of them (the console's hurried frames) */
void game_set_draw(bool draw);

/* What the port does when a frame is done (the VBLANK's events are over): its controls, game_end_frame, its
   output */
extern void (*game_frame_done)(void);

/* Closes the frame's sound (the sound board catches up with the frame's end): its samples (game_audio) and hashes */
void game_end_frame(void);

/* The frame drawn when the last VBLANK ended */
const simpsons_frame *game_frame(void);

/* The last frame's sound, stereo (left, right interleaved): 'count' stereo samples, at game_audio_rate() a second */
const int16_t *game_audio(size_t *count);
uint32_t game_audio_rate(void);

/* The last frame's sound hashes (the samples, the sound board's state), as kref --sound-hashes writes them */
uint64_t game_sound_samples_hash(void);
uint64_t game_sound_state_hash(void);

/* The sound in stereo (The Simpsons' SOUND: STEREO): the YM2151's two outputs each on its own side, the channels the
   program pans right alone heard on the right; off, the board's (its left output on both sides), every sample the
   same as the cabinet's. From the next game_end_frame on; game_reset keeps it, and the hashes are the same either way
   (simpsons/sound_board.h's simpsons_sound_stereo) */
void game_set_stereo(bool stereo);

#ifdef __cplusplus
}
#endif

#endif

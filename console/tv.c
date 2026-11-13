/**
 *    Name:        tv.c
 *    Description: the menu's screen: the TV the attract mode's last screen and the score ranking show, with the
 *                 title's logo on its screen, and the menus' song
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The TV is layer B's map of scene 2, area 0 (the stage cards' TV and the score ranking's: the family watches it
 *    on the couch and then the Konami logo shows on it, attract_last_screen_start), drawn by the game itself with
 *    palette set 1's colours for layer B, as the last screen draws it. Its screen is the background colour, black here
 *    as the stage cards have it, and on it the title's logo (the title's object, kind 0E, animation 9F, with set 7's
 *    sprite colours: attract_title_draw) at a little more than half its size by the K053247's zoom (the attract's
 *    opening zooms it the same way). The menus write their lines on the screen under it (menu.c). The menus' song is
 *    the attract mode's (song 80, `attract` in tools/sound_music.py), asked for again when it ends.
 */
#include "console_internal.h"

#include "simpsons/sound_board.h"    /* (before game.h: ram.h's names are macros) */
#include "game.h"

#define CLEAR_LAYERS        0u         /* the job that clears the three layers */
#define BACKGROUND          3u         /* palette requests: the background colour (the TV's screen, black), */
#define PALETTE_SETS        1u         /* and loads of palette sets: */
#define TV_PALETTES         0x0401u    /* set 1's load 2, layer B's: the TV's (the last screen's 0D01) */
#define LOGO_PALETTES       0x0807u    /* set 7's load 3, the sprites': the title's logo (the title's 0F07) */
#define TV_SCENE            2u         /* the stage cards' TV and the score ranking's, in layer B */
#define TV_AREA             0u
#define LAYERS_SCROLL_X     0xFFF0u    /* the layers' x scroll the last screen draws them with */
#define LOGO_OBJECT         14         /* the title's own */
#define LOGO_KIND           0x0Eu
#define LOGO_ANIMATION      0x9Fu      /* (bit 7: from its first frame) */
#define LOGO_X              0xD0u      /* the title's x for it, */
#define LOGO_Y              0x138u     /* a little higher than the title's 128 (the TV's screen's top: a sprite's y goes up), */
#define LOGO_ZOOM           0x70u      /* and its size: 40 is the title's own, more is smaller (0.57 of it) */
#define OBJECT_ZOOMED       0x90u      /* (bit 4: drawn again each frame, at its zoom: the opening's) */
#define OVER_LAYERS         0x10u      /* attributes: its sprites' priority over the layers (the title's) */
#define MENU_SONG           0x8000u    /* song 80, the attract mode's: the menus' */
#define SONG_START_FRAMES   30         /* the song's command reaching the sound board, its channels taken */
#define CHANNELS            12         /* the sound program's channel records (F003-F3C2: the YM2151's 8, the
                                          K053260's 4), 80 bytes each, their command at +10 (bit 7: a song's) */
#define CHANNEL_FIRST       0x0003u
#define CHANNEL_SIZE        0x50u
#define CHANNEL_COMMAND     0x10u

static int state;                  /* 0 to set up, 1 the layers clearing, 2 shown */
static int song_frames = -1;       /* frames since the song was asked for (-1: not asked) */

void tv_forget(void)
{
	state = 0;
	song_frames = -1;
}

bool tv_shown(void)
{
	return state == 2;
}

/* Whether a song plays: a channel record has a song's command */
static bool music_playing(void)
{
	for (unsigned channel = 0; channel < CHANNELS; channel++)
		if (simpsons_sound.ram[CHANNEL_FIRST + channel * CHANNEL_SIZE + CHANNEL_COMMAND] & 0x80u)
			return true;
	return false;
}

/* The title's logo on the TV's screen, smaller */
static void put_logo(void)
{
	const uint16_t object = (uint16_t)OBJECT_ADDRESS(LOGO_OBJECT);

	OBJECT(object)->kind = LOGO_KIND;
	OBJECT(object)->animation = LOGO_ANIMATION;
	OBJECT(object)->animation_flags = 0x80u;
	OBJECT(object)->x = LOGO_X;
	OBJECT(object)->y = LOGO_Y;
	OBJECT(object)->zoom_x = LOGO_ZOOM;
	OBJECT(object)->zoom_y = LOGO_ZOOM;
	OBJECT(object)->attributes = OVER_LAYERS;
	OBJECT(object)->flags = OBJECT_ZOOMED;
}

void tv_frame(void)
{
	uint16_t place = 0, sprite_place = 0, job = 0;

	switch (state)
	{
		case 0:
			/* (as the attract's screens start: the objects cleared, the layers' clear queued, the TV's colours and
			   the logo's, its screen black) */
			vector_clear_objects(&place, &sprite_place);
			clear_sprites();
			queue_job(CLEAR_LAYERS);
			palette_request_kind = BACKGROUND;
			job = palette_request(job, 0u);
			palette_request_kind = PALETTE_SETS;
			job = palette_request(job, TV_PALETTES);
			palette_request(job, LOGO_PALETTES);
			state = 1;
			break;
		case 1:
			if (layer_clear_pending != 0u)
				break;
			/* (the TV drawn into layer B as the last screen draws it: the tile banks of the attract's maps, the
			   layers' scroll) */
			map_scene = 0u;
			map_area = 0u;
			set_area_tile_banks();
			attract_reset_layers();
			layer_a_scroll_x = LAYERS_SCROLL_X;
			layer_b_scroll_x = LAYERS_SCROLL_X;
			map_scene = TV_SCENE;
			map_area = TV_AREA;
			attract_draw_layer_b();
			put_logo();
			state = 2;
			break;
		default:
			/* (the title's logo on the menu's page alone: the other pages write over the TV's screen) */
			OBJECT(OBJECT_ADDRESS(LOGO_OBJECT))->flags = console.page == PAGE_MENU ? OBJECT_ZOOMED : 0u;
			break;
	}
	if (song_frames < 0 || (++song_frames > SONG_START_FRAMES && !music_playing()))
	{
		song_frames = 0;
		queue_sound_always(MENU_SONG);
	}
}

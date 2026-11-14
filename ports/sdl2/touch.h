/**
 *    Name:        touch.h
 *    Description: the touch screen's controls (Android): a stick where the left thumb lands, ATTACK and JUMP at the
 *                 right, START (The Simpsons) or COIN (The Simpsons Arcade Game) over them, drawn over the picture, and
 *                 the controls the fingers on them hold: player 1's
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    As tmnt-port's touch.h.
 */
#ifndef SIMPSONS_PORT_TOUCH_H
#define SIMPSONS_PORT_TOUCH_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

/* The controls' letters, from font.bmp in the folder 'base' (with its separator; Android's: "", the APK's assets);
   shown from the start where there is a touch screen */
void touch_open(SDL_Renderer *renderer, const char *base);

/* An event: a finger on the controls; a game controller's button or stick, or a key, hides them until a finger
   touches the screen again (but the system's keys: Android's back, the volume) */
void touch_event(const SDL_Event *event);

/* The board's controls the fingers hold (SIMPSONS_LEFT ... SIMPSONS_JUMP), and whether one holds START (COIN in The
   Simpsons Arcade Game) */
uint8_t touch_controls(void);
bool touch_start(void);

/* The controls' places on a screen of 'width' x 'height' with the picture at 'picture': over it, or, on a screen
   taller than the picture by a fifth or more (an open Fold's, a tablet's) while they are shown, in the room under it,
   the picture moved to the top (its y set to 0) */
void touch_layout(int width, int height, SDL_Rect *picture);

/* The controls over a screen of 'width' x 'height', before it is presented (nothing while they are hidden) */
void touch_draw(SDL_Renderer *renderer, int width, int height);

/* The screen's density in dots an inch, as SDL_GetDisplayDPI tells it for the first display (diagonal, horizontal,
   vertical; any may be NULL): false when it does not. A build for the checks on a computer
   (SIMPSONS_TOUCH_CHECK) takes it from the environment's SIMPSONS_TOUCH_DPI instead, a phone's or a tablet's */
bool touch_display_dpi(float *diagonal, float *horizontal, float *vertical);

#endif

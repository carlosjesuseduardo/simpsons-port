/**
 *    Name:        draw.c
 *    Description: the console's own picture over the frame the video drew: the game's own letters from the fixed
 *                 layer's character set at any size, small letters of its own, Kenney Mini, shades and fills
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The menus write with the game's own letters: the K052109's fixed layer's font, as the game's texts write them
 *    (the tile is the character less 30h, '@' the space: digits 00-09, A-Z 11-2A; '.' 77, ',' 74, '-' 79, '/' 78,
 *    '(' 7B, ')' 7C, the apostrophe 75, '?' 0E, '+' 70, the copyright sign 2B, which the texts write '['), their pen 1
 *    the letter and 2 its shadow, in one colour of
 *    the console's; with an '@', ':', '<' and '>' of its own in the font's manner. "@cheviman" goes in small letters of
 *    the console's own. Kenney Mini (Kenney, CC0: font.bmp beside the executable) loads too, for what the game's font
 *    has not. The letters come from the player's assets as the board has them (game/data.h's tile memory), the way
 *    runtime/src/video.c draws its tiles: nothing of the game's is in the console. As tmnt-port's console/draw.c.
 */
#include "console_internal.h"

#include "game/data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_FILE      "font.bmp"
#define FONT_FIRST      0x20
#define FONT_LAST       0x7E
#define FONT_CELL       16          /* pixels: a character's cell, 16 a row in the picture */
#define FONT_ROW        16
#define FONT_SPACING    1           /* pixels between two characters, past the lit columns */
#define SPACE_WIDTH     4
#define SHADOW          0x000000u

#define WIDTH           SIMPSONS_VIDEO_WIDTH
#define HEIGHT          SIMPSONS_VIDEO_HEIGHT
#define TILES           (GAME_TILES_SIZE / 32)
#define TEXT_FIRST      0x30u       /* the tile of a character is it less this */
#define LETTER_PEN      1           /* the font's pens: the letter, its shadow */
#define SHADOW_PEN      2
#define SHADOW_KEEP     72          /* of 256: the shadow's light, of the letter's colour */
#define OWN_AT          0x10000     /* (no tile: the console's own '@', ':', '<' and '>', own_glyphs) */
#define OWN_COLON       0x10001
#define OWN_LESS        0x10002
#define OWN_GREATER     0x10003

static struct
{
	bool loaded;
	uint8_t lit[6 * FONT_CELL][FONT_ROW * FONT_CELL];    /* the picture's pixels (6 rows of cells): lit or not */
	int widths[FONT_LAST - FONT_FIRST + 1];
	int top;                    /* the first lit row of a capital, in its cell */
} font;

/* ---- Kenney Mini ---- */

static uint32_t read_le(const uint8_t *bytes, int count)
{
	uint32_t value = 0;

	for (int index = count - 1; index >= 0; index--)
		value = value << 8 | bytes[index];
	return value;
}

bool draw_load_font(const char *folder)
{
	char path[1100];
	uint8_t header[54];
	uint8_t *pixels;
	FILE *input;
	int width, height, bits, stride;
	uint32_t offset;
	bool bottom_up;

	snprintf(path, sizeof(path), "%s" FONT_FILE, folder ? folder : "");
	if ((input = fopen(path, "rb")) == NULL)
		return false;
	if (fread(header, 1, sizeof(header), input) != sizeof(header) || header[0] != 'B' || header[1] != 'M')
	{
		fclose(input);
		return false;
	}
	offset = read_le(&header[10], 4);
	width = (int)read_le(&header[18], 4);
	height = (int)(int32_t)read_le(&header[22], 4);
	bits = (int)read_le(&header[28], 2);
	bottom_up = height > 0;
	height = abs(height);
	if ((bits != 24 && bits != 32) || width > FONT_ROW * FONT_CELL || height > (int)(sizeof(font.lit) / sizeof(font.lit[0])))
	{
		fclose(input);
		return false;
	}
	stride = (width * (bits / 8) + 3) & ~3;
	pixels = malloc((size_t)stride * (size_t)height);
	if (!pixels || fseek(input, (long)offset, SEEK_SET) != 0 || fread(pixels, 1, (size_t)stride * (size_t)height, input)
		!= (size_t)stride * (size_t)height)
	{
		free(pixels);
		fclose(input);
		return false;
	}
	fclose(input);
	memset(&font, 0, sizeof(font));
	for (int y = 0; y < height; y++)
	{
		const uint8_t *row = &pixels[(size_t)(bottom_up ? height - 1 - y : y) * (size_t)stride];

		for (int x = 0; x < width; x++)
		{
			const uint8_t *pixel = &row[x * (bits / 8)];

			font.lit[y][x] = (pixel[0] | pixel[1] | pixel[2]) != 0;
		}
	}
	free(pixels);
	/* each character as wide as its last lit column; the capitals' first lit row */
	font.top = FONT_CELL;
	for (int index = 0; index <= FONT_LAST - FONT_FIRST; index++)
	{
		const int left = (index % FONT_ROW) * FONT_CELL, top = (index / FONT_ROW) * FONT_CELL;

		for (int y = 0; y < FONT_CELL; y++)
			for (int x = 0; x < FONT_CELL; x++)
				if (font.lit[top + y][left + x])
				{
					if (x + 1 > font.widths[index])
						font.widths[index] = x + 1;
					if (index + FONT_FIRST >= 'A' && index + FONT_FIRST <= 'Z' && y < font.top)
						font.top = y;
				}
	}
	font.widths[0] = SPACE_WIDTH;
	if (font.top == FONT_CELL)
		font.top = 0;
	font.loaded = true;
	return true;
}

int draw_text_width(const char *text, int scale)
{
	int width = 0;

	if (!font.loaded)
		return 0;
	for (; *text; text++)
		if (*text >= FONT_FIRST && *text <= FONT_LAST)
			width += (font.widths[*text - FONT_FIRST] + FONT_SPACING) * scale;
	return width > 0 ? width - FONT_SPACING * scale : 0;
}

/* The picture's pixel at the board's x, y (columns past 0 and 319 when the picture is wider) */
#define PIXEL(frame, x, y)  ((frame)->pixels[(y) * (frame)->stride + (frame)->left + (x)])

static void put(console_picture *frame, int x, int y, uint32_t colour)
{
	if (x >= PICTURE_FIRST(frame) && x < PICTURE_END(frame) && y >= 0 && y < HEIGHT)
		PIXEL(frame, x, y) = colour;
}

int draw_centred_x(const console_picture *frame, int width)
{
	return PICTURE_FIRST(frame) + (PICTURE_END(frame) - PICTURE_FIRST(frame) - width) / 2;
}

static void put_block(console_picture *frame, int x, int y, int scale, uint32_t colour)
{
	for (int dy = 0; dy < scale; dy++)
		for (int dx = 0; dx < scale; dx++)
			put(frame, x + dx, y + dy, colour);
}

static void letters(console_picture *frame, int x, int y, int scale, uint32_t colour, const char *text)
{
	for (; *text; text++)
	{
		int index, left, top;

		if (*text < FONT_FIRST || *text > FONT_LAST)
			continue;
		index = *text - FONT_FIRST;
		left = (index % FONT_ROW) * FONT_CELL;
		top = (index / FONT_ROW) * FONT_CELL;
		for (int row = font.top; row < FONT_CELL; row++)
			for (int column = 0; column < font.widths[index]; column++)
				if (font.lit[top + row][left + column])
					put_block(frame, x + column * scale, y + (row - font.top) * scale, scale, colour);
		x += (font.widths[index] + FONT_SPACING) * scale;
	}
}

void draw_text(console_picture *frame, int x, int y, int scale, uint32_t colour, const char *text)
{
	if (!font.loaded)
		return;
	letters(frame, x + scale, y + scale, scale, SHADOW, text);
	letters(frame, x, y, scale, colour, text);
}

/* ---- rectangles ---- */

static uint32_t darkened(uint32_t colour, int keep)
{
	const uint32_t red = ((colour >> 16) & 0xFFu) * (uint32_t)keep >> 8;
	const uint32_t green = ((colour >> 8) & 0xFFu) * (uint32_t)keep >> 8;
	const uint32_t blue = (colour & 0xFFu) * (uint32_t)keep >> 8;

	return red << 16 | green << 8 | blue;
}

void draw_shade(console_picture *frame, int x, int y, int width, int height, int keep)
{
	const int first = x < PICTURE_FIRST(frame) ? PICTURE_FIRST(frame) : x;

	for (int row = y < 0 ? 0 : y; row < y + height && row < HEIGHT; row++)
		for (int column = first; column < x + width && column < PICTURE_END(frame); column++)
			PIXEL(frame, column, row) = darkened(PIXEL(frame, column, row), keep);
}

void draw_fill(console_picture *frame, int x, int y, int width, int height, uint32_t colour)
{
	const int first = x < PICTURE_FIRST(frame) ? PICTURE_FIRST(frame) : x;

	for (int row = y < 0 ? 0 : y; row < y + height && row < HEIGHT; row++)
		for (int column = first; column < x + width && column < PICTURE_END(frame); column++)
			PIXEL(frame, column, row) = colour;
}

void draw_outline(console_picture *frame, int x, int y, int width, int height, int thickness, uint32_t colour)
{
	draw_fill(frame, x, y, width, thickness, colour);
	draw_fill(frame, x, y + height - thickness, width, thickness, colour);
	draw_fill(frame, x, y, thickness, height, colour);
	draw_fill(frame, x + width - thickness, y, thickness, height, colour);
}

/* ---- the game's own letters for the menus ---- */

/* A tile's pen (the K052109's: four planes, a byte each for a row of 8, as runtime/src/video.c reads them) */
static int tile_pen(int code, int x, int y)
{
	const uint8_t *data = &game_tiles[(size_t)(code % TILES) * 32u + (size_t)y * 4u];
	const int column = x & 7;

	return ((data[3] >> (7 - column)) & 1) << 3 | ((data[2] >> (7 - column)) & 1) << 2
		| ((data[1] >> (7 - column)) & 1) << 1 | ((data[0] >> (7 - column)) & 1);
}

/* The fixed layer's tile of a character, as the game's texts write them; -1 for a space or one the font has not */
static int game_glyph(char character)
{
	if (character >= 'a' && character <= 'z')
		character = (char)(character - 'a' + 'A');
	if ((character >= '0' && character <= '9') || (character >= 'A' && character <= 'Z'))
		return character - (int)TEXT_FIRST;
	switch (character)
	{
		case '.': return 0x77;
		case ',': return 0x74;
		case '-': return 0x79;
		case '/': return 0x78;
		case '(': return 0x7B;
		case ')': return 0x7C;
		case '\'': return 0x75;
		case '?': return 0x0E;
		case '+': return 0x70;
		case '[': return 0x2B;    /* (the copyright sign: the game's texts write it '[', "[@1991@KONAMI") */
		case '@': return OWN_AT;
		case ':': return OWN_COLON;
		case '<': return OWN_LESS;
		case '>': return OWN_GREATER;
		default: return -1;
	}
}

/* Letters of the console's own in the font's manner, which it has not: '@' (its '@' is its space), ':', '<' and '>'
   (a value LEFT and RIGHT change, CONTROLS' PLAYER); their pens as the font's letters have them, 1 the letter, 2 its
   shadow (tmnt-port's) */
static const char *const own_glyphs[4][8] = {
	{
		".11111..",
		"1122211.",
		"11.111.1",
		"11.1211.",
		"11.111.1",
		"112.222.",
		".11111..",
		"..22222.",
	},
	{
		"........",
		"..11....",
		"..112...",
		"...22...",
		"..11....",
		"..112...",
		"...22...",
		"........",
	},
	{
		"....11..",
		"...1122.",
		"..1122..",
		".1122...",
		"..11....",
		"...11...",
		"....11..",
		".....22.",
	},
	{
		".11.....",
		"..11....",
		"...11...",
		"....11..",
		"...1122.",
		"..1122..",
		".1122...",
		"..22....",
	},
};

static int letter_pen(int code, int x, int y)
{
	if (code >= OWN_AT)
	{
		const char pen = own_glyphs[code - OWN_AT][y][x];

		return pen == '1' ? LETTER_PEN : pen == '2' ? SHADOW_PEN : 0;
	}
	return tile_pen(code, x, y);
}

int draw_game_letters_width(const char *text, int scale)
{
	return (int)strlen(text) * GAME_LETTER * scale;
}

void draw_game_letters(console_picture *frame, int x, int y, int scale, uint32_t colour, const char *text)
{
	const uint32_t shadow = darkened(colour, SHADOW_KEEP);

	for (; *text; text++, x += GAME_LETTER * scale)
	{
		const int code = game_glyph(*text);

		if (code < 0)
			continue;
		for (int row = 0; row < 8; row++)
			for (int column = 0; column < 8; column++)
			{
				const int pen = letter_pen(code, column, row);

				if (pen == LETTER_PEN)
					put_block(frame, x + column * scale, y + row * scale, scale, colour);
				else if (pen == SHADOW_PEN)
					put_block(frame, x + column * scale, y + row * scale, scale, shadow);
			}
	}
}

void draw_game_letters_centred(console_picture *frame, int y, int scale, uint32_t colour, const char *text)
{
	draw_game_letters(frame, draw_centred_x(frame, draw_game_letters_width(text, scale)), y, scale, colour, text);
}

/* ---- the signature's small letters ---- */

/* A small font of the console's own, 5 pixels high, for "@cheviman" and the version under the title (tmnt-port's):
   the letters they need, '#' lit; a shadow under them */
static const struct
{
	char character;
	const char *rows[5];
} small_letters[] = {
	{ '@', { ".###.", "#.#.#", "#.###", "#....", ".###." } },
	{ 'c', { "...", ".##", "#..", "#..", ".##" } },
	{ 'h', { "#..", "#..", "##.", "#.#", "#.#" } },
	{ 'e', { "...", ".#.", "###", "#..", ".##" } },
	{ 'v', { "...", "#.#", "#.#", "#.#", ".#." } },
	{ 'i', { "#", ".", "#", "#", "#" } },
	{ 'm', { ".....", "##.#.", "#.#.#", "#.#.#", "#.#.#" } },
	{ 'a', { "...", ".##", "#.#", "#.#", ".##" } },
	{ 'n', { "...", "##.", "#.#", "#.#", "#.#" } },
	/* (the version after it: V0.1.0, and a testing build's -TEST and number) */
	{ 'V', { "#.#", "#.#", "#.#", "#.#", ".#." } },
	{ 'T', { "###", ".#.", ".#.", ".#.", ".#." } },
	{ 'E', { "###", "#..", "##.", "#..", "###" } },
	{ 'S', { ".##", "#..", ".#.", "..#", "##." } },
	{ '0', { ".#.", "#.#", "#.#", "#.#", ".#." } },
	{ '1', { ".#", "##", ".#", ".#", ".#" } },
	{ '2', { "##.", "..#", ".#.", "#..", "###" } },
	{ '3', { "##.", "..#", ".#.", "..#", "##." } },
	{ '4', { "#.#", "#.#", "###", "..#", "..#" } },
	{ '5', { "###", "#..", "##.", "..#", "##." } },
	{ '6', { ".##", "#..", "##.", "#.#", ".#." } },
	{ '7', { "###", "..#", ".#.", ".#.", ".#." } },
	{ '8', { ".#.", "#.#", ".#.", "#.#", ".#." } },
	{ '9', { ".#.", "#.#", ".##", "..#", "##." } },
	{ '.', { ".", ".", ".", ".", "#" } },
	{ '-', { "..", "..", "##", "..", ".." } },
};

static int small_letter(char character)
{
	for (int index = 0; index < (int)(sizeof(small_letters) / sizeof(small_letters[0])); index++)
		if (small_letters[index].character == character)
			return index;
	return -1;
}

static int small_letters_width(const char *text)
{
	int width = 0;

	for (; *text; text++)
	{
		const int index = small_letter(*text);

		width += (index < 0 ? 2 : (int)strlen(small_letters[index].rows[0])) + 1;
	}
	return width > 0 ? width - 1 : 0;
}

void draw_small_letters_centred(console_picture *frame, int y, uint32_t colour, const char *text)
{
	int x = draw_centred_x(frame, small_letters_width(text));

	for (; *text; text++)
	{
		const int index = small_letter(*text);
		int width;

		if (index < 0)
		{
			x += 3;
			continue;
		}
		width = (int)strlen(small_letters[index].rows[0]);
		for (int pass = 0; pass < 2; pass++)
			for (int row = 0; row < 5; row++)
				for (int column = 0; column < width; column++)
					if (small_letters[index].rows[row][column] == '#')
						put(frame, x + column + (pass ? 0 : 1), y + row + (pass ? 0 : 1), pass ? colour : SHADOW);
		x += width + 1;
	}
}

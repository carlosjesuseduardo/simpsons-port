/**
 *    Name:        assets_check.c
 *    Description: the assets pack's check: the pack of the assets folder and the pack of the player's ROM set (through
 *                 the assets' layout) the same, byte for byte; and the layout's writer
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Usage: simpsons_assets_check [--assets FOLDER] [--rom ROM_SET] [--out FOLDER]
 *           simpsons_assets_check --write-layout [--assets FOLDER] [FILE]
 *
 *    The check loads the assets folder (--assets, assets/ by default: what tools/extract_assets.py writes) as the game
 *    loads it and writes its pack (OUT/assets_folder.assets; --out, build/ by default); loads that pack back and
 *    writes it again (OUT/assets_again.assets); loads the ROM set (--rom, rom/simpsons.zip by default) through the
 *    layout and writes its pack (OUT/assets_rom.assets); and compares the three byte for byte, saying where the first
 *    difference is (the region, and the place in it). 0 when the three are the same.
 *    --write-layout loads the folder and writes runtime/src/assets_layout.c (or FILE) from what the load gave
 *    (game_assets_loaded_spans): places and lengths alone, no byte of the ROM. The file is written only when its
 *    spans change. A development tool: the game's executables do not carry it.
 */
#include "game/assets.h"
#include "game/data.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define USAGE "Usage: simpsons_assets_check [--assets FOLDER] [--rom ROM_SET] [--out FOLDER]\n" \
	"       simpsons_assets_check --write-layout [--assets FOLDER] [FILE]\n"
#define LAYOUT_FILE     "runtime/src/assets_layout.c"
#define LAYOUT_CREATED  "13-11-2026"
#define PATH_SIZE       1024
#define SPANS_A_LINE    5

/* the memories the game loads into (runtime/src/game.c's game_load: the programs' data as its pieces only) */
static const game_assets_memory GAME_MEMORIES[GAME_ASSETS_REGIONS] = {
	{ NULL, NULL }, { NULL, NULL }, { game_tiles, NULL }, { game_sprites, NULL }, { game_samples, NULL },
	{ game_eeprom, NULL } };

static double seconds_since(clock_t started)
{
	return (double)(clock() - started) / CLOCKS_PER_SEC;
}

static unsigned char *read_whole(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	unsigned char *data = NULL;
	long length = 0;

	if (!file)
		return NULL;
	if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) >= 0 && fseek(file, 0, SEEK_SET) == 0
		&& (data = malloc((size_t)length + 1)) != NULL && fread(data, 1, (size_t)length, file) != (size_t)length)
	{
		free(data);
		data = NULL;
	}
	if (data)
	{
		data[length] = 0;
		*size = (size_t)length;
	}
	fclose(file);
	return data;
}

/* ---- the layout's writer ---- */

typedef struct
{
	char *text;
	size_t size, capacity;
} text_buffer;

static void add(text_buffer *buffer, const char *format, ...)
{
	va_list arguments;
	int length;

	va_start(arguments, format);
	length = vsnprintf(NULL, 0, format, arguments);
	va_end(arguments);
	if (buffer->size + (size_t)length + 1 > buffer->capacity)
	{
		buffer->capacity = (buffer->size + (size_t)length + 1) * 2;
		if (!(buffer->text = realloc(buffer->text, buffer->capacity)))
		{
			fprintf(stderr, "no memory\n");
			exit(2);
		}
	}
	va_start(arguments, format);
	vsnprintf(buffer->text + buffer->size, (size_t)length + 1, format, arguments);
	va_end(arguments);
	buffer->size += (size_t)length;
}

/* what each region's spans are */
static const char *const REGION_TEXTS[GAME_ASSETS_REGIONS] = {
	"the 053248 program's data: its pieces, by their places in its 512 KB (a bank's window resolved)",
	"the Z80 program's data: its pieces, by their places in its region",
	"the K052109's tiles: the stretches the sheets give",
	"the K053246's sprite cells: the stretches the sheets give",
	"the K053260's samples: the stretches the samples give",
	"the ER5911 as the board leaves the factory" };

static const char LAYOUT_HEADER[] =
	"/**\n"
	" *    Name:        assets_layout.c\n"
	" *    Description: the assets' layout: where the bytes the assets give are, region by region; places and lengths\n"
	" *                 only, no byte of the ROM\n"
	" *    Version:     0.1.0\n"
	" *    Created:     " LAYOUT_CREATED "\n"
	" *    Modified:    %s\n"
	" *    Author:      Sebastian Toyos\n"
	" *\n"
	" *    license:BSD-3-Clause\n"
	" *    Written by simpsons_assets_check --write-layout (ports/headless/assets_check.c) from the assets folder\n"
	" *    tools/extract_assets.py writes, as runtime/src/assets.c loads it (game_assets_loaded_spans): the given\n"
	" *    stretches of the tiles, the sprites, the samples and the EEPROM, and the pieces of both programs' data by the\n"
	" *    places the programs know them by. The assets pack holds these spans (runtime/src/assets_pack.c), its version\n"
	" *    is theirs with the tables the C reads by name, and the ROM set's load takes their bytes from the player's set.\n"
	" *    Write it again when what the assets give changes (the extractor, symbols.txt, sound_symbols.txt):\n"
	" *    simpsons_assets_check says when the folder's layout is not this one. It is written only when its spans\n"
	" *    change.\n"
	" */\n";

/* The layout's body (all but its header) from what the last load gave */
static void layout_body(text_buffer *body)
{
	add(body, "#include \"assets_layout.h\"\n");
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const game_assets_span *spans;
		const size_t count = game_assets_loaded_spans((game_assets_region)region, &spans);

		if (!count)
			continue;
		add(body, "\n/* %s: %zu */\nstatic const game_assets_span %s_spans[] = {\n", REGION_TEXTS[region], count,
			game_assets_name((game_assets_region)region));
		for (size_t index = 0; index < count; index++)
			add(body, "%s{ 0x%06X, %u },%s", index % SPANS_A_LINE ? " " : "\t", (unsigned)spans[index].place,
				(unsigned)spans[index].length, index % SPANS_A_LINE == SPANS_A_LINE - 1 || index + 1 == count ? "\n" : "");
		add(body, "};\n");
	}
	add(body, "\nconst assets_layout_region assets_layout[GAME_ASSETS_REGIONS] = {\n");
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const game_assets_span *spans;
		const size_t count = game_assets_loaded_spans((game_assets_region)region, &spans);

		if (count)
			add(body, "\t{ %s_spans, %zu },\n", game_assets_name((game_assets_region)region), count);
		else
			add(body, "\t{ NULL, 0 },\n");
	}
	add(body, "};\n");
}

/* The file's body as it is (after its header, without '\r'), or NULL */
static char *written_body(const char *path)
{
	size_t size = 0, kept = 0;
	char *text = (char *)read_whole(path, &size), *body;

	if (!text)
		return NULL;
	for (size_t index = 0; index < size; index++)
		if (text[index] != '\r')
			text[kept++] = text[index];
	text[kept] = 0;
	if (!(body = strstr(text, "\n */\n")))
	{
		free(text);
		return NULL;
	}
	memmove(text, body + 5, strlen(body + 5) + 1);
	return text;
}

static int write_layout(const char *folder, const char *path)
{
	text_buffer body = { NULL, 0, 0 };
	const char *problem;
	char *old, today[16];
	const time_t now = time(NULL);
	FILE *file;
	int same;

	if ((problem = game_assets_load_into(folder, GAME_MEMORIES)) != NULL)
	{
		fprintf(stderr, "The assets in %s cannot be read: %s\n", folder, problem);
		return 1;
	}
	layout_body(&body);
	for (int region = 0; region < GAME_ASSETS_REGIONS; region++)
	{
		const game_assets_span *spans;
		const size_t count = game_assets_loaded_spans((game_assets_region)region, &spans);
		size_t bytes = 0;

		for (size_t index = 0; index < count; index++)
			bytes += spans[index].length;
		printf("%-8s %6zu spans, %8zu bytes\n", game_assets_name((game_assets_region)region), count, bytes);
	}
	old = written_body(path);
	same = old && !strcmp(old, body.text);
	free(old);
	if (same)
	{
		printf("%s: as it was\n", path);
		free(body.text);
		return 0;
	}
	strftime(today, sizeof(today), "%d-%m-%Y", localtime(&now));
	if (!(file = fopen(path, "wb")))
	{
		fprintf(stderr, "%s: cannot be written\n", path);
		free(body.text);
		return 1;
	}
	fprintf(file, LAYOUT_HEADER, today);
	fwrite(body.text, 1, body.size, file);
	same = fclose(file) == 0;
	free(body.text);
	if (!same)
	{
		fprintf(stderr, "%s: cannot be written\n", path);
		return 1;
	}
	printf("%s: written (build the game again for its version: the one now built is %08X)\n", path,
		(unsigned)game_assets_version());
	return 0;
}

/* ---- the check ---- */

/* Where a byte of a pack is: its header, a region's count, a span's place or length, a byte of a span, padding */
static void describe(const unsigned char *pack, size_t size, size_t offset, char *text, size_t text_size)
{
	size_t at = 12;

	if (offset < at)
	{
		snprintf(text, text_size, "the header (%s)", offset < 4 ? "magic" : offset < 8 ? "format" : "version");
		return;
	}
	for (int region = 0; region < GAME_ASSETS_REGIONS && at + 4 <= size; region++)
	{
		const char *name = game_assets_name((game_assets_region)region);
		const uint32_t count = pack[at] | (uint32_t)pack[at + 1] << 8 | (uint32_t)pack[at + 2] << 16
			| (uint32_t)pack[at + 3] << 24;
		size_t bytes = 0, spans = at + 4, start;

		if (offset < at + 4)
		{
			snprintf(text, text_size, "the %s region's count of spans", name);
			return;
		}
		if (spans + 8u * (size_t)count > size)
			break;
		if (offset < spans + 8u * (size_t)count)
		{
			snprintf(text, text_size, "the %s region's span %zu (its %s)", name, (offset - spans) / 8u,
				(offset - spans) % 8u < 4 ? "place" : "length");
			return;
		}
		start = spans + 8u * (size_t)count;
		for (uint32_t index = 0; index < count; index++)
		{
			const unsigned char *span = pack + spans + 8u * index;
			const uint32_t place = span[0] | (uint32_t)span[1] << 8 | (uint32_t)span[2] << 16 | (uint32_t)span[3] << 24;
			const uint32_t length = span[4] | (uint32_t)span[5] << 8 | (uint32_t)span[6] << 16 | (uint32_t)span[7] << 24;

			if (offset < start + bytes + length)
			{
				snprintf(text, text_size, "the %s region's byte at %06X (span %u)", name,
					(unsigned)(place + (offset - start - bytes)), (unsigned)index);
				return;
			}
			bytes += length;
		}
		at = start + bytes + (4u - bytes % 4u) % 4u;
		if (offset < at)
		{
			snprintf(text, text_size, "the %s region's padding", name);
			return;
		}
	}
	snprintf(text, text_size, "past the pack's regions");
}

/* Two packs byte for byte: 1 when they are the same; else says where they first differ */
static int same_packs(const char *one_path, const char *other_path)
{
	size_t one_size = 0, other_size = 0, at = 0;
	unsigned char *one = read_whole(one_path, &one_size), *other = read_whole(other_path, &other_size);
	int same;

	if (!one || !other)
	{
		printf("  %s cannot be read\n", one ? other_path : one_path);
		free(one);
		free(other);
		return 0;
	}
	while (at < one_size && at < other_size && one[at] == other[at])
		at++;
	same = one_size == other_size && at == one_size;
	if (same)
		printf("  %s = %s: %zu bytes, the same\n", one_path, other_path, one_size);
	else
	{
		char where[256];

		describe(one, one_size, at, where, sizeof(where));
		printf("  %s (%zu bytes) and %s (%zu bytes) differ: the first difference at %zu, %s\n", one_path, one_size,
			other_path, other_size, at, where);
	}
	free(one);
	free(other);
	return same;
}

static void make_folder(const char *path)
{
#ifdef _WIN32
	_mkdir(path);
#else
	mkdir(path, 0777);
#endif
}

static int written(const char *what, const char *problem, const char *path, clock_t started)
{
	if (problem)
	{
		fprintf(stderr, "%s: %s\n", what, problem);
		return 0;
	}
	printf("%s: %s, in %.1f s\n", what, path, seconds_since(started));
	return 1;
}

int main(int argc, char **argv)
{
	const char *folder = "assets", *rom = "rom/simpsons.zip", *out = "build", *layout = NULL, *problem;
	char folder_pack[PATH_SIZE], again_pack[PATH_SIZE], rom_pack[PATH_SIZE];
	int write_layout_mode = 0, same;
	clock_t started;

	for (int index = 1; index < argc; index++)
	{
		const int has_value = index + 1 < argc;

		if (!strcmp(argv[index], "--assets") && has_value)
			folder = argv[++index];
		else if (!strcmp(argv[index], "--rom") && has_value)
			rom = argv[++index];
		else if (!strcmp(argv[index], "--out") && has_value)
			out = argv[++index];
		else if (!strcmp(argv[index], "--write-layout"))
			write_layout_mode = 1;
		else if (write_layout_mode && !layout && argv[index][0] != '-')
			layout = argv[index];
		else
		{
			fprintf(stderr, "%s", USAGE);
			return 2;
		}
	}
	if (write_layout_mode)
		return write_layout(folder, layout ? layout : LAYOUT_FILE);
	make_folder(out);
	snprintf(folder_pack, sizeof(folder_pack), "%s/assets_folder.assets", out);
	snprintf(again_pack, sizeof(again_pack), "%s/assets_again.assets", out);
	snprintf(rom_pack, sizeof(rom_pack), "%s/assets_rom.assets", out);
	printf("this build's assets: version %08X\n", (unsigned)game_assets_version());

	started = clock();
	problem = game_assets_load_into(folder, GAME_MEMORIES);
	if (!problem)
		problem = game_assets_pack_write(folder_pack);
	if (!written("the folder's pack", problem, folder_pack, started))
		return 1;

	started = clock();
	problem = game_assets_pack_load_into(folder_pack, GAME_MEMORIES);
	if (!problem)
		problem = game_assets_pack_write(again_pack);
	if (!written("the folder's pack loaded and written again", problem, again_pack, started))
		return 1;

	started = clock();
	problem = game_assets_rom_set_load_into(rom, GAME_MEMORIES);
	if (!problem)
		problem = game_assets_pack_write(rom_pack);
	if (!written("the ROM set's pack", problem, rom_pack, started))
		return 1;

	same = same_packs(folder_pack, again_pack);
	same &= same_packs(folder_pack, rom_pack);
	printf(same ? "the packs are the same\n" : "the packs differ\n");
	return same ? 0 : 1;
}

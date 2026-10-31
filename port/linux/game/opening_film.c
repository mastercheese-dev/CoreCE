/* The match opening plays a film of the original loading screen. The
retail disc has no loading.tga, which is the only picture the sweep
can draw. The frames ship with the game, and a copy already in
maps/opening_frames.bin is used when it is there. */
#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "interface/progress_bar.h"
#include "platform.h"
#include "opening_film.h"

#include "../../third_party/zlib/zlib_prefixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct opening_film_frame
{
	unsigned long size;
	unsigned char *bytes;
};

static int film_state;
static long film_width;
static long film_height;
static long film_count;
static unsigned long film_duration_ms;
static struct opening_film_frame *film_frames;
static unsigned char *film_pixels;
static long film_decoded;

/* A campaign load. 0 is off, 1 is the film, 2 is the fade off the level. */
#define LOAD_STEP_MS 48UL
#define LOAD_FADE_MS 400UL
static int load_state;
static int load_armed;
static unsigned long load_shown_ms;
static unsigned long load_last_ms;
static unsigned long load_fade_ms;
static unsigned long load_fade_shown;

static int read_u32(FILE *file, unsigned long *value)
{
	unsigned char bytes[4];

	if (fread(bytes, 1, 4, file) != 4)
		return 0;
	*value = (unsigned long)bytes[0] | ((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) | ((unsigned long)bytes[3] << 24);
	return 1;
}

static void film_release(void)
{
	long index;

	if (film_frames)
	{
		for (index = 0; index < film_count; index++)
			free(film_frames[index].bytes);
		free(film_frames);
	}
	free(film_pixels);
	film_frames = NULL;
	film_pixels = NULL;
	film_count = 0;
	film_decoded = -1;
}

static int film_load(void)
{
	FILE *file;
	unsigned long version;
	unsigned long width;
	unsigned long height;
	unsigned long count;
	unsigned long duration_ms;
	unsigned long index;
	char magic[4];

	file = platform_open_opening_film();
	if (!file)
		return 0;
	if (fread(magic, 1, 4, file) != 4 || memcmp(magic, "HLOF", 4) != 0 ||
		!read_u32(file, &version) || version != 1 ||
		!read_u32(file, &width) || !read_u32(file, &height) ||
		!read_u32(file, &count) || !read_u32(file, &duration_ms) ||
		width < 1 || height < 1 || width > 1920 || height > 1080 ||
		count < 1 || count > 120 || duration_ms < 1)
	{
		fclose(file);
		platform_log("opening film: maps/opening_frames.bin is not a film");
		return 0;
	}
	film_frames = calloc(count, sizeof(*film_frames));
	film_pixels = malloc((size_t)width * (size_t)height * 4);
	if (!film_frames || !film_pixels)
	{
		fclose(file);
		film_release();
		return 0;
	}
	for (index = 0; index < count; index++)
	{
		unsigned long size;

		if (!read_u32(file, &size) || size < 1 || size > 8 * 1024 * 1024)
		{
			fclose(file);
			film_release();
			platform_log("opening film: a frame could not be read");
			return 0;
		}
		film_frames[index].bytes = malloc(size);
		film_frames[index].size = size;
		if (!film_frames[index].bytes ||
			fread(film_frames[index].bytes, 1, size, file) != size)
		{
			fclose(file);
			film_release();
			platform_log("opening film: a frame could not be read");
			return 0;
		}
	}
	fclose(file);
	film_width = (long)width;
	film_height = (long)height;
	film_count = (long)count;
	film_duration_ms = duration_ms;
	film_decoded = -1;
	return 1;
}

static int film_decode(long index)
{
	z_stream stream;
	int result;

	if (index == film_decoded)
		return 1;
	memset(&stream, 0, sizeof(stream));
	if (inflateInit(&stream) != Z_OK)
		return 0;
	stream.next_in = film_frames[index].bytes;
	stream.avail_in = (uInt)film_frames[index].size;
	stream.next_out = film_pixels;
	stream.avail_out = (uInt)((size_t)film_width * (size_t)film_height * 4);
	result = inflate(&stream, Z_FINISH);
	inflateEnd(&stream);
	if (result != Z_STREAM_END)
		return 0;
	film_decoded = index;
	return 1;
}

unsigned long opening_film_duration_ms(void)
{
	if (film_state == 0)
		film_state = film_load() ? 1 : 2;
	if (film_state != 1)
		return 0;
	return film_duration_ms;
}

unsigned long opening_film_step_ms(void)
{
	unsigned long duration = opening_film_duration_ms();
	unsigned long count = film_count > 0 ? (unsigned long)film_count : 0;
	unsigned long step;

	if (duration == 0 || count == 0)
		return 42UL;
	step = duration / count;
	if (step == 0)
		return 1UL;
	return step;
}

boolean opening_film_show(unsigned long shown_ms, real opacity)
{
	unsigned long t;
	long index;

	if (film_state == 0)
		film_state = film_load() ? 1 : 2;
	if (film_state != 1)
		return FALSE;
	t = shown_ms;
	if (t > film_duration_ms)
		t = film_duration_ms;
	index = (long)((t * (unsigned long)(film_count - 1)) / film_duration_ms);
	if (index < 0)
		index = 0;
	if (index >= film_count)
		index = film_count - 1;
	if (!film_decode(index))
		return FALSE;
	progress_bar_show_picture(film_width, film_height, film_pixels, opacity);
	return TRUE;
}

void opening_film_load_arm(void)
{
	load_armed = 1;
}

boolean opening_film_load_take_arm(void)
{
	boolean armed;

	armed = load_armed ? TRUE : FALSE;
	load_armed = 0;
	return armed;
}

void opening_film_load_cancel(void)
{
	load_state = 0;
	load_armed = 0;
	load_shown_ms = 0;
	load_last_ms = 0;
	load_fade_ms = 0;
	load_fade_shown = 0;
}

void opening_film_load_begin(void)
{
	load_state = 1;
	load_armed = 0;
	load_shown_ms = 0;
	load_last_ms = 0;
	load_fade_ms = 0;
	load_fade_shown = 0;
	platform_log("opening film: a match load plays the film");
}

boolean opening_film_load_active(void)
{
	return load_state == 1 || load_state == 2;
}

/* Presented frames advance the film. A stall while the map is built does
   not skip the rest of the animation. */
static void load_advance(void)
{
	unsigned long now;
	unsigned long step;

	now = system_milliseconds();
	if (load_last_ms == 0)
	{
		load_last_ms = now;
		return;
	}
	step = now - load_last_ms;
	load_last_ms = now;
	if (step > LOAD_STEP_MS)
		step = LOAD_STEP_MS;
	load_shown_ms += step;
}

boolean opening_film_load_frame(void)
{
	if (load_state != 1)
		return FALSE;
	load_advance();
	/* a frame that does not draw stays on this film. the original sweep
	does not take the screen. */
	opening_film_show(load_shown_ms, 1.f);
	return TRUE;
}

void opening_film_load_overlay(void)
{
	unsigned long step;
	unsigned long now;
	real alpha;

	if (load_state == 0)
		return;
	load_advance();
	if (load_state == 1)
	{
		/* The last moment of the film plays while it fades off the level. */
		if (film_state != 1 || load_shown_ms + LOAD_FADE_MS < film_duration_ms)
		{
			if (opening_film_show(load_shown_ms, 1.f))
				return;
			opening_film_load_cancel();
			return;
		}
		load_state = 2;
		load_fade_ms = 0;
		load_fade_shown = 0;
	}
	if (load_state != 2)
		return;
	now = system_milliseconds();
	if (load_fade_ms == 0)
		load_fade_ms = now;
	else
	{
		step = now - load_fade_ms;
		if (step > LOAD_STEP_MS)
			step = LOAD_STEP_MS;
		load_fade_shown += step;
		load_fade_ms = now;
	}
	if (load_fade_shown >= LOAD_FADE_MS)
	{
		opening_film_load_cancel();
		progress_bar_end();
		return;
	}
	alpha = 1.f - (real)load_fade_shown / (real)LOAD_FADE_MS;
	if (alpha > 0.998f)
		alpha = 0.998f;
	if (!opening_film_show(load_shown_ms, alpha))
	{
		opening_film_load_cancel();
		progress_bar_end();
	}
}

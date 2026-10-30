/*
UPDATE_CHOICE.C

Picks a release out of the JSON GitHub returns. The objects are walked
with strings skipped, so a note in a release cannot look like a tag.
*/

#include "update_choice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *json_string_end(const char *open_quote)
{
	const char *p = open_quote + 1;

	while (*p)
	{
		if (*p == '\\')
		{
			if (!p[1])
				return NULL;
			p += 2;
			continue;
		}
		if (*p == '"')
			return p;
		p++;
	}
	return NULL;
}

/* the byte after the matching close, the opening brace included in the count */
static const char *json_object_end(const char *open)
{
	int depth = 0;
	const char *p = open;

	while (*p)
	{
		if (*p == '"')
		{
			const char *end = json_string_end(p);

			if (!end)
				return NULL;
			p = end + 1;
			continue;
		}
		if (*p == '{')
			depth++;
		else if (*p == '}')
		{
			depth--;
			if (depth == 0)
				return p + 1;
		}
		p++;
	}
	return NULL;
}

static int slice_has_string(const char *start, const char *end, const char *value)
{
	size_t length = strlen(value);
	const char *p = start;

	while (p < end)
	{
		if (*p == '"')
		{
			const char *close = json_string_end(p);

			if (!close || close >= end)
				return 0;
			if ((size_t)(close - (p + 1)) == length && !memcmp(p + 1, value, length))
				return 1;
			p = close + 1;
			continue;
		}
		p++;
	}
	return 0;
}

/* the point after a depth-1 key's closing quote, else null */
static const char *key_at_depth_one(const char *start, const char *end, const char *key)
{
	int depth = 0;
	size_t length = strlen(key);
	const char *p = start;

	while (p < end)
	{
		if (*p == '"')
		{
			const char *close = json_string_end(p);

			if (!close || close >= end)
				return NULL;
			if (depth == 1 && (size_t)(close - (p + 1)) == length && !memcmp(p + 1, key, length))
				return close + 1;
			p = close + 1;
			continue;
		}
		if (*p == '{' || *p == '[')
			depth++;
		else if (*p == '}' || *p == ']')
			depth--;
		p++;
	}
	return NULL;
}

static const char *skip_space(const char *p, const char *end)
{
	while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
		p++;
	return p;
}

static int field_bool(const char *start, const char *end, const char *key, int *value)
{
	const char *p = key_at_depth_one(start, end, key);

	if (!p)
		return 0;
	p = skip_space(p, end);
	if (p >= end || *p != ':')
		return 0;
	p = skip_space(p + 1, end);
	if (p + 4 <= end && !memcmp(p, "true", 4))
	{
		*value = 1;
		return 1;
	}
	if (p + 5 <= end && !memcmp(p, "false", 5))
	{
		*value = 0;
		return 1;
	}
	return 0;
}

static int field_string(const char *start, const char *end, const char *key, char *out, size_t out_size)
{
	const char *p = key_at_depth_one(start, end, key);
	const char *close;
	size_t length;

	if (!p || out_size == 0)
		return 0;
	p = skip_space(p, end);
	if (p >= end || *p != ':')
		return 0;
	p = skip_space(p + 1, end);
	if (p >= end || *p != '"')
		return 0;
	close = json_string_end(p);
	if (!close || close >= end)
		return 0;
	length = (size_t)(close - (p + 1));
	if (length >= out_size)
		return 0;
	memcpy(out, p + 1, length);
	out[length] = 0;
	return 1;
}

/* build-12 or testing-12, and nothing after the digits. 0 when it is not. */
static long channel_number(const char *tag, const char *prefix)
{
	size_t length = strlen(prefix);
	char *end;
	long number;

	if (strncmp(tag, prefix, length) || !tag[length])
		return 0;
	number = strtol(tag + length, &end, 10);
	if (end == tag + length || *end || number <= 0)
		return 0;
	return number;
}

static void consider_object(const char *start, const char *end, const char *asset, int want_testing,
	long *best, char *best_tag, size_t tag_size)
{
	char tag[64];
	int draft = 0;
	int prerelease = 0;
	long number;
	const char *prefix;

	if (!field_string(start, end, "tag_name", tag, sizeof(tag)))
		return;
	field_bool(start, end, "draft", &draft);
	field_bool(start, end, "prerelease", &prerelease);
	if (draft)
		return;
	prefix = want_testing ? "testing-" : "build-";
	number = channel_number(tag, prefix);
	if (!number || number <= *best)
		return;
	/* testing stays a prerelease, so a normal release of that tag cannot
	become the release GitHub calls latest and pull main clients across */
	if (want_testing)
	{
		if (!prerelease)
			return;
	}
	else if (prerelease)
	{
		return;
	}
	if (!slice_has_string(start, end, asset))
		return;
	*best = number;
	if (tag_size)
		snprintf(best_tag, tag_size, "%s", tag);
}

static void scan_value(const char *json, const char *asset, int want_testing, long *best, char *best_tag, size_t tag_size)
{
	const char *p;

	if (!json)
		return;
	p = skip_space(json, json + strlen(json));
	if (*p == '{')
	{
		const char *end = json_object_end(p);

		if (end)
			consider_object(p, end, asset, want_testing, best, best_tag, tag_size);
		return;
	}
	if (*p != '[')
		return;
	p++;
	while (*p)
	{
		p = skip_space(p, p + strlen(p));
		if (*p == '{')
		{
			const char *end = json_object_end(p);

			if (!end)
				return;
			consider_object(p, end, asset, want_testing, best, best_tag, tag_size);
			p = end;
			continue;
		}
		if (*p == ']')
			return;
		if (!*p)
			return;
		p++;
	}
}

int update_choice_select(const char *list_json, const char *latest_json, const char *asset,
	int want_testing, int on_testing, long current, struct update_choice *choice)
{
	long best = 0;
	char tag[64];

	tag[0] = 0;
	if (!asset || !choice)
		return 0;
	scan_value(list_json, asset, want_testing, &best, tag, sizeof(tag));
	scan_value(latest_json, asset, want_testing, &best, tag, sizeof(tag));
	if (best <= 0 || !tag[0])
		return 0;
	if (want_testing && on_testing && best <= current)
		return 0;
	if (!want_testing && !on_testing && best <= current)
		return 0;
	choice->number = best;
	snprintf(choice->tag, sizeof(choice->tag), "%s", tag);
	return 1;
}

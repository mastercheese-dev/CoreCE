/*
MATCHMAKING.C

The matchmaking service. Its broker is not the one invites use (p2p_signal.c):
that list is left as it is, and this address stays in the game rather than
on the website. Topics are only under hceu/. The native game speaks MQTT
3.1.1. The web client has its own address.

The catalog is the menu. One client id is kept for the install. Choosing a
playlist opens this machine's private lobby on that playlist's port and
publishes hceu/3/mm/v1/queue/{clientId}, retained, every 5 seconds. Find
Game sets ready. The queue also says how many humans are in this lobby.
While this machine is hosting it publishes the invite, retained, every 5
seconds on hceu/3/mm/v1/hosted/{clientId}. An empty publish on that topic
is only when this machine is no longer the host. The queue stays until the
match starts or the player leaves, and an empty retained publish clears
it. A friend who joined this lobby's link clears their own queue. Offers
on this search are ignored until that queue has been published. role host
updates the lobby already open. role wait keeps it from starting. role
server or join, with an invite and phase search, leaves this lobby and
joins that invite.
*/

#include "platform.h"
#include "posix.h"
#include "p2p.h"
#include "halo_port_limits.h"
#include "matchmaking.h"

#include <stdio.h>
#include <string.h>

enum
{
	MATCHMAKING_PORT = 1883,
	QUEUE_INTERVAL = 5000,
	KEEP_ALIVE = 30,
	LINK_BUFFER = 16384,
	/* one id for this install, 8 to 64 of the broker's characters */
	ID_GENERATED = 16,
	ID_LIMIT = 64
};

static const char *const broker_host = "net.corece.org";

static pthread_mutex_t matchmaking_lock = PTHREAD_MUTEX_INITIALIZER;

static struct
{
	int started;
	int listening;
	int queueing;
	int published_queue;
	int want_clear;
	int committed;
	int want_hosted;
	int hosted_sent;
	/* the retained invite is removed once this machine is not that host */
	int want_unhost;
	int host_mark;
	/* offers are ignored until this search has published */
	int search;
	int accept_offers;
	int epoch;
	/* in the private lobby: closing the playlist screen must not stop the queue */
	int in_party;
	/* this machine followed someone else's join link. it stays through the match. */
	int joined_link;
	int ready;
	int locals;
	/* humans in this lobby, not the seats the service fills */
	int players;
	/* wait_seconds 0 is a backstop, not an immediate start */
	unsigned long backstop_at;
	int backstop_seated;
	int backstop_hold;
	int backstop_absent;
	int party_mark;
	int link;
	int id_ready;
	unsigned short game_port;
	unsigned short host_port;
	char id[ID_LIMIT + 1];
	char playlist[16];
	char name[12];
	char host_playlist[16];
	int offer_ready;
	struct matchmaking_offer offer;
	int catalog_ready;
	struct matchmaking_catalog catalog;
} matchmaking;

/* sockaddr ports are network order, the same swap the invite broker uses */
static unsigned short host_order_short(unsigned short value)
{
	return (unsigned short)((value << 8) | (value >> 8));
}

static void backstop_clear_locked(void)
{
	matchmaking.backstop_at = 0;
	matchmaking.backstop_seated = 0;
	matchmaking.backstop_hold = 0;
	matchmaking.backstop_absent = 0;
}

static int same_word(const char *text, const char *end, const char *word)
{
	int length = (int)strlen(word);

	return end - text == length && !memcmp(text, word, (size_t)length);
}

static const char *skip_space(const char *text)
{
	while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r')
		text++;
	return text;
}

static const char *skip_json(const char *text)
{
	text = skip_space(text);
	if (*text == '"')
	{
		text++;
		while (*text && *text != '"')
			text += *text == '\\' && text[1] ? 2 : 1;
		return *text ? text + 1 : text;
	}
	if (*text == '{' || *text == '[')
	{
		char open = *text;
		char close = open == '{' ? '}' : ']';
		int depth = 0;

		do
		{
			if (*text == '"')
			{
				text = skip_json(text);
				continue;
			}
			if (*text == open)
				depth++;
			else if (*text == close)
				depth--;
			if (*text)
				text++;
		} while (*text && depth);
		return text;
	}
	while (*text && *text != ',' && *text != '}' && *text != ']' && *text != ' ' && *text != '\t' &&
		*text != '\n' && *text != '\r')
		text++;
	return text;
}

static int object_field(const char *object, const char *key, const char **start, const char **end)
{
	const char *text = skip_space(object);

	if (*text != '{')
		return 0;
	text++;
	while (*text)
	{
		const char *name;
		const char *name_end;
		const char *value;

		text = skip_space(text);
		if (*text == '}')
			return 0;
		if (*text != '"')
			return 0;
		name = ++text;
		while (*text && *text != '"')
			text++;
		name_end = text;
		if (*text == '"')
			text++;
		text = skip_space(text);
		if (*text != ':')
			return 0;
		text = skip_space(text + 1);
		value = text;
		text = skip_json(text);
		if (same_word(name, name_end, key))
		{
			*start = value;
			*end = text;
			return 1;
		}
		text = skip_space(text);
		if (*text == ',')
			text++;
	}
	return 0;
}

static int copy_json_string(const char *start, const char *end, char *text, int size)
{
	int length = 0;

	start = skip_space(start);
	if (size < 1 || *start != '"')
		return 0;
	start++;
	while (start < end && *start != '"')
	{
		char character = *start++;

		if (character == '\\' && start < end)
		{
			character = *start++;
			if (character == 'n')
				character = '\n';
			else if (character == 't')
				character = '\t';
		}
		if (length < size - 1)
			text[length++] = character;
	}
	text[length] = 0;
	return 1;
}

static int json_string(const char *object, const char *key, char *text, int size)
{
	const char *start;
	const char *end;

	text[0] = 0;
	if (!object_field(object, key, &start, &end))
		return 0;
	return copy_json_string(start, end, text, size);
}

static int json_number(const char *object, const char *key, long *value)
{
	const char *start;
	const char *end;
	char *stopped;
	long number;

	if (!object_field(object, key, &start, &end))
		return 0;
	start = skip_space(start);
	number = strtol(start, &stopped, 10);
	if (stopped == start)
		return 0;
	*value = number;
	return 1;
}

static int json_bool(const char *object, const char *key, int *value)
{
	const char *start;
	const char *end;

	if (!object_field(object, key, &start, &end))
		return 0;
	start = skip_space(start);
	if (!strncmp(start, "true", 4))
		*value = 1;
	else if (!strncmp(start, "false", 5))
		*value = 0;
	else
		return 0;
	return 1;
}

static int weapon_named(const char *name)
{
	if (!strcmp(name, "assault_rifle"))
		return 2;
	if (!strcmp(name, "pistol"))
		return 3;
	if (!strcmp(name, "shotgun"))
		return 4;
	if (!strcmp(name, "sniper_rifle"))
		return 5;
	if (!strcmp(name, "rocket_launcher"))
		return 6;
	if (!strcmp(name, "plasma_pistol"))
		return 7;
	if (!strcmp(name, "plasma_rifle"))
		return 8;
	if (!strcmp(name, "needler"))
		return 9;
	if (!strcmp(name, "random"))
		return 1;
	return 0;
}

static int mode_named(const char *name)
{
	if (!strcmp(name, "oddball"))
		return MATCHMAKING_MODE_ODDBALL;
	if (!strcmp(name, "king"))
		return MATCHMAKING_MODE_KING;
	if (!strcmp(name, "ctf"))
		return MATCHMAKING_MODE_CTF;
	return MATCHMAKING_MODE_SLAYER;
}

static int map_index(const char *map)
{
	static const char *const maps[] = { "chillout", "prisoner", "carousel", "hangemhigh", "beavercreek" };
	int index;

	for (index = 0; index < (int)(sizeof(maps) / sizeof(maps[0])); index++)
	{
		if (!strcmp(map, maps[index]))
			return index;
	}
	return 0;
}

static void rules_default(struct matchmaking_rules *rules, const char *playlist, const char *map)
{
	static const int objective_modes[5] = { MATCHMAKING_MODE_ODDBALL, MATCHMAKING_MODE_KING, MATCHMAKING_MODE_CTF,
		MATCHMAKING_MODE_ODDBALL, MATCHMAKING_MODE_KING };
	int objective = !strcmp(playlist, "objective");
	int hardcore = !strcmp(playlist, "hardcore");

	memset(rules, 0, sizeof(*rules));
	rules->mode = objective ? objective_modes[map_index(map)] : MATCHMAKING_MODE_SLAYER;
	rules->respawn_ticks = 150;
	rules->goal_radar = 1;
	rules->radar_players = 2;
	rules->primary = 2;
	rules->secondary = 3;
	rules->score = rules->mode == MATCHMAKING_MODE_SLAYER ? 50 : rules->mode == MATCHMAKING_MODE_CTF ? 3 : 5;
	rules->no_death_bonus = 1;
	rules->no_kill_penalty = 1;
	rules->teams = objective || hardcore;
	rules->ball_spawn_count = 1;
}

static void rules_read(struct matchmaking_rules *rules, const char *object)
{
	const char *rules_text = NULL;
	const char *rules_end = NULL;
	char word[32];
	long number;
	int flag;
	char stored[1024];

	if (!object_field(object, "rules", &rules_text, &rules_end))
		return;
	if ((int)(rules_end - rules_text) >= (int)sizeof(stored))
		return;
	memcpy(stored, rules_text, (size_t)(rules_end - rules_text));
	stored[rules_end - rules_text] = 0;
	if (json_string(stored, "mode", word, sizeof(word)) || json_string(object, "mode", word, sizeof(word)))
		rules->mode = mode_named(word);
	if (json_number(stored, "respawn_ticks", &number))
		rules->respawn_ticks = number;
	if (json_number(stored, "respawn_growth_ticks", &number))
		rules->respawn_growth_ticks = number;
	if (json_number(stored, "suicide_penalty_ticks", &number))
		rules->suicide_penalty_ticks = number;
	if (json_bool(stored, "motion_sensor", &flag))
		rules->motion_sensor = flag;
	if (json_number(stored, "goal_radar", &number))
		rules->goal_radar = number;
	if (json_string(stored, "radar_players", word, sizeof(word)))
		rules->radar_players = !strcmp(word, "all") ? 0 : !strcmp(word, "friends") ? 1 : 2;
	else if (json_number(stored, "radar_players", &number))
		rules->radar_players = (int)number;
	if (json_string(stored, "primary", word, sizeof(word)))
		rules->primary = weapon_named(word);
	if (json_string(stored, "secondary", word, sizeof(word)))
		rules->secondary = weapon_named(word);
	if (json_number(stored, "time_limit_ticks", &number))
		rules->time_limit_ticks = number;
	if (json_number(stored, "score", &number))
		rules->score = number;
	if (json_bool(stored, "no_death_bonus", &flag))
		rules->no_death_bonus = flag;
	if (json_bool(stored, "no_kill_penalty", &flag))
		rules->no_kill_penalty = flag;
	if (json_bool(stored, "teams", &flag))
		rules->teams = flag;
	if (json_bool(stored, "moving_hill", &flag))
		rules->moving_hill = flag;
	if (json_number(stored, "ball_spawn_count", &number) || json_number(stored, "balls", &number) ||
		json_number(stored, "ball_count", &number))
		rules->ball_spawn_count = number;
}

static int role_named(const char *name)
{
	if (!strcmp(name, "wait"))
		return MATCHMAKING_ROLE_WAIT;
	if (!strcmp(name, "server"))
		return MATCHMAKING_ROLE_SERVER;
	if (!strcmp(name, "host"))
		return MATCHMAKING_ROLE_HOST;
	if (!strcmp(name, "join"))
		return MATCHMAKING_ROLE_JOIN;
	if (!strcmp(name, "error"))
		return MATCHMAKING_ROLE_ERROR;
	return MATCHMAKING_ROLE_NONE;
}

static int port_ok(long port)
{
	return port > 0 && port < 65536 && port != 5150 && port != 5151;
}

/* the catalog's port for this playlist, else the ports the service uses */
static unsigned short playlist_port(const char *playlist)
{
	int index;

	for (index = 0; index < matchmaking.catalog.count; index++)
	{
		if (!strcmp(matchmaking.catalog.playlist[index].id, playlist) &&
			port_ok(matchmaking.catalog.playlist[index].port))
			return matchmaking.catalog.playlist[index].port;
	}
	if (!strcmp(playlist, "ffa"))
		return 5161;
	if (!strcmp(playlist, "hardcore"))
		return 5162;
	if (!strcmp(playlist, "objective"))
		return 5160;
	return 0;
}

static int players_for(const char *playlist)
{
	return !strcmp(playlist, "hardcore") ? 4 : 8;
}

static int map_ok(const char *map)
{
	int index;

	if (!map[0] || strlen(map) > 31)
		return 0;
	for (index = 0; map[index]; index++)
	{
		char character = map[index];

		if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') || character == '_'))
			return 0;
	}
	return 1;
}

static int array_item(const char *array, int index, const char **start, const char **end)
{
	const char *text = skip_space(array);
	int at = 0;

	if (*text != '[')
		return 0;
	text++;
	while (*text)
	{
		const char *value;

		text = skip_space(text);
		if (*text == ']')
			return 0;
		value = text;
		text = skip_json(text);
		if (at == index)
		{
			*start = value;
			*end = text;
			return 1;
		}
		at++;
		text = skip_space(text);
		if (*text == ',')
			text++;
	}
	return 0;
}

static void catalog_store(const char *payload)
{
	struct matchmaking_catalog catalog;
	const char *list;
	const char *list_end;
	char stored[4096];
	long number;
	int index;

	memset(&catalog, 0, sizeof(catalog));
	if (json_number(payload, "network", &number))
		catalog.network = (int)number;
	if (!object_field(payload, "playlists", &list, &list_end))
		return;
	if ((int)(list_end - list) >= (int)sizeof(stored))
		return;
	memcpy(stored, list, (size_t)(list_end - list));
	stored[list_end - list] = 0;
	for (index = 0; index < (int)(sizeof(catalog.playlist) / sizeof(catalog.playlist[0])); index++)
	{
		struct matchmaking_playlist *row = &catalog.playlist[catalog.count];
		const char *item;
		const char *item_end;
		char object[1024];
		const char *modes;
		const char *modes_end;
		char mode_list[256];
		int mode;

		if (!array_item(stored, index, &item, &item_end))
			break;
		if ((int)(item_end - item) >= (int)sizeof(object))
			break;
		memcpy(object, item, (size_t)(item_end - item));
		object[item_end - item] = 0;
		if (!json_string(object, "id", row->id, sizeof(row->id)) || !row->id[0])
			continue;
		/* the chooser keeps ffa and hardcore */
		if (!strcmp(row->id, "objective"))
			continue;
		json_string(object, "name", row->name, sizeof(row->name));
		if (!row->name[0])
			snprintf(row->name, sizeof(row->name), "%s", row->id);
		if (json_number(object, "players", &number) && number >= 2 && number <= 128)
			row->players = (int)number;
		if (json_bool(object, "teams", &mode))
			row->teams = mode ? 1 : 0;
		if (json_number(object, "team_size", &number) && number >= 1 && number <= 16)
			row->team_size = (int)number;
		row->local_max = 4;
		if (json_number(object, "local_max", &number) && number >= 1 && number <= 4)
			row->local_max = (int)number;
		if (json_number(object, "port", &number) && port_ok(number))
			row->port = (unsigned short)number;
		if (object_field(object, "modes", &modes, &modes_end) &&
			(int)(modes_end - modes) < (int)sizeof(mode_list))
		{
			memcpy(mode_list, modes, (size_t)(modes_end - modes));
			mode_list[modes_end - modes] = 0;
			for (mode = 0; mode < 4; mode++)
			{
				const char *mode_text;
				const char *mode_end;

				if (!array_item(mode_list, mode, &mode_text, &mode_end))
					break;
				if (copy_json_string(mode_text, mode_end, row->modes[row->mode_count], sizeof(row->modes[0])))
					row->mode_count++;
			}
		}
		catalog.count++;
	}
	if (!catalog.count)
		return;
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.catalog = catalog;
	matchmaking.catalog_ready = 1;
	pthread_mutex_unlock(&matchmaking_lock);
}

static void offer_name_keep(char *text)
{
	int read = 0;
	int write = 0;

	while (text[read] && write < MATCHMAKING_LOBBY_NAME_LENGTH - 1)
	{
		unsigned char character = (unsigned char)text[read++];

		if (character >= 32)
			text[write++] = (char)character;
	}
	text[write] = 0;
}

static void offer_names(struct matchmaking_offer *offer, const char *payload)
{
	const char *list;
	const char *list_end;
	char stored[1024];
	int length;
	int index;

	if (!object_field(payload, "names", &list, &list_end))
		return;
	length = (int)(list_end - list);
	if (length < 0 || length >= (int)sizeof(stored))
		return;
	memcpy(stored, list, (size_t)length);
	stored[length] = 0;
	for (index = 0; index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		const char *item;
		const char *item_end;

		if (!array_item(stored, index, &item, &item_end))
			break;
		if (!copy_json_string(item, item_end, offer->names[offer->name_count],
			MATCHMAKING_LOBBY_NAME_LENGTH))
			continue;
		offer_name_keep(offer->names[offer->name_count]);
		if (offer->names[offer->name_count][0])
			offer->name_count++;
	}
}

static int offer_name_slot(struct matchmaking_offer *offer, const char *text)
{
	int index;

	for (index = 0; index < offer->name_count; index++)
	{
		if (!strcmp(offer->names[index], text))
			return index;
	}
	if (offer->name_count >= MATCHMAKING_LOBBY_NAMES)
		return -1;
	index = offer->name_count++;
	snprintf(offer->names[index], MATCHMAKING_LOBBY_NAME_LENGTH, "%s", text);
	return index;
}

/* "parties": [["Alice", "Bob"], ["Carol", "Dave"]]. each inner array shares a team. */
static void offer_parties(struct matchmaking_offer *offer, const char *payload)
{
	const char *list;
	const char *list_end;
	char stored[2048];
	int length;
	int party;

	if (!object_field(payload, "parties", &list, &list_end))
		return;
	length = (int)(list_end - list);
	if (length < 0 || length >= (int)sizeof(stored))
		return;
	memcpy(stored, list, (size_t)length);
	stored[length] = 0;
	for (party = 0; party < 4; party++)
	{
		const char *item;
		const char *item_end;
		char inner[512];
		int inner_length;
		int member;
		int counted = 0;

		if (!array_item(stored, party, &item, &item_end))
			break;
		inner_length = (int)(item_end - item);
		if (inner_length < 0 || inner_length >= (int)sizeof(inner))
			break;
		memcpy(inner, item, (size_t)inner_length);
		inner[inner_length] = 0;
		for (member = 0; member < MATCHMAKING_LOBBY_NAMES; member++)
		{
			const char *name;
			const char *name_end;
			char text[MATCHMAKING_LOBBY_NAME_LENGTH];
			int slot;

			if (!array_item(inner, member, &name, &name_end))
				break;
			if (!copy_json_string(name, name_end, text, (int)sizeof(text)))
				continue;
			offer_name_keep(text);
			if (!text[0])
				continue;
			slot = offer_name_slot(offer, text);
			if (slot < 0)
				break;
			offer->name_party[slot] = party;
			counted = 1;
		}
		if (counted)
			offer->party_count = party + 1;
	}
}

static void offer_clear(struct matchmaking_offer *offer)
{
	int index;

	memset(offer, 0, sizeof(*offer));
	offer->joined = -1;
	offer->wait_seconds = -1;
	offer->bots = -1;
	for (index = 0; index < MATCHMAKING_LOBBY_NAMES; index++)
		offer->name_party[index] = -1;
}

static void offer_read(struct matchmaking_offer *offer, const char *payload, const char *playlist)
{
	char word[32];
	long number;
	int teams;

	offer_clear(offer);
	offer->local_max = 4;
	snprintf(offer->playlist, sizeof(offer->playlist), "%s", playlist);
	if (!json_string(payload, "playlist", offer->playlist, sizeof(offer->playlist)))
		snprintf(offer->playlist, sizeof(offer->playlist), "%s", playlist);
	json_string(payload, "map", offer->map, sizeof(offer->map));
	json_string(payload, "name", offer->name, sizeof(offer->name));
	json_string(payload, "invite", offer->invite, sizeof(offer->invite));
	if (!map_ok(offer->map))
		offer->map[0] = 0;
	rules_default(&offer->rules, offer->playlist, offer->map[0] ? offer->map : "chillout");
	rules_read(&offer->rules, payload);
	if (json_string(payload, "mode", word, sizeof(word)))
		offer->rules.mode = mode_named(word);
	offer->players = players_for(offer->playlist);
	if (json_number(payload, "players", &number) && number >= 2 && number <= 128)
		offer->players = (int)number;
	if (json_number(payload, "joined", &number) && number >= 0 && number <= 128)
		offer->joined = (int)number;
	if (json_number(payload, "wait_seconds", &number) && number >= 0 && number <= 600)
		offer->wait_seconds = (int)number;
	if (json_string(payload, "phase", word, sizeof(word)))
	{
		if (!strcmp(word, "party"))
			offer->phase = MATCHMAKING_PHASE_PARTY;
		else if (!strcmp(word, "search"))
			offer->phase = MATCHMAKING_PHASE_SEARCH;
	}
	if (json_number(payload, "local_max", &number) && number >= 1 && number <= 4)
		offer->local_max = (int)number;
	offer_names(offer, payload);
	offer_parties(offer, payload);
	if (json_number(payload, "bots", &number) && number >= 0 && number <= 16)
		offer->bots = (int)number;
	if (json_bool(payload, "teams", &teams))
		offer->rules.teams = teams ? 1 : 0;
	offer->teams = offer->rules.teams;
	if (json_number(payload, "team_size", &number) && number >= 1 && number <= 16)
		offer->team_size = (int)number;
	offer->port = 0;
	if (json_number(payload, "port", &number) && port_ok(number))
		offer->port = (unsigned short)number;
	else if (port_ok(playlist_port(offer->playlist)))
		offer->port = playlist_port(offer->playlist);
	if (json_number(payload, "network", &number))
		offer->network = (int)number;
	if (json_string(payload, "error", word, sizeof(word)) && !strcmp(word, "network"))
		offer->role = MATCHMAKING_ROLE_ERROR;
	else
	{
		if (!json_string(payload, "role", word, sizeof(word)))
			word[0] = 0;
		offer->role = role_named(word);
	}
}

static int put_short_length(unsigned char *bytes, int value)
{
	bytes[0] = (unsigned char)((value >> 8) & 255);
	bytes[1] = (unsigned char)(value & 255);
	return 2;
}

static int put_text(unsigned char *bytes, const char *text)
{
	int length = (int)strlen(text);

	put_short_length(bytes, length);
	memcpy(bytes + 2, text, (size_t)length);
	return 2 + length;
}

static int put_remaining(unsigned char *bytes, int value)
{
	int size = 0;

	do
	{
		unsigned char byte = (unsigned char)(value & 127);

		value >>= 7;
		bytes[size++] = (unsigned char)(value ? byte | 128 : byte);
	} while (value && size < 4);
	return size;
}

static int get_remaining(const unsigned char *bytes, int size, int *offset, int *value)
{
	int shift = 0;

	*value = 0;
	for (;;)
	{
		unsigned char byte;

		if (*offset >= size || shift > 21)
			return 0;
		byte = bytes[(*offset)++];
		*value += (byte & 127) << shift;
		if (!(byte & 128))
			return 1;
		shift += 7;
	}
}

static int json_escape(char *destination, int size, const char *text)
{
	int length = 0;

	while (*text && length < size - 2)
	{
		char character = *text++;

		if (character == '"' || character == '\\')
		{
			if (length >= size - 3)
				break;
			destination[length++] = '\\';
		}
		else if ((unsigned char)character < 32)
			character = '?';
		destination[length++] = character;
	}
	destination[length] = 0;
	return length;
}

struct matchmaking_link
{
	int socket;
	int tcp;
	int ready;
	int epoch;
	int published;
	unsigned long published_at;
	unsigned long last_queue;
	unsigned long last_io;
	int subscribed;
	unsigned long subscribed_at;
	/* a publish arrived after this subscribe, so its retained offer was read */
	int retain_seen;
	/* one more read after that, so a second retained packet is not this search */
	int retain_wait;
	int search;
	int party_mark;
	int hosted_queued;
	unsigned long last_hosted;
	int host_mark;
	int clear_queued;
	int unhost_queued;
	unsigned char input[LINK_BUFFER];
	int input_size;
	unsigned char output[LINK_BUFFER];
	int output_size;
};

static struct matchmaking_link link_state;

static void link_set(int state)
{
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.link = state;
	pthread_mutex_unlock(&matchmaking_lock);
}

static void link_close(struct matchmaking_link *link)
{
	if (link->socket >= 0)
		posix_socket_close(link->socket);
	memset(link, 0, sizeof(*link));
	link->socket = -1;
	link_set(0);
}

static int link_flush(struct matchmaking_link *link)
{
	while (link->output_size)
	{
		int sent = posix_socket_send(link->socket, link->output, link->output_size, 0);

		if (sent < 0)
		{
			int error = posix_socket_last_error();

			if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS)
				return 1;
			return 0;
		}
		if (!sent)
			return 0;
		memmove(link->output, link->output + sent, (size_t)(link->output_size - sent));
		link->output_size -= sent;
		link->last_io = GetTickCount();
	}
	return 1;
}

static int link_send(struct matchmaking_link *link, const unsigned char *packet, int size)
{
	if (size < 0 || link->output_size + size > (int)sizeof(link->output))
		return 0;
	memcpy(link->output + link->output_size, packet, (size_t)size);
	link->output_size += size;
	return link_flush(link);
}

static int link_packet(struct matchmaking_link *link, unsigned char header, const unsigned char *body, int size)
{
	unsigned char packet[2048];
	int remaining;

	if (size < 0 || size > (int)sizeof(packet) - 5)
		return 0;
	packet[0] = header;
	remaining = 1 + put_remaining(packet + 1, size);
	memcpy(packet + remaining, body, (size_t)size);
	return link_send(link, packet, remaining + size);
}

static int link_connect(struct matchmaking_link *link)
{
	struct sockaddr_in address;
	unsigned long host;
	int socket;

	host = posix_resolve_ipv4(broker_host);
	if (!host)
	{
		link_set(-1);
		return 0;
	}
	socket = posix_socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0)
	{
		link_set(-1);
		return 0;
	}
	posix_socket_set_nonblocking(socket, 1);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = host_order_short(MATCHMAKING_PORT);
	address.sin_addr.s_addr = host;
	if (posix_socket_connect(socket, &address, sizeof(address)) != 0)
	{
		int error = posix_socket_last_error();

		if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
		{
			posix_socket_close(socket);
			link_set(-1);
			return 0;
		}
	}
	link->socket = socket;
	link->last_io = GetTickCount();
	return 1;
}

/* the broker has accepted the TCP connection, so the session can start */
static int link_greet(struct matchmaking_link *link, const char *id)
{
	unsigned char body[128];
	int size = 0;

	size += put_text(body, "MQTT");
	body[size++] = 4;
	body[size++] = 0x02;
	body[size++] = 0;
	body[size++] = KEEP_ALIVE;
	size += put_text(body + size, id);
	return link_packet(link, 0x10, body, size);
}

static int link_connected(struct matchmaking_link *link)
{
	int error = 0;
	int length = (int)sizeof(error);
	int ready[1];
	int ready_count = 1;

	ready[0] = link->socket;
	if (posix_socket_select(NULL, NULL, ready, &ready_count, NULL, NULL, 0, 0, 0) <= 0 || !ready_count)
		return 0;
	/* 0x1007 is Winsock's SO_ERROR; the port turns it into the host's */
	if (posix_socket_getsockopt(link->socket, SOL_SOCKET, 0x1007, &error, &length) != 0 || error)
		return -1;
	return 1;
}

static int link_subscribe(struct matchmaking_link *link, const char *id)
{
	unsigned char body[256];
	char topic[96];
	int size = 0;

	body[size++] = 0;
	body[size++] = 1;
	size += put_text(body + size, "hceu/3/mm/v1/catalog");
	body[size++] = 0;
	snprintf(topic, sizeof(topic), "hceu/3/mm/v1/offer/%s", id);
	size += put_text(body + size, topic);
	body[size++] = 0;
	return link_packet(link, 0x82, body, size);
}

static int link_publish(struct matchmaking_link *link, const char *topic, const char *payload, int retain)
{
	unsigned char body[640];
	int size = 0;
	int length = (int)strlen(payload);

	if (length > 400)
		return 0;
	size += put_text(body, topic);
	memcpy(body + size, payload, (size_t)length);
	size += length;
	/* retain keeps the queue, and an empty payload removes it */
	return link_packet(link, retain ? 0x31 : 0x30, body, size);
}

static void offer_store(const char *payload, int epoch, const char *playlist)
{
	struct matchmaking_offer offer;

	offer_read(&offer, payload, playlist);
	pthread_mutex_lock(&matchmaking_lock);
	if (epoch == matchmaking.epoch && matchmaking.queueing && matchmaking.accept_offers)
	{
		if (offer.role == MATCHMAKING_ROLE_NONE)
			matchmaking.offer_ready = 0;
		else
		{
			matchmaking.offer = offer;
			matchmaking.offer_ready = 1;
		}
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

static void link_publish_in(struct matchmaking_link *link, const unsigned char *packet, int size, int epoch,
	const char *playlist, const char *id)
{
	int offset = 1;
	int remaining;
	int topic_length;
	char topic[96];
	char wanted[96];
	const unsigned char *body;
	char payload[4096];
	int payload_size;

	if (size < 3 || (packet[0] & 0xF0) != 0x30)
		return;
	if (!get_remaining(packet, size, &offset, &remaining) || offset + remaining > size)
		return;
	body = packet + offset;
	if (remaining < 2)
		return;
	topic_length = (body[0] << 8) | body[1];
	if (topic_length < 0 || topic_length > (int)sizeof(topic) - 1 || 2 + topic_length > remaining)
		return;
	memcpy(topic, body + 2, (size_t)topic_length);
	topic[topic_length] = 0;
	link->retain_seen = 1;
	if (!strcmp(topic, "hceu/3/mm/v1/catalog"))
	{
		payload_size = 2 + topic_length;
		if ((packet[0] & 0x06) != 0)
		{
			if (payload_size + 2 > remaining)
				return;
			payload_size += 2;
		}
		if (payload_size > remaining || remaining - payload_size >= (int)sizeof(payload))
			return;
		memcpy(payload, body + payload_size, (size_t)(remaining - payload_size));
		payload[remaining - payload_size] = 0;
		catalog_store(payload);
		return;
	}
	snprintf(wanted, sizeof(wanted), "hceu/3/mm/v1/offer/%s", id);
	if (strcmp(topic, wanted))
		return;
	/* the retained offer from the last search arrives with the subscribe.
	this search has not published yet, so that offer is not ours. */
	if (!link->published)
		return;
	payload_size = 2 + topic_length;
	if ((packet[0] & 0x06) != 0)
	{
		if (payload_size + 2 > remaining)
			return;
		payload_size += 2;
	}
	if (payload_size > remaining || remaining - payload_size >= (int)sizeof(payload))
		return;
	memcpy(payload, body + payload_size, (size_t)(remaining - payload_size));
	payload_size = remaining - payload_size;
	payload[payload_size] = 0;
	offer_store(payload, epoch, playlist);
}

static void link_take(struct matchmaking_link *link, int epoch, const char *playlist, const char *id)
{
	for (;;)
	{
		int offset = 0;
		int remaining;
		int total;

		if (link->input_size < 2)
			return;
		offset = 1;
		if (!get_remaining(link->input, link->input_size, &offset, &remaining))
		{
			if (link->input_size >= 5)
			{
				link_close(link);
				link_set(-1);
			}
			return;
		}
		total = offset + remaining;
		if (total > (int)sizeof(link->input))
		{
			link_close(link);
			link_set(-1);
			return;
		}
		if (link->input_size < total)
			return;
		if ((link->input[0] & 0xF0) == 0x20)
		{
			if (remaining >= 2 && link->input[offset + 1] == 0)
			{
				link->ready = 1;
				link_set(1);
			}
			else
			{
				link_close(link);
				link_set(-1);
				return;
			}
		}
		else if ((link->input[0] & 0xF0) == 0x30)
			link_publish_in(link, link->input, total, epoch, playlist, id);
		memmove(link->input, link->input + total, (size_t)(link->input_size - total));
		link->input_size -= total;
	}
}

static void link_read(struct matchmaking_link *link, int epoch, const char *playlist, const char *id)
{
	int socket = link->socket;
	int ready_count = 1;

	if (socket < 0)
		return;
	if (posix_socket_select(&socket, &ready_count, NULL, NULL, NULL, NULL, 0, 200000, 0) > 0 && ready_count)
	{
		int got = posix_socket_recv(link->socket, link->input + link->input_size,
			(int)sizeof(link->input) - link->input_size, 0);

		if (got < 0)
		{
			int error = posix_socket_last_error();

			if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
			{
				link_close(link);
				link_set(-1);
			}
			return;
		}
		if (!got)
		{
			link_close(link);
			link_set(-1);
			return;
		}
		link->input_size += got;
		link->last_io = GetTickCount();
	}
	link_take(link, epoch, playlist, id);
}

static void queue_payload(char *payload, int size, const char *playlist, const char *name, int ready, int locals,
	int players)
{
	char escaped[32];
	int cap = players_for(playlist);

	if (locals < 1)
		locals = 1;
	if (locals > 4)
		locals = 4;
	if (players < 1)
		players = 1;
	if (cap < 1)
		cap = 8;
	if (players > cap)
		players = cap;
	json_escape(escaped, sizeof(escaped), name);
	snprintf(payload, (size_t)size,
		"{\"playlist\":\"%s\",\"name\":\"%s\",\"network\":%d,\"ready\":%s,\"locals\":%d,\"players\":%d}",
		playlist, escaped, HALO_PORT_NETWORK_VERSION, ready ? "true" : "false", locals, players);
}

static void *matchmaking_thread(void *unused)
{
	struct matchmaking_link *link = &link_state;

	(void)unused;
	link->socket = -1;
	for (;;)
	{
		int listening;
		int queueing;
		int want_clear;
		int want_hosted;
		int hosted_sent;
		int want_unhost;
		int host_mark;
		int search;
		int epoch;
		int queue_ready;
		int queue_locals;
		int queue_players;
		int party_mark;
		unsigned short host_port;
		char id[ID_LIMIT + 1];
		char playlist[16];
		char name[12];
		char host_playlist[16];
		char topic[96];
		char payload[320];

		pthread_mutex_lock(&matchmaking_lock);
		listening = matchmaking.listening;
		queueing = matchmaking.queueing;
		want_clear = matchmaking.want_clear;
		want_hosted = matchmaking.want_hosted;
		hosted_sent = matchmaking.hosted_sent;
		want_unhost = matchmaking.want_unhost;
		host_mark = matchmaking.host_mark;
		search = matchmaking.search;
		epoch = matchmaking.epoch;
		queue_ready = matchmaking.ready;
		queue_locals = matchmaking.locals;
		queue_players = matchmaking.players;
		party_mark = matchmaking.party_mark;
		host_port = matchmaking.host_port;
		memcpy(id, matchmaking.id, sizeof(id));
		memcpy(playlist, matchmaking.playlist, sizeof(playlist));
		memcpy(name, matchmaking.name, sizeof(name));
		memcpy(host_playlist, matchmaking.host_playlist, sizeof(host_playlist));
		pthread_mutex_unlock(&matchmaking_lock);
		if (link->search != search)
		{
			link->search = search;
			link->published = 0;
			link->last_queue = 0;
			link->party_mark = party_mark;
			link->unhost_queued = 0;
		}
		/* locals or Find Game changed: the next publish goes now, then every 5 seconds */
		if (link->party_mark != party_mark)
		{
			link->party_mark = party_mark;
			if (link->published)
				link->last_queue = 0;
		}
		if (link->host_mark != host_mark)
		{
			link->host_mark = host_mark;
			link->hosted_queued = 0;
			link->last_hosted = 0;
		}
		if (!want_unhost)
			link->unhost_queued = 0;
		/* stay up for the catalog, the queue, the empty clear, or the invite */
		if (!listening && !queueing && !want_clear && !want_hosted && !want_unhost)
		{
			if (link->output_size)
				link_flush(link);
			if (!link->output_size && link->socket >= 0)
				link_close(link);
			posix_socket_select(NULL, NULL, NULL, NULL, NULL, NULL, 0, 200000, 0);
			continue;
		}
		if (link->socket >= 0 && link->epoch != epoch)
			link_close(link);
		if (link->socket < 0)
		{
			if (!link_connect(link))
			{
				posix_socket_select(NULL, NULL, NULL, NULL, NULL, NULL, 1, 0, 0);
				continue;
			}
			link->epoch = epoch;
			link->subscribed = 0;
			link->published = 0;
			link->retain_seen = 0;
			link->retain_wait = 0;
		}
		if (!link->tcp)
		{
			int connected = link_connected(link);

			if (connected < 0 || GetTickCount() - link->last_io > 8000)
			{
				link_close(link);
				link_set(-1);
				continue;
			}
			if (!connected)
			{
				posix_socket_select(NULL, NULL, NULL, NULL, NULL, NULL, 0, 200000, 0);
				continue;
			}
			if (!link_greet(link, id))
			{
				link_close(link);
				link_set(-1);
				continue;
			}
			link->tcp = 1;
		}
		if (!link_flush(link))
		{
			link_close(link);
			link_set(-1);
			continue;
		}
		link_read(link, epoch, playlist, id);
		if (link->socket < 0)
			continue;
		if (!link->ready)
		{
			if (GetTickCount() - link->last_io > 8000)
			{
				link_close(link);
				link_set(-1);
			}
			continue;
		}
		/* subscribe, then read what is already retained, before the first publish */
		if (!link->subscribed)
		{
			if (link_subscribe(link, id))
			{
				link->subscribed = 1;
				link->subscribed_at = GetTickCount();
				link->retain_seen = 0;
				link->retain_wait = 0;
			}
			continue;
		}
		if (queueing && link->subscribed && (!link->published || GetTickCount() - link->last_queue >= QUEUE_INTERVAL))
		{
			int wait_retain = 0;

			/* the subscribe delivers the previous offer. ignore that, then publish. */
			if (!link->published)
			{
				if (link->input_size > 0)
					wait_retain = 1;
				else if (!link->retain_seen && GetTickCount() - link->subscribed_at < 1000)
					wait_retain = 1;
				else if (link->retain_seen && !link->retain_wait)
				{
					link->retain_wait = 1;
					wait_retain = 1;
				}
			}
			if (wait_retain)
				;
			else
			{
				int still;

				pthread_mutex_lock(&matchmaking_lock);
				still = matchmaking.queueing && matchmaking.search == search;
				if (still)
				{
					queue_ready = matchmaking.ready;
					queue_locals = matchmaking.locals;
					queue_players = matchmaking.players;
				}
				pthread_mutex_unlock(&matchmaking_lock);
				if (still)
				{
					snprintf(topic, sizeof(topic), "hceu/3/mm/v1/queue/%s", id);
					queue_payload(payload, sizeof(payload), playlist, name, queue_ready, queue_locals,
						queue_players);
					if (link_publish(link, topic, payload, 1))
					{
						link->published = 1;
						link->published_at = GetTickCount();
						link->last_queue = GetTickCount();
						pthread_mutex_lock(&matchmaking_lock);
						if (matchmaking.epoch == epoch && matchmaking.search == search && matchmaking.queueing)
						{
							matchmaking.accept_offers = 1;
							matchmaking.published_queue = 1;
						}
						pthread_mutex_unlock(&matchmaking_lock);
					}
				}
			}
		}
		/* the same 5 seconds as the queue. one publish is not enough. an empty
		publish is the unhost path, and only that path. */
		if (want_hosted && link->subscribed && port_ok(host_port) &&
			(!link->last_hosted || GetTickCount() - link->last_hosted >= QUEUE_INTERVAL))
		{
			char invite[256];

			if (p2p_invite_link(invite, sizeof(invite)) && !strncmp(invite, "halo://join/", 12))
			{
				char escaped[512];

				json_escape(escaped, sizeof(escaped), invite);
				snprintf(topic, sizeof(topic), "hceu/3/mm/v1/hosted/%s", id);
				snprintf(payload, sizeof(payload), "{\"playlist\":\"%s\",\"invite\":\"%s\",\"port\":%u}",
					host_playlist, escaped, (unsigned)host_port);
				if (link_publish(link, topic, payload, 1))
				{
					link->hosted_queued = 1;
					link->last_hosted = GetTickCount();
				}
			}
		}
		if (want_hosted && link->hosted_queued && !link->output_size)
		{
			pthread_mutex_lock(&matchmaking_lock);
			if (matchmaking.epoch == epoch && matchmaking.host_mark == host_mark)
				matchmaking.hosted_sent = 1;
			pthread_mutex_unlock(&matchmaking_lock);
		}
		/* this machine is no longer that host. an empty retain removes the invite. */
		if (want_unhost && !want_hosted && link->subscribed)
		{
			int still_unhost;

			pthread_mutex_lock(&matchmaking_lock);
			still_unhost = matchmaking.want_unhost && !matchmaking.want_hosted;
			pthread_mutex_unlock(&matchmaking_lock);
			if (still_unhost && !link->unhost_queued)
			{
				snprintf(topic, sizeof(topic), "hceu/3/mm/v1/hosted/%s", id);
				if (link_publish(link, topic, "", 1))
					link->unhost_queued = 1;
			}
			if (!still_unhost)
				link->unhost_queued = 0;
			else if (link->unhost_queued && !link->output_size)
			{
				pthread_mutex_lock(&matchmaking_lock);
				if (matchmaking.epoch == epoch && matchmaking.want_unhost && !matchmaking.want_hosted)
				{
					matchmaking.want_unhost = 0;
					matchmaking.hosted_sent = 0;
				}
				pthread_mutex_unlock(&matchmaking_lock);
				link->unhost_queued = 0;
			}
		}
		/* the queue is cleared after a hosted publish that is still going out */
		if (want_clear && link->subscribed && !(want_hosted && !hosted_sent))
		{
			int still_clear;

			pthread_mutex_lock(&matchmaking_lock);
			still_clear = matchmaking.want_clear && !matchmaking.queueing;
			pthread_mutex_unlock(&matchmaking_lock);
			if (still_clear && !link->clear_queued)
			{
				snprintf(topic, sizeof(topic), "hceu/3/mm/v1/queue/%s", id);
				if (link_publish(link, topic, "", 1))
					link->clear_queued = 1;
			}
			if (!still_clear)
				link->clear_queued = 0;
			else if (link->clear_queued && !link->output_size)
			{
				pthread_mutex_lock(&matchmaking_lock);
				if (matchmaking.epoch == epoch && matchmaking.want_clear && !matchmaking.queueing)
				{
					matchmaking.want_clear = 0;
					matchmaking.published_queue = 0;
				}
				pthread_mutex_unlock(&matchmaking_lock);
				link->clear_queued = 0;
				link->published = 0;
			}
		}
		if (GetTickCount() - link->last_io > (KEEP_ALIVE - 5) * 1000)
		{
			unsigned char ping[2] = { 0xC0, 0 };

			link_send(link, ping, 2);
		}
	}
	return NULL;
}

static void matchmaking_start(void)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, matchmaking_thread, NULL) != 0)
	{
		pthread_mutex_lock(&matchmaking_lock);
		matchmaking.started = 0;
		pthread_mutex_unlock(&matchmaking_lock);
		return;
	}
	pthread_detach(thread);
}

static void make_id(char *id)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
	unsigned char bytes[ID_GENERATED];
	int index;

	posix_random_bytes(bytes, sizeof(bytes));
	for (index = 0; index < ID_GENERATED; index++)
		id[index] = alphabet[bytes[index] % (sizeof(alphabet) - 1)];
	id[ID_GENERATED] = 0;
}

static int id_ok(const char *id)
{
	int length = 0;

	if (!id)
		return 0;
	while (id[length])
	{
		char character = id[length];

		if (!((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
			(character >= '0' && character <= '9') || character == '_' || character == '-'))
			return 0;
		length++;
		if (length > ID_LIMIT)
			return 0;
	}
	return length >= 8;
}

static void id_path(char *path, int size)
{
	/* the same folder as profiles, which is writable on Mac, Windows, and Linux */
	const char *root = platform_save_root();

	if (!root || !root[0])
		root = ".";
	snprintf(path, (size_t)size, "%s/matchmaking.id", root);
}

/* the same id for this install, so a later search replaces the last queue */
static void id_load(void)
{
	char path[1024];
	char stored[ID_LIMIT + 8];
	FILE *file;
	int ready;

	pthread_mutex_lock(&matchmaking_lock);
	ready = matchmaking.id_ready;
	pthread_mutex_unlock(&matchmaking_lock);
	if (ready)
		return;
	memset(stored, 0, sizeof(stored));
	id_path(path, sizeof(path));
	file = fopen(path, "rb");
	if (file)
	{
		if (fgets(stored, sizeof(stored), file))
		{
			int length = (int)strlen(stored);

			while (length > 0 && (stored[length - 1] == '\n' || stored[length - 1] == '\r'))
				stored[--length] = 0;
		}
		fclose(file);
	}
	if (!id_ok(stored))
	{
		make_id(stored);
		file = fopen(path, "wb");
		if (file)
		{
			fputs(stored, file);
			fputc('\n', file);
			fclose(file);
		}
	}
	pthread_mutex_lock(&matchmaking_lock);
	if (!matchmaking.id_ready)
	{
		memcpy(matchmaking.id, stored, sizeof(matchmaking.id));
		matchmaking.id[sizeof(matchmaking.id) - 1] = 0;
		matchmaking.id_ready = 1;
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

static int playlist_listed(const char *playlist)
{
	int index;

	if (!playlist || !playlist[0])
		return 0;
	for (index = 0; index < matchmaking.catalog.count; index++)
	{
		if (!strcmp(matchmaking.catalog.playlist[index].id, playlist))
			return 1;
	}
	return 0;
}

static void matchmaking_ensure(void)
{
	int start;

	pthread_mutex_lock(&matchmaking_lock);
	start = !matchmaking.started;
	if (start)
		matchmaking.started = 1;
	pthread_mutex_unlock(&matchmaking_lock);
	if (start)
		matchmaking_start();
}

static void copy_short(char *destination, int size, const char *text, int limit)
{
	int length = 0;

	if (!text)
		text = "";
	while (*text && length < limit && length < size - 1)
	{
		unsigned char character = (unsigned char)*text++;

		destination[length++] = character >= 32 && character < 127 ? (char)character : '?';
	}
	destination[length] = 0;
}

void matchmaking_watch(void)
{
	id_load();
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.listening = 1;
	pthread_mutex_unlock(&matchmaking_lock);
	matchmaking_ensure();
}

int matchmaking_catalog(struct matchmaking_catalog *catalog)
{
	int result = 0;

	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.catalog_ready)
	{
		*catalog = matchmaking.catalog;
		result = !catalog->network || catalog->network == HALO_PORT_NETWORK_VERSION ? 1 : -1;
	}
	pthread_mutex_unlock(&matchmaking_lock);
	return result;
}

/* the retained invite comes down. the queue is a separate publish. */
static void hosted_drop_locked(void)
{
	matchmaking.want_hosted = 0;
	if (matchmaking.hosted_sent)
		matchmaking.want_unhost = 1;
}

/* machines that sat in this lobby before Find Game, this host's machine,
and a later human the offer does not list. a filled seat is not here.
two searches both publish Player, which is also this host's name, so the
name cannot say who sat here. */
static unsigned char screen_party_machine[16];

static int offer_name_matches(const wchar_t *player_name, const char *offer_name);

static void screen_party_clear(void)
{
	memset(screen_party_machine, 0, sizeof(screen_party_machine));
}

static void screen_party_mark(int machine)
{
	if (machine < 0 || machine >= 128)
		return;
	screen_party_machine[machine >> 3] |= (unsigned char)(1u << (machine & 7));
}

static void screen_party_unmark(int machine)
{
	if (machine < 0 || machine >= 128)
		return;
	screen_party_machine[machine >> 3] &= (unsigned char)~(1u << (machine & 7));
}

/* 1 when the offer lists this name. have_parties is 1 when the offer has
named any party, so a missing list is not treated as an unlisted friend. */
static int screen_offer_lists(const wchar_t *name, int *have_parties)
{
	struct matchmaking_offer offer;
	int index;

	if (have_parties)
		*have_parties = 0;
	if (!name || !name[0])
		return 0;
	pthread_mutex_lock(&matchmaking_lock);
	offer = matchmaking.offer;
	pthread_mutex_unlock(&matchmaking_lock);
	if (have_parties)
		*have_parties = offer.party_count > 0;
	if (offer.party_count <= 0)
		return 0;
	for (index = 0; index < offer.name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		if (offer.name_party[index] < 0)
			continue;
		if (offer_name_matches(name, offer.names[index]))
			return 1;
	}
	return 0;
}

void matchmaking_screen_party_note(int machine, int host_local, const wchar_t *name)
{
	int have_parties = 0;

	if (!matchmaking_game_port())
		return;
	/* still the private lobby: Find Game has not been pressed. once the
	match starts, queueing is cleared and a later arrival is not the lobby. */
	if (host_local || (matchmaking_queue_active() && !matchmaking_is_ready()))
	{
		screen_party_mark(machine);
		return;
	}
	if (screen_offer_lists(name, &have_parties) || !have_parties)
		return;
	screen_party_mark(machine);
}

void matchmaking_screen_party_clear_machine(int machine)
{
	screen_party_unmark(machine);
}

void matchmaking_screen_party_bits(unsigned char bits[16])
{
	if (!bits)
		return;
	memcpy(bits, screen_party_machine, 16);
}

void matchmaking_queue(const char *playlist, const char *name)
{
	id_load();
	pthread_mutex_lock(&matchmaking_lock);
	if (!playlist_listed(playlist) ||
		(matchmaking.catalog.network && matchmaking.catalog.network != HALO_PORT_NETWORK_VERSION))
	{
		pthread_mutex_unlock(&matchmaking_lock);
		return;
	}
	copy_short(matchmaking.playlist, sizeof(matchmaking.playlist), playlist, 15);
	copy_short(matchmaking.name, sizeof(matchmaking.name), name && name[0] ? name : "Player", 11);
	matchmaking.listening = 1;
	matchmaking.queueing = 1;
	matchmaking.committed = 0;
	matchmaking.in_party = 0;
	matchmaking.joined_link = 0;
	matchmaking.ready = 0;
	matchmaking.locals = 1;
	matchmaking.players = 1;
	backstop_clear_locked();
	matchmaking.party_mark++;
	hosted_drop_locked();
	matchmaking.want_clear = 0;
	matchmaking.offer_ready = 0;
	offer_clear(&matchmaking.offer);
	matchmaking.accept_offers = 0;
	matchmaking.search++;
	pthread_mutex_unlock(&matchmaking_lock);
	screen_party_clear();
	matchmaking_ensure();
}

void matchmaking_set_locals(int locals)
{
	if (locals < 1)
		locals = 1;
	if (locals > 4)
		locals = 4;
	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.queueing && !matchmaking.ready && locals != matchmaking.locals)
	{
		matchmaking.locals = locals;
		matchmaking.party_mark++;
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_set_players(int players)
{
	if (players < 1)
		players = 1;
	if (players > 16)
		players = 16;
	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.queueing && players != matchmaking.players)
	{
		matchmaking.players = players;
		matchmaking.party_mark++;
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

int matchmaking_name_is_filler(const wchar_t *name)
{
	struct matchmaking_offer offer;
	char host[12];
	int index;
	int filler = 0;

	if (!name || !name[0])
		return 0;
	pthread_mutex_lock(&matchmaking_lock);
	memcpy(host, matchmaking.name, sizeof(host));
	offer = matchmaking.offer;
	pthread_mutex_unlock(&matchmaking_lock);
	/* this client's own name is in parties[0] and is not a filled seat */
	if (offer_name_matches(name, host))
		return 0;
	if (offer.party_count <= 0)
		return 0;
	for (index = 0; index < offer.name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		if (offer.name_party[index] < 0)
			continue;
		if (offer_name_matches(name, offer.names[index]))
			filler = 1;
	}
	return filler;
}

void matchmaking_find_game(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.queueing && !matchmaking.ready)
	{
		matchmaking.ready = 1;
		matchmaking.party_mark++;
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

int matchmaking_queue_active(void)
{
	int active;

	pthread_mutex_lock(&matchmaking_lock);
	active = matchmaking.queueing;
	pthread_mutex_unlock(&matchmaking_lock);
	return active;
}

int matchmaking_is_ready(void)
{
	int ready;

	pthread_mutex_lock(&matchmaking_lock);
	ready = matchmaking.queueing && matchmaking.ready;
	pthread_mutex_unlock(&matchmaking_lock);
	return ready;
}

int matchmaking_offer_copy(struct matchmaking_offer *offer)
{
	int ready;

	pthread_mutex_lock(&matchmaking_lock);
	ready = matchmaking.offer.role != MATCHMAKING_ROLE_NONE;
	if (ready && offer)
		*offer = matchmaking.offer;
	pthread_mutex_unlock(&matchmaking_lock);
	return ready;
}

void matchmaking_leave(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	/* the playlist screen closes because the lobby opened. the queue stays. */
	if (matchmaking.in_party && matchmaking.queueing)
	{
		pthread_mutex_unlock(&matchmaking_lock);
		return;
	}
	matchmaking.listening = 0;
	matchmaking.queueing = 0;
	matchmaking.in_party = 0;
	matchmaking.ready = 0;
	matchmaking.offer_ready = 0;
	matchmaking.accept_offers = 0;
	if (!matchmaking.committed)
	{
		hosted_drop_locked();
		matchmaking.game_port = 0;
		if (matchmaking.published_queue)
			matchmaking.want_clear = 1;
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_reset(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.listening = 0;
	matchmaking.queueing = 0;
	matchmaking.committed = 0;
	matchmaking.in_party = 0;
	matchmaking.joined_link = 0;
	matchmaking.ready = 0;
	matchmaking.locals = 1;
	backstop_clear_locked();
	hosted_drop_locked();
	matchmaking.offer_ready = 0;
	matchmaking.accept_offers = 0;
	matchmaking.game_port = 0;
	matchmaking.search++;
	if (matchmaking.published_queue)
		matchmaking.want_clear = 1;
	pthread_mutex_unlock(&matchmaking_lock);
	screen_party_clear();
}

void matchmaking_stop(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	if (!matchmaking.queueing && !matchmaking.published_queue)
	{
		pthread_mutex_unlock(&matchmaking_lock);
		return;
	}
	matchmaking.queueing = 0;
	matchmaking.in_party = 0;
	matchmaking.ready = 0;
	matchmaking.locals = 1;
	matchmaking.accept_offers = 0;
	matchmaking.offer_ready = 0;
	/* the match started and this machine is still the host, so the invite stays.
	the game is loading. a later leave of a menu must not drop the port. */
	matchmaking.committed = 1;
	matchmaking.search++;
	if (matchmaking.published_queue)
		matchmaking.want_clear = 1;
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_commit_join(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.committed = 1;
	matchmaking.in_party = 1;
	hosted_drop_locked();
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_joined_link(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.queueing = 0;
	matchmaking.ready = 0;
	matchmaking.in_party = 1;
	matchmaking.committed = 1;
	matchmaking.accept_offers = 0;
	matchmaking.offer_ready = 0;
	matchmaking.joined_link = 1;
	hosted_drop_locked();
	if (matchmaking.published_queue)
		matchmaking.want_clear = 1;
	pthread_mutex_unlock(&matchmaking_lock);
}

int matchmaking_joined_by_link(void)
{
	int joined;

	pthread_mutex_lock(&matchmaking_lock);
	joined = matchmaking.joined_link;
	pthread_mutex_unlock(&matchmaking_lock);
	return joined;
}

void matchmaking_commit_party(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	matchmaking.committed = 1;
	matchmaking.in_party = 1;
	copy_short(matchmaking.host_playlist, sizeof(matchmaking.host_playlist), matchmaking.playlist, 15);
	if (port_ok(matchmaking.game_port))
		matchmaking.host_port = matchmaking.game_port;
	matchmaking.want_hosted = 1;
	matchmaking.want_unhost = 0;
	matchmaking.hosted_sent = 0;
	matchmaking.host_mark++;
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_release_host(void)
{
	pthread_mutex_lock(&matchmaking_lock);
	hosted_drop_locked();
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_party_offer(struct matchmaking_offer *offer, const struct matchmaking_playlist *row)
{
	unsigned short port;

	if (!offer || !row || !row->id[0])
		return;
	offer_clear(offer);
	offer->role = MATCHMAKING_ROLE_HOST;
	offer->phase = MATCHMAKING_PHASE_PARTY;
	copy_short(offer->playlist, sizeof(offer->playlist), row->id, 15);
	copy_short(offer->name, sizeof(offer->name), row->name[0] ? row->name : row->id, 31);
	copy_short(offer->map, sizeof(offer->map), "chillout", 31);
	port = port_ok(row->port) ? row->port : 0;
	if (!port && !strcmp(row->id, "ffa"))
		port = 5161;
	else if (!port && !strcmp(row->id, "hardcore"))
		port = 5162;
	else if (!port && !strcmp(row->id, "objective"))
		port = 5160;
	offer->port = port;
	offer->players = row->players > 1 ? row->players : players_for(row->id);
	offer->teams = row->teams;
	offer->team_size = row->team_size > 0 ? row->team_size : offer->teams ? 2 : 1;
	offer->local_max = row->local_max > 0 ? row->local_max : 4;
	offer->joined = -1;
	offer->wait_seconds = -1;
	rules_default(&offer->rules, row->id, offer->map);
}

void matchmaking_yield_to_invite(const char *text)
{
	int yield = 0;

	if (!text || !text[0])
		return;
	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.queueing)
	{
		yield = !matchmaking.offer.invite[0] || !strstr(text, matchmaking.offer.invite);
	}
	if (yield)
	{
		matchmaking.queueing = 0;
		matchmaking.in_party = 0;
		matchmaking.joined_link = 0;
		matchmaking.ready = 0;
		matchmaking.locals = 1;
		matchmaking.accept_offers = 0;
		matchmaking.offer_ready = 0;
		matchmaking.committed = 0;
		matchmaking.game_port = 0;
		backstop_clear_locked();
		hosted_drop_locked();
		matchmaking.search++;
		if (matchmaking.published_queue)
			matchmaking.want_clear = 1;
		screen_party_clear();
	}
	pthread_mutex_unlock(&matchmaking_lock);
}

void matchmaking_commit_host(const char *playlist, unsigned short port)
{
	pthread_mutex_lock(&matchmaking_lock);
	copy_short(matchmaking.host_playlist, sizeof(matchmaking.host_playlist), playlist, 15);
	matchmaking.host_port = port;
	matchmaking.committed = 1;
	matchmaking.in_party = 1;
	matchmaking.want_hosted = 1;
	matchmaking.want_unhost = 0;
	matchmaking.hosted_sent = 0;
	matchmaking.host_mark++;
	pthread_mutex_unlock(&matchmaking_lock);
}

int matchmaking_take_offer(struct matchmaking_offer *offer)
{
	int ready;

	pthread_mutex_lock(&matchmaking_lock);
	ready = matchmaking.offer_ready;
	if (ready)
	{
		*offer = matchmaking.offer;
		matchmaking.offer_ready = 0;
	}
	pthread_mutex_unlock(&matchmaking_lock);
	return ready;
}

int matchmaking_link(void)
{
	int state;

	pthread_mutex_lock(&matchmaking_lock);
	state = matchmaking.link;
	pthread_mutex_unlock(&matchmaking_lock);
	return state;
}

unsigned short matchmaking_game_port(void)
{
	return matchmaking.game_port;
}

void matchmaking_set_game_port(unsigned short port)
{
	/* 5150 is an ordinary lobby. 5151 is this machine's client port. */
	if (!port_ok(port))
		port = 0;
	matchmaking.game_port = port;
}

static int offer_name_matches(const wchar_t *player_name, const char *offer_name)
{
	int index;

	if (!player_name || !offer_name)
		return 0;
	for (index = 0; index < MATCHMAKING_LOBBY_NAME_LENGTH - 1; index++)
	{
		wchar_t left = player_name[index];
		wchar_t right = (wchar_t)(unsigned char)offer_name[index];

		if (left >= L'A' && left <= L'Z')
			left = (wchar_t)(left - L'A' + L'a');
		if (right >= L'A' && right <= L'Z')
			right = (wchar_t)(right - L'A' + L'a');
		if (left != right)
			return 0;
		if (!left)
			return 1;
	}
	return 1;
}

/* parties[0] is the host's team and parties[1] is the other. a later party
joins the team that already has fewer players. a tie stays with the first. */
static int party_seat(const struct matchmaking_offer *offer, int party)
{
	int counts[2];
	int index;
	int which;

	if (party <= 0)
		return 0;
	if (party == 1)
		return 1;
	counts[0] = 0;
	counts[1] = 0;
	for (index = 0; index < offer->name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		if (offer->name_party[index] == 0)
			counts[0]++;
		else if (offer->name_party[index] == 1)
			counts[1]++;
	}
	for (which = 2; which <= party && which < 4; which++)
	{
		int members = 0;
		int team = counts[0] <= counts[1] ? 0 : 1;

		for (index = 0; index < offer->name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
		{
			if (offer->name_party[index] == which)
				members++;
		}
		if (which == party)
			return team;
		counts[team] += members;
	}
	return counts[0] <= counts[1] ? 0 : 1;
}

static int party_team(const struct matchmaking_offer *offer, int party)
{
	if (party <= 0)
		return 0;
	if (party == 1)
		return 1;
	return party_seat(offer, party);
}

static int name_listed_in_party(const struct matchmaking_offer *offer, const wchar_t *name, int party)
{
	int index;

	for (index = 0; index < offer->name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		if (offer->name_party[index] != party)
			continue;
		if (offer_name_matches(name, offer->names[index]))
			return 1;
	}
	return 0;
}

int matchmaking_join_team(const wchar_t *name, int host_local)
{
	struct matchmaking_offer offer;
	char host_name[12];
	int index;

	if (!matchmaking_game_port())
		return -1;
	pthread_mutex_lock(&matchmaking_lock);
	offer = matchmaking.offer;
	memcpy(host_name, matchmaking.name, sizeof(host_name));
	pthread_mutex_unlock(&matchmaking_lock);
	host_name[sizeof(host_name) - 1] = 0;
	if (!offer.teams)
		return -1;
	/* no parties listed: the people in this hosted game share a team */
	if (offer.party_count <= 0)
		return 0;
	/* the host's own player is team 0, even when a remote human has the same name */
	if (host_local)
		return 0;
	/* a filler name is unique and belongs to the party that lists it */
	if (matchmaking_name_is_filler(name))
	{
		for (index = 0; index < offer.name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
		{
			if (!offer_name_matches(name, offer.names[index]))
				continue;
			return party_team(&offer, offer.name_party[index]);
		}
	}
	/* both humans can publish the same name. the remote one is the other
	team only when parties[1] lists that name too. the first string match
	would put every copy, and the filler beside the first, on team 0. */
	if (host_name[0] && offer_name_matches(name, host_name) &&
		name_listed_in_party(&offer, name, 1))
		return 1;
	for (index = 0; index < offer.name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		if (!offer_name_matches(name, offer.names[index]))
			continue;
		return party_team(&offer, offer.name_party[index]);
	}
	/* not listed: this human joined the link and stays on the host's team. */
	return 0;
}

int matchmaking_may_count(int player_count, int maximum_players)
{
	struct matchmaking_offer offer;

	if (!matchmaking_game_port() || !matchmaking_offer_copy(&offer))
		return 0;
	/* role wait keeps this lobby. server and join leave it for the other invite. */
	if (offer.role != MATCHMAKING_ROLE_HOST)
		return 0;
	/* the private lobby omits wait_seconds. it does not start. */
	if (offer.phase == MATCHMAKING_PHASE_PARTY || !matchmaking_is_ready())
		return 0;
	if (offer.wait_seconds >= 0)
		return 1;
	/* Find Game, and the seats already fill the cap. an omitted wait starts then. */
	return maximum_players > 0 && player_count >= maximum_players &&
		offer.phase == MATCHMAKING_PHASE_SEARCH;
}

int matchmaking_listed_absent(const wchar_t *const *names, int count)
{
	struct matchmaking_offer offer;
	unsigned char used[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	int index;

	if (!names || count < 0)
		count = 0;
	if (count > HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
		count = HALO_PORT_MAXIMUM_NETWORK_PLAYERS;
	if (!matchmaking_offer_copy(&offer) || offer.party_count <= 0)
		return 0;
	memset(used, 0, (size_t)count);
	for (index = 0; index < offer.name_count && index < MATCHMAKING_LOBBY_NAMES; index++)
	{
		int player;
		int found = 0;

		if (offer.name_party[index] < 0 || !offer.names[index][0])
			continue;
		for (player = 0; player < count; player++)
		{
			if (used[player] || !names[player])
				continue;
			if (!offer_name_matches(names[player], offer.names[index]))
				continue;
			used[player] = 1;
			found = 1;
			break;
		}
		if (!found)
			return 1;
	}
	return 0;
}

void matchmaking_backstop_observe(int listed_absent)
{
	unsigned long now = GetTickCount();

	pthread_mutex_lock(&matchmaking_lock);
	if (matchmaking.offer.wait_seconds != 0 || matchmaking.offer.phase == MATCHMAKING_PHASE_PARTY)
	{
		backstop_clear_locked();
		pthread_mutex_unlock(&matchmaking_lock);
		return;
	}
	if (!matchmaking.backstop_at)
		matchmaking.backstop_at = now ? now : 1;
	matchmaking.backstop_absent = listed_absent ? 1 : 0;
	if (listed_absent)
		matchmaking.backstop_seated = 0;
	else if (!matchmaking.backstop_seated)
	{
		matchmaking.backstop_seated = 1;
		matchmaking.backstop_hold = 1;
	}
	else
		matchmaking.backstop_hold = 0;
	pthread_mutex_unlock(&matchmaking_lock);
}

int matchmaking_wait_omitted(void)
{
	struct matchmaking_offer offer;

	if (!matchmaking_offer_copy(&offer))
		return 0;
	return offer.wait_seconds < 0 && offer.phase == MATCHMAKING_PHASE_SEARCH;
}

unsigned long matchmaking_countdown_ms(void)
{
	struct matchmaking_offer offer;
	unsigned long now;
	unsigned long at;
	int absent;
	int hold;
	int wait;

	pthread_mutex_lock(&matchmaking_lock);
	offer = matchmaking.offer;
	at = matchmaking.backstop_at;
	absent = matchmaking.backstop_absent;
	hold = matchmaking.backstop_hold;
	pthread_mutex_unlock(&matchmaking_lock);
	if (offer.wait_seconds < 0)
		return 0;
	wait = offer.wait_seconds;
	if (wait > 100)
		wait = 100;
	if (wait > 0)
		return (unsigned long)wait * 1000UL;
	/* wait_seconds 0 waits for every listed name. the tick that first sees
	them seated holds. the next tick starts. a name still missing 15 seconds
	after this 0 arrived starts then. */
	now = GetTickCount();
	if (!at || absent)
	{
		if (at && now - at >= 15000UL)
			return 0;
		return 15000UL;
	}
	if (hold)
		return 15000UL;
	return 0;
}

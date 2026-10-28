/*
PLAYER_QUEUES_NEW.C

symbols in this file:
000A80A0 0040:
	_update_server_add_player (0000)
000A80E0 0090:
	_update_client_new (0000)
000A8170 0040:
	_update_client_delete (0000)
000A81B0 00c0:
	_update_client_start (0000)
000A8270 0040:
	_update_client_add_player (0000)
000A82B0 0030:
	_update_client_queue (0000)
000A82E0 0020:
	_update_client_queue_push (0000)
000A8300 0010:
	_update_client_get_maximum_actions (0000)
000A8310 0050:
	_update_client_build_client_update (0000)
000A8360 0050:
	_player_new_queue (0000)
000A83B0 0060:
	_update_server_get_update (0000)
000A8410 0030:
	_update_client_get_update (0000)
000A8440 0080:
	_update_server_new (0000)
000A84C0 0060:
	_update_server_delete (0000)
000A8520 00c0:
	_update_server_start (0000)
000A85E0 00d0:
	_update_server_build_server_update (0000)
000A86B0 02d0:
	_update_client_dequeue (0000)
000A8980 0050:
	_update_client_get_maximum_possible_server_time (0000)
000A89D0 0150:
	_update_server_handle_client_update (0000)
000A8B20 00d0:
	_update_client_handle_server_update (0000)
000A8BF0 0110:
	_update_queues_reset_and_fill_with_lies (0000)
000A8D00 00e0:
	_update_server_next_update (0000)
000A8DE0 00c0:
	_update_client_local_ticks (0000)
0025C87C 0012:
	??_C@_0BC@HKAHJFKD@queue_index?$CB?$DNNONE?$AA@ (0000)
0025C890 0028:
	??_C@_0CI@PBIGBCAB@c?3?2halo?2SOURCE?2game?2player_queue@ (0000)
0025C8B8 0015:
	??_C@_0BF@LPJMFPAI@update?5client?5queues?$AA@ (0000)
0025C8D0 0023:
	??_C@_0CD@GPGLDMIM@?$CBupdate_client_globals?4initializ@ (0000)
0025C8F4 0022:
	??_C@_0CC@HHIIJIMP@update_client_globals?4initialize@ (0000)
0025C918 0037:
	??_C@_0DH@JINOOJDL@action_collection?5?$CG?$CG?5update_clie@ (0000)
0025C950 0022:
	??_C@_0CC@PJOBEMLF@update_server_globals?4initialize@ (0000)
0025C974 0015:
	??_C@_0BF@JPNPAAMJ@update?5server?5queues?$AA@ (0000)
0025C98C 0023:
	??_C@_0CD@OBACOIPG@?$CBupdate_server_globals?4initializ@ (0000)
0025C9B0 002c:
	??_C@_0CM@MEBNPPKK@machine_index?$DMMAXIMUM_NETWORK_MA@ (0000)
0025C9DC 003d:
	??_C@_0DN@MONJPEIP@update?5?$CG?$CG?5update_number?5?$CG?$CG?5updat@ (0000)
0025CA20 0062:
	??_C@_0GC@JKNAOGKH@?$CINONE?5?$DN?$DN?5actions?$FLqueue_index?$FN?4de@ (0000)
0025CA88 00b3:
	??_C@_0LD@JLDAFPEL@?$CINONE?5?$DN?$DN?5actions?$FLqueue_index?$FN?4de@ (0000)
0025CB40 00ac:
	??_C@_0KM@CAKKNBBN@?$CINONE?5?$DN?$DN?5actions?$FLqueue_index?$FN?4de@ (0000)
0025CBF0 0046:
	??_C@_0EG@JJEOPHKN@?$CINONE?5?$DN?$DN?5queue?9?$DOdesired_zoom_lev@ (0000)
0025CC38 0089:
	??_C@_0IJ@ENHLLOPB@?$CINONE?5?$DN?$DN?5queue?9?$DOdesired_grenade_@ (0000)
0025CCC8 0082:
	??_C@_0IC@PGBPIKNJ@?$CINONE?5?$DN?$DN?5queue?9?$DOdesired_weapon_i@ (0000)
0025CD4C 0029:
	??_C@_0CJ@GIKNCIIL@queue?9?$DOcurrent_action?4desired_fa@ (0000)
0025CD78 002b:
	??_C@_0CL@PLNKAOG@queue?9?$DOcurrent_action?4desired_fa@ (0000)
0025CDA8 0043:
	??_C@_0ED@GEKLIPAE@if?5you?8re?5playing?5single?5player?1@ (0000)
0025CDEC 003e:
	??_C@_0DO@EODINBPH@if?5you?8re?5in?5a?5multiplayer?5game?0@ (0000)
0025CE2C 0031:
	??_C@_0DB@BKCMOFLJ@failed?5to?5get?5an?5update?5?$CI?$CD?$CFd?$CJ?$DL?5s@ (0000)
0025CE60 0007:
	??_C@_06HDLLMMEJ@update?$AA@ (0000)
0025CE68 002a:
	??_C@_0CK@MPOAFLDM@game_connection?$CI?$CJ?$DN?$DN_game_connect@ (0000)
0043EE60 410c:
	_update_server_globals (0000)
00442F70 10494:
	_update_client_globals (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/cseries_windows.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "game/player_queues_new.h"
#include "game/players.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "memory/data.h"
#include "networking/network_game_globals.h"
#include "units/units.h"
#include "game/game_engine.h"
#include "text/unicode.h"
#include "port_config.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "items/weapons.h"
#include "tag_files/tag_files.h"
#include "ai/path.h"
#include "ai/path_structure_bsp.h"
#include "units/bipeds.h"
#include "scenario/scenario.h"
#include "game/bots.h"
#include <string.h>

/* ---------- constants */

enum
{
	/* the native builds' session limits (port/linux/include/halo_port_limits.h) */
	MAXIMUM_NUMBER_OF_PLAYERS = HALO_PORT_MAXIMUM_NETWORK_PLAYERS,
	MAXIMUM_NETWORK_MACHINE_COUNT = HALO_PORT_MAXIMUM_NETWORK_MACHINES,

	MAXIMUM_SERVER_UPDATES = 32,
	MAXIMUM_CLIENT_UPDATES = 128,

	/* the host's ticks after a client machine's player's last input that
	the player's input is held: after them it stands still
	(update_server_next_update) */
	DISTRIBUTED_INPUT_SILENCE_TICKS = 10,

	/* the one-shot buttons: they fire on the tick they are first seen and stay latched until released */
	LATCHED_CONTROL_FLAGS =
		FLAG(_unit_control_integrated_light_bit) |
		FLAG(_unit_control_action_bit) |
		FLAG(_unit_control_use_equipment_bit) |
		FLAG(_unit_control_weapon_reload_bit) |
		/* port: latched with the action it goes with (units.h) */
		FLAG(UNIT_CONTROL_PORT_ACTION_ONLY_BIT),
};

/* ---------- macros */

/* ---------- structures */

struct player_action_collection
{
	struct player_action actions[MAXIMUM_LOCAL_PLAYERS];
};

struct server_update
{
	word action_count;
	short pad;
	struct player_action actions[MAXIMUM_NUMBER_OF_PLAYERS];
};

struct update
{
	long update_number;
	struct server_update update;
};

struct update_server_queue_datum
{
	short identifier;
	short pad;
	long next_update_number;
	struct player_action current_action;
};

struct update_client_queue_datum
{
	short identifier;
	short pad;
	unsigned long control_flags;
	unsigned long latched_control_flags;
	real_euler_angles2d desired_facing;
	real_vector2d throttle;
	real primary_trigger;
	short desired_weapon_index;
	short desired_grenade_index;
	short desired_zoom_level;
	short pad2;
};

struct update_server_globals
{
	boolean initialized;
	long next_update_number_to_build;
	struct data_array *queues;
	struct update updates[MAXIMUM_SERVER_UPDATES];
};

struct update_client_globals
{
	boolean initialized;
	long next_update_number_to_dequeue;
	long latest_update_number_received;
	struct player_action_collection saved_action_collection;
	long current_local_player;
	struct data_array *queues;
	struct update updates[MAXIMUM_CLIENT_UPDATES];
};

typedef char player_action_collection_size_assert[
	sizeof(struct player_action_collection) == 0x80 ? 1 : -1];
/* the update arrays follow the session limit (the networking units' copies
of struct server_update must have the same size) */
typedef char server_update_size_assert[
	sizeof(struct server_update) == 4 + MAXIMUM_NUMBER_OF_PLAYERS * 0x20 ? 1 : -1];
typedef char update_size_assert[
	sizeof(struct update) == 4 + sizeof(struct server_update) ? 1 : -1];
typedef char update_server_queue_datum_size_assert[
	sizeof(struct update_server_queue_datum) == 0x28 ? 1 : -1];
typedef char update_client_queue_datum_size_assert[
	sizeof(struct update_client_queue_datum) == 0x28 ? 1 : -1];
typedef char update_server_globals_size_assert[
	sizeof(struct update_server_globals) == 0xC + MAXIMUM_SERVER_UPDATES * sizeof(struct update) ? 1 : -1];
typedef char update_server_globals_queues_offset_assert[
	offsetof(struct update_server_globals, queues) == 0x8 ? 1 : -1];
typedef char update_client_globals_size_assert[
	sizeof(struct update_client_globals) == 0x94 + MAXIMUM_CLIENT_UPDATES * sizeof(struct update) ? 1 : -1];
typedef char update_client_globals_saved_actions_offset_assert[
	offsetof(struct update_client_globals, saved_action_collection) == 0xC ? 1 : -1];
typedef char update_client_globals_current_local_player_offset_assert[
	offsetof(struct update_client_globals, current_local_player) == 0x8C ? 1 : -1];
typedef char update_client_globals_queues_offset_assert[
	offsetof(struct update_client_globals, queues) == 0x90 ? 1 : -1];

/* ---------- prototypes */

static struct update *update_server_get_update(
	long update_number);
static struct update *update_client_get_update(
	long update_number);
static void update_server_take_local_actions(
	void);
static void update_server_generate_bot_actions(
	void);

/* ---------- globals */

/* The host takes a client's input as it comes, each packet replacing the
last, and a client sends one a frame (several per tick): a button pressed
in only one packet between two of the host's ticks would be lost. Every
button seen since the host's last tick stays down for the next. */
static unsigned long update_server_pending_control_flags[MAXIMUM_NUMBER_OF_PLAYERS];

/* the distributed netcode (port/linux/NETCODE.md): the latest action the
host relayed for each player, the update it is of, and the buttons of every
relayed update since this client's last tick */
static struct
{
	boolean valid;
	long update_number;
	struct player_action action;
	unsigned long pending_control_flags;
} update_client_relayed_actions[MAXIMUM_NUMBER_OF_PLAYERS];

/* ... the host: the last tick of each client machine's player's it has
had (their buttons of that tick and before it are in), and the host's game
time when it had it (update_server_next_update: a machine gone silent) */
static struct
{
	boolean valid;
	long tick;
	long received_time;
} update_server_distributed_inputs[MAXIMUM_NUMBER_OF_PLAYERS];

/* ... a client: each local player's last tick, its action, and the buttons
of the ticks up to it, newest first */
static struct
{
	boolean valid;
	long tick;
	struct player_action action;
	unsigned short control_flags[DISTRIBUTED_INPUT_HISTORY];
} update_client_local_inputs[MAXIMUM_LOCAL_PLAYERS];

static struct update_server_globals update_server_globals = { 0 };
static struct update_client_globals update_client_globals = { 0 };

/* ---------- public code */

boolean update_server_new(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0xAC,
		!update_server_globals.initialized);
	csmemset(&update_server_globals, 0, sizeof(update_server_globals));
	update_server_globals.queues = data_new(
		"update server queues",
		MAXIMUM_NUMBER_OF_PLAYERS,
		sizeof(struct update_server_queue_datum));
	if (update_server_globals.queues)
	{
		csmemset(update_server_globals.updates, 0, sizeof(update_server_globals.updates));
		if (update_client_new())
		{
			update_server_globals.initialized = TRUE;
		}
	}

	return update_server_globals.initialized;
}

void update_server_delete(
	void)
{
	if (update_server_globals.queues)
	{
		data_dispose(update_server_globals.queues);
		update_server_globals.queues = NULL;
	}
	update_server_globals.initialized = FALSE;
	update_server_globals.next_update_number_to_build = 0;
	if (update_client_globals.queues)
	{
		data_dispose(update_client_globals.queues);
		update_client_globals.queues = NULL;
	}
	update_client_globals.next_update_number_to_dequeue = 0;
	update_client_globals.initialized = FALSE;
	update_client_globals.latest_update_number_received = NONE;

	return;
}

void update_server_start(
	void)
{
	struct data_iterator iterator;
	long queue_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0xCF,
		update_server_globals.initialized);
	data_make_valid(update_server_globals.queues);
	data_delete_all(update_server_globals.queues);
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		queue_index = datum_new_at_index(update_server_globals.queues, iterator.datum_index);
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0xDD,
			queue_index!=NONE);
	}
	update_client_start();

	return;
}

/* port: a player's queue is at its player's index, and the slot of a player
who left the game is reused (network_game_spawn_player): that player's queue
goes first, which datum_new_at_index would find in the way. The same player
added again keeps its queue. TRUE when the slot is the player's already */
static boolean update_queue_make_room(
	struct data_array *queues,
	long player_index)
{
	short absolute_index = (short)player_index;
	struct datum_header *header;

	if (absolute_index < 0 || absolute_index >= queues->maximum_count)
		return FALSE;
	header = (struct datum_header *)((byte *)queues->data + queues->size * absolute_index);
	if (!header->identifier)
		return FALSE;
	if (header->identifier == (short)(player_index >> 16))
		return TRUE;
	datum_delete(queues, (long)header->identifier << 16 | (unsigned short)absolute_index);

	return FALSE;
}

void update_server_add_player(
	long player_index)
{
	long queue_index;

	if (update_queue_make_room(update_server_globals.queues, player_index))
		return;
	queue_index = datum_new_at_index(update_server_globals.queues, player_index);
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0xEB,
		queue_index!=NONE);

	return;
}

void update_server_next_update(
	void)
{
	long update_number = update_server_globals.next_update_number_to_build;
	struct update *update;
	struct update_server_queue_datum *queue;
	short queue_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0xFA,
		update_server_globals.initialized);
	update_server_globals.next_update_number_to_build += 1;
	if (game_connection() == _game_connection_network_server)
	{
		update_server_take_local_actions();
		update_server_generate_bot_actions();
	}
	update = update_server_get_update(update_number);
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x100,
		update);
	update->update_number = update_number;
	update->update.action_count = 0;
	queue = (struct update_server_queue_datum *)update_server_globals.queues->data;
	for (queue_index = 0; queue_index<update_server_globals.queues->count; ++queue_index, ++queue)
	{
		/* port: a slot no player holds (the distributed netcode's players keep
		their slots in the host's player list, which may leave gaps:
		network_game_manager.c) has an idle action, not what its queue's
		memory last held (a weapon index past the unit's, which every machine
		dequeuing it asserted on) */
		if (!queue->identifier)
		{
			struct player_action *action = &update->update.actions[queue_index];

			csmemset(action, 0, sizeof(*action));
			action->desired_weapon_index = NONE;
			action->desired_grenade_index = NONE;
			action->desired_zoom_level = NONE;
			update_server_pending_control_flags[queue_index] = 0;
			update->update.action_count += 1;
			continue;
		}
		csmemcpy(
			&update->update.actions[queue_index],
			&queue->current_action,
			sizeof(struct player_action));
		update->update.actions[queue_index].control_flags |= update_server_pending_control_flags[queue_index];
		update_server_pending_control_flags[queue_index] = 0;
		/* port: a client machine's player whose input has stopped coming
		(its network lost, until the host drops it) stands still, its
		buttons and trigger let go, where it faces: its last input held
		would walk it off a ledge, or empty its weapon. (A datagram or two
		lost is not that: the last input is held a third of a second.) */
		if (update_server_distributed_inputs[queue_index].valid &&
			game_time_initialized() &&
			game_time_get() - update_server_distributed_inputs[queue_index].received_time >
				DISTRIBUTED_INPUT_SILENCE_TICKS)
		{
			struct player_action *action = &update->update.actions[queue_index];

			action->throttle.i = 0.f;
			action->throttle.j = 0.f;
			action->primary_trigger = 0.f;
			action->control_flags = 0;
		}
		update->update.action_count += 1;
	}
	update_client_handle_server_update(&update->update, update_number);

	return;
}

void update_server_build_server_update(
	long machine_index,
	struct server_update *update,
	long *update_number)
{
	long start_time = system_milliseconds();
	struct update_server_queue_datum *queue = NULL;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x11A,
		update && update_number && update_server_globals.initialized);
	if (machine_index!=NONE)
	{
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0x11E,
			machine_index<MAXIMUM_NETWORK_MACHINE_COUNT);
		queue = datum_get(update_server_globals.queues, machine_index);
		if (queue->next_update_number<update_server_globals.next_update_number_to_build)
		{
			*update_number = queue->next_update_number;
		}
		else
		{
			*update_number = NONE;
			return;
		}
	}
	if (*update_number!=NONE)
	{
		struct update *server_update = update_server_get_update(*update_number);

		if (server_update)
		{
			csmemcpy(update, &server_update->update, sizeof(*update));
		}
		if (queue)
		{
			queue->next_update_number += 1;
		}
	}

	return;
}

boolean update_client_new(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x146,
		!update_client_globals.initialized);
	csmemset(&update_client_globals, 0, sizeof(update_client_globals));
	update_client_globals.queues = data_new(
		"update client queues",
		MAXIMUM_NUMBER_OF_PLAYERS,
		sizeof(struct update_client_queue_datum));
	if (update_client_globals.queues)
	{
		csmemset(update_client_globals.updates, NONE, sizeof(update_client_globals.updates));
		update_client_globals.latest_update_number_received = NONE;
		update_client_globals.next_update_number_to_dequeue = 0;
		update_client_globals.initialized = TRUE;
	}

	return update_client_globals.initialized;
}

void update_client_delete(
	void)
{
	if (update_client_globals.queues)
	{
		data_dispose(update_client_globals.queues);
		update_client_globals.queues = NULL;
	}
	update_client_globals.latest_update_number_received = NONE;
	update_client_globals.next_update_number_to_dequeue = 0;
	update_client_globals.initialized = FALSE;

	return;
}

void update_client_start(
	void)
{
	struct data_iterator iterator;
	long queue_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x168,
		update_client_globals.initialized);
	data_make_valid(update_client_globals.queues);
	data_delete_all(update_client_globals.queues);
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		queue_index = datum_new_at_index(update_client_globals.queues, iterator.datum_index);
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0x176,
			queue_index!=NONE);
	}

	return;
}

void update_client_add_player(
	long player_index)
{
	long queue_index;

	if (update_queue_make_room(update_client_globals.queues, player_index))
		return;
	queue_index = datum_new_at_index(update_client_globals.queues, player_index);
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x182,
		queue_index!=NONE);

	return;
}

/* The native builds draw several frames per 30 Hz tick
(port/linux/game/render_interpolation.c) and build an action every frame, and
only the last one before a tick reaches it: a button pressed and released
between two ticks, or a press seen only on its first frame (zoom, grenade
and weapon switches), would be lost. Every control held on any frame since
the last tick stays held until a tick has run, and the trigger stays as far
down as it went (weapons with an analog rate of fire read that, not the
flag). */
static unsigned long update_client_pending_control_flags[MAXIMUM_LOCAL_PLAYERS];
static real update_client_pending_primary_triggers[MAXIMUM_LOCAL_PLAYERS];
static long update_client_pending_game_time = NONE;

void update_client_queue(
	struct player_action const *action)
{
	/* port: player_control.c queues the action of each local player that
	has a player, in order: kept at that local player's index (the Xbox
	game's at its count, which the dequeue and the host read as the index:
	a single player on the second controller, or players on the first and
	third, got the wrong or no action) */
	while (update_client_globals.current_local_player < MAXIMUM_LOCAL_PLAYERS &&
		local_player_get_player_index((short)update_client_globals.current_local_player) == NONE)
	{
		++update_client_globals.current_local_player;
	}
	if (update_client_globals.current_local_player < MAXIMUM_LOCAL_PLAYERS)
	{
		struct player_action *saved = &update_client_globals.saved_action_collection.actions[
			update_client_globals.current_local_player];
		unsigned long *pending = &update_client_pending_control_flags[
			update_client_globals.current_local_player];
		real *pending_primary_trigger = &update_client_pending_primary_triggers[
			update_client_globals.current_local_player];

		*saved = *action;
		*pending |= action->control_flags;
		*pending_primary_trigger = MAX(*pending_primary_trigger, action->primary_trigger);
		saved->control_flags = *pending;
		saved->primary_trigger = *pending_primary_trigger;
	}
	++update_client_globals.current_local_player;

	return;
}

void update_client_queue_push(
	void)
{
	/* while the clock is stopped no tick will take them */
	if (update_client_pending_game_time != game_time_get() ||
		game_time_get_paused() || game_time_held())
	{
		update_client_pending_game_time = game_time_get();
		csmemset(
			update_client_pending_control_flags,
			0,
			sizeof(update_client_pending_control_flags));
		csmemset(
			update_client_pending_primary_triggers,
			0,
			sizeof(update_client_pending_primary_triggers));
	}
	update_client_globals.current_local_player = 0;
	csmemset(
		&update_client_globals.saved_action_collection,
		0,
		sizeof(update_client_globals.saved_action_collection));

	return;
}

/* the local player (of this machine) controlling the player at player_index,
or NONE */
static short update_client_local_player_index(
	short player_index)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		long local_player = local_player_get_player_index(local_player_index);

		if (local_player != NONE && DATUM_INDEX_TO_ABSOLUTE_INDEX(local_player) == player_index)
			return local_player_index;
	}
	return NONE;
}

/* the distributed netcode's tick (port/linux/NETCODE.md): this machine's
players from its own input at once, the others from what the host last
relayed */
static boolean update_client_dequeue_distributed(
	struct player_action *actions)
{
	struct update_client_queue_datum *queue = (struct update_client_queue_datum *)update_client_globals.queues->data;
	short queue_index;

	for (queue_index = 0; queue_index < update_client_globals.queues->count; ++queue_index, ++queue)
	{
		struct player_action action;
		short local_player_index = update_client_local_player_index(queue_index);

		csmemset(&action, 0, sizeof(action));
		action.desired_weapon_index = NONE;
		action.desired_grenade_index = NONE;
		action.desired_zoom_level = NONE;
		if (local_player_index != NONE)
		{
			action = update_client_globals.saved_action_collection.actions[local_player_index];
			if (local_player_index < MAXIMUM_LOCAL_PLAYERS)
			{
				/* (for the host: network_distributed.c) */
				update_client_local_inputs[local_player_index].valid = TRUE;
				update_client_local_inputs[local_player_index].tick = game_time_get();
				update_client_local_inputs[local_player_index].action = action;
				csmemmove(&update_client_local_inputs[local_player_index].control_flags[1],
					&update_client_local_inputs[local_player_index].control_flags[0],
					(DISTRIBUTED_INPUT_HISTORY - 1) * sizeof(unsigned short));
				update_client_local_inputs[local_player_index].control_flags[0] = (unsigned short)action.control_flags;
			}
		}
		else if (queue_index < MAXIMUM_NUMBER_OF_PLAYERS && update_client_relayed_actions[queue_index].valid)
		{
			action = update_client_relayed_actions[queue_index].action;
			action.control_flags |= update_client_relayed_actions[queue_index].pending_control_flags;
			update_client_relayed_actions[queue_index].pending_control_flags = 0;
		}
		actions[queue_index].control_flags = action.control_flags & ~queue->latched_control_flags;
		queue->latched_control_flags = action.control_flags & LATCHED_CONTROL_FLAGS;
		actions[queue_index].desired_facing = action.desired_facing;
		actions[queue_index].throttle = action.throttle;
		actions[queue_index].primary_trigger = action.primary_trigger;
		actions[queue_index].desired_weapon_index = action.desired_weapon_index;
		actions[queue_index].desired_grenade_index = action.desired_grenade_index;
		actions[queue_index].desired_zoom_level = action.desired_zoom_level;
	}
	update_client_globals.next_update_number_to_dequeue += 1;
	return TRUE;
}

boolean update_client_dequeue(
	struct player_action *actions)
{
	struct update *update;
	struct update_client_queue_datum *queue;
	short queue_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x1AF,
		update_client_globals.initialized);
	if (game_connection() == _game_connection_network_client)
		return update_client_dequeue_distributed(actions);
	update = update_client_get_update(update_client_globals.next_update_number_to_dequeue);
	if (!update ||
		update_client_globals.next_update_number_to_dequeue>update_client_globals.latest_update_number_received ||
		update->update.action_count<=0 ||
		update->update.action_count>MAXIMUM_NUMBER_OF_PLAYERS)
	{
		return FALSE;
	}

	queue = (struct update_client_queue_datum *)update_client_globals.queues->data;
	for (queue_index = 0; queue_index<update_client_globals.queues->count; ++queue_index, ++queue)
	{
		if (queue_index<update->update.action_count)
		{
			struct player_action *action = &update->update.actions[queue_index];

			queue->control_flags = action->control_flags;
			queue->desired_facing = action->desired_facing;
			queue->throttle = action->throttle;
			queue->primary_trigger = action->primary_trigger;
			queue->desired_weapon_index = action->desired_weapon_index;
			queue->desired_grenade_index = action->desired_grenade_index;
			queue->desired_zoom_level = action->desired_zoom_level;
			match_assert(
				"c:\\halo\\SOURCE\\game\\player_queues_new.c",
				0x1C9,
				(NONE == queue->desired_weapon_index) || (queue->desired_weapon_index>=0 && queue->desired_weapon_index<MAXIMUM_WEAPONS_PER_UNIT));
			match_assert(
				"c:\\halo\\SOURCE\\game\\player_queues_new.c",
				0x1CA,
				(NONE == queue->desired_grenade_index) || (queue->desired_grenade_index>=0 && queue->desired_grenade_index<NUMBER_OF_UNIT_GRENADE_TYPES));
			match_assert(
				"c:\\halo\\SOURCE\\game\\player_queues_new.c",
				0x1CB,
				(NONE == queue->desired_zoom_level) || (queue->desired_zoom_level>=0));
		}
	}

	queue = (struct update_client_queue_datum *)update_client_globals.queues->data;
	for (queue_index = 0; queue_index<update_client_globals.queues->count; ++queue_index, ++queue)
	{
		actions[queue_index].control_flags = queue->control_flags & ~queue->latched_control_flags;
		queue->latched_control_flags = queue->control_flags & LATCHED_CONTROL_FLAGS;
		actions[queue_index].desired_facing = queue->desired_facing;
		actions[queue_index].throttle = queue->throttle;
		actions[queue_index].primary_trigger = queue->primary_trigger;
		actions[queue_index].desired_weapon_index = queue->desired_weapon_index;
		actions[queue_index].desired_grenade_index = queue->desired_grenade_index;
		actions[queue_index].desired_zoom_level = queue->desired_zoom_level;
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0x1E6,
			(NONE == actions[queue_index].desired_weapon_index) || (actions[queue_index].desired_weapon_index>=0 && actions[queue_index].desired_weapon_index<MAXIMUM_WEAPONS_PER_UNIT));
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0x1E7,
			(NONE == actions[queue_index].desired_grenade_index) || (actions[queue_index].desired_grenade_index>=0 && actions[queue_index].desired_grenade_index<NUMBER_OF_UNIT_GRENADE_TYPES));
		match_assert(
			"c:\\halo\\SOURCE\\game\\player_queues_new.c",
			0x1E8,
			(NONE == actions[queue_index].desired_zoom_level) || (actions[queue_index].desired_zoom_level>=0));
	}
	update_client_globals.next_update_number_to_dequeue += 1;

	return TRUE;
}

long update_client_get_maximum_possible_server_time(
	void)
{
	long update_number = update_client_globals.next_update_number_to_dequeue;

	while (update_number<=update_client_globals.latest_update_number_received)
	{
		struct update *update = update_client_get_update(update_number);

		if (!update ||
			update->update.action_count<=0 ||
			update->update.action_count>MAXIMUM_NUMBER_OF_PLAYERS)
		{
			break;
		}
		update_number += 1;
	}

	return update_number;
}

void update_client_local_ticks(
	short ticks)
{
	struct player_action_collection action_collection;
	struct server_update update;
	long update_number;
	long machine_index = 0;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x20B,
		game_connection()==_game_connection_local);
	update_client_build_client_update(&action_collection);
	update_server_handle_client_update(machine_index, action_collection.actions);
	while (ticks-->0)
	{
		update_server_next_update();
		update_server_build_server_update(machine_index, &update, &update_number);
	}

	return;
}

void update_server_handle_client_update(
	long machine_index,
	struct player_action *actions)
{
	long *player_list = machine_get_player_list(machine_index);
	long action_index = 0;
	long player_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x22A,
		update_server_globals.initialized);
	/* (the distributed netcode takes another machine's players' input from
	its own message, update_server_handle_distributed_input, and the host's
	own players' at each tick, update_server_take_local_actions) */
	if (game_connection() == _game_connection_network_server)
	{
		return;
	}
	for (player_index = 0; player_index<MAXIMUM_LOCAL_PLAYERS; ++player_index)
	{
		if (player_list[player_index]!=NONE)
		{
			struct update_server_queue_datum *queue = datum_get(
				update_server_globals.queues,
				player_list[player_index]);
			struct player_datum *player = player_try_and_get(player_list[player_index]);

			/* port: the actions are at their local players' indices
			(update_client_queue) */
			action_index = player ? player->local_player_index : NONE;
			if (action_index<0 || action_index>=MAXIMUM_LOCAL_PLAYERS)
			{
				continue;
			}
			queue->current_action = actions[action_index];
			update_server_pending_control_flags[DATUM_INDEX_TO_ABSOLUTE_INDEX(player_list[player_index])] |=
				queue->current_action.control_flags;
			match_assert_valid_real(
				"c:\\halo\\SOURCE\\game\\player_queues_new.c",
				0x238,
				queue->current_action.desired_facing.pitch);
			match_assert_valid_real(
				"c:\\halo\\SOURCE\\game\\player_queues_new.c",
				0x239,
				queue->current_action.desired_facing.yaw);
		}
	}

	return;
}

void update_client_build_client_update(
	struct player_action_collection *action_collection)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x244,
		action_collection && update_client_globals.initialized);
	csmemcpy(
		action_collection,
		&update_client_globals.saved_action_collection,
		sizeof(*action_collection));

	return;
}

void update_client_handle_server_update(
	struct server_update *update,
	long update_number)
{
	struct update *client_update = update_client_get_update(update_number);

	if (client_update)
	{
		client_update->update_number = update_number;
		csmemcpy(&client_update->update, update, sizeof(client_update->update));
		if (update_number>update_client_globals.latest_update_number_received)
		{
			if (update_client_globals.latest_update_number_received+1<update_number)
			{
				client_update->update.action_count = NONE;
			}
			update_client_globals.latest_update_number_received = update_number;
		}
	}
	else if (!global_network_game_client_get() || !global_network_game_server_get())
	{
		/* (only the host and a local game build updates now, which go
		out of sync with nobody) */
		if (!main_menu_is_active())
		{
			error(_error_silent, "failed to get an update (#%d); sp scenario= '%s'", update_number, main_get_map_name());
		}
	}

	switch (game_connection())
	{
	case _game_connection_local:
	case _game_connection_network_client:
	case _game_connection_network_server:
		/* film recording of the update was compiled out of this build; only its menu check remains */
		main_menu_is_active();
		break;
	}

	return;
}

/* port: the distributed netcode's record of each player's latest input,
forgotten for a new game (network_distributed_new_game) as after loading
one: a game counts its ticks and the host its updates from the start again,
so the last game's latest, kept, would be later than any of this game's and
every new input taken for an old one (the players driven by the last game's
last input: aiming where they last aimed, running if they last ran) */
void update_queues_distributed_reset(
	void)
{
	csmemset(update_server_pending_control_flags, 0, sizeof(update_server_pending_control_flags));
	csmemset(update_client_relayed_actions, 0, sizeof(update_client_relayed_actions));
	csmemset(update_server_distributed_inputs, 0, sizeof(update_server_distributed_inputs));
	csmemset(update_client_local_inputs, 0, sizeof(update_client_local_inputs));
}

void update_queues_reset_and_fill_with_lies(
	void)
{
	update_queues_distributed_reset();
	if (update_server_globals.initialized)
	{
		update_server_globals.next_update_number_to_build = 0;
		csmemset(update_server_globals.updates, 0, sizeof(update_server_globals.updates));
	}
	if (update_client_globals.initialized)
	{
		long game_time;
		long update_number;
		long update_index;

		csmemset(update_client_globals.updates, NONE, sizeof(update_client_globals.updates));
		csmemset(
			&update_client_globals.saved_action_collection,
			0,
			sizeof(update_client_globals.saved_action_collection));
		update_client_globals.latest_update_number_received = NONE;
		update_client_globals.next_update_number_to_dequeue = 0;
		game_time = game_time_get();
		update_number = MAX(0, game_time-MAXIMUM_CLIENT_UPDATES);
		for (update_index = 0; update_number<game_time; ++update_index, ++update_number)
		{
			update_client_globals.updates[update_index].update_number = update_number;
			update_client_globals.updates[update_index].update.action_count = 1;
			csmemset(
				update_client_globals.updates[update_index].update.actions,
				0,
				sizeof(update_client_globals.updates[update_index].update.actions));
		}
		update_client_globals.next_update_number_to_dequeue = game_time;
		update_client_globals.latest_update_number_received = game_time-1;
		update_server_globals.next_update_number_to_build = game_time;
	}
	if (update_server_globals.initialized)
	{
		struct data_iterator iterator;
		struct update_server_queue_datum *queue;

		update_server_start();
		/* port: co-op player slots can have gaps, including slot zero.
		Reset every live queue after a checkpoint load, not just slot zero. */
		data_iterator_new(&iterator, update_server_globals.queues);
		while ((queue = data_iterator_next(&iterator)) != NULL)
			queue->next_update_number = update_server_globals.next_update_number_to_build;
	}
	else if (update_client_globals.initialized)
	{
		update_client_start();
	}

	return;
}

long player_new_queue(
	long player_index)
{
	long queue_index;

	queue_index = datum_new_at_index(update_server_globals.queues, player_index);
	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x292,
		queue_index!=NONE);

	return queue_index;
}

/* the distributed netcode's host: its own players take their input at its
tick, as a client's own do (update_client_dequeue_distributed), not from
its own client's update, which goes ten times a second (a press between two
was lost, and the aim moved in steps) */
static void update_server_take_local_actions(
	void)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		long player_index = local_player_get_player_index(local_player_index);
		struct update_server_queue_datum *queue = player_index != NONE ?
			(struct update_server_queue_datum *)datum_try_and_get(update_server_globals.queues, player_index) : NULL;

		if (queue)
		{
			queue->current_action = update_client_globals.saved_action_collection.actions[local_player_index];
		}
	}
}

/* H1PB Beta 4 combat tables (blam_server, skill 0..3). Reaction is
350, 280, 210, 150 ms at the 30 Hz simulation. */
static real const bot_aim_error[4] = { 0.16f, 0.09f, 0.055f, 0.03f };
static real const bot_aim_slack[4] = { 0.10f, 0.07f, 0.05f, 0.035f };
static long const bot_reaction_ticks[4] = { 11, 8, 6, 5 };
static long const bot_grenade_chance[4] = { 2, 4, 6, 8 };
static long const bot_jump_chance[4] = { 0, 2, 4, 6 };

/* name, score, style (0 single, 1 automatic, 2 explosive), near, far.
plasma pistol is listed before pistol so the longer name wins. */
struct bot_weapon_kind
{
	char const *name;
	long score;
	long style;
	real near_range;
	real far_range;
};

static struct bot_weapon_kind const bot_weapon_kinds[] =
{
	{ "rocket launcher", 9, 2, 7.0f, 18.0f },
	{ "sniper rifle", 8, 0, 10.0f, 28.0f },
	{ "shotgun", 8, 0, 1.5f, 5.0f },
	{ "plasma pistol", 3, 0, 3.0f, 12.0f },
	{ "pistol", 8, 0, 4.0f, 16.0f },
	{ "plasma cannon", 7, 2, 7.0f, 18.0f },
	{ "plasma_cannon", 7, 2, 7.0f, 18.0f },
	{ "assault rifle", 6, 1, 2.5f, 10.0f },
	{ "plasma rifle", 6, 1, 3.0f, 12.0f },
	{ "needler", 5, 1, 4.0f, 14.0f },
	{ "flamethrower", 4, 1, 1.5f, 5.0f }
};

/* one bot's aim and path, kept across ticks */
struct bot_state
{
	long life_unit;
	boolean started;
	long target_index;
	boolean target_visible;
	long reaction_ticks;
	long unseen_ticks;
	real yaw_error;
	real pitch_error;
	real strafe;
	real strafe_strength;
	long strafe_ticks;
	real wander_yaw;
	long look_ticks;
	long stuck_ticks;
	long melee_ticks;
	long reload_ticks;
	long grenade_ticks;
	long burst_ticks;
	long burst_pause;
	long switch_ticks;
	short held_slot;
	real facing_yaw;
	real facing_pitch;
	real_point3d last_position;
	real_point3d last_seen;
	boolean last_seen_valid;
	real_point3d roam_goal;
	boolean roam_valid;
	struct path_result route;
	long route_ticks;
	real_point3d route_goal;
};

static struct bot_state bot_states[MAXIMUM_NUMBER_OF_PLAYERS];
static unsigned long bot_rng = 1;

/* the same generator H1PB uses, so a roll of 0..65535 maps through 2^-15 */
static unsigned long bot_random(void)
{
	bot_rng = bot_rng * 0x19660dul + 0x3c6ef35ful;
	return bot_rng;
}

static real bot_random_real(void)
{
	return (real)((bot_random() >> 8) & 0xffff) * 0.000030517578125f - 1.0f;
}

/* one seat a lobby bot occupies. skill is H1PB's 0..3 */
struct bot_seat
{
	boolean used;
	short machine_index;
	short controller_index;
	long skill;
};

static struct bot_seat bot_seats[MAXIMUM_LOCAL_PLAYERS];

static struct bot_seat *bot_seat_find(short machine_index, short controller_index)
{
	long index;

	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		if (bot_seats[index].used && bot_seats[index].machine_index == machine_index &&
			bot_seats[index].controller_index == controller_index)
			return &bot_seats[index];
	}
	return NULL;
}

void bot_controller_set(short machine_index, short controller_index, long skill)
{
	struct bot_seat *seat;
	long index;

	if (skill < 0)
		skill = 0;
	if (skill > 3)
		skill = 3;
	seat = bot_seat_find(machine_index, controller_index);
	if (!seat)
	{
		for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
		{
			if (!bot_seats[index].used)
			{
				seat = &bot_seats[index];
				break;
			}
		}
	}
	if (!seat)
		return;
	seat->used = TRUE;
	seat->machine_index = machine_index;
	seat->controller_index = controller_index;
	seat->skill = skill;
}

void bot_controller_clear(short machine_index, short controller_index)
{
	struct bot_seat *seat = bot_seat_find(machine_index, controller_index);

	if (seat)
		seat->used = FALSE;
}

boolean bot_controller_is_bot(short machine_index, short controller_index)
{
	return bot_seat_find(machine_index, controller_index) != NULL;
}

long bot_controller_skill(short machine_index, short controller_index)
{
	struct bot_seat *seat = bot_seat_find(machine_index, controller_index);

	return seat ? seat->skill : NONE;
}

/* structure and the objects that block sight. A biped is not one of them,
so the ray can end on the player it is aimed at */
static boolean bot_can_see(real_point3d const *from, real_point3d const *to, long ignore_object_index)
{
	struct collision_result collision;
	unsigned long flags;

	flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_objects_bit) |
		_collision_test_objects_sight_blocking_flags;
	return !collision_test_line(flags, from, to, ignore_object_index, &collision);
}

/* the longest table name contained in the weapon tag. plasma pistol beats pistol */
static struct bot_weapon_kind const *bot_weapon_kind(long weapon_index)
{
	struct weapon_datum *weapon;
	char const *name;
	long index;
	struct bot_weapon_kind const *best;
	long best_length;

	if (weapon_index == NONE)
		return NULL;
	weapon = weapon_get(weapon_index);
	name = tag_get_name(weapon->definition_index);
	if (!name)
		return NULL;
	best = NULL;
	best_length = 0;
	for (index = 0; index < (long)NUMBEROF(bot_weapon_kinds); index++)
	{
		long length;

		if (!strstr(name, bot_weapon_kinds[index].name))
			continue;
		length = (long)strlen(bot_weapon_kinds[index].name);
		if (length > best_length)
		{
			best = &bot_weapon_kinds[index];
			best_length = length;
		}
	}
	return best;
}

/* H1PB weapon_ready: a loaded round, or an energy gun with no ballistic reserve */
static boolean bot_weapon_ready(long weapon_index)
{
	struct weapon_datum *weapon;
	short magazine;
	boolean loaded;
	boolean spare;

	if (weapon_index == NONE || !weapon_can_be_fired(weapon_index))
		return FALSE;
	weapon = weapon_get(weapon_index);
	loaded = FALSE;
	spare = FALSE;
	for (magazine = 0; magazine < 2; magazine++)
	{
		if (weapon->weapon.magazines[magazine].rounds_loaded > 0 ||
			weapon->weapon.magazines[magazine].rounds_fractional_recharged > 0)
			loaded = TRUE;
		if (weapon->weapon.magazines[magazine].rounds_total > 0)
			spare = TRUE;
	}
	if (loaded)
		return TRUE;
	return !spare;
}

static boolean bot_weapon_has_reserve(long weapon_index)
{
	struct weapon_datum *weapon;
	short magazine;

	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_get(weapon_index);
	for (magazine = 0; magazine < 2; magazine++)
	{
		if (weapon->weapon.magazines[magazine].rounds_total > 0)
			return TRUE;
	}
	return FALSE;
}

/* a ballistic magazine that is empty, and not already cycling. An energy
gun keeps its charge in the loaded or fractional field, so it is not this */
static boolean bot_should_reload(long weapon_index)
{
	struct weapon_datum *weapon;
	short magazine;
	boolean spare;

	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_get(weapon_index);
	if (weapon->weapon.state == _weapon_state_primary_reload ||
		weapon->weapon.state == _weapon_state_secondary_reload ||
		weapon->weapon.state == _weapon_state_primary_chamber ||
		weapon->weapon.state == _weapon_state_secondary_chamber ||
		weapon->weapon.state == _weapon_state_primary_recoil ||
		weapon->weapon.state == _weapon_state_secondary_recoil ||
		weapon->weapon.state == _weapon_state_put_away)
		return FALSE;
	spare = FALSE;
	for (magazine = 0; magazine < 2; magazine++)
	{
		if (weapon->weapon.magazines[magazine].rounds_loaded > 0 ||
			weapon->weapon.magazines[magazine].rounds_fractional_recharged > 0)
			return FALSE;
		if (weapon->weapon.magazines[magazine].rounds_total > 0)
			spare = TRUE;
	}
	return spare;
}

/* prefer a carried gun whose near/far band contains the fight, else the highest score */
static short bot_choose_slot(long unit_index, real distance)
{
	short slot;
	short best_slot;
	short band_slot;
	long best_score;
	long band_score;

	best_slot = 0;
	band_slot = NONE;
	best_score = -1;
	band_score = -1;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon_index;
		struct bot_weapon_kind const *kind;
		long score;
		boolean usable;

		weapon_index = unit_inventory_get_weapon(unit_index, slot);
		if (weapon_index == NONE)
			continue;
		usable = bot_weapon_ready(weapon_index) || bot_weapon_has_reserve(weapon_index);
		if (!usable)
			continue;
		kind = bot_weapon_kind(weapon_index);
		score = kind ? kind->score : 1;
		if (score > best_score)
		{
			best_score = score;
			best_slot = slot;
		}
		if (kind && distance >= kind->near_range && distance <= kind->far_range && score > band_score)
		{
			band_score = score;
			band_slot = slot;
		}
	}
	if (band_slot != NONE)
		return band_slot;
	return best_slot;
}

/* a step of 1.5 to that side, the distance H1PB's side_clear tests */
static boolean bot_side_clear(struct unit_datum *unit, real_point3d const *origin, long unit_index, long direction)
{
	real_point3d from;
	real_point3d to;

	from = *origin;
	from.z += 0.62f;
	to.x = origin->x + unit->object.forward.j * (real)direction * 1.5f;
	to.y = origin->y - unit->object.forward.i * (real)direction * 1.5f;
	to.z = origin->z + 0.31f;
	return bot_can_see(&from, &to, unit_index);
}

/* cone is atan(0.30 / range) plus the skill slack, widened by 1.5 while strafing */
static boolean bot_on_target(real_point3d const *eye, real_point3d const *target,
	real_vector3d const *aim, long skill, boolean widen)
{
	real_vector3d delta;
	real range;
	real cone;

	vector_from_points3d(eye, target, &delta);
	range = normalize3d(&delta);
	if (range == 0.0f)
		return TRUE;
	if (range < 0.5f)
		range = 0.5f;
	cone = arctangent(0.30f, range) + bot_aim_slack[skill];
	if (widen)
		cone *= 1.5f;
	return dot_product3d(&delta, aim) >= cosine(cone);
}

static real bot_angle_delta(real from, real to)
{
	real delta = to - from;

	while (delta > _pi)
		delta -= _pi * 2.0f;
	while (delta < -_pi)
		delta += _pi * 2.0f;
	return delta;
}

static real bot_slew(real current, real goal, real limit)
{
	real delta = bot_angle_delta(current, goal);

	if (delta > limit)
		delta = limit;
	if (delta < -limit)
		delta = -limit;
	return current + delta;
}

static real bot_yaw_between(real_point3d const *from, real_point3d const *to)
{
	return arctangent(to->y - from->y, to->x - from->x);
}

/* a chest-high ray. A wall closer than the probe stops that step */
static boolean bot_ray_clear(real_point3d const *feet, real yaw, real distance, long ignore_object_index)
{
	real_point3d from;
	real_point3d to;

	from = *feet;
	from.z += 0.45f;
	to.x = from.x + cosine(yaw) * distance;
	to.y = from.y + sine(yaw) * distance;
	to.z = from.z;
	return bot_can_see(&from, &to, ignore_object_index);
}

/* the first open heading, starting at the one toward the goal */
static boolean bot_clear_yaw(real_point3d const *feet, real desired, long ignore_object_index, real *yaw)
{
	static real const offsets[] = {
		0.0f, 0.35f, -0.35f, 0.7f, -0.7f, 1.15f, -1.15f, 1.7f, -1.7f, 2.4f, -2.4f
	};
	long index;

	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real candidate = desired + offsets[index];

		if (bot_ray_clear(feet, candidate, 0.65f, ignore_object_index) &&
			bot_ray_clear(feet, candidate, 1.3f, ignore_object_index))
		{
			*yaw = candidate;
			return TRUE;
		}
	}
	return FALSE;
}

/* H1PB's engine navigator: the game's own ground path, rebuilt when the
goal moves. The memory is one path at a time, shared by every bot */
static void bot_refresh_route(struct bot_state *state, long unit_index, real_point3d const *feet,
	real_point3d const *goal)
{
	struct path_input input;
	real_point3d start_point;
	real_point2d known;
	real_point2d query;
	struct structure_bsp *bsp;
	long start_surface;
	long goal_surface;
	real bound;
	static struct path_state memory;

	state->route_ticks++;
	if (state->route.valid && state->route_ticks < 20 &&
		distance2d((real_point2d const *)&state->route_goal, (real_point2d const *)goal) < 3.0f)
		return;
	state->route.valid = FALSE;
	state->route_ticks = 0;
	state->route_goal = *goal;
	if (unit_get(unit_index)->object.type != _object_type_biped)
		return;
	start_surface = biped_find_pathfinding_surface_index(unit_index, &start_point);
	if (start_surface == NONE)
		return;
	bsp = global_structure_bsp_get();
	if (!bsp)
		return;
	known.x = start_point.x;
	known.y = start_point.y;
	query.x = goal->x;
	query.y = goal->y;
	goal_surface = structure_surface_index_from_point(bsp, FALSE, &known, start_surface, &query);
	if (goal_surface == NONE)
		return;
	path_input_new(&input, 0.3f, FALSE, unit_index);
	path_input_set_start(&input, &start_point, start_surface);
	bound = distance3d(&start_point, goal) + 8.0f;
	if (bound < 16.0f)
		bound = 16.0f;
	path_input_set_search_bounds(&input, bound);
	path_state_new(&input, &memory, NULL);
	path_state_destination(&memory, goal, goal_surface, 0.8f);
	if (!path_state_find(&memory) || !path_state_build_path(&memory, &state->route))
		state->route.valid = FALSE;
}

/* walk toward goal around walls. throttle is in the facing's frame:
positive j is to the right. returns FALSE when every probe hits */
static boolean bot_walk(struct bot_state *state, long unit_index, real_point3d const *feet,
	real_point3d const *goal, struct player_action *action)
{
	real_point3d step;
	real desired;
	real walk;
	real rel;
	long guard;

	bot_refresh_route(state, unit_index, feet, goal);
	step = *goal;
	if (state->route.valid)
	{
		guard = 0;
		while (state->route.step_index < state->route.step_count && guard < 4)
		{
			real dx;
			real dy;

			step = state->route.steps[state->route.step_index].point;
			dx = step.x - feet->x;
			dy = step.y - feet->y;
			if (square_root(dx * dx + dy * dy) > 0.45f ||
				state->route.step_index + 1 >= state->route.step_count)
				break;
			state->route.step_index++;
			guard++;
		}
	}
	desired = bot_yaw_between(feet, &step);
	if (!bot_clear_yaw(feet, desired, unit_index, &walk))
	{
		action->throttle.i = 0.0f;
		action->throttle.j = 0.0f;
		return FALSE;
	}
	rel = bot_angle_delta(state->facing_yaw, walk);
	action->throttle.i = cosine(rel);
	action->throttle.j = -sine(rel);
	return TRUE;
}

/* the facing the game uses for a player's aim, plus this bot's current error,
written on the action and on a local player's camera */
static void bot_face(struct player_datum *player, struct player_action *action,
	real_point3d const *from, real_point3d const *to, real yaw_error, real pitch_error,
	real_vector3d *aim)
{
	real_vector3d direction;
	real_euler_angles2d angles;

	vector_from_points3d(from, to, &direction);
	if (normalize3d(&direction) == 0.0f)
		return;
	euler_angles2d_from_vector3d(&angles, &direction);
	angles.yaw += yaw_error;
	angles.pitch = PIN(angles.pitch + pitch_error, -1.2f, 1.2f);
	vector3d_from_euler_angles2d(&direction, &angles);
	if (aim)
		*aim = direction;
	if (player->local_player_index >= 0 && player->local_player_index < MAXIMUM_LOCAL_PLAYERS)
	{
		player_control_set_facing(player->local_player_index, &direction);
		action->desired_facing = *player_control_get_facing_angles(player->local_player_index);
	}
	else
	{
		if (angles.yaw < 0.0f)
			angles.yaw += _pi * 2.0f;
		action->desired_facing = angles;
	}
}

/* the host fills in a player whose name starts with Bot. Skill 0 to 3
uses H1PB Beta 4's reaction, aim error, aim cone, weapon bands and strafe */
static void update_server_generate_bot_actions(
	void)
{
	long player_index;
	long skill;
	long config_skill;

	if (game_connection() != _game_connection_network_server)
		return;
	if (!game_in_progress() || !player_data || !player_data->valid)
		return;

	config_skill = config_integer("network.bot_skill");
	if (config_skill < 0)
		config_skill = 0;
	if (config_skill > 3)
		config_skill = 3;
	skill = config_skill;

	for (player_index = 0; player_index < MAXIMUM_NUMBER_OF_PLAYERS; player_index++)
	{
		struct player_datum *player;
		struct unit_datum *unit;
		struct update_server_queue_datum *queue;
		struct player_action action;
		struct bot_state *state;
		struct game_variant *variant;
		real_point3d feet;
		real_point3d eye;
		real_point3d aim_point;
		real_vector3d aim;
		real moved;
		real distance;
		real near_range;
		real far_range;
		long seen_index;
		long heard_index;
		long engage;
		long old_target;
		real seen_distance;
		real heard_distance;
		boolean was_visible;
		boolean visible;
		boolean heard;
		long weapon_index;
		struct bot_weapon_kind const *kind;
		short chosen;
		long style;
		long target_player_index;

		player = player_try_and_get(player_index);
		if (!player || !network_player_is_valid(&player->network_player_data))
			continue;
		if (!bot_controller_is_bot(player->network_player_data.machine_index,
				player->network_player_data.controller_index) &&
			ustrncmp(player->name, L"Bot", 3) != 0)
			continue;
		{
			long seat_skill = bot_controller_skill(player->network_player_data.machine_index,
				player->network_player_data.controller_index);

			skill = seat_skill == NONE ? config_skill : seat_skill;
		}
		if (player->unit_index == NONE)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		queue = datum_try_and_get(update_server_globals.queues, player_index);
		if (!queue)
			continue;

		state = &bot_states[player_index];
		if (!state->started || state->life_unit != player->unit_index)
		{
			csmemset(state, 0, sizeof(*state));
			state->started = TRUE;
			state->life_unit = player->unit_index;
			state->target_index = NONE;
			state->held_slot = NONE;
			state->strafe_strength = 0.8f;
			state->facing_yaw = arctangent(unit->object.forward.j, unit->object.forward.i);
			object_get_origin(player->unit_index, &state->last_position);
		}
		if (state->reaction_ticks > 0)
			state->reaction_ticks--;
		if (state->strafe_ticks > 0)
			state->strafe_ticks--;
		if (state->look_ticks > 0)
			state->look_ticks--;
		if (state->melee_ticks > 0)
			state->melee_ticks--;
		if (state->reload_ticks > 0)
			state->reload_ticks--;
		if (state->grenade_ticks > 0)
			state->grenade_ticks--;
		if (state->switch_ticks > 0)
			state->switch_ticks--;
		if (state->burst_pause > 0)
			state->burst_pause--;

		csmemset(&action, 0, sizeof(action));
		action.desired_weapon_index = NONE;
		action.desired_grenade_index = NONE;
		action.desired_zoom_level = NONE;
		object_get_origin(player->unit_index, &feet);
		eye = feet;
		eye.z += 0.62f;
		moved = distance3d(&state->last_position, &feet);
		state->last_position = feet;
		aim = unit->object.forward;
		variant = game_engine_get_variant();

		seen_index = NONE;
		heard_index = NONE;
		seen_distance = 1.0e9f;
		heard_distance = 1.0e9f;
		for (target_player_index = 0; target_player_index < MAXIMUM_NUMBER_OF_PLAYERS; target_player_index++)
		{
			struct player_datum *target_player;
			struct unit_datum *target_unit;
			real_point3d point;
			real_point3d point_eye;
			real dx;
			real dy;
			real horiz;
			real separation;

			if (target_player_index == player_index)
				continue;
			target_player = player_try_and_get(target_player_index);
			if (!target_player || !network_player_is_valid(&target_player->network_player_data))
				continue;
			if (target_player->unit_index == NONE)
				continue;
			if (variant && variant->universal_variant.teams &&
				target_player->team_index == player->team_index)
				continue;
			target_unit = unit_try_and_get(target_player->unit_index);
			if (!target_unit || TEST_FLAG(target_unit->object.damage_flags, _object_dead_bit))
				continue;

			object_get_origin(target_player->unit_index, &point);
			point_eye = point;
			point_eye.z += 0.62f;
			dx = point.x - feet.x;
			dy = point.y - feet.y;
			horiz = square_root(dx * dx + dy * dy);
			separation = distance3d(&eye, &point_eye);
			if (bot_can_see(&eye, &point_eye, player->unit_index))
			{
				if (separation < seen_distance)
				{
					seen_distance = separation;
					seen_index = target_player_index;
				}
			}
			else if (horiz <= 8.0f && fabs(point.z - feet.z) <= 2.5f && horiz < heard_distance)
			{
				heard_distance = horiz;
				heard_index = target_player_index;
			}
		}

		old_target = state->target_index;
		was_visible = state->target_visible;
		if (seen_index != NONE)
			engage = seen_index;
		else if (old_target != NONE && state->unseen_ticks < 90)
			engage = old_target;
		else if (heard_index != NONE)
			engage = heard_index;
		else
			engage = NONE;

		visible = FALSE;
		heard = FALSE;
		distance = 12.0f;
		aim_point = eye;
		if (engage != NONE)
		{
			struct player_datum *target_player;
			struct unit_datum *target_unit;
			real_point3d point;
			real dx;
			real dy;
			real horiz;

			target_player = player_try_and_get(engage);
			target_unit = NULL;
			if (target_player && target_player->unit_index != NONE)
				target_unit = unit_try_and_get(target_player->unit_index);
			if (!target_unit || TEST_FLAG(target_unit->object.damage_flags, _object_dead_bit))
				engage = NONE;
			else
			{
				object_get_origin(target_player->unit_index, &point);
				aim_point = point;
				aim_point.z += 0.62f;
				dx = point.x - feet.x;
				dy = point.y - feet.y;
				horiz = square_root(dx * dx + dy * dy);
				visible = bot_can_see(&eye, &aim_point, player->unit_index);
				heard = !visible && horiz <= 8.0f && fabs(point.z - feet.z) <= 2.5f;
				if (visible || heard)
				{
					state->last_seen = aim_point;
					state->last_seen_valid = TRUE;
					if (visible)
						state->unseen_ticks = 0;
				}
				else if (state->last_seen_valid)
					aim_point = state->last_seen;
				if (!visible)
					state->unseen_ticks++;
				distance = distance3d(&eye, &aim_point);
				if (!visible && !heard && state->last_seen_valid && distance < 1.5f)
				{
					state->target_index = NONE;
					state->last_seen_valid = FALSE;
					state->look_ticks = 0;
					engage = NONE;
				}
			}
		}
		state->target_visible = visible;

		if (engage != old_target)
		{
			state->target_index = engage;
			state->reaction_ticks = engage == NONE ? 0 : bot_reaction_ticks[skill];
			state->unseen_ticks = 0;
			state->yaw_error = bot_random_real() * bot_aim_error[skill];
			state->pitch_error = bot_random_real() * bot_aim_error[skill] * 0.5f;
			state->burst_pause = 0;
		}
		else if (visible && !was_visible)
			state->reaction_ticks = bot_reaction_ticks[skill];

		weapon_index = unit_inventory_get_weapon(player->unit_index, unit->unit.current_weapon_index);
		kind = bot_weapon_kind(weapon_index);
		near_range = kind ? kind->near_range : 2.5f;
		far_range = kind ? kind->far_range : 12.0f;
		style = kind ? kind->style : 1;
		if (state->switch_ticks <= 0)
		{
			chosen = bot_choose_slot(player->unit_index, engage == NONE ? 12.0f : distance);
			if (chosen != unit->unit.current_weapon_index)
			{
				state->held_slot = chosen;
				state->switch_ticks = 45;
			}
		}
		if (state->held_slot != NONE && state->held_slot != unit->unit.current_weapon_index)
			action.desired_weapon_index = state->held_slot;
		if (bot_should_reload(weapon_index))
			action.control_flags |= FLAG(_unit_control_weapon_reload_bit);

		{
			real_point3d move_goal;
			real_point3d look;
			real goal_yaw;
			real goal_pitch;
			real limit;
			boolean walking;

			move_goal = feet;
			goal_yaw = state->facing_yaw;
			goal_pitch = 0.0f;
			if (engage == NONE)
			{
				real dx;
				real dy;
				real turn;

				dx = state->roam_goal.x - feet.x;
				dy = state->roam_goal.y - feet.y;
				if (!state->roam_valid || state->look_ticks <= 0 ||
					square_root(dx * dx + dy * dy) < 1.5f)
				{
					turn = 0.35f + (real)fabs(bot_random_real()) * 0.55f;
					if (bot_random() & 0x100ul)
						turn = -turn;
					state->wander_yaw = state->facing_yaw + turn;
					turn = 8.0f + (real)((bot_random() >> 8) % 7);
					state->roam_goal.x = feet.x + cosine(state->wander_yaw) * turn;
					state->roam_goal.y = feet.y + sine(state->wander_yaw) * turn;
					state->roam_goal.z = feet.z;
					state->roam_valid = TRUE;
					state->look_ticks = 40 + (long)((bot_random() >> 8) % 40);
				}
				move_goal = state->roam_goal;
				goal_yaw = bot_yaw_between(&feet, &move_goal);
			}
			else
			{
				real_vector3d direction;
				real_euler_angles2d angles;

				vector_from_points3d(&eye, &aim_point, &direction);
				if (normalize3d(&direction) != 0.0f)
				{
					euler_angles2d_from_vector3d(&angles, &direction);
					goal_yaw = angles.yaw + state->yaw_error;
					goal_pitch = PIN(angles.pitch + state->pitch_error, -1.0f, 1.0f);
				}
				if (visible && distance < near_range)
				{
					real away = bot_yaw_between(&feet, &aim_point) + _pi;

					move_goal.x = feet.x + cosine(away) * 6.0f;
					move_goal.y = feet.y + sine(away) * 6.0f;
					move_goal.z = feet.z;
				}
				else
					move_goal = aim_point;
			}
			limit = engage != NONE && visible ? 0.14f : 0.05f;
			state->facing_yaw = bot_slew(state->facing_yaw, goal_yaw, limit);
			state->facing_pitch = bot_slew(state->facing_pitch, goal_pitch, 0.05f);
			look.x = eye.x + cosine(state->facing_yaw) * cosine(state->facing_pitch);
			look.y = eye.y + sine(state->facing_yaw) * cosine(state->facing_pitch);
			look.z = eye.z + sine(state->facing_pitch);
			bot_face(player, &action, &eye, &look, 0.0f, 0.0f, &aim);
			walking = bot_walk(state, player->unit_index, &feet, &move_goal, &action);
			if (walking && engage != NONE && state->strafe_ticks <= 0)
			{
				state->strafe_ticks = (long)((bot_random() >> 8) % 25) + 8;
				if ((long)((bot_random() >> 8) % 100) < bot_jump_chance[skill])
					action.control_flags |= FLAG(_unit_control_jump_bit);
			}
		}
		if (engage != NONE)
		{
			boolean widen;
			boolean on_target;
			short grenade;

			widen = action.throttle.j > 0.25f || action.throttle.j < -0.25f;
			on_target = visible && bot_on_target(&eye, &aim_point, &aim, skill, widen);

			if (visible && distance < 1.5f && state->melee_ticks <= 0)
			{
				action.control_flags |= FLAG(_unit_control_action_bit);
				state->melee_ticks = 15;
			}
			else if (visible && state->grenade_ticks <= 0 && distance >= 5.0f && distance <= 18.0f)
			{
				grenade = NONE;
				if (unit->unit.current_grenade_index >= 0 &&
					unit->unit.current_grenade_index < NUMBER_OF_UNIT_GRENADE_TYPES &&
					unit->unit.grenade_counts[(long)unit->unit.current_grenade_index] > 0)
					grenade = unit->unit.current_grenade_index;
				else
				{
					short type;

					for (type = 0; type < NUMBER_OF_UNIT_GRENADE_TYPES; type++)
					{
						if (unit->unit.grenade_counts[type] > 0)
						{
							grenade = type;
							break;
						}
					}
				}
				if (grenade != NONE && (long)((bot_random() >> 8) % 100) < bot_grenade_chance[skill])
				{
					action.desired_grenade_index = grenade;
					action.control_flags |= FLAG(_unit_control_throw_grenade_bit);
				}
				state->grenade_ticks = 30;
			}
			else if (state->reaction_ticks <= 0 && on_target && bot_weapon_ready(weapon_index) &&
				!(style == 2 && distance < near_range))
			{
				boolean shoot;

				shoot = FALSE;
				if (style == 1)
					shoot = TRUE;
				else if (state->burst_pause <= 0)
				{
					shoot = TRUE;
					state->burst_pause = style == 2 ? 20 : (far_range >= 20.0f ? 14 : 8);
				}
				if (shoot)
				{
					action.primary_trigger = 1.0f;
					action.control_flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
				}
			}
		}

		if (moved < 0.02f && action.throttle.i > 0.0f)
			state->stuck_ticks++;
		else
			state->stuck_ticks = 0;
		if (state->stuck_ticks > 20)
		{
			action.control_flags |= FLAG(_unit_control_jump_bit);
			state->strafe = -state->strafe;
			state->stuck_ticks = 0;
		}

		queue->current_action = action;
	}
}

/* an action fit to take: finite, its choices in range */
static boolean distributed_action_valid(
	struct player_action const *action)
{
	return valid_real(action->desired_facing.yaw) && valid_real(action->desired_facing.pitch) &&
		valid_real(action->throttle.i) && valid_real(action->throttle.j) && valid_real(action->primary_trigger) &&
		(action->desired_weapon_index == NONE ||
			(action->desired_weapon_index >= 0 && action->desired_weapon_index < MAXIMUM_WEAPONS_PER_UNIT)) &&
		(action->desired_grenade_index == NONE ||
			(action->desired_grenade_index >= 0 && action->desired_grenade_index < NUMBER_OF_UNIT_GRENADE_TYPES)) &&
		(action->desired_zoom_level == NONE || action->desired_zoom_level >= 0);
}

/* the buttons of the ticks after last up to tick (control_flags newest
first), added to pending; FALSE for a tick already had */
static boolean distributed_add_new_ticks(
	boolean *valid,
	long *last,
	long tick,
	unsigned short const *control_flags,
	short count,
	unsigned long *pending)
{
	short index;

	if (*valid && tick <= *last)
		return FALSE;
	/* (the first: its own tick only) */
	if (!*valid || tick - *last < count)
		count = (short)(*valid ? tick - *last : 1);
	for (index = 0; index < count; index++)
		*pending |= control_flags[index];
	*valid = TRUE;
	*last = tick;
	return TRUE;
}

void update_server_handle_distributed_input(
	long player_index,
	long tick,
	struct player_action const *action,
	unsigned short const *control_flags,
	short count)
{
	struct update_server_queue_datum *queue;
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

	if (!update_server_globals.initialized || absolute_index < 0 || absolute_index >= MAXIMUM_NUMBER_OF_PLAYERS ||
		count <= 0 || !distributed_action_valid(action))
	{
		return;
	}
	queue = (struct update_server_queue_datum *)datum_try_and_get(update_server_globals.queues, player_index);
	if (!queue)
		return;
	if (!distributed_add_new_ticks(&update_server_distributed_inputs[absolute_index].valid,
		&update_server_distributed_inputs[absolute_index].tick, tick, control_flags, count,
		&update_server_pending_control_flags[absolute_index]))
	{
		return;
	}
	update_server_distributed_inputs[absolute_index].received_time =
		game_time_initialized() ? game_time_get() : 0;
	queue->current_action = *action;
}

long update_server_ticked_update_number(
	void)
{
	return update_server_globals.initialized && update_client_globals.initialized ?
		update_client_globals.next_update_number_to_dequeue - 1 : NONE;
}

struct player_action const *update_server_update_actions(
	long update_number,
	short *count)
{
	struct update *update = update_server_globals.initialized ? update_server_get_update(update_number) : NULL;

	if (!update || update->update_number != update_number)
		return NULL;
	*count = (short)update->update.action_count;
	return update->update.actions;
}

void update_client_handle_relayed_action(
	short player_index,
	long update_number,
	struct player_action const *action,
	unsigned short const *control_flags,
	short count)
{
	if (player_index < 0 || player_index >= MAXIMUM_NUMBER_OF_PLAYERS || count <= 0 || !distributed_action_valid(action))
		return;
	if (distributed_add_new_ticks(&update_client_relayed_actions[player_index].valid,
		&update_client_relayed_actions[player_index].update_number, update_number, control_flags, count,
		&update_client_relayed_actions[player_index].pending_control_flags))
	{
		update_client_relayed_actions[player_index].action = *action;
	}
}

boolean update_client_distributed_input(
	short local_player_index,
	long *tick,
	struct player_action *action,
	unsigned short *control_flags)
{
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		!update_client_local_inputs[local_player_index].valid)
	{
		return FALSE;
	}
	*tick = update_client_local_inputs[local_player_index].tick;
	*action = update_client_local_inputs[local_player_index].action;
	csmemcpy(control_flags, update_client_local_inputs[local_player_index].control_flags,
		sizeof(update_client_local_inputs[local_player_index].control_flags));
	return TRUE;
}

/* ---------- private code */

static struct update *update_server_get_update(
	long update_number)
{
	struct update *update;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_queues_new.c",
		0x29E,
		update_server_globals.initialized);
	if (update_number<update_server_globals.next_update_number_to_build &&
		update_number>=update_server_globals.next_update_number_to_build-MAXIMUM_SERVER_UPDATES)
	{
		update = &update_server_globals.updates[update_number&(MAXIMUM_SERVER_UPDATES-1)];
	}
	else
	{
		update = NULL;
	}

	return update;
}

static struct update *update_client_get_update(
	long update_number)
{
	if (update_number>=update_client_globals.next_update_number_to_dequeue &&
		update_number<update_client_globals.next_update_number_to_dequeue+MAXIMUM_CLIENT_UPDATES)
	{
		return &update_client_globals.updates[update_number&(MAXIMUM_CLIENT_UPDATES-1)];
	}

	return NULL;
}

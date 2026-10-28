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
#include "physics/collision_bsp.h"
#include "items/weapons.h"
#include "tag_files/tag_files.h"
#include "physics/collision_bsp_definitions.h"
#include "ai/path.h"
#include "ai/path_structure_bsp.h"
#include "structures/structure_bsp_definitions.h"
#include "units/bipeds.h"
#include "units/biped_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "game/bots.h"
#include "game/game_globals.h"
#include "items/item_definitions.h"
#include "items/equipment_definitions.h"
#include "items/projectile_definitions.h"
#include "hardcore_rules.h"
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
350, 280, 210, 150 ms, turned into 30 Hz ticks as (ms * 30) / 1000 plus a
roll of 0, 1, or 2. Impossible continues that same step so the lobby's
fifth skill still climbs. */
/* radians of aim offset, and the extra cone that still counts as aimed. */
static real const bot_aim_error[BOT_SKILL_COUNT] = { 0.16f, 0.09f, 0.055f, 0.03f, 0.017f };
static real const bot_aim_slack[BOT_SKILL_COUNT] = { 0.10f, 0.07f, 0.05f, 0.035f, 0.024f };
static long const bot_reaction_ms[BOT_SKILL_COUNT] = { 350, 280, 210, 150, 90 };
/* the player's look sensitivity for that difficulty: easy 3, normal 4,
heroic 5, legendary 6, impossible 10. The beta writes facing straight from
its aim; this is how fast the port can turn while it also walks. */
static long const bot_look_sensitivity[BOT_SKILL_COUNT] = { 3, 4, 5, 6, 10 };
/* rolls out of 1000 while a grenade is allowed. The beta's four are 2, 4, 6, 8. */
static long const bot_grenade_chance[BOT_SKILL_COUNT] = { 2, 4, 6, 8, 10 };
/* rolls out of 100 while a strafe is underway. The beta's four are 0, 2, 4, 6. */
static long const bot_jump_chance[BOT_SKILL_COUNT] = { 0, 2, 4, 6, 8 };

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
	/* ticks to keep a teammate's call after their sight ray misses once */
	short call_hold;
	long reaction_ticks;
	long unseen_ticks;
	real yaw_error;
	real pitch_error;
	real strafe;
	real strafe_strength;
	long strafe_ticks;
	/* one hop when a strafe period starts, the beta's strafe_turn roll */
	boolean strafe_jump;
	real wander_yaw;
	long look_ticks;
	long stuck_ticks;
	long melee_ticks;
	long reload_ticks;
	long grenade_ticks;
	/* ticks of backing up after a throw, before the grenade exists */
	long nade_hold;
	/* a close throw while this bot is about to die. Stay with it. */
	boolean nade_trade;
	/* the aim held until the grenade leaves the hand. A turn during
	the wind-up is the throw into the wall. */
	real grenade_yaw;
	real grenade_pitch;
	boolean grenade_aim;
	/* ticks to keep one end of a corridor after the other end did not move */
	short hall_lock;
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
	real_point3d hunt_goal;
	boolean hunt_valid;
	long hunt_ticks;
	/* the last place an enemy was known, after they have been forgotten.
	the scan starts only once the bot is standing on that spot. */
	real_point3d search_position;
	long search_ticks;
	boolean search_arrived;
	long look_timer;
	real look_yaw;
	boolean look_valid;
	/* the step already chosen. A new one off to the other side waits,
	or the bot turns left and right and never takes it. */
	real step_yaw;
	long step_hold;
	/* the way they committed to. A reverse before this runs out is a
	bounce down the hall, so a side opening is tried first. */
	real commit_yaw;
	long commit_ticks;
	long idle_ticks;
	long wait_ticks;
	long fetch_fail;
	long fight_idle;
	long avoid_ticks;
	boolean powerup_ready;
	real_point3d avoid_point;
	/* four weapon spots this bot already failed to reach */
	real_point3d given_up[4];
	long given_up_ticks[4];
	/* ticks until the next sight test. A new target starts unseen. */
	long sight_ticks;
	long unknown_ticks;
	boolean trigger_down;
	/* 0 on the ground, 1 running at a lip, 2 in the jump off that lip */
	short vault;
	boolean vault_air;
	long vault_ticks;
	short vault_run;
	real vault_yaw;
	/* the source pad of a teleporter that comes out closer to the goal */
	boolean pad_valid;
	real_point3d pad_point;
	short pad_cool;
	struct path_result route;
	long route_ticks;
	real_point3d route_goal;
	/* the route has to change floors. The straight line stays on this one. */
	boolean route_floor;
	boolean route_is_floor;
	long route_surface;
	/* a trip across the map. Follow the path, do not retarget every hall. */
	boolean route_tour;
	/* places this tour already walked. The next goal is away from these. */
	real_point3d tour_been[12];
	short tour_been_count;
	short tour_been_next;
	/* facing the step, but not yet lined up, so the feet stay put.
	the next tick may step past a point they have already crossed. */
	boolean move_thwarted;
	/* still inside this spot. Ten seconds here, and a teammate's fight
	is a reason to leave it. */
	real_point3d area_anchor;
	boolean area_valid;
	long area_ticks;
	boolean assist_valid;
	long assist_player;
	long assist_ticks;
	boolean assist_floor;
	long assist_surface;
	real_point3d assist_goal;
	/* one trip at a time. A fight or a grenade pauses it. The next
	rule does not get to pick a new heading while this is set. */
	short job;
	short job_pad;
	real_point3d job_goal;
	real_point3d job_resume;
	long job_surface;
	long job_resume_surface;
	boolean job_floor;
	boolean job_resume_floor;
	boolean job_climb;
	boolean job_blocked;
	long job_still;
	long job_age;
	long job_path_fail;
	/* same-floor trips in a row. After a couple, another floor is the tour. */
	short floor_trips;
	real_point3d job_anchor;
	real job_ladder_yaw;
	/* a powerup they were already next to. The trip stays; this is the step onto it. */
	short job_grab;
	real_point3d job_grab_point;
	/* the place this trip is going. job_goal is only the next link. */
	real_point3d job_far;
	long job_far_surface;
	boolean job_far_valid;
	boolean job_far_floor;
	short job_hop;
	short job_avoid;
	short job_avoid_ticks;
	/* the stored link currently copied into route. NONE until then. */
	short route_link;
	/* a gap the jump will clear. 0 means they are not lining one up. */
	short gap_ticks;
	real gap_yaw;
	/* the jump has been committed. Facing and throttle stay on gap_yaw until they land. */
	boolean gap_hold;
	boolean gap_left;
	long stall_ticks;
	/* a forced open step, so a zero-throttle tick does not put them back on the wall */
	short escape_ticks;
	real escape_yaw;
};

enum
{
	_bot_job_none = 0,
	_bot_job_travel = 1,
	_bot_job_fetch = 2,
	_bot_job_ladder = 3,
	_bot_job_assist = 4
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

static struct bot_seat bot_seats[BOT_CONTROLLER_LIMIT];

static struct bot_seat *bot_seat_find(short machine_index, short controller_index)
{
	long index;

	for (index = 0; index < BOT_CONTROLLER_LIMIT; index++)
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
	if (skill > BOT_SKILL_IMPOSSIBLE)
		skill = BOT_SKILL_IMPOSSIBLE;
	seat = bot_seat_find(machine_index, controller_index);
	if (!seat)
	{
		for (index = 0; index < BOT_CONTROLLER_LIMIT; index++)
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

/* another storey. A step or a lip is not one: those stay a fight or a vault. */
static boolean bot_different_floor(real_point3d const *feet, real_point3d const *point)
{
	real dz = feet->z - point->z;

	if (dz < 0.0f)
		dz = -dz;
	return dz > 1.6f;
}

/* the ground the enemy is actually walking, so the route can leave this floor */
static boolean bot_enemy_floor(long unit_index, real_point3d *goal, long *surface)
{
	struct biped_datum *biped;
	long index;

	*surface = NONE;
	biped = biped_try_and_get(unit_index);
	if (!biped)
		return FALSE;
	index = biped->biped.pathfinding_surface_index;
	if (index == NONE)
		index = biped->biped.last_pathfinding_surface_index;
	if (index == NONE)
		return FALSE;
	*surface = index;
	*goal = biped->biped.pathfinding_point;
	return TRUE;
}

/* Temporary. Off while we check whether knowing a body through a wall
is the freeze. Set TRUE to track by hearing and to hunt a hidden position. */
static boolean const bot_know_hidden_enemies = FALSE;

/* H1PB within_hearing: 8 on the ground, 2.5 up or down. */
static boolean bot_within_hearing(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x;
	real dy = a->y - b->y;
	real dz = a->z - b->z;

	if (dz < 0.0f)
		dz = -dz;
	return dx * dx + dy * dy < 64.0f && dz < 2.5f;
}

/* (ms * 30) / 1000, then 0, 1, or 2 ticks. A new target starts unseen, so
the first time they are in sight this is the wait before the trigger. */
static long bot_reaction_time(long skill)
{
	long ticks;

	if (skill < 0)
		skill = 0;
	if (skill > BOT_SKILL_IMPOSSIBLE)
		skill = BOT_SKILL_IMPOSSIBLE;
	ticks = bot_reaction_ms[skill] * 30 / 1000;
	return ticks + (long)((bot_random() >> 8) % 3);
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

/* a ballistic magazine that is empty, and the weapon is sitting ready.
An energy gun keeps its charge in the loaded or fractional field, so it is
not this. Firing and chambering are not, either: those empty a round for
a moment and a reload press then would start another reload */
static boolean bot_should_reload(long weapon_index)
{
	struct weapon_datum *weapon;
	short magazine;
	boolean spare;

	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_get(weapon_index);
	if (weapon->weapon.state != _weapon_state_idle &&
		weapon->weapon.state != _weapon_state_ready)
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

boolean bot_refuses_weapon(long unit_index, long weapon_index)
{
	(void)unit_index;
	(void)weapon_index;
	return FALSE;
}

/* a gun changes only for a strictly higher score. A flag or an oddball stays in hand. */
static boolean bot_slot_usable(long unit_index, short slot)
{
	long weapon_index;

	if (slot < 0 || slot >= MAXIMUM_WEAPONS_PER_UNIT)
		return FALSE;
	weapon_index = unit_inventory_get_weapon(unit_index, slot);
	if (weapon_index == NONE)
		return FALSE;
	return bot_weapon_ready(weapon_index) || bot_weapon_has_reserve(weapon_index);
}

static long bot_slot_score(long unit_index, short slot)
{
	struct bot_weapon_kind const *kind;
	long weapon_index;

	weapon_index = unit_inventory_get_weapon(unit_index, slot);
	if (weapon_index == NONE)
		return 0;
	kind = bot_weapon_kind(weapon_index);
	/* a gun the table does not name */
	return kind ? kind->score : 4;
}

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

		weapon_index = unit_inventory_get_weapon(unit_index, slot);
		if (weapon_index == NONE || !bot_slot_usable(unit_index, slot))
			continue;
		kind = bot_weapon_kind(weapon_index);
		score = kind ? kind->score : 4;
		if (score > best_score)
		{
			best_score = score;
			best_slot = slot;
		}
		{
			real near_range = kind ? kind->near_range : 3.0f;
			real far_range = kind ? kind->far_range : 12.0f;

			if (distance >= near_range && distance <= far_range && score > band_score)
			{
				band_score = score;
				band_slot = slot;
			}
		}
	}
	if (best_score < 0)
		return NONE;
	if (band_slot != NONE && band_score >= best_score)
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

/* cone is atan(0.30 / range) plus the skill slack, widened by 1.5 while the trigger is down */
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

/* radians the bot may turn in one tick. Sensitivity 1 is 80 degrees of yaw
per second, and each step adds 20. Pitch is half of that. */
static real bot_look_step(long skill, boolean pitch)
{
	real degrees;

	if (skill < 0)
		skill = 0;
	if (skill > BOT_SKILL_IMPOSSIBLE)
		skill = BOT_SKILL_IMPOSSIBLE;
	degrees = 80.0f + (real)(bot_look_sensitivity[skill] - 1) * 20.0f;
	if (pitch)
		degrees *= 0.5f;
	return DEGREES_TO_RADIANS(degrees) / (real)TICKS_PER_SECOND;
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

/* structure only, so a player standing there is not a wall or a floor */
static boolean bot_structure_clear(real_point3d const *from, real_point3d const *to, long ignore_object_index)
{
	struct collision_result collision;
	unsigned long flags;

	flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit);
	return !collision_test_line(flags, from, to, ignore_object_index, &collision);
}

static boolean bot_ray_clear_z(real_point3d const *feet, real yaw, real distance, real height,
	long ignore_object_index)
{
	real_point3d from;
	real_point3d to;

	from = *feet;
	from.z += height;
	to.x = from.x + cosine(yaw) * distance;
	to.y = from.y + sine(yaw) * distance;
	to.z = from.z;
	return bot_structure_clear(&from, &to, ignore_object_index);
}

/* the first distance that hits structure at this height. Negative if the reach stays open. */
static real bot_first_block(real_point3d const *feet, real yaw, real height, real reach,
	long ignore_object_index)
{
	static real const marks[] = { 0.40f, 0.70f, 1.00f, 1.35f, 1.70f, 2.10f, 2.60f };
	long index;

	for (index = 0; index < (long)NUMBEROF(marks); index++)
	{
		if (marks[index] > reach)
			break;
		if (!bot_ray_clear_z(feet, yaw, marks[index], height, ignore_object_index))
			return marks[index];
	}
	return -1.0f;
}

/* the marker sits inside a solid, not against the far side of an open pickup.
the face is well short of the point, so running at the point is into the object. */
static boolean bot_solid_before_point(real_point3d const *feet, real_point3d const *point,
	real dist, long unit_index)
{
	real hit;

	if (dist < 0.45f)
		return FALSE;
	hit = bot_first_block(feet, bot_yaw_between(feet, point), 0.45f, dist, unit_index);
	if (hit < 0.0f)
		return FALSE;
	return hit + 0.35f < dist;
}

/* already on the face of that solid, not still walking up to it */
static boolean bot_against_solid(real_point3d const *feet, real_point3d const *point,
	real dist, long unit_index)
{
	real hit;

	if (!bot_solid_before_point(feet, point, dist, unit_index))
		return FALSE;
	hit = bot_first_block(feet, bot_yaw_between(feet, point), 0.45f, dist, unit_index);
	return hit >= 0.0f && hit <= 0.85f;
}

/* how high the feet can get: the jump apex, plus the lift from crouching
at the top. The last fraction is the pill, which does not end up standing
on the lip. */
static real bot_mantle_reach(long unit_index)
{
	struct biped_datum *biped;
	struct biped_definition *definition;
	real jump;
	real lift;
	real reach;

	biped = biped_try_and_get(unit_index);
	if (!biped)
		return 0.95f;
	definition = biped_definition_get(biped->definition_index);
	if (!definition)
		return 0.95f;
	jump = definition->biped.jump_velocity;
	if (jump > 1.0f)
		jump = jump / (real)TICKS_PER_SECOND;
	if (jump < 0.02f || jump > 0.40f)
		return 0.95f;
	reach = (jump * jump) / (2.0f * 0.0035651792f);
	lift = definition->biped.collision_height_standing -
		definition->biped.collision_height_crouching;
	if (lift > 0.0f && lift < 0.6f)
		reach += lift;
	reach -= 0.12f;
	if (reach < 0.75f)
		reach = 0.75f;
	if (reach > 1.15f)
		reach = 1.15f;
	return reach;
}

/* a short drop from just above the lip. The ground on the far side of a
pillar is not a landing. */
static boolean bot_lip_has_floor(real_point3d const *feet, real yaw, real distance, real lip,
	long ignore_object_index)
{
	real_point3d above;
	real_point3d below;

	if (!bot_ray_clear_z(feet, yaw, distance, lip + 0.08f, ignore_object_index))
		return FALSE;
	above.x = feet->x + cosine(yaw) * distance;
	above.y = feet->y + sine(yaw) * distance;
	above.z = feet->z + lip + 0.05f;
	below = above;
	below.z = feet->z + lip - 0.30f;
	if (below.z < feet->z + 0.25f)
		below.z = feet->z + 0.25f;
	return !bot_structure_clear(&above, &below, ignore_object_index);
}

/* a lip a crouch jump can stand on. The face has to end below the jump,
and the floor has to be that lip. A pillar that stays solid through the
jump, or only opens onto the ground behind it, is not one. */
static boolean bot_ledge(real_point3d const *feet, real yaw, real reach, long unit_index, real *face)
{
	static real const lips[] = { 0.64f, 0.76f, 0.88f, 1.00f, 1.12f };
	real hit;
	real mantle;
	real lip;
	long index;

	hit = bot_first_block(feet, yaw, 0.40f, reach, unit_index);
	if (hit < 0.0f)
		return FALSE;
	if (bot_ray_clear_z(feet, yaw, hit, 0.62f, unit_index))
		return FALSE;
	mantle = bot_mantle_reach(unit_index);
	lip = -1.0f;
	for (index = 0; index < (long)NUMBEROF(lips); index++)
	{
		if (lips[index] > mantle)
			break;
		if (bot_ray_clear_z(feet, yaw, hit, lips[index], unit_index))
		{
			lip = lips[index];
			break;
		}
	}
	if (lip < 0.0f && bot_ray_clear_z(feet, yaw, hit, mantle, unit_index))
		lip = mantle;
	if (lip < 0.0f)
		return FALSE;
	if (!bot_lip_has_floor(feet, yaw, hit + 0.30f, lip, unit_index) &&
		!bot_lip_has_floor(feet, yaw, hit + 0.60f, lip, unit_index))
		return FALSE;
	*face = hit;
	return TRUE;
}

static real bot_open_distance(real_point3d const *feet, real yaw, long ignore_object_index);

/* the lip in front, a side lip when the way ahead is closing, or either
wall of a trench. The trench check is what gets them out of Hang 'em High:
the floor ahead stays open, so a search that only looks forward never
turns square to the edge. */
static boolean bot_find_ledge(real_point3d const *feet, real facing, long ignore_object_index,
	boolean trench, real *yaw)
{
	real face;
	real left_face;
	real right_face;
	real left_yaw;
	real right_yaw;
	boolean left;
	boolean right;

	if (bot_ledge(feet, facing, 2.60f, ignore_object_index, &face))
	{
		*yaw = facing;
		return TRUE;
	}
	left_yaw = facing + _pi * 0.5f;
	right_yaw = facing - _pi * 0.5f;
	left = bot_ledge(feet, left_yaw, 2.60f, ignore_object_index, &left_face);
	right = bot_ledge(feet, right_yaw, 2.60f, ignore_object_index, &right_face);
	if (bot_open_distance(feet, facing, ignore_object_index) < 2.5f)
	{
		if (left && (!right || left_face <= right_face))
		{
			*yaw = left_yaw;
			return TRUE;
		}
		if (right)
		{
			*yaw = right_yaw;
			return TRUE;
		}
	}
	if (!trench || !left || !right)
		return FALSE;
	*yaw = left_face <= right_face ? left_yaw : right_yaw;
	return TRUE;
}

/* downward speed, per tick, where a landing starts to hurt. The globals
tag bakes that speed. A missing one is a fall of about a story and a half. */
static real bot_harmful_fall_speed(void)
{
	struct game_globals *globals;
	void *element;
	real speed;
	real distance;

	globals = scenario_get_game_globals();
	if (!globals)
		return 0.11f;
	element = tag_block_get_element_fast(&globals->falling_damage, 0, 0x98);
	if (!element)
		return 0.11f;
	speed = *(real *)((char *)element + 0x90);
	if (speed > 0.02f && speed < 1.0f)
		return speed;
	distance = *(real *)((char *)element + 8);
	if (distance < 0.4f)
		distance = 1.6f;
	return square_root(2.0f * 0.0035651792f * distance);
}

/* a teleporter whose exit is closer to the goal than the bot is. The
ground path does not cross these, so the pad has to be the walk. The
source that just fired is skipped until they have stepped off it. */
static boolean bot_teleporter_pad(real_point3d const *feet, real_point3d const *goal,
	long unit_index, long skip_index, real_point3d *pad, boolean floor_pad)
{
	struct scenario *scenario;
	long index;
	real best;
	boolean found;

	(void)unit_index;
	scenario = global_scenario_get();
	if (!scenario || !goal)
		return FALSE;
	best = 1.0e9f;
	found = FALSE;
	for (index = 0; index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *source;
		struct scenario_netgame_flag *target;
		long target_index;
		real here;
		real there;
		real trip;
		real through;

		source = TAG_BLOCK_GET_ELEMENT(
			&scenario->netgame_flags, index, struct scenario_netgame_flag);
		if (!source || source->type != _netgame_flag_teleporter_source)
			continue;
		if (index == skip_index)
			continue;
		if (skip_index != NONE && distance3d(feet, &source->position) < 1.2f)
			continue;
		target = NULL;
		for (target_index = 0; target_index < scenario->netgame_flags.count; target_index++)
		{
			struct scenario_netgame_flag *candidate;

			candidate = TAG_BLOCK_GET_ELEMENT(
				&scenario->netgame_flags, target_index, struct scenario_netgame_flag);
			if (!candidate || candidate->type != _netgame_flag_teleporter_target)
				continue;
			if (candidate->team_index != source->team_index)
				continue;
			target = candidate;
			break;
		}
		if (!target)
			continue;
		here = distance3d(feet, goal);
		there = distance3d(&target->position, goal);
		trip = distance3d(feet, &source->position);
		through = trip + there;
		if (bot_different_floor(feet, goal))
		{
			real pad_z = source->position.z - feet->z;
			real exit_z = target->position.z - goal->z;
			real goal_z = feet->z - goal->z;

			if (!floor_pad)
				continue;
			if (pad_z < 0.0f)
				pad_z = -pad_z;
			if (exit_z < 0.0f)
				exit_z = -exit_z;
			if (goal_z < 0.0f)
				goal_z = -goal_z;
			/* this floor's pad, and the exit is on the floor they want.
			the straight line between those floors is through the ground. */
			if (pad_z > 2.0f || exit_z + 2.0f >= goal_z)
				continue;
		}
		else
		{
			/* the pad has to shorten the trip by a real walk. A blocked
			straight line is not enough: that is how every catwalk pad wins. */
			if (there + 10.0f >= here)
				continue;
			if (through + 18.0f >= here)
				continue;
		}
		if (through < best)
		{
			best = through;
			*pad = source->position;
			found = TRUE;
		}
	}
	return found;
}

/* how far a chest-high ray gets before a wall, in steps */
static real bot_open_distance(real_point3d const *feet, real yaw, long ignore_object_index)
{
	static real const marks[] = { 0.8f, 1.6f, 2.5f, 3.8f, 5.5f, 8.0f };
	real clear = 0.0f;
	long index;

	for (index = 0; index < (long)NUMBEROF(marks); index++)
	{
		if (!bot_ray_clear(feet, yaw, marks[index], ignore_object_index))
			break;
		clear = marks[index];
	}
	return clear;
}

/* a step around the solid, on the side that is already more open */
static boolean bot_slide_around(real_point3d const *feet, real toward, real facing,
	long unit_index, real *yaw)
{
	real left_yaw;
	real right_yaw;
	real left;
	real right;
	real left_turn;
	real right_turn;

	left_yaw = toward + _pi * 0.5f;
	right_yaw = toward - _pi * 0.5f;
	left = bot_open_distance(feet, left_yaw, unit_index);
	right = bot_open_distance(feet, right_yaw, unit_index);
	if (left < 1.0f && right < 1.0f)
		return FALSE;
	if (right < 1.0f || left > right + 0.6f)
		*yaw = left_yaw;
	else if (left < 1.0f || right > left + 0.6f)
		*yaw = right_yaw;
	else
	{
		left_turn = bot_angle_delta(facing, left_yaw);
		right_turn = bot_angle_delta(facing, right_yaw);
		if (left_turn < 0.0f)
			left_turn = -left_turn;
		if (right_turn < 0.0f)
			right_turn = -right_turn;
		*yaw = left_turn <= right_turn ? left_yaw : right_yaw;
	}
	return TRUE;
}

/* ahead closes up and both shoulders are already in a wall: a corner.
A corridor has those same walls and still has room to walk, so a step
of clearance is not a corner. */
static boolean bot_heading_is_corner(real_point3d const *feet, real yaw, long ignore_object_index)
{
	if (bot_open_distance(feet, yaw, ignore_object_index) >= 1.15f)
		return FALSE;
	if (bot_ray_clear(feet, yaw + _pi * 0.5f, 0.4f, ignore_object_index))
		return FALSE;
	if (bot_ray_clear(feet, yaw - _pi * 0.5f, 0.4f, ignore_object_index))
		return FALSE;
	return TRUE;
}

/* 1 is a sniper, 2 is a rocket. The tag is often just "sniper" or
"rocket_launcher", which the long combat names do not contain. */
static long bot_power_gun(char const *name)
{
	if (!name)
		return 0;
	if (strstr(name, "rocket"))
		return 2;
	if (strstr(name, "sniper"))
		return 1;
	return 0;
}

/* an opening that is not the way they just walked, and not straight back.
the first one clear for a couple of metres is the other path. */
static boolean bot_side_path(real_point3d const *feet, real forward, long unit_index, real *yaw)
{
	static real const offsets[] = { 1.05f, -1.05f, 1.6f, -1.6f, 2.2f, -2.2f };
	long index;
	real best;
	real best_clear;
	boolean found;

	best = forward;
	best_clear = 0.0f;
	found = FALSE;
	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real candidate = forward + offsets[index];
		real clear = bot_open_distance(feet, candidate, unit_index);

		if (clear < 1.5f)
			continue;
		if (bot_heading_is_corner(feet, candidate, unit_index))
			continue;
		if (clear > best_clear)
		{
			best_clear = clear;
			best = candidate;
			found = TRUE;
		}
	}
	if (!found)
		return FALSE;
	*yaw = best;
	return TRUE;
}

/* the open end of a corridor, chosen toward prefer. The reverse of the
way they are already walking loses to a side opening, so a dead end
becomes another path instead of a trip back down the hall. */
static boolean bot_hall_heading(real_point3d const *feet, real facing, real prefer,
	real previous, long unit_index, real *yaw)
{
	real forward;
	real back;
	real left;
	real right;
	real along;
	real across;
	real dirs[6];
	real open[6];
	long count;
	long index;
	long best;
	real best_score;
	real prefer_clear;

	forward = bot_open_distance(feet, facing, unit_index);
	back = bot_open_distance(feet, facing + _pi, unit_index);
	left = bot_open_distance(feet, facing + _pi * 0.5f, unit_index);
	right = bot_open_distance(feet, facing - _pi * 0.5f, unit_index);
	along = forward > back ? forward : back;
	across = left > right ? left : right;
	count = 0;
	if (across < 1.35f && along >= 1.6f)
	{
		dirs[count] = facing;
		open[count++] = forward;
		dirs[count] = facing + _pi;
		open[count++] = back;
	}
	else if (along >= 2.5f && across < 1.6f && across + 1.0f < along)
	{
		dirs[count] = facing;
		open[count++] = forward;
		dirs[count] = facing + _pi;
		open[count++] = back;
	}
	else if (along < 1.35f && across >= 1.6f)
	{
		dirs[count] = facing + _pi * 0.5f;
		open[count++] = left;
		dirs[count] = facing - _pi * 0.5f;
		open[count++] = right;
	}
	else
		return FALSE;
	if (left >= 1.5f)
	{
		dirs[count] = facing + _pi * 0.5f;
		open[count++] = left;
	}
	if (right >= 1.5f)
	{
		dirs[count] = facing - _pi * 0.5f;
		open[count++] = right;
	}
	prefer_clear = 0.0f;
	for (index = 0; index < count; index++)
	{
		real delta = bot_angle_delta(prefer, dirs[index]);

		if (delta < 0.0f)
			delta = -delta;
		if (delta < 0.8f && open[index] > prefer_clear)
			prefer_clear = open[index];
	}
	best = NONE;
	best_score = -1.0e9f;
	for (index = 0; index < count; index++)
	{
		real delta;
		real reverse;
		real score;
		real side;

		if (open[index] < 1.3f)
			continue;
		delta = bot_angle_delta(prefer, dirs[index]);
		if (delta < 0.0f)
			delta = -delta;
		reverse = bot_angle_delta(previous + _pi, dirs[index]);
		if (reverse < 0.0f)
			reverse = -reverse;
		side = bot_angle_delta(facing + _pi * 0.5f, dirs[index]);
		if (side < 0.0f)
			side = -side;
		if (side > _pi * 0.5f)
			side = _pi - side;
		score = open[index] - delta * 6.0f;
		/* straight back down the corridor is the bounce */
		if (reverse < 0.7f)
			score -= 12.0f;
		/* the hall ahead is already short, and a doorway is open.
		take it before another lap of the corridor. */
		if (prefer_clear < 3.0f && side < 0.45f && open[index] >= 1.5f)
			score += 14.0f;
		if (score > best_score)
		{
			best_score = score;
			best = index;
		}
	}
	if (best == NONE)
		return FALSE;
	*yaw = dirs[best];
	return TRUE;
}

/* the path step, unless it meets a nearby wall at a hard angle. Running
along the wall is the step. About 45 degrees into it is the scrape:
they are still moving, and the wall takes the speed. A head-on step is
the opening, and a turn sharper than this is the corner, so those stay. */
static real bot_route_heading(real_point3d const *feet, real desired, real prefer, long unit_index)
{
	static real const offsets[] = { 0.0f, 0.4f, -0.4f, 0.8f, -0.8f, 1.2f, -1.2f };
	long index;
	real best_hit;
	real into;
	real delta;
	real tangent_a;
	real tangent_b;
	real turn_a;
	real turn_b;
	real tangent;

	best_hit = 1.05f;
	into = desired;
	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real yaw = desired + offsets[index];
		real hit = bot_first_block(feet, yaw, 0.45f, 1.05f, unit_index);

		if (hit >= 0.0f && hit < best_hit)
		{
			best_hit = hit;
			into = yaw;
		}
	}
	if (best_hit >= 1.05f)
		return desired;
	delta = bot_angle_delta(into, desired);
	if (delta < 0.0f)
		delta = -delta;
	/* already along the wall, or aimed at the frame itself */
	if (delta < 0.30f || delta > 1.15f)
		return desired;
	/* a wall two metres off is the hall, not a scrape. A door frame is
	closer than that, and waiting until it is under the nose is the hit. */
	if (bot_first_block(feet, desired, 0.45f, 1.35f, unit_index) < 0.0f)
		return desired;
	tangent_a = into + _pi * 0.5f;
	tangent_b = into - _pi * 0.5f;
	turn_a = bot_angle_delta(desired, tangent_a);
	turn_b = bot_angle_delta(desired, tangent_b);
	if (turn_a < 0.0f)
		turn_a = -turn_a;
	if (turn_b < 0.0f)
		turn_b = -turn_b;
	{
		real face_a = bot_angle_delta(prefer, tangent_a);
		real face_b = bot_angle_delta(prefer, tangent_b);
		real gap = turn_a - turn_b;

		if (face_a < 0.0f)
			face_a = -face_a;
		if (face_b < 0.0f)
			face_b = -face_b;
		if (gap < 0.0f)
			gap = -gap;
		/* both slides fit the step. Keep the one already faced, or the
		look flips from wall to wall every tick. */
		if (gap < 0.35f)
		{
			if (face_a <= face_b)
				tangent = tangent_a;
			else
			{
				tangent = tangent_b;
				turn_a = turn_b;
			}
		}
		else if (turn_a <= turn_b)
			tangent = tangent_a;
		else
		{
			tangent = tangent_b;
			turn_a = turn_b;
		}
	}
	/* a real turn into an opening is not a scrape along this wall.
	a tangent that dies in the next meter is the alcove wall, not a slide. */
	if (turn_a > 0.95f)
		return desired;
	if (bot_first_block(feet, tangent, 0.45f, 1.2f, unit_index) >= 0.0f)
		return desired;
	return tangent;
}

/* the chest ray is in the frame and a step to one side is the opening.
a small turn now is the miss. Waiting puts the shoulder on the post. */
static real bot_door_yaw(real_point3d const *feet, real yaw, long unit_index)
{
	real center;
	real left;
	real right;
	real left_clear;
	real right_clear;

	center = bot_first_block(feet, yaw, 0.45f, 2.40f, unit_index);
	if (center < 0.0f)
		return yaw;
	left = bot_first_block(feet, yaw + 0.40f, 0.45f, 2.40f, unit_index);
	right = bot_first_block(feet, yaw - 0.40f, 0.45f, 2.40f, unit_index);
	left_clear = left < 0.0f ? 2.40f : left;
	right_clear = right < 0.0f ? 2.40f : right;
	if (left_clear <= center + 0.35f && right_clear <= center + 0.35f)
		return yaw;
	if (left_clear >= right_clear)
		return yaw + 0.40f;
	return yaw - 0.40f;
}

/* the first open heading, starting at the one toward the goal. A heading
into a corner is not open */
static boolean bot_clear_yaw(real_point3d const *feet, real desired, long ignore_object_index, real *yaw)
{
	static real const offsets[] = {
		0.0f, 0.6f, -0.6f, 1.2f, -1.2f, 1.9f, -1.9f, 2.6f, -2.6f, _pi, 0.3f, -0.3f
	};
	long index;

	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real candidate = desired + offsets[index];

		if (bot_open_distance(feet, candidate, ignore_object_index) < 1.6f)
			continue;
		if (bot_heading_is_corner(feet, candidate, ignore_object_index))
			continue;
		*yaw = candidate;
		return TRUE;
	}
	return FALSE;
}

static boolean bot_drop_ahead(real_point3d const *feet, real yaw);

/* an open step that is still on this floor. The first heading out of a
wall is often the drop beside it, and rejecting that one used to cancel
the rest, which is the stand. */
static boolean bot_escape_yaw(real_point3d const *feet, real desired, long ignore_object_index, real *yaw)
{
	static real const offsets[] = {
		0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 1.6f, -1.6f, 2.2f, -2.2f, _pi, 0.25f, -0.25f
	};
	long index;
	real best;
	real best_yaw;

	best = -1.0f;
	best_yaw = desired;
	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real candidate = desired + offsets[index];
		real open = bot_open_distance(feet, candidate, ignore_object_index);

		if (open < 0.85f)
			continue;
		if (open < 2.0f && bot_heading_is_corner(feet, candidate, ignore_object_index))
			continue;
		if (bot_drop_ahead(feet, candidate))
			continue;
		if (open >= 1.4f)
		{
			*yaw = candidate;
			return TRUE;
		}
		if (open > best)
		{
			best = open;
			best_yaw = candidate;
		}
	}
	if (best < 0.85f)
		return FALSE;
	*yaw = best_yaw;
	return TRUE;
}

/* H1PB's engine navigator: the game's own ground path, rebuilt when the
goal moves. The memory is one path at a time, shared by every bot */
static boolean bot_ground_at(real_point3d const *point, real_point3d *ground, long *surface);

/* the pathfinding byte for a collision surface. Bit 6 is walkable. */
static boolean bot_surface_walkable(long surface_index)
{
	struct structure_bsp *bsp;
	byte const *flags;

	bsp = global_structure_bsp_get();
	if (!bsp || surface_index < 0 || surface_index >= bsp->pathfinding_surfaces.count)
		return FALSE;
	if (!bsp->pathfinding_surfaces.address)
		return FALSE;
	flags = (byte const *)bsp->pathfinding_surfaces.address;
	return (flags[surface_index] & (byte)(1 << 6)) != 0;
}

/* one path memory for every bot, and for the route graph while it is built.
the searches never overlap: the graph runs before the bots think. */
static struct path_state bot_path_memory;

/* a stored walk link replaces the search when the goal is that link's end. */
static boolean bot_route_install(struct bot_state *state, real_point3d const *goal);
/* a jump link faces the landing. The walk search stops at the gap. */
static boolean bot_route_jump_yaw(struct bot_state *state, real_point3d const *feet,
	real_point3d const *goal, real *yaw);

/* the portals between here and the goal, not the straight shortcut.
a shortcut is what meets the door frame. Each step is the middle of
the next opening, and the last of the four is where the walk continues. */
static void bot_route_from_search(struct bot_state *state, real_point3d const *feet)
{
	struct path_step window[8];
	struct path_node *node;
	struct path_node *child;
	short node_index;
	short cursor;
	short filled;
	short hops;
	short kept;
	short look;
	boolean overflow;

	state->route.valid = FALSE;
	state->route.step_count = 0;
	state->route.step_index = 0;
	state->route.steps_finish_path = FALSE;
	node_index = bot_path_memory.closest_node_index;
	if (node_index == NONE)
		return;
	/* the walk starts at the goal and follows parents home. The window
	keeps the portals next to the bot when the chain is long. */
	cursor = 0;
	filled = 0;
	overflow = FALSE;
	child = NULL;
	hops = 0;
	while (node_index != NONE && hops < 1024)
	{
		struct path_step step;

		node = path_get_node(&bot_path_memory, node_index);
		step.surface_index = node->surface_index;
		if (child == NULL)
			step.point = bot_path_memory.destination.point;
		else
			step.point = child->entry_point;
		window[cursor] = step;
		cursor++;
		if (cursor >= 8)
			cursor = 0;
		if (filled < 8)
			filled++;
		else
			overflow = TRUE;
		child = node;
		node_index = node->parent_node_index;
		hops++;
	}
	kept = 0;
	for (look = 0; look < filled && kept < 4; look++)
	{
		short slot;
		real dx;
		real dy;

		slot = (short)(cursor - 1 - look);
		while (slot < 0)
			slot = (short)(slot + 8);
		dx = window[slot].point.x - feet->x;
		dy = window[slot].point.y - feet->y;
		if (dx * dx + dy * dy < 0.35f * 0.35f)
			continue;
		state->route.steps[kept] = window[slot];
		kept++;
	}
	if (kept <= 0)
		return;
	state->route.step_count = (char)kept;
	state->route.step_index = 0;
	state->route.steps_finish_path = !overflow && look >= filled;
	state->route.valid = TRUE;
	state->route.endpoint.point = bot_path_memory.destination.point;
	state->route.endpoint.surface_index = bot_path_memory.destination.surface_index;
	state->route.endpoint.target_radius = 0.0f;
}

static void bot_refresh_route(struct bot_state *state, long unit_index, real_point3d const *feet,
	real_point3d const *goal)
{
	struct path_input input;
	real_point3d start_point;
	real_point3d destination;
	real_point2d known;
	real_point2d query;
	struct structure_bsp *bsp;
	long start_surface;
	long goal_surface;
	real bound;
	boolean around;

	/* the next link is already a finished walk. Do not search again. */
	if (bot_route_install(state, goal))
		return;
	state->route_link = NONE;
	state->route_ticks++;
	/* four portals are one piece of a longer walk. The next piece starts
	when they reach the last one. Rebuilding earlier aims them at a new
	corner while they are still in the doorway. */
	if (state->route.valid && state->route.step_count > 0 &&
		state->route.step_index >= state->route.step_count - 1)
	{
		real_point3d const *last;
		real dx;
		real dy;
		real gx;
		real gy;

		last = &state->route.steps[state->route.step_index].point;
		dx = last->x - feet->x;
		dy = last->y - feet->y;
		gx = goal->x - last->x;
		gy = goal->y - last->y;
		if (dx * dx + dy * dy < 0.9f * 0.9f &&
			(!state->route.steps_finish_path || gx * gx + gy * gy > 4.0f * 4.0f))
			state->route.valid = FALSE;
	}
	if (state->route.valid && state->route_ticks < 20 &&
		state->route_is_floor == state->route_floor &&
		distance2d((real_point2d const *)&state->route_goal, (real_point2d const *)goal) < 3.0f)
		return;
	state->route.valid = FALSE;
	state->route_ticks = 0;
	state->route_goal = *goal;
	state->route_is_floor = state->route_floor;
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
	destination = *goal;
	around = FALSE;
	if (state->route_floor && state->route_surface != NONE)
		goal_surface = state->route_surface;
	else
	{
		long line_surface;
		real_point3d ground;
		long ground_surface;

		/* the straight line stops on the near wall. The floor under the
		goal is the room the path has to reach, through the doorway. */
		line_surface = structure_surface_index_from_point(bsp, FALSE, &known, start_surface, &query);
		ground_surface = NONE;
		if (bot_ground_at(goal, &ground, &ground_surface) && bot_surface_walkable(ground_surface))
		{
			goal_surface = ground_surface;
			destination = ground;
			around = ground_surface != line_surface;
		}
		else
			goal_surface = line_surface;
		if (goal_surface == NONE)
			return;
	}
	path_input_new(&input, 0.45f, FALSE, unit_index);
	path_input_set_start(&input, &start_point, start_surface);
	/* a floor change, or a room past a wall, walks the long way around.
	the straight-line bound stops the search at the near wall. */
	bound = distance3d(&start_point, goal) + 8.0f;
	if ((state->route_floor || around || distance3d(&start_point, goal) > 12.0f) &&
		bound < 96.0f)
		bound = 96.0f;
	else if (bound < 16.0f)
		bound = 16.0f;
	path_input_set_search_bounds(&input, bound);
	path_state_new(&input, &bot_path_memory, NULL);
	path_state_destination(&bot_path_memory, &destination, goal_surface, 0.8f);
	if (!path_state_find(&bot_path_memory))
		state->route.valid = FALSE;
	else
		bot_route_from_search(state, feet);
}

/* the floor a step ahead is gone. A platform edge has no wall, so a
clear chest ray is not a path. */
static boolean bot_drop_ahead(real_point3d const *feet, real yaw);
static boolean bot_ramp_ahead(real_point3d const *feet, real yaw);

/* a gap with a platform on the far side, inside a running jump. */
static boolean bot_jump_across(real_point3d const *feet, real yaw, long unit_index,
	real *edge, real_point3d *landing);

static real bot_frame_yaw(real_point3d const *feet, real yaw, long unit_index);

/* the step is around a corner that is still ahead. Looking at it from
here is the glance. Keep the hall until the corner is close. */
static real bot_wall_yaw(real_point3d const *feet, real_point3d const *step,
	real facing, long unit_index)
{
	real desired;
	real dx;
	real dy;
	real dist;
	real probe;
	real hit;
	real along;

	desired = bot_yaw_between(feet, step);
	dx = step->x - feet->x;
	dy = step->y - feet->y;
	dist = square_root(dx * dx + dy * dy);
	probe = dist;
	if (probe > 2.2f)
		probe = 2.2f;
	if (probe < 0.4f)
		probe = 0.4f;
	hit = bot_first_block(feet, desired, 0.45f, probe, unit_index);
	if (hit >= 0.0f && hit + 0.45f < dist && dist > 1.8f)
	{
		along = bot_angle_delta(facing, desired);
		if (along < 0.0f)
			along = -along;
		if (along < 1.2f &&
			bot_first_block(feet, facing, 0.45f, 1.1f, unit_index) < 0.0f &&
			!bot_drop_ahead(feet, facing))
			return facing;
	}
	return bot_frame_yaw(feet, desired, unit_index);
}

/* one shoulder is in the frame and the other is not. A small aim away
from that shoulder is the doorway. Both shoulders, or a nudge that
leaves the floor, stays on the step. */
static real bot_frame_yaw(real_point3d const *feet, real yaw, long unit_index)
{
	real_point3d left_from;
	real_point3d right_from;
	real left;
	real right;
	real nudge;
	real aimed;

	left_from = *feet;
	right_from = *feet;
	left_from.x += cosine(yaw + _pi * 0.5f) * 0.24f;
	left_from.y += sine(yaw + _pi * 0.5f) * 0.24f;
	right_from.x += cosine(yaw - _pi * 0.5f) * 0.24f;
	right_from.y += sine(yaw - _pi * 0.5f) * 0.24f;
	left = bot_first_block(&left_from, yaw, 0.45f, 0.80f, unit_index);
	right = bot_first_block(&right_from, yaw, 0.45f, 0.80f, unit_index);
	nudge = 0.0f;
	if (left >= 0.0f && left < 0.75f && (right < 0.0f || left + 0.30f < right))
		nudge = -0.10f;
	else if (right >= 0.0f && right < 0.75f && (left < 0.0f || right + 0.30f < left))
		nudge = 0.10f;
	if (nudge == 0.0f)
		return yaw;
	aimed = yaw + nudge;
	if (aimed > _pi)
		aimed -= _pi * 2.0f;
	else if (aimed < -_pi)
		aimed += _pi * 2.0f;
	if (bot_drop_ahead(feet, aimed))
		return yaw;
	return aimed;
}

/* the heading down the hall. A corner starts turning while there is
still room, instead of at the frame. A slide that leaves the floor is
the drop off the side, so that one keeps the step. */
static real bot_step_heading(struct bot_state *state, real_point3d const *feet,
	real_point3d const *step, long unit_index)
{
	real desired;
	real slid;
	boolean turned;

	desired = bot_yaw_between(feet, step);
	turned = FALSE;
	if (state->route.valid && state->route.step_index + 1 < state->route.step_count)
	{
		real_point3d const *next;
		real out;
		real turn;
		real mag;
		real dx;
		real dy;
		real dist;
		real need;

		next = &state->route.steps[state->route.step_index + 1].point;
		out = bot_yaw_between(step, next);
		turn = bot_angle_delta(desired, out);
		mag = turn < 0.0f ? -turn : turn;
		dx = step->x - feet->x;
		dy = step->y - feet->y;
		dist = square_root(dx * dx + dy * dy);
		/* a right angle needs about two meters. Starting at the frame is the hit. */
		need = 0.85f + mag * 1.05f;
		if (need > 2.6f)
			need = 2.6f;
		if (mag > 0.40f && dist < need)
		{
			real mix;
			real aim;

			mix = (need - dist) / need;
			aim = desired + turn * mix;
			if (aim > _pi)
				aim -= _pi * 2.0f;
			else if (aim < -_pi)
				aim += _pi * 2.0f;
			if (bot_first_block(feet, aim, 0.45f, 0.90f, unit_index) < 0.0f &&
				!bot_drop_ahead(feet, aim))
			{
				desired = aim;
				turned = TRUE;
			}
		}
	}
	if (turned)
		return desired;
	slid = bot_route_heading(feet, desired, state->facing_yaw, unit_index);
	if (slid != desired && bot_drop_ahead(feet, slid) && !bot_drop_ahead(feet, desired))
		return desired;
	return slid;
}

/* the heading a step toward goal can actually take. The distant point and
the first free step are often not the same, and facing the point while the
step is off to the side leaves the bot turning in place. */
static boolean bot_step_yaw(struct bot_state *state, long unit_index, real_point3d const *feet,
	real_point3d const *goal, real *yaw)
{
	real_point3d step;
	real desired;
	long guard;

	if (bot_route_jump_yaw(state, feet, goal, yaw))
		return TRUE;
	bot_refresh_route(state, unit_index, feet, goal);
	step = *goal;
	if (state->route.valid)
	{
		/* the actor's step. On the point, or already past it while
		turning and still beside the line to the next one. */
		guard = 0;
		while (state->route.step_index + 1 < state->route.step_count && guard < 4)
		{
			real_point3d const *current;
			real_point3d const *next;
			real dx;
			real dy;
			real nx;
			real ny;
			boolean reached;

			current = &state->route.steps[state->route.step_index].point;
			next = &state->route.steps[state->route.step_index + 1].point;
			dx = current->x - feet->x;
			dy = current->y - feet->y;
			nx = next->x - current->x;
			ny = next->y - current->y;
			reached = dx * dx + dy * dy < 0.15f * 0.15f;
			if (!reached && state->move_thwarted)
			{
				real along;
				real span;
				real face;
				real cross;

				along = dx * nx + dy * ny;
				span = nx * nx + ny * ny;
				face = bot_angle_delta(state->facing_yaw, bot_yaw_between(current, next));
				if (face < 0.0f)
					face = -face;
				if (face < _pi * 0.5f && along < 0.0f && span > 0.0001f)
				{
					cross = dx * ny - dy * nx;
					if ((cross * cross) / span < 0.25f * 0.25f)
						reached = TRUE;
				}
			}
			if (!reached)
				break;
			state->route.step_index++;
			guard++;
		}
		if (state->route.step_index < state->route.step_count)
			step = state->route.steps[state->route.step_index].point;
	}
	/* another floor. The open ground toward their column is the circle
	under the ceiling. The route is the stairs. A wall on this floor is
	not that: a missing path walks an open heading instead of standing. */
	if (state->route_floor || bot_different_floor(feet, goal))
	{
		if (state->route.valid && state->route_floor)
		{
			*yaw = bot_wall_yaw(feet, &step, state->facing_yaw, unit_index);
			return TRUE;
		}
		if (bot_different_floor(feet, goal) || !state->route_tour)
			return FALSE;
	}
	/* a map trip. The step is the walk. No path on this floor is a
	short open step, not a stand. Another floor still waits for the route. */
	if (state->route_tour)
	{
		if (state->route.valid)
		{
			*yaw = bot_wall_yaw(feet, &step, state->facing_yaw, unit_index);
			return TRUE;
		}
		if (!bot_different_floor(feet, goal))
		{
			desired = bot_yaw_between(feet, goal);
			if (bot_open_distance(feet, desired, unit_index) >= 1.2f &&
				!bot_drop_ahead(feet, desired))
			{
				*yaw = desired;
				return TRUE;
			}
			/* the goal is through a wall. A side heading is the way around it. */
			if (bot_escape_yaw(feet, desired, unit_index, yaw))
				return TRUE;
		}
		return FALSE;
	}
	desired = bot_yaw_between(feet, goal);
	if (bot_open_distance(feet, desired, unit_index) >= 1.6f &&
		!bot_heading_is_corner(feet, desired, unit_index) &&
		!bot_drop_ahead(feet, desired))
	{
		*yaw = desired;
		return TRUE;
	}
	desired = bot_yaw_between(feet, &step);
	return bot_clear_yaw(feet, desired, unit_index, yaw);
}

/* the graph already chose the next link, including a pad or a jump. */
static boolean bot_hop_owned(struct bot_state const *state);

/* throttle along a heading already chosen. positive j is to the right.
returns FALSE when that heading is blocked. hold_facing is a fight: the
aim stays on the target and the bot may strafe. */
static boolean bot_apply_heading(struct bot_state *state, long unit_index, real_point3d const *feet,
	real walk, struct player_action *action, boolean hold_facing, boolean along_route)
{
	real rel;
	real clear;
	real scale;

	state->move_thwarted = FALSE;
	clear = bot_open_distance(feet, walk, unit_index);
	rel = bot_angle_delta(state->facing_yaw, walk);
	/* the next step leaves the floor. A platform edge reads as open
	because there is no wall. A jump that lands on the far platform is
	the step. Anything else stops, and the tour picks again.
	an item already closer than that probe is still on this floor. */
	if (!state->gap_hold && cosine(rel) > 0.2f && bot_drop_ahead(feet, walk) &&
		!(along_route && state->route.valid && bot_ramp_ahead(feet, walk)))
	{
		boolean grabbing_short = FALSE;

		if (state->job_grab > 0)
		{
			real gx = state->job_grab_point.x - feet->x;
			real gy = state->job_grab_point.y - feet->y;

			if (gx * gx + gy * gy < 1.05f * 1.05f)
				grabbing_short = TRUE;
		}
		/* still approaching a jump. Stop only once the edge is under
		them and the landing is gone. Stopping a meter early is a miss. */
		if (state->gap_ticks > 0)
		{
			real_point3d nose;
			real_point3d nose_ground;
			long nose_surface;
			boolean nose_gone;

			nose.x = feet->x + cosine(state->gap_yaw) * 0.50f;
			nose.y = feet->y + sine(state->gap_yaw) * 0.50f;
			nose.z = feet->z;
			nose_gone = !bot_ground_at(&nose, &nose_ground, &nose_surface) ||
				feet->z - nose_ground.z > 0.80f;
			if (nose_gone &&
				!bot_jump_across(feet, state->gap_yaw, unit_index, NULL, NULL))
			{
				action->throttle.i = 0.0f;
				action->throttle.j = 0.0f;
				state->gap_ticks = -25;
				return TRUE;
			}
		}
		else if (!state->job_climb &&
			bot_jump_across(feet, walk, unit_index, NULL, NULL))
		{
			state->gap_ticks = 50;
			state->gap_yaw = walk;
		}
		else if (!grabbing_short)
		{
			action->throttle.i = 0.0f;
			action->throttle.j = 0.0f;
			/* the commitment failed. Keeping it is the stall on the lip.
			a grab beside the edge is a pause, not a failed trip. */
			if (state->job != _bot_job_none && state->job_grab <= 0)
				state->job_blocked = TRUE;
			else if (state->route_tour && state->look_ticks > 15)
				state->look_ticks = 15;
			return TRUE;
		}
	}
	/* walk the turn. Stop only when the nose is in a wall, or the step
	is behind them and a forward step would be into that wall. */
	if (along_route)
	{
		real nose;

		nose = bot_first_block(feet, state->facing_yaw, 0.45f, 0.45f, unit_index);
		action->throttle.j = 0.0f;
		if (nose >= 0.0f || cosine(rel) <= 0.25f)
		{
			action->throttle.i = 0.0f;
			state->move_thwarted = TRUE;
		}
		else
			action->throttle.i = 1.0f;
		return TRUE;
	}
	if (clear < 1.1f || bot_heading_is_corner(feet, walk, unit_index))
	{
		action->throttle.i = 0.0f;
		action->throttle.j = 0.0f;
		return FALSE;
	}
	scale = clear < 2.8f ? clear / 2.8f : 1.0f;
	action->throttle.i = cosine(rel) * scale;
	action->throttle.j = -sine(rel) * scale;
	/* the nose is still coming around. Keep the part of the step that is
	not into the wall ahead. Stopping here is the pan: the step moves,
	the facing never catches it, and the throttle stays at zero. */
	if (!hold_facing && (rel > 0.6f || rel < -0.6f) &&
		action->throttle.i > 0.0f &&
		bot_open_distance(feet, state->facing_yaw, unit_index) < 1.0f)
		action->throttle.i = 0.0f;
	if (action->throttle.i == 0.0f && action->throttle.j == 0.0f)
		return FALSE;
	return TRUE;
}

/* walk a corridor now. A big turn used to zero the throttle until the
look arrived, which is the stand-and-scan in a hall. */
static boolean bot_walk_hall(struct bot_state *state, long unit_index,
	real_point3d const *feet, real hall, struct player_action *action)
{
	real rel;
	real ahead;

	rel = bot_angle_delta(state->facing_yaw, hall);
	ahead = bot_open_distance(feet, state->facing_yaw, unit_index);
	action->throttle.i = cosine(rel);
	action->throttle.j = -sine(rel);
	if (action->throttle.i > 0.0f && ahead < 1.0f)
		action->throttle.i = 0.0f;
	if (action->throttle.i > 0.45f)
		action->throttle.i = 1.0f;
	else if (action->throttle.i < -0.45f)
		action->throttle.i = -1.0f;
	if (action->throttle.j > 0.45f)
		action->throttle.j = 1.0f;
	else if (action->throttle.j < -0.45f)
		action->throttle.j = -1.0f;
	return action->throttle.i != 0.0f || action->throttle.j != 0.0f;
}

/* walk toward goal around walls. returns FALSE when every probe hits. */
static boolean bot_walk(struct bot_state *state, long unit_index, real_point3d const *feet,
	real_point3d const *goal, struct player_action *action, boolean hold_facing)
{
	real walk;

	if (!bot_step_yaw(state, unit_index, feet, goal, &walk))
	{
		action->throttle.i = 0.0f;
		action->throttle.j = 0.0f;
		return FALSE;
	}
	return bot_apply_heading(state, unit_index, feet, walk, action, hold_facing,
		(state->route_tour || state->route_floor) && state->route.valid);
}

/* TRUE when this projectile tag is one of the grenades in the globals. */
static boolean bot_is_grenade_projectile(struct game_globals *globals, long definition_index)
{
	long count;
	long type;

	count = globals->grenades.count;
	if (count > 8)
		count = 8;
	for (type = 0; type < count; type++)
	{
		struct game_globals_grenade *info;

		info = TAG_BLOCK_GET_ELEMENT(&globals->grenades, type, struct game_globals_grenade);
		if (info && info->projectile.index == definition_index)
			return TRUE;
	}
	return FALSE;
}

/* how far to stand from a live grenade. The tag's danger radius, with a
floor so a small value still keeps them out of the blast. */
static real bot_grenade_keep(long definition_index)
{
	struct projectile_definition const *definition;
	real danger;

	definition = projectile_definition_get(definition_index);
	danger = definition ? definition->projectile.danger_radius : 0.0f;
	if (danger < 2.5f)
		danger = 2.5f;
	if (danger > 5.5f)
		danger = 5.5f;
	return danger;
}

/* the grenade's blast. A missing tag still has to be a frag, not a
point, or the last throw never counts as close enough. */
static real bot_grenade_danger(short type)
{
	struct game_globals *globals;
	struct game_globals_grenade *info;
	struct projectile_definition const *definition;
	real danger;

	globals = scenario_get_game_globals();
	danger = 3.2f;
	if (!globals || type < 0 || type >= globals->grenades.count)
		return danger;
	info = TAG_BLOCK_GET_ELEMENT(&globals->grenades, type, struct game_globals_grenade);
	if (!info || info->projectile.index == NONE)
		return danger;
	definition = projectile_definition_get(info->projectile.index);
	if (!definition)
		return danger;
	danger = definition->projectile.danger_radius;
	if (danger < 2.0f)
		danger = 3.2f;
	return danger;
}

/* an arm's length in front of them. Inside the strong part of the
blast from that landing is a kill, not the fringe. */
static boolean bot_grenade_can_finish(real separation, short type)
{
	real reach;

	reach = bot_grenade_danger(type) * 0.55f + 0.80f;
	return separation <= reach;
}

static boolean bot_about_to_die(struct unit_datum const *unit)
{
	return unit->object.shield_vitality <= 0.15f && unit->object.body_vitality < 0.35f;
}

/* the line the grenade will actually travel. A wall or a solid in
front of the hand is a bounce. The enemy at the end of it is the throw. */
static boolean bot_throw_reaches(real_point3d const *from, real_point3d const *to,
	long self_unit, long enemy_unit)
{
	struct collision_result collision;
	unsigned long flags;

	flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_objects_bit) |
		FLAG(_collision_test_objects_bipeds_bit) |
		FLAG(_collision_test_objects_vehicles_bit) |
		FLAG(_collision_test_objects_scenery_bit) |
		FLAG(_collision_test_objects_machines_bit) |
		FLAG(_collision_test_objects_controls_bit);
	if (!collision_test_line(flags, from, to, self_unit, &collision))
		return TRUE;
	if (enemy_unit != NONE && collision.object_index == enemy_unit)
		return TRUE;
	return collision.t >= 0.85f;
}

/* an open heading off the grenade. A wall straight away is not a path:
the way out can be a side opening, and only then anything still open. */
static boolean bot_grenade_escape_yaw(real_point3d const *feet, real away, long unit_index, real *yaw)
{
	static real const offsets[] = {
		0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 1.5f, -1.5f, 2.1f, -2.1f, 2.6f, -2.6f, 3.14159274f
	};
	long pass;
	long index;
	real best;
	real best_score;
	boolean found;

	best = away;
	best_score = -1.0e9f;
	found = FALSE;
	for (pass = 0; pass < 2 && !found; pass++)
	{
		for (index = 0; index < (long)NUMBEROF(offsets); index++)
		{
			real heading = away + offsets[index];
			real delta = bot_angle_delta(away, heading);
			real away_part;
			real clear;
			real score;

			if (delta < 0.0f)
				delta = -delta;
			away_part = cosine(delta);
			/* first the headings that actually leave. Then any opening
			that is not the wall in front of them. */
			if (pass == 0 && away_part < 0.2f)
				continue;
			clear = bot_open_distance(feet, heading, unit_index);
			if (clear < 1.6f || bot_heading_is_corner(feet, heading, unit_index))
				continue;
			score = clear + away_part * 6.0f;
			if (score > best_score)
			{
				best_score = score;
				best = heading;
				found = TRUE;
			}
		}
	}
	if (!found)
		return FALSE;
	*yaw = best;
	return TRUE;
}

/* the closest live grenade. own is TRUE when this unit threw it.
one on another floor is not a blast they are standing in. */
static boolean bot_nearest_grenade(long unit_index, real_point3d const *feet,
	real_point3d *point, real *keep, boolean *own)
{
	struct game_globals *globals;
	struct object_iterator iterator;
	struct object_datum *object;
	real best;
	boolean found;

	globals = scenario_get_game_globals();
	if (!globals || globals->grenades.count <= 0)
		return FALSE;
	best = 1.0e9f;
	found = FALSE;
	object_iterator_new(&iterator, _object_mask_projectile, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		real_point3d origin;
		real dx;
		real dy;
		real dz;
		real dist;

		if (!bot_is_grenade_projectile(globals, object->definition_index))
			continue;
		object_get_origin(iterator.index, &origin);
		dx = origin.x - feet->x;
		dy = origin.y - feet->y;
		dz = origin.z - feet->z;
		if (dz < 0.0f)
			dz = -dz;
		if (dz > 6.0f)
			continue;
		dist = dx * dx + dy * dy;
		/* where it will be in a moment. A grenade still in the air is
		already the thing to leave, not the spot it left. */
		{
			real_point3d soon;
			real sx;
			real sy;
			real soon_dist;

			soon.x = origin.x + object->object.translational_velocity.i * 0.5f;
			soon.y = origin.y + object->object.translational_velocity.j * 0.5f;
			soon.z = origin.z + object->object.translational_velocity.k * 0.5f;
			sx = soon.x - feet->x;
			sy = soon.y - feet->y;
			soon_dist = sx * sx + sy * sy;
			if (soon_dist < dist)
			{
				origin = soon;
				dx = sx;
				dy = sy;
				dist = soon_dist;
			}
		}
		if (dist > 18.0f * 18.0f)
			continue;
		if (dist < best)
		{
			best = dist;
			*point = origin;
			*keep = bot_grenade_keep(object->definition_index);
			*own = object->object.owner_object_index == unit_index;
			found = TRUE;
		}
	}
	return found;
}

/* pull the step that would walk back into a grenade that is still close */
static void bot_dont_close_on_grenade(struct bot_state *state, real_point3d const *feet,
	real_point3d const *nade, struct player_action *action)
{
	real rel;
	real closing;

	rel = bot_angle_delta(state->facing_yaw, bot_yaw_between(feet, nade));
	closing = action->throttle.i * cosine(rel) - action->throttle.j * sine(rel);
	if (closing > 0.05f)
	{
		action->throttle.i -= cosine(rel) * closing;
		action->throttle.j += sine(rel) * closing;
	}
}

static void bot_face(struct player_datum *player, struct player_action *action,
	real_point3d const *from, real_point3d const *to, real yaw_error, real pitch_error,
	real_vector3d *aim);

/* run along an open step away from the grenade. The step ends at the
wall, and the next one is the opening, instead of holding the same
heading into the face. */
static void bot_run_from_grenade(struct bot_state *state, struct player_datum *player,
	long unit_index, real_point3d const *feet, real away, struct player_action *action)
{
	real escape;
	real reach;
	real walk;
	real_point3d goal;
	real_point3d eye;
	real_point3d look;
	real_vector3d aim;

	action->throttle.i = 0.0f;
	action->throttle.j = 0.0f;
	if (!bot_grenade_escape_yaw(feet, away, unit_index, &escape))
		return;
	reach = bot_open_distance(feet, escape, unit_index);
	if (reach > 6.0f)
		reach = 6.0f;
	goal.x = feet->x + cosine(escape) * reach;
	goal.y = feet->y + sine(escape) * reach;
	goal.z = feet->z;
	state->route_floor = FALSE;
	if (!bot_step_yaw(state, unit_index, feet, &goal, &walk))
		return;
	state->facing_yaw = bot_slew(state->facing_yaw, walk, 0.28f);
	state->facing_pitch = bot_slew(state->facing_pitch, 0.0f, 0.2f);
	eye = *feet;
	eye.z += 0.62f;
	look.x = eye.x + cosine(state->facing_yaw);
	look.y = eye.y + sine(state->facing_yaw);
	look.z = eye.z;
	bot_face(player, action, &eye, &look, 0.0f, 0.0f, &aim);
	bot_apply_heading(state, unit_index, feet, walk, action, FALSE, FALSE);
	if (state->vault == 1)
		state->vault = 0;
}

/* a live grenade. Their own: get out of the blast, and do not step back
in while it is still close. A trade stays where it was thrown. Anyone
else's, including the player's, is a run, not a sidestep. */
static void bot_respect_grenades(struct bot_state *state, struct player_datum *player,
	long unit_index, real_point3d const *feet, struct player_action *action)
{
	real_point3d nade;
	real keep;
	real flee;
	real dx;
	real dy;
	real dist;
	real away;
	boolean own;

	if (state->vault == 2)
		return;
	if (!bot_nearest_grenade(unit_index, feet, &nade, &keep, &own))
	{
		state->nade_trade = FALSE;
		if (state->nade_hold > 0)
			bot_run_from_grenade(state, player, unit_index, feet, state->facing_yaw + _pi, action);
		return;
	}
	state->nade_hold = 0;
	if (own && state->nade_trade)
		return;
	dx = feet->x - nade.x;
	dy = feet->y - nade.y;
	dist = square_root(dx * dx + dy * dy);
	if (dist < 0.2f)
		away = state->facing_yaw + _pi;
	else
		away = bot_yaw_between(&nade, feet);
	/* someone else's grenade is left from well outside the blast, while
	it is still in the air */
	flee = keep;
	if (!own)
	{
		flee = keep + 6.0f;
		if (flee < 8.0f)
			flee = 8.0f;
	}
	if (dist < flee)
	{
		bot_run_from_grenade(state, player, unit_index, feet, away, action);
		return;
	}
	if (dist < flee + 1.5f)
		bot_dont_close_on_grenade(state, feet, &nade, action);
}

/* an idle goal in open ground. A blocked facing takes the smallest turn
that is open. Starting the search from a half turn makes them spin in
place whenever the way ahead is short. */
static void bot_choose_roam(struct bot_state *state, real_point3d const *feet, long unit_index)
{
	static real const turns[] = {
		0.0f, 0.45f, -0.45f, 0.9f, -0.9f, 1.4f, -1.4f, 2.2f, -2.2f, 3.14159274f
	};
	real best_yaw;
	real best_clear;
	real best_turn;
	real fallback_yaw;
	real fallback_clear;
	real reach;
	long index;

	/* the way ahead is already open: keep walking it. Picking whichever
	ray is longest turns the bot out of a hall and into a corner.
	an open ray off a platform is a fall, not a path. */
	best_clear = bot_open_distance(feet, state->facing_yaw, unit_index);
	best_yaw = state->facing_yaw;
	if (bot_drop_ahead(feet, best_yaw))
		best_clear = 0.0f;
	if (best_clear < 1.6f)
	{
		best_yaw = state->facing_yaw;
		best_clear = 0.0f;
		best_turn = 4.0f;
		fallback_yaw = state->facing_yaw;
		fallback_clear = 0.0f;
		for (index = 0; index < (long)NUMBEROF(turns); index++)
		{
			real turn = turns[index];
			real yaw = state->facing_yaw + turn;
			real clear = bot_open_distance(feet, yaw, unit_index);
			real magnitude = turn < 0.0f ? -turn : turn;

			if (clear > fallback_clear)
			{
				fallback_clear = clear;
				fallback_yaw = yaw;
			}
			if (bot_heading_is_corner(feet, yaw, unit_index))
				continue;
			if (bot_drop_ahead(feet, yaw))
				continue;
			if (clear >= 2.5f && magnitude < best_turn)
			{
				best_turn = magnitude;
				best_clear = clear;
				best_yaw = yaw;
			}
			else if (best_turn >= 4.0f && clear > best_clear)
			{
				best_clear = clear;
				best_yaw = yaw;
			}
		}
		if (best_clear < 1.6f)
		{
			best_yaw = fallback_yaw;
			best_clear = fallback_clear;
		}
	}
	state->wander_yaw = best_yaw;
	if (best_clear < 1.2f)
		reach = 1.2f;
	else
	{
		reach = best_clear * 0.7f;
		if (reach > 9.0f)
			reach = 9.0f;
		if (reach < 1.5f)
			reach = 1.5f;
	}
	state->roam_goal.x = feet->x + cosine(best_yaw) * reach;
	state->roam_goal.y = feet->y + sine(best_yaw) * reach;
	state->roam_goal.z = feet->z;
	state->roam_valid = TRUE;
	state->route.valid = FALSE;
	state->look_ticks = 70 + (long)((bot_random() >> 8) % 50);
}

/* the floor under a map point. The path to another story needs that
surface, not the bot's own. */
static boolean bot_ground_at(real_point3d const *point, real_point3d *ground, long *surface)
{
	struct collision_bsp *bsp;
	struct collision_bsp_test_vector_result result;
	real_point3d origin;
	real_vector3d down;

	*surface = NONE;
	*ground = *point;
	bsp = global_collision_bsp_get();
	if (!bsp || !global_down3d)
		return FALSE;
	origin = *point;
	origin.z += 0.6f;
	scale_vector3d(global_down3d, 4.0f, &down);
	if (!collision_bsp_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit),
			bsp, 0, NULL, &origin, &down, 1.0f, &result) ||
		result.surface_index == NONE)
		return FALSE;
	ground->x = origin.x + down.i * result.t;
	ground->y = origin.y + down.j * result.t;
	ground->z = origin.z + down.k * result.t;
	/* a hole under the marker is not the ledge it was marking */
	if (point->z - ground->z > 2.5f)
		return FALSE;
	*surface = result.surface_index;
	return TRUE;
}

static boolean bot_drop_ahead(real_point3d const *feet, real yaw)
{
	real_point3d probe;
	real_point3d ground;
	long surface;

	if (!global_collision_bsp_get())
		return FALSE;
	probe.x = feet->x + cosine(yaw) * 1.05f;
	probe.y = feet->y + sine(yaw) * 1.05f;
	probe.z = feet->z;
	if (!bot_ground_at(&probe, &ground, &surface))
		return TRUE;
	return feet->z - ground.z > 1.5f;
}

/* the path already chose this step. A ramp is still under them as it
descends, at the near step and again further on. The end of a catwalk
is a fall: the floor is there, then it is gone. */
static boolean bot_ramp_ahead(real_point3d const *feet, real yaw)
{
	real_point3d probe;
	real_point3d ground;
	long surface;
	real drop[3];
	long index;
	static real const reach[3] = { 0.35f, 0.95f, 1.70f };

	for (index = 0; index < 3; index++)
	{
		probe.x = feet->x + cosine(yaw) * reach[index];
		probe.y = feet->y + sine(yaw) * reach[index];
		probe.z = feet->z;
		if (!bot_ground_at(&probe, &ground, &surface))
			return FALSE;
		drop[index] = feet->z - ground.z;
		if (drop[index] < 0.0f)
			drop[index] = 0.0f;
	}
	/* already off the floor at the first step */
	if (drop[0] > 0.45f)
		return FALSE;
	/* a cliff between probes. A stair is a smaller step than that. */
	if (drop[1] > drop[0] + 0.75f || drop[2] > drop[1] + 0.80f)
		return FALSE;
	return TRUE;
}

/* how far a running jump carries them back to the same height. The
fraction is the landing, not the theoretical apex. */
static real bot_jump_span(long unit_index)
{
	struct biped_datum *biped;
	struct biped_definition *definition;
	real jump;
	real ticks;
	real span;

	biped = biped_try_and_get(unit_index);
	if (!biped)
		return 2.4f;
	definition = biped_definition_get(biped->definition_index);
	if (!definition)
		return 2.4f;
	jump = definition->biped.jump_velocity;
	if (jump > 1.0f)
		jump = jump / (real)TICKS_PER_SECOND;
	if (jump < 0.02f || jump > 0.40f)
		return 2.4f;
	ticks = 2.0f * jump / 0.0035651792f;
	span = 2.25f * ticks / (real)TICKS_PER_SECOND;
	span *= 0.72f;
	if (span < 1.4f)
		span = 1.4f;
	if (span > 4.2f)
		span = 4.2f;
	return span;
}

/* the floor ends, then comes back inside the jump, at about this height.
a pit, a wall, or a step they should vault is not one. */
static boolean bot_jump_across(real_point3d const *feet, real yaw, long unit_index,
	real *edge, real_point3d *landing)
{
	real span;
	real dist;
	real lip;
	real_point3d probe;
	real_point3d ground;
	long surface;

	span = bot_jump_span(unit_index);
	lip = -1.0f;
	for (dist = 0.40f; dist <= 2.20f; dist += 0.20f)
	{
		probe.x = feet->x + cosine(yaw) * dist;
		probe.y = feet->y + sine(yaw) * dist;
		probe.z = feet->z;
		if (!bot_ground_at(&probe, &ground, &surface) || feet->z - ground.z > 1.2f)
		{
			lip = dist;
			break;
		}
	}
	if (lip < 0.0f)
		return FALSE;
	/* span is how far the jump carries them from where they stand, not
	from the edge. A landing past that is a fall. */
	for (dist = lip + 0.35f; dist <= span; dist += 0.25f)
	{
		if (dist > lip + 0.55f &&
			!bot_ray_clear_z(feet, yaw, dist - 0.30f, 0.90f, unit_index))
			return FALSE;
		probe.x = feet->x + cosine(yaw) * dist;
		probe.y = feet->y + sine(yaw) * dist;
		probe.z = feet->z;
		if (!bot_ground_at(&probe, &ground, &surface))
			continue;
		/* a low platform is still a landing. A wall with a floor on top is not. */
		if (ground.z > feet->z + 0.50f)
			continue;
		if (feet->z - ground.z > 0.70f)
			continue;
		{
			real_point3d past;
			real_point3d past_ground;
			long past_surface;
			real rise;

			/* the first ground past the gap can be the lip itself.
			the platform continues. A sliver over the void does not. */
			past.x = probe.x + cosine(yaw) * 0.45f;
			past.y = probe.y + sine(yaw) * 0.45f;
			past.z = ground.z;
			if (!bot_ground_at(&past, &past_ground, &past_surface))
				continue;
			rise = past_ground.z - ground.z;
			if (rise < 0.0f)
				rise = -rise;
			if (rise > 0.50f)
				continue;
		}
		if (edge)
			*edge = lip;
		if (landing)
			*landing = ground;
		return TRUE;
	}
	return FALSE;
}

/* a landing off the straight step, closer to where they are going.
the camo platform is a diagonal, and the hall step never points at it. */
static boolean bot_jump_choice(real_point3d const *feet, long unit_index,
	real_point3d const *goal, real *yaw)
{
	static real const offsets[] = { 0.0f, 0.45f, -0.45f, 0.90f, -0.90f, 1.25f, -1.25f };
	real base;
	real here;
	real best;
	real chosen;
	long index;
	boolean found;

	base = bot_yaw_between(feet, goal);
	here = (goal->x - feet->x) * (goal->x - feet->x) + (goal->y - feet->y) * (goal->y - feet->y);
	best = here;
	chosen = base;
	found = FALSE;
	for (index = 0; index < (long)NUMBEROF(offsets); index++)
	{
		real try_yaw;
		real_point3d landing;
		real left;

		try_yaw = base + offsets[index];
		if (!bot_jump_across(feet, try_yaw, unit_index, NULL, &landing))
			continue;
		left = (goal->x - landing.x) * (goal->x - landing.x) +
			(goal->y - landing.y) * (goal->y - landing.y);
		if (left + 0.35f >= here || left >= best)
			continue;
		best = left;
		chosen = try_yaw;
		found = TRUE;
	}
	if (!found)
		return FALSE;
	*yaw = chosen;
	return TRUE;
}

/* collision surface flags, in field order: two sided, invisible, climbable, breakable */
enum
{
	_bot_collision_surface_climbable_bit = 2
};

/* a ladder polygon with a place to stand in front of it. top is the
top of that face, and yaw looks into the face so physics will grab it. */
static boolean bot_ladder_base(struct collision_bsp *bsp, long surface_index,
	real_point3d *stand, real *top, real *yaw)
{
	struct collision_surface *surface;
	real_plane3d plane;
	real_point3d points[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
	short count;
	short index;
	real_point3d mid;
	real lowest;
	real highest;
	real_point3d ground;
	long ground_surface;
	real_point3d origin;
	real_vector3d into;
	struct collision_bsp_test_vector_result hit;
	boolean facing_ok;
	real sign;

	if (!bsp || surface_index < 0 || surface_index >= bsp->surfaces.count)
		return FALSE;
	surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
	if (!surface || !TEST_FLAG(surface->flags, _bot_collision_surface_climbable_bit))
		return FALSE;
	bsp3d_get_plane_from_designator(&bsp->bsp3d, surface->plane_designator, &plane);
	if (plane.n.k > 0.40f || plane.n.k < -0.40f)
		return FALSE;
	count = collision_surface_polygon(bsp, surface_index, points);
	if (count < 3)
		return FALSE;
	mid = points[0];
	lowest = highest = points[0].z;
	for (index = 1; index < count; index++)
	{
		mid.x += points[index].x;
		mid.y += points[index].y;
		mid.z += points[index].z;
		if (points[index].z < lowest)
			lowest = points[index].z;
		if (points[index].z > highest)
			highest = points[index].z;
	}
	mid.x /= (real)count;
	mid.y /= (real)count;
	mid.z /= (real)count;
	if (highest - lowest < 1.15f)
		return FALSE;
	facing_ok = FALSE;
	sign = 1.0f;
	*stand = mid;
	for (index = 0; index < 2 && !facing_ok; index++)
	{
		stand->x = mid.x + plane.n.i * 0.70f * sign;
		stand->y = mid.y + plane.n.j * 0.70f * sign;
		stand->z = lowest;
		if (bot_ground_at(stand, &ground, &ground_surface) && lowest - ground.z <= 1.6f)
		{
			*stand = ground;
			origin = *stand;
			origin.z += 0.85f;
			into.i = mid.x - origin.x;
			into.j = mid.y - origin.y;
			into.k = 0.0f;
			if (collision_bsp_test_vector(
					FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_back_facing_surfaces_bit),
					bsp, 0, NULL, &origin, &into, 1.0f, &hit) &&
				hit.surface_index == surface_index)
				facing_ok = TRUE;
		}
		sign = -1.0f;
	}
	if (!facing_ok)
		return FALSE;
	/* stand is the side the ray hit from, so this looks into the face. */
	*yaw = bot_yaw_between(stand, &mid);
	*top = highest;
	return TRUE;
}

/* already on a climbable face. The yaw looks into that face. */
static boolean bot_support_ladder_yaw(long unit_index, real *yaw)
{
	struct biped_datum *biped;
	struct collision_bsp *bsp;
	struct collision_surface *surface;

	biped = biped_try_and_get(unit_index);
	bsp = global_collision_bsp_get();
	if (!biped || !bsp || biped->biped.support_surface_index == NONE)
		return FALSE;
	if (biped->biped.support_surface_index < 0 ||
		biped->biped.support_surface_index >= bsp->surfaces.count)
		return FALSE;
	surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces,
		biped->biped.support_surface_index, struct collision_surface);
	if (!surface || !TEST_FLAG(surface->flags, _bot_collision_surface_climbable_bit))
		return FALSE;
	/* the ground plane is the one physics already accepted. A floor's
	normal points up, and that is not a climb. */
	if (biped->biped.ground_plane.n.k > 0.55f || biped->biped.ground_plane.n.k < -0.55f)
		return FALSE;
	*yaw = arctangent(-biped->biped.ground_plane.n.j, -biped->biped.ground_plane.n.i);
	return TRUE;
}

/* a climbable face within a few meters that still rises above the bot.
Twelve headings, two heights, so a ladder ahead is not a gap between rays. */
static boolean bot_ladder_near(real_point3d const *feet, real *yaw)
{
	struct collision_bsp *bsp;
	long height;
	long step;
	real best;
	boolean found;

	bsp = global_collision_bsp_get();
	if (!bsp)
		return FALSE;
	best = 6.0f;
	found = FALSE;
	for (height = 0; height < 2; height++)
	{
		for (step = 0; step < 12; step++)
		{
			real heading = (real)step * (_pi * 2.0f / 12.0f);
			real_point3d origin;
			real_vector3d ray;
			struct collision_bsp_test_vector_result hit;
			real_plane3d plane;
			real_point3d poly[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
			short count;
			short vertex;
			real top;
			real dist;

			origin = *feet;
			origin.z += height == 0 ? 0.55f : 1.15f;
			ray.i = cosine(heading) * 6.0f;
			ray.j = sine(heading) * 6.0f;
			ray.k = 0.0f;
			if (!collision_bsp_test_vector(
					FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_back_facing_surfaces_bit),
					bsp, 0, NULL, &origin, &ray, 1.0f, &hit) ||
				hit.surface_index == NONE ||
				!TEST_FLAG(hit.flags, _bot_collision_surface_climbable_bit))
				continue;
			bsp3d_get_plane_from_designator(&bsp->bsp3d, hit.plane_designator, &plane);
			if (plane.n.k > 0.40f || plane.n.k < -0.40f)
				continue;
			count = collision_surface_polygon(bsp, hit.surface_index, poly);
			if (count < 3)
				continue;
			top = poly[0].z;
			for (vertex = 1; vertex < count; vertex++)
			{
				if (poly[vertex].z > top)
					top = poly[vertex].z;
			}
			/* the face still goes up from here. A lip at foot level does not. */
			if (top < feet->z + 1.4f)
				continue;
			dist = hit.t * 6.0f;
			if (dist < best)
			{
				best = dist;
				/* the ray that hit the face is the direction into it. */
				*yaw = heading;
				found = TRUE;
			}
		}
	}
	return found;
}

static boolean bot_point_banned(struct bot_state const *state, real_point3d const *point)
{
	long index;

	for (index = 0; index < 4; index++)
	{
		real dx;
		real dy;

		if (state->given_up_ticks[index] <= 0)
			continue;
		dx = point->x - state->given_up[index].x;
		dy = point->y - state->given_up[index].y;
		if (dx * dx + dy * dy < 6.0f * 6.0f)
			return TRUE;
	}
	return FALSE;
}

static void bot_ban_point(struct bot_state *state, real_point3d const *point, long ticks)
{
	long slot = 0;
	long index;

	for (index = 1; index < 4; index++)
	{
		if (state->given_up_ticks[index] < state->given_up_ticks[slot])
			slot = index;
	}
	state->given_up[slot] = *point;
	state->given_up_ticks[slot] = ticks;
}

/* places a player would stand, and the short links between them. Built
once for the loaded map. A trip walks one link, then the next. */
enum
{
	_bot_link_walk = 0,
	_bot_link_jump = 1,
	_bot_link_ladder = 2,
	_bot_link_pad = 3
};

enum
{
	BOT_ROUTE_NODES = 96,
	BOT_ROUTE_LINKS = 384
};

enum
{
	_bot_route_phase_seed = 0,
	_bot_route_phase_ladder = 1,
	_bot_route_phase_flood = 2,
	_bot_route_phase_link = 3,
	_bot_route_phase_ready = 4,
	_bot_route_phase_failed = 5,
	_bot_route_phase_jumps = 6
};

struct bot_route_node
{
	real_point3d point;
	long surface;
};

struct bot_route_link
{
	short from;
	short to;
	short kind;
	real yaw;
	char step_count;
	real_point3d steps[4];
};

static struct bot_route_node bot_route_nodes[BOT_ROUTE_NODES];
static struct bot_route_link bot_route_links[BOT_ROUTE_LINKS];
static short bot_route_node_count;
static short bot_route_link_count;
static short bot_route_phase;
static short bot_route_head[BOT_ROUTE_NODES];
static short bot_route_next[BOT_ROUTE_LINKS];
static void const *bot_route_map;
static long bot_route_ladder_index;
static long bot_route_build_ticks;
static short bot_route_flood_i;
static short bot_route_flood_dir;
static short bot_route_flood_wave;
static short bot_route_flood_limit;
static short bot_route_pair_i;
static short bot_route_pair_j;
static short bot_route_pair_dir;
static short bot_route_pair_pass;
static short bot_route_split_min;

static void bot_route_reset(void const *map)
{
	long index;

	bot_route_map = map;
	bot_route_node_count = 0;
	bot_route_link_count = 0;
	bot_route_phase = _bot_route_phase_seed;
	bot_route_ladder_index = 0;
	bot_route_build_ticks = 0;
	bot_route_flood_i = 0;
	bot_route_flood_dir = 0;
	bot_route_flood_wave = 0;
	bot_route_flood_limit = 0;
	bot_route_pair_i = 0;
	bot_route_pair_j = 1;
	bot_route_pair_dir = 0;
	bot_route_pair_pass = 0;
	bot_route_split_min = BOT_ROUTE_NODES;
	for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
	{
		bot_states[index].job_hop = NONE;
		bot_states[index].route_link = NONE;
		bot_states[index].route.valid = FALSE;
	}
}

/* snap onto walkable floor. A point already covered returns that node.
sep is how close a second node would be the same place. */
static short bot_route_add(real_point3d const *point, real sep)
{
	real_point3d ground;
	long surface;
	long index;
	short found;
	real best;

	if (!bot_ground_at(point, &ground, &surface) || !bot_surface_walkable(surface))
		return NONE;
	best = sep * sep;
	found = NONE;
	for (index = 0; index < bot_route_node_count; index++)
	{
		real dx;
		real dy;
		real dz;

		dx = ground.x - bot_route_nodes[index].point.x;
		dy = ground.y - bot_route_nodes[index].point.y;
		dz = ground.z - bot_route_nodes[index].point.z;
		if (dz < 0.0f)
			dz = -dz;
		if (dz > 1.6f)
			continue;
		if (dx * dx + dy * dy < best)
		{
			best = dx * dx + dy * dy;
			found = (short)index;
		}
	}
	if (found != NONE)
		return found;
	if (bot_route_node_count >= BOT_ROUTE_NODES)
		return NONE;
	/* every chest ray dies immediately: the point is inside a solid. */
	if (bot_open_distance(&ground, 0.0f, NONE) < 0.40f &&
		bot_open_distance(&ground, _pi * 0.5f, NONE) < 0.40f &&
		bot_open_distance(&ground, _pi, NONE) < 0.40f &&
		bot_open_distance(&ground, -_pi * 0.5f, NONE) < 0.40f)
		return NONE;
	bot_route_nodes[bot_route_node_count].point = ground;
	bot_route_nodes[bot_route_node_count].surface = surface;
	bot_route_node_count++;
	return (short)(bot_route_node_count - 1);
}

static boolean bot_route_has(short from, short to, short kind)
{
	long index;

	for (index = 0; index < bot_route_link_count; index++)
	{
		if (bot_route_links[index].from == from && bot_route_links[index].to == to &&
			bot_route_links[index].kind == kind)
			return TRUE;
	}
	return FALSE;
}

static boolean bot_route_push(short from, short to, short kind, real yaw,
	struct path_result const *path)
{
	struct bot_route_link *link;
	long count;
	long index;

	if (from < 0 || to < 0 || from == to)
		return FALSE;
	if (bot_route_link_count >= BOT_ROUTE_LINKS)
		return FALSE;
	if (bot_route_has(from, to, kind))
		return FALSE;
	link = &bot_route_links[bot_route_link_count];
	link->from = from;
	link->to = to;
	link->kind = kind;
	link->yaw = yaw;
	link->step_count = 0;
	if (path && kind == _bot_link_walk)
	{
		count = path->step_count;
		if (count > 4)
			count = 4;
		if (count < 0)
			count = 0;
		link->step_count = (char)count;
		for (index = 0; index < count; index++)
			link->steps[index] = path->steps[index].point;
	}
	bot_route_link_count++;
	return TRUE;
}

static short bot_route_ladder_top(real_point3d const *stand, real top, real yaw)
{
	long index;

	for (index = 0; index < 3; index++)
	{
		real_point3d probe;
		real_point3d ground;
		long surface;
		short node;

		probe = *stand;
		probe.z = top;
		if (index == 1)
		{
			probe.x += cosine(yaw) * 0.9f;
			probe.y += sine(yaw) * 0.9f;
		}
		else if (index == 2)
		{
			probe.x -= cosine(yaw) * 0.9f;
			probe.y -= sine(yaw) * 0.9f;
		}
		if (!bot_ground_at(&probe, &ground, &surface))
			continue;
		if (ground.z < stand->z + 1.2f)
			continue;
		if (top - ground.z > 1.6f || ground.z - top > 1.6f)
			continue;
		node = bot_route_add(&ground, 6.0f);
		if (node != NONE)
			return node;
	}
	return NONE;
}

static void bot_route_seed(void)
{
	struct scenario *scenario;
	long index;

	scenario = global_scenario_get();
	if (!scenario)
	{
		bot_route_phase = _bot_route_phase_failed;
		return;
	}
	for (index = 0; index < scenario->players.count; index++)
	{
		struct player_starting_location *spawn;

		spawn = TAG_BLOCK_GET_ELEMENT(&scenario->players, index,
			struct player_starting_location);
		if (spawn)
			bot_route_add(&spawn->position, 6.0f);
	}
	for (index = 0; index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *flag;

		flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index,
			struct scenario_netgame_flag);
		if (flag)
			bot_route_add(&flag->position, 6.0f);
	}
	for (index = 0; index < scenario->netgame_equipment.count; index++)
	{
		struct scenario_netgame_equipment *item;

		item = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, index,
			struct scenario_netgame_equipment);
		if (item)
			bot_route_add(&item->position, 6.0f);
	}
	for (index = 0; index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *source;
		struct scenario_netgame_flag *target;
		long target_index;
		short from;
		short to;

		source = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index,
			struct scenario_netgame_flag);
		if (!source || source->type != _netgame_flag_teleporter_source)
			continue;
		target = NULL;
		for (target_index = 0; target_index < scenario->netgame_flags.count; target_index++)
		{
			struct scenario_netgame_flag *candidate;

			candidate = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, target_index,
				struct scenario_netgame_flag);
			if (!candidate || candidate->type != _netgame_flag_teleporter_target)
				continue;
			if (candidate->team_index != source->team_index)
				continue;
			target = candidate;
			break;
		}
		if (!target)
			continue;
		from = bot_route_add(&source->position, 6.0f);
		to = bot_route_add(&target->position, 6.0f);
		if (from != NONE && to != NONE)
			bot_route_push(from, to, _bot_link_pad, 0.0f, NULL);
	}
	if (bot_route_node_count < 1)
		bot_route_phase = _bot_route_phase_failed;
	else
	{
		bot_route_phase = _bot_route_phase_ladder;
		bot_route_ladder_index = 0;
	}
}

static void bot_route_ladders_tick(void)
{
	struct collision_bsp *bsp;
	long budget;

	bsp = global_collision_bsp_get();
	if (!bsp)
	{
		bot_route_phase = _bot_route_phase_failed;
		return;
	}
	budget = 200;
	while (budget > 0 && bot_route_ladder_index < bsp->surfaces.count)
	{
		real_point3d stand;
		real top;
		real yaw;
		short base;
		short up;

		budget--;
		if (bot_ladder_base(bsp, bot_route_ladder_index, &stand, &top, &yaw))
		{
			base = bot_route_add(&stand, 6.0f);
			up = bot_route_ladder_top(&stand, top, yaw);
			if (base != NONE && up != NONE)
				bot_route_push(base, up, _bot_link_ladder, yaw, NULL);
		}
		bot_route_ladder_index++;
	}
	if (bot_route_ladder_index >= bsp->surfaces.count)
	{
		bot_route_phase = _bot_route_phase_flood;
		bot_route_flood_i = 0;
		bot_route_flood_dir = 0;
		bot_route_flood_wave = 0;
		bot_route_flood_limit = bot_route_node_count;
	}
}

static void bot_route_flood_tick(void)
{
	long budget;

	budget = 80;
	while (budget > 0 && bot_route_phase == _bot_route_phase_flood)
	{
		real_point3d probe;
		real_point3d ground;
		long surface;
		real yaw;
		real dz;
		struct bot_route_node const *node;

		budget--;
		if (bot_route_flood_i >= bot_route_flood_limit ||
			bot_route_node_count >= BOT_ROUTE_NODES)
		{
			bot_route_flood_wave++;
			if (bot_route_flood_wave >= 4 ||
				bot_route_flood_limit == bot_route_node_count ||
				bot_route_node_count >= BOT_ROUTE_NODES)
			{
				bot_route_phase = _bot_route_phase_link;
				bot_route_pair_i = 0;
				bot_route_pair_j = 1;
				bot_route_pair_dir = 0;
				bot_route_pair_pass = 0;
				bot_route_split_min = BOT_ROUTE_NODES;
				return;
			}
			bot_route_flood_limit = bot_route_node_count;
			bot_route_flood_i = 0;
			bot_route_flood_dir = 0;
			continue;
		}
		node = &bot_route_nodes[bot_route_flood_i];
		yaw = (real)bot_route_flood_dir * (_pi * 0.25f);
		probe.x = node->point.x + cosine(yaw) * 8.0f;
		probe.y = node->point.y + sine(yaw) * 8.0f;
		probe.z = node->point.z;
		if (bot_ground_at(&probe, &ground, &surface))
		{
			dz = ground.z - node->point.z;
			if (dz < 0.0f)
				dz = -dz;
			/* the floor under a catwalk is a different place. A probe
			that falls onto it must not become a neighbor. */
			if (dz <= 1.3f)
				bot_route_add(&ground, 6.0f);
		}
		bot_route_flood_dir++;
		if (bot_route_flood_dir >= 8)
		{
			bot_route_flood_dir = 0;
			bot_route_flood_i++;
		}
	}
}

/* a sample on the chord. A meter of air under it is the corner cut off
a catwalk, even when the drop test still calls the hole shallow. */
static boolean bot_route_sample_safe(real_point3d const *feet, real yaw)
{
	real_point3d probe;
	real_point3d ground;
	long surface;
	real drop;

	if (bot_drop_ahead(feet, yaw))
		return bot_ramp_ahead(feet, yaw);
	probe.x = feet->x + cosine(yaw) * 1.05f;
	probe.y = feet->y + sine(yaw) * 1.05f;
	probe.z = feet->z;
	if (!bot_ground_at(&probe, &ground, &surface))
		return FALSE;
	drop = feet->z - ground.z;
	if (drop > 1.15f && !bot_ramp_ahead(feet, yaw))
		return FALSE;
	return TRUE;
}

static boolean bot_route_segment_safe(real_point3d const *from, real_point3d const *to)
{
	real yaw;
	real dx;
	real dy;
	real dist;
	real travel;

	dx = to->x - from->x;
	dy = to->y - from->y;
	dist = square_root(dx * dx + dy * dy);
	yaw = bot_yaw_between(from, to);
	if (!bot_route_sample_safe(from, yaw))
		return FALSE;
	for (travel = 0.5f; travel < dist; travel += 0.5f)
	{
		real_point3d mid;

		mid.x = from->x + cosine(yaw) * travel;
		mid.y = from->y + sine(yaw) * travel;
		mid.z = from->z + (to->z - from->z) * (travel / dist);
		if (!bot_route_sample_safe(&mid, yaw))
			return FALSE;
	}
	return bot_route_sample_safe(to, yaw);
}

static boolean bot_route_steps_safe(real_point3d const *from, struct path_result const *path)
{
	real_point3d prev;
	long index;

	if (path->step_count <= 0)
		return FALSE;
	prev = *from;
	for (index = 0; index < path->step_count; index++)
	{
		if (!bot_route_segment_safe(&prev, &path->steps[index].point))
			return FALSE;
		prev = path->steps[index].point;
	}
	return TRUE;
}

/* the obstacle search refuses a missing body. The nearest living player
is only that body. The walk itself still runs from node to node. */
static long bot_route_unit(real_point3d const *near)
{
	long index;
	long best;
	real best_dist;

	best = NONE;
	best_dist = 1.0e9f;
	for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
	{
		struct player_datum *player;
		struct unit_datum *unit;
		real_point3d origin;
		real dx;
		real dy;
		real dist;

		player = player_try_and_get(index);
		if (!player || player->unit_index == NONE)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		object_get_origin(player->unit_index, &origin);
		dx = origin.x - near->x;
		dy = origin.y - near->y;
		dist = dx * dx + dy * dy;
		if (dist < best_dist)
		{
			best_dist = dist;
			best = player->unit_index;
		}
	}
	return best;
}

/* 0 failed, 1 a finished walk, 2 a finished stretch that does not reach.
-1 means no living body yet, so the pair has to wait. */
static short bot_route_search(short from, short to, struct path_result *path)
{
	struct path_input input;
	real_point3d destination;
	real dx;
	real dy;
	real dist;
	real bound;
	long unit_index;

	unit_index = bot_route_unit(&bot_route_nodes[from].point);
	if (unit_index == NONE)
		return -1;
	dx = bot_route_nodes[to].point.x - bot_route_nodes[from].point.x;
	dy = bot_route_nodes[to].point.y - bot_route_nodes[from].point.y;
	dist = square_root(dx * dx + dy * dy);
	path_input_new(&input, 0.45f, FALSE, unit_index);
	path_input_set_start(&input, &bot_route_nodes[from].point, bot_route_nodes[from].surface);
	bound = dist + 8.0f;
	if (bound < 32.0f)
		bound = 32.0f;
	path_input_set_search_bounds(&input, bound);
	path_state_new(&input, &bot_path_memory, NULL);
	destination = bot_route_nodes[to].point;
	path_state_destination(&bot_path_memory, &destination, bot_route_nodes[to].surface, 0.8f);
	if (!path_state_find(&bot_path_memory) ||
		!path_state_build_path(&bot_path_memory, path) || !path->valid)
		return 0;
	if (!bot_route_steps_safe(&bot_route_nodes[from].point, path))
		return 0;
	if (path->steps_finish_path)
		return 1;
	return 2;
}

static void bot_route_split(short from, struct path_result const *path)
{
	short before;
	short mid;
	real dx;
	real dy;

	if (bot_route_pair_pass != 0 || path->step_count <= 0)
		return;
	if (bot_route_node_count >= BOT_ROUTE_NODES)
		return;
	before = bot_route_node_count;
	mid = bot_route_add(&path->steps[path->step_count - 1].point, 4.0f);
	if (mid == NONE || mid == from)
		return;
	dx = bot_route_nodes[mid].point.x - bot_route_nodes[from].point.x;
	dy = bot_route_nodes[mid].point.y - bot_route_nodes[from].point.y;
	if (dx * dx + dy * dy < 4.0f * 4.0f)
		return;
	if (mid >= before && mid < bot_route_split_min)
		bot_route_split_min = mid;
	bot_route_push(from, mid, _bot_link_walk, 0.0f, path);
}

/* the floor ends along this heading. approach is the last ground
before the gap, not the node in the middle of the room. */
static boolean bot_route_find_lip(real_point3d const *origin, real yaw, real_point3d *approach)
{
	real dist;
	real_point3d last;
	boolean moved;

	last = *origin;
	moved = FALSE;
	for (dist = 0.8f; dist <= 7.0f; dist += 0.5f)
	{
		real_point3d probe;
		real_point3d ground;
		long surface;
		real drop;

		probe.x = origin->x + cosine(yaw) * dist;
		probe.y = origin->y + sine(yaw) * dist;
		probe.z = origin->z;
		if (!bot_ground_at(&probe, &ground, &surface))
		{
			if (!moved)
				return FALSE;
			*approach = last;
			return TRUE;
		}
		drop = origin->z - ground.z;
		if (drop < 0.0f)
			drop = -drop;
		if (drop > 0.80f)
		{
			if (!moved)
				return FALSE;
			*approach = last;
			return TRUE;
		}
		last = ground;
		moved = TRUE;
	}
	return FALSE;
}

/* jumps are measured from the lip, including a diagonal. A test from
the middle of the room never sees the gap. */
static void bot_route_scan_jumps(short from)
{
	long unit;
	long heading;
	short added;

	unit = bot_route_unit(&bot_route_nodes[from].point);
	if (unit == NONE)
		return;
	added = 0;
	for (heading = 0; heading < 16 && added < 3; heading++)
	{
		real yaw;
		real_point3d approach;
		real_point3d landing;
		short dest;
		real dx;
		real dy;
		real dz;
		struct bot_route_link *link;

		if (bot_route_link_count >= BOT_ROUTE_LINKS)
			return;
		yaw = (real)heading * (_pi * 2.0f / 16.0f);
		if (!bot_route_find_lip(&bot_route_nodes[from].point, yaw, &approach))
			continue;
		if (!bot_jump_across(&approach, yaw, unit, NULL, &landing))
			continue;
		dx = landing.x - bot_route_nodes[from].point.x;
		dy = landing.y - bot_route_nodes[from].point.y;
		if (dx * dx + dy * dy < 1.6f * 1.6f)
			continue;
		dest = bot_route_add(&landing, 3.5f);
		if (dest == NONE || dest == from)
			continue;
		dz = landing.z - bot_route_nodes[dest].point.z;
		if (dz < 0.0f)
			dz = -dz;
		if (dz > 0.80f)
			continue;
		if (bot_route_has(from, dest, _bot_link_walk) ||
			bot_route_has(from, dest, _bot_link_jump))
			continue;
		if (!bot_route_push(from, dest, _bot_link_jump, yaw, NULL))
			return;
		link = &bot_route_links[bot_route_link_count - 1];
		link->steps[0] = approach;
		link->steps[1] = landing;
		link->step_count = 2;
		added++;
	}
}

static void bot_route_index_links(void)
{
	long index;

	for (index = 0; index < bot_route_node_count; index++)
		bot_route_head[index] = NONE;
	for (index = 0; index < bot_route_link_count; index++)
	{
		short from;

		from = bot_route_links[index].from;
		if (from < 0 || from >= bot_route_node_count)
			continue;
		bot_route_next[index] = bot_route_head[from];
		bot_route_head[from] = (short)index;
	}
}

static void bot_route_begin_jumps(void)
{
	if (bot_route_node_count < 2)
	{
		bot_route_index_links();
		bot_route_phase = _bot_route_phase_ready;
		return;
	}
	bot_route_flood_i = 0;
	bot_route_phase = _bot_route_phase_jumps;
}

static boolean bot_route_pair_done(void)
{
	if (bot_route_node_count < 2)
		return TRUE;
	if (bot_route_pair_i >= bot_route_node_count - 1)
		return TRUE;
	if (bot_route_pair_j >= bot_route_node_count &&
		bot_route_pair_i >= bot_route_node_count - 1)
		return TRUE;
	return FALSE;
}

static void bot_route_link_tick(void)
{
	long searches;

	if (bot_route_link_count >= BOT_ROUTE_LINKS - 80 || bot_route_node_count < 2)
	{
		bot_route_begin_jumps();
		return;
	}
	searches = 0;
	while (searches < 2 && bot_route_phase == _bot_route_phase_link)
	{
		struct path_result path;
		short from;
		short to;
		short result;
		real dx;
		real dy;
		real dist;

		if (bot_route_pair_done())
		{
			if (bot_route_pair_pass == 0 && bot_route_split_min < bot_route_node_count)
			{
				bot_route_pair_pass = 1;
				bot_route_pair_i = 0;
				bot_route_pair_j = bot_route_split_min;
				bot_route_pair_dir = 0;
				if (bot_route_pair_j <= bot_route_pair_i)
					bot_route_pair_j = (short)(bot_route_pair_i + 1);
				continue;
			}
			bot_route_begin_jumps();
			return;
		}
		if (bot_route_pair_j >= bot_route_node_count || bot_route_pair_j <= bot_route_pair_i)
		{
			bot_route_pair_i++;
			bot_route_pair_j = (short)(bot_route_pair_i + 1);
			bot_route_pair_dir = 0;
			if (bot_route_pair_pass == 1 && bot_route_pair_j < bot_route_split_min)
				bot_route_pair_j = bot_route_split_min;
			continue;
		}
		dx = bot_route_nodes[bot_route_pair_j].point.x - bot_route_nodes[bot_route_pair_i].point.x;
		dy = bot_route_nodes[bot_route_pair_j].point.y - bot_route_nodes[bot_route_pair_i].point.y;
		dist = square_root(dx * dx + dy * dy);
		if (dist < 0.8f || dist > 14.0f)
		{
			bot_route_pair_dir = 1;
			bot_route_pair_j++;
			bot_route_pair_dir = 0;
			if (bot_route_pair_pass == 1 && bot_route_pair_j < bot_route_split_min)
				bot_route_pair_j = bot_route_split_min;
			if (bot_route_pair_j >= bot_route_node_count)
			{
				bot_route_pair_i++;
				bot_route_pair_j = (short)(bot_route_pair_i + 1);
				if (bot_route_pair_pass == 1 && bot_route_pair_j < bot_route_split_min)
					bot_route_pair_j = bot_route_split_min;
			}
			continue;
		}
		if (bot_route_pair_dir == 0)
		{
			from = bot_route_pair_i;
			to = bot_route_pair_j;
		}
		else
		{
			from = bot_route_pair_j;
			to = bot_route_pair_i;
		}
		result = bot_route_search(from, to, &path);
		if (result < 0)
			return;
		searches++;
		if (result == 1)
			bot_route_push(from, to, _bot_link_walk, 0.0f, &path);
		else if (result == 2)
			bot_route_split(from, &path);
		if (bot_route_pair_dir == 0)
			bot_route_pair_dir = 1;
		else
		{
			bot_route_pair_dir = 0;
			bot_route_pair_j++;
			if (bot_route_pair_pass == 1 && bot_route_pair_j < bot_route_split_min)
				bot_route_pair_j = bot_route_split_min;
			if (bot_route_pair_j >= bot_route_node_count)
			{
				bot_route_pair_i++;
				bot_route_pair_j = (short)(bot_route_pair_i + 1);
				if (bot_route_pair_pass == 1 && bot_route_pair_j < bot_route_split_min)
					bot_route_pair_j = bot_route_split_min;
			}
		}
	}
}

static void bot_route_jumps_tick(void)
{
	long budget;

	if (bot_route_link_count >= BOT_ROUTE_LINKS || bot_route_node_count < 2)
	{
		bot_route_index_links();
		bot_route_phase = _bot_route_phase_ready;
		return;
	}
	budget = 2;
	while (budget > 0 && bot_route_flood_i < bot_route_node_count)
	{
		if (bot_route_unit(&bot_route_nodes[bot_route_flood_i].point) == NONE)
			return;
		bot_route_scan_jumps(bot_route_flood_i);
		bot_route_flood_i++;
		budget--;
		if (bot_route_link_count >= BOT_ROUTE_LINKS)
			break;
	}
	if (bot_route_flood_i >= bot_route_node_count ||
		bot_route_link_count >= BOT_ROUTE_LINKS)
	{
		bot_route_index_links();
		bot_route_phase = _bot_route_phase_ready;
	}
}

static void bot_routes_tick(void)
{
	void const *map;

	map = global_collision_bsp_get();
	if (!map)
		return;
	if (map != bot_route_map)
		bot_route_reset(map);
	if (bot_route_phase == _bot_route_phase_ready ||
		bot_route_phase == _bot_route_phase_failed)
		return;
	bot_route_build_ticks++;
	if (bot_route_build_ticks > 1100)
	{
		bot_route_index_links();
		bot_route_phase = _bot_route_phase_ready;
		return;
	}
	if (bot_route_build_ticks > 900 && bot_route_phase != _bot_route_phase_jumps)
	{
		bot_route_begin_jumps();
		return;
	}
	if (bot_route_phase == _bot_route_phase_seed)
		bot_route_seed();
	else if (bot_route_phase == _bot_route_phase_ladder)
		bot_route_ladders_tick();
	else if (bot_route_phase == _bot_route_phase_flood)
		bot_route_flood_tick();
	else if (bot_route_phase == _bot_route_phase_link)
		bot_route_link_tick();
	else if (bot_route_phase == _bot_route_phase_jumps)
		bot_route_jumps_tick();
}

static short bot_route_nearest(real_point3d const *point, real max_horiz, long unit_index)
{
	long index;
	short best;
	real best_score;

	best = NONE;
	best_score = 1.0e9f;
	for (index = 0; index < bot_route_node_count; index++)
	{
		real dx;
		real dy;
		real dz;
		real horiz;
		real score;

		dx = point->x - bot_route_nodes[index].point.x;
		dy = point->y - bot_route_nodes[index].point.y;
		dz = point->z - bot_route_nodes[index].point.z;
		horiz = square_root(dx * dx + dy * dy);
		if (horiz > max_horiz)
			continue;
		if (dz < 0.0f)
			dz = -dz;
		if (dz > 3.5f)
			continue;
		score = horiz;
		if (dz > 1.6f)
			score += 24.0f;
		if (unit_index != NONE && horiz > 0.8f &&
			bot_solid_before_point(point, &bot_route_nodes[index].point, horiz, unit_index))
			score += 10.0f;
		if (score < best_score)
		{
			best_score = score;
			best = (short)index;
		}
	}
	return best;
}

/* a walk costs its length. A pad is not one hop: the walk to it is
already in the path, and the pad itself has to save a long trip. */
static real bot_route_link_cost(short link)
{
	struct bot_route_link const *item;
	real_point3d const *from;
	real_point3d const *to;
	real dx;
	real dy;
	real dz;
	real dist;

	item = &bot_route_links[link];
	if (item->from < 0 || item->to < 0 ||
		item->from >= bot_route_node_count || item->to >= bot_route_node_count)
		return 1.0e6f;
	from = &bot_route_nodes[item->from].point;
	to = &bot_route_nodes[item->to].point;
	dx = from->x - to->x;
	dy = from->y - to->y;
	dz = from->z - to->z;
	if (dz < 0.0f)
		dz = -dz;
	dist = square_root(dx * dx + dy * dy);
	if (item->kind == _bot_link_pad)
		return dist + 36.0f;
	if (item->kind == _bot_link_ladder)
		return 12.0f + dz * 2.0f;
	if (item->kind == _bot_link_jump)
		return dist + 6.0f;
	if (dist < 1.0f)
		dist = 1.0f;
	return dist;
}

/* the first link from the bot toward far. link NONE means they are already
in the room: walk straight to the place. */
static boolean bot_route_first(struct bot_state *state, real_point3d const *feet,
	real_point3d const *far, long unit_index, short avoid, short *link_out,
	real_point3d *hop, short *kind_out)
{
	short start;
	short goal;
	short came[BOT_ROUTE_NODES];
	short came_link[BOT_ROUTE_NODES];
	real cost[BOT_ROUTE_NODES];
	boolean settled[BOT_ROUTE_NODES];
	long index;

	if (bot_route_phase != _bot_route_phase_ready)
		return FALSE;
	start = bot_route_nearest(feet, 12.0f, unit_index);
	goal = bot_route_nearest(far, 18.0f, NONE);
	if (state && state->job_hop >= 0 && state->job_hop < bot_route_link_count && avoid == NONE)
	{
		short from;
		short to;
		real dx;
		real dy;
		real dz;
		real from_dist;
		real to_dist;

		from = bot_route_links[state->job_hop].from;
		to = bot_route_links[state->job_hop].to;
		if (from >= 0 && from < bot_route_node_count)
		{
			dx = bot_route_nodes[from].point.x - feet->x;
			dy = bot_route_nodes[from].point.y - feet->y;
			dz = bot_route_nodes[from].point.z - feet->z;
			if (dz < 0.0f)
				dz = -dz;
			from_dist = dx * dx + dy * dy;
			to_dist = 1.0e9f;
			if (to >= 0 && to < bot_route_node_count)
			{
				dx = bot_route_nodes[to].point.x - feet->x;
				dy = bot_route_nodes[to].point.y - feet->y;
				to_dist = dx * dx + dy * dy;
			}
			/* still on the near side. Past the far node, the next
			search starts where they landed, so a jump is not taken back. */
			if (from_dist < 5.0f * 5.0f && dz < 1.6f && from_dist <= to_dist + 1.0f)
				start = from;
		}
	}
	if (start == NONE || goal == NONE)
		return FALSE;
	if (start == goal)
	{
		*link_out = NONE;
		*hop = *far;
		*kind_out = _bot_link_walk;
		return TRUE;
	}
	for (index = 0; index < bot_route_node_count; index++)
	{
		came[index] = NONE;
		came_link[index] = NONE;
		cost[index] = 1.0e9f;
		settled[index] = FALSE;
	}
	came[start] = start;
	cost[start] = 0.0f;
	while (TRUE)
	{
		short best;
		short link;
		real best_cost;

		best = NONE;
		best_cost = 1.0e9f;
		for (index = 0; index < bot_route_node_count; index++)
		{
			if (settled[index] || cost[index] >= best_cost)
				continue;
			best_cost = cost[index];
			best = (short)index;
		}
		if (best == NONE || best == goal)
			break;
		settled[best] = TRUE;
		for (link = bot_route_head[best]; link != NONE; link = bot_route_next[link])
		{
			short next;
			real alt;

			if (link == avoid)
				continue;
			next = bot_route_links[link].to;
			if (next < 0 || next >= bot_route_node_count || settled[next])
				continue;
			alt = cost[best] + bot_route_link_cost(link);
			if (alt >= cost[next])
				continue;
			cost[next] = alt;
			came[next] = best;
			came_link[next] = link;
		}
	}
	if (came[goal] == NONE)
		return FALSE;
	{
		short cursor;
		short first;

		cursor = goal;
		first = NONE;
		index = 0;
		while (cursor != start && index < BOT_ROUTE_NODES)
		{
			first = came_link[cursor];
			if (first == NONE)
				return FALSE;
			cursor = bot_route_links[first].from;
			index++;
		}
		if (cursor != start || first == NONE)
			return FALSE;
		*link_out = first;
		if (bot_route_links[first].kind == _bot_link_jump &&
			bot_route_links[first].step_count > 0)
			*hop = bot_route_links[first].steps[0];
		else if (bot_route_links[first].kind == _bot_link_walk ||
			bot_route_links[first].kind == _bot_link_jump)
			*hop = bot_route_nodes[bot_route_links[first].to].point;
		else
			*hop = bot_route_nodes[bot_route_links[first].from].point;
		*kind_out = bot_route_links[first].kind;
		return TRUE;
	}
}

static boolean bot_route_install(struct bot_state *state, real_point3d const *goal)
{
	struct bot_route_link const *link;
	real dx;
	real dy;
	long index;

	if (bot_route_phase != _bot_route_phase_ready)
		return FALSE;
	if (state->job_hop < 0 || state->job_hop >= bot_route_link_count)
		return FALSE;
	link = &bot_route_links[state->job_hop];
	if (link->kind == _bot_link_jump && link->step_count > 0)
	{
		/* one step, the lip. A search to the landing aims across the gap. */
		dx = goal->x - link->steps[0].x;
		dy = goal->y - link->steps[0].y;
		if (dx * dx + dy * dy > 3.0f * 3.0f)
			return FALSE;
		if (state->route_link == state->job_hop && state->route.valid &&
			state->route.step_count == 1)
			return TRUE;
		state->route.valid = TRUE;
		state->route.steps_finish_path = TRUE;
		state->route.step_count = 1;
		state->route.step_index = 0;
		state->route.steps[0].point = link->steps[0];
		state->route.steps[0].surface_index =
			link->from >= 0 && link->from < bot_route_node_count ?
			bot_route_nodes[link->from].surface : NONE;
		state->route_goal = *goal;
		state->route_ticks = 0;
		state->route_link = state->job_hop;
		state->route_is_floor = state->route_floor;
		return TRUE;
	}
	if (link->kind != _bot_link_walk || link->step_count <= 0)
		return FALSE;
	if (link->to < 0 || link->to >= bot_route_node_count)
		return FALSE;
	dx = goal->x - bot_route_nodes[link->to].point.x;
	dy = goal->y - bot_route_nodes[link->to].point.y;
	if (dx * dx + dy * dy > 3.0f * 3.0f)
		return FALSE;
	if (state->route_link == state->job_hop && state->route.valid &&
		state->route.step_count == link->step_count)
		return TRUE;
	state->route.valid = TRUE;
	state->route.steps_finish_path = TRUE;
	state->route.step_count = link->step_count;
	state->route.step_index = 0;
	for (index = 0; index < link->step_count; index++)
	{
		state->route.steps[index].point = link->steps[index];
		state->route.steps[index].surface_index = bot_route_nodes[link->to].surface;
	}
	state->route_goal = *goal;
	state->route_ticks = 0;
	state->route_link = state->job_hop;
	state->route_is_floor = state->route_floor;
	return TRUE;
}

static boolean bot_route_jump_yaw(struct bot_state *state, real_point3d const *feet,
	real_point3d const *goal, real *yaw)
{
	struct bot_route_link const *link;
	real dx;
	real dy;
	real dist;
	real face;
	real delta;

	(void)goal;
	if (bot_route_phase != _bot_route_phase_ready)
		return FALSE;
	if (state->job_hop < 0 || state->job_hop >= bot_route_link_count)
		return FALSE;
	link = &bot_route_links[state->job_hop];
	if (link->kind != _bot_link_jump || link->step_count < 1)
		return FALSE;
	dx = link->steps[0].x - feet->x;
	dy = link->steps[0].y - feet->y;
	dist = square_root(dx * dx + dy * dy);
	/* still walking to the lip. Facing the jump from across the room
	is a run off the side of the platform. */
	if (dist > 2.0f)
		return FALSE;
	if (dist > 0.15f)
	{
		face = bot_yaw_between(feet, &link->steps[0]);
		delta = bot_angle_delta(face, link->yaw);
		if (delta < 0.0f)
			delta = -delta;
		if (dist > 1.20f && delta > 1.00f)
			return FALSE;
	}
	*yaw = link->yaw;
	state->route.valid = FALSE;
	if (!state->gap_hold && state->gap_ticks <= 0)
	{
		state->gap_ticks = 40;
		state->gap_yaw = link->yaw;
	}
	else if (!state->gap_hold)
		state->gap_yaw = link->yaw;
	return TRUE;
}

static boolean bot_hop_owned(struct bot_state const *state)
{
	if (bot_route_phase != _bot_route_phase_ready)
		return FALSE;
	return state->job_hop >= 0 && state->job_hop < bot_route_link_count;
}

static void bot_tour_remember(struct bot_state *state, real_point3d const *point)
{
	if (state->tour_been_next < 0 || state->tour_been_next >= 12)
		state->tour_been_next = 0;
	state->tour_been[state->tour_been_next] = *point;
	state->tour_been_next++;
	if (state->tour_been_next >= 12)
		state->tour_been_next = 0;
	if (state->tour_been_count < 12)
		state->tour_been_count++;
}

/* farther from the rooms already walked. Height is a nudge, not the
whole decision, so one catwalk does not become the only trip. */
static real bot_tour_score(struct bot_state *state, real_point3d const *feet,
	real_point3d const *point, real min_dist)
{
	real dx;
	real dy;
	real dist;
	real rise;
	real nearest;
	real score;
	long index;
	long remembered;

	dx = point->x - feet->x;
	dy = point->y - feet->y;
	dist = square_root(dx * dx + dy * dy);
	if (dist < min_dist)
		return -1.0e9f;
	rise = point->z - feet->z;
	if (rise < -1.6f)
	{
		real_point3d ground;
		long surface;

		if (!bot_ground_at(point, &ground, &surface))
			return -1.0e9f;
	}
	if (bot_point_banned(state, point))
		return -1.0e9f;
	nearest = dist;
	remembered = state->tour_been_count;
	if (remembered > 12)
		remembered = 12;
	for (index = 0; index < remembered; index++)
	{
		real px = point->x - state->tour_been[index].x;
		real py = point->y - state->tour_been[index].y;
		real pd = square_root(px * px + py * py);

		if (pd < nearest)
			nearest = pd;
	}
	if (nearest > 48.0f)
		nearest = 48.0f;
	/* length counts, so a short unvisited corner does not win over the
	rest of the map. */
	score = nearest + dist * 0.45f;
	if (rise > 1.2f)
		score += 8.0f;
	if (state->floor_trips >= 2 && (rise > 1.6f || rise < -1.6f))
		score += 12.0f;
	return score;
}

/* a spawn, a flag, a ladder, or a room the map walk already found.
item pads are a fetch, not a place to tour. */
static boolean bot_choose_tour(struct bot_state *state, real_point3d const *feet, long unit_index)
{
	struct scenario *scenario;
	struct collision_bsp *bsp;
	real_point3d points[32];
	long count;
	long index;
	real best_score;
	real_point3d chosen;
	boolean found;

	(void)unit_index;
	scenario = global_scenario_get();
	if (!scenario)
		return FALSE;
	count = 0;
	for (index = 0; index < scenario->players.count && count < 32; index++)
	{
		struct player_starting_location *spawn;

		spawn = TAG_BLOCK_GET_ELEMENT(&scenario->players, index, struct player_starting_location);
		if (spawn)
			points[count++] = spawn->position;
	}
	for (index = 0; index < scenario->netgame_flags.count && count < 32; index++)
	{
		struct scenario_netgame_flag *flag;

		flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);
		if (flag)
			points[count++] = flag->position;
	}
	best_score = -1.0e9f;
	found = FALSE;
	chosen = *feet;
	for (index = 0; index < count; index++)
	{
		real score = bot_tour_score(state, feet, &points[index], 14.0f);

		if (score > best_score)
		{
			best_score = score;
			chosen = points[index];
			found = TRUE;
		}
	}
	bsp = global_collision_bsp_get();
	if (bsp)
	{
		for (index = 0; index < bsp->surfaces.count; index++)
		{
			real_point3d stand;
			real top;
			real face;
			real rise;
			real score;

			if (!bot_ladder_base(bsp, index, &stand, &top, &face))
				continue;
			(void)face;
			rise = top - feet->z;
			if (rise < 1.6f)
				continue;
			/* the stand is the bottom. From a platform, that goal is the walk down. */
			if (stand.z < feet->z - 1.6f)
				continue;
			score = bot_tour_score(state, feet, &stand, 4.0f);
			if (score > best_score)
			{
				best_score = score;
				chosen = stand;
				found = TRUE;
			}
		}
	}
	for (index = 0; index < bot_route_node_count; index++)
	{
		real score = bot_tour_score(state, feet, &bot_route_nodes[index].point, 14.0f);

		if (score > best_score)
		{
			best_score = score;
			chosen = bot_route_nodes[index].point;
			found = TRUE;
		}
	}
	if (!found)
		return FALSE;
	bot_tour_remember(state, &chosen);
	state->roam_goal = chosen;
	state->roam_valid = TRUE;
	state->route.valid = FALSE;
	state->wander_yaw = bot_yaw_between(feet, &state->roam_goal);
	state->look_ticks = 30 * TICKS_PER_SECOND;
	return TRUE;
}

/* walk into a ladder, or keep holding forward once physics has grabbed
it. The pathfinder never treats a ladder as a floor change, and a chest
ray treats the face as a wall, so this replaces that step. */
static void bot_use_ladder(struct bot_state *state, struct player_datum *player,
	long unit_index, real_point3d const *feet, long engage, struct player_action *action)
{
	real yaw;
	boolean on;
	real rel;
	real dx;
	real dy;
	real_point3d eye;
	real_point3d look;
	real_vector3d aim;

	if (engage != NONE)
		return;
	on = bot_support_ladder_yaw(unit_index, &yaw);
	if (!on)
	{
		real near_yaw;
		boolean near;

		if (state->job != _bot_job_ladder)
			return;
		dx = state->job_goal.x - feet->x;
		dy = state->job_goal.y - feet->y;
		near = bot_ladder_near(feet, &near_yaw);
		if (!state->job_climb && (dx * dx + dy * dy > 3.2f * 3.2f || !near))
			return;
		yaw = near ? near_yaw : state->job_ladder_yaw;
	}
	rel = bot_angle_delta(state->facing_yaw, yaw);
	state->facing_yaw = bot_slew(state->facing_yaw, yaw, on ? 0.50f : 0.32f);
	eye = *feet;
	eye.z += 0.62f;
	look.x = eye.x + cosine(state->facing_yaw) * 4.0f;
	look.y = eye.y + sine(state->facing_yaw) * 4.0f;
	look.z = eye.z + (on ? 1.1f : 0.35f);
	bot_face(player, action, &eye, &look, 0.0f, 0.0f, &aim);
	state->facing_pitch = action->desired_facing.pitch;
	action->throttle.j = 0.0f;
	if (on || (rel <= 0.45f && rel >= -0.45f))
		action->throttle.i = 1.0f;
	else
		action->throttle.i = 0.0f;
}

/* one bot's item to fetch this tick. Weapons and powerups share the map's
spawn clock, and two bots are not sent to the same spot. */
struct bot_item_goal
{
	boolean valid;
	boolean present;
	/* ticks until this spot comes back. 0 when the item is on the ground. */
	long wait;
	real_point3d point;
};

static struct bot_item_goal bot_item_goals[MAXIMUM_NUMBER_OF_PLAYERS];

static void bot_job_clear(struct bot_state *state)
{
	state->job = _bot_job_none;
	state->job_climb = FALSE;
	state->job_blocked = FALSE;
	state->job_floor = FALSE;
	state->job_age = 0;
	state->job_still = 0;
	state->job_path_fail = 0;
	state->job_grab = 0;
	state->job_far_valid = FALSE;
	state->job_hop = NONE;
	state->job_avoid = NONE;
	state->job_avoid_ticks = 0;
	state->route_link = NONE;
	state->route.valid = FALSE;
}

static void bot_job_begin(struct bot_state *state, short kind, real_point3d const *goal,
	long surface, boolean floor)
{
	state->job = kind;
	state->job_goal = *goal;
	state->job_surface = surface;
	state->job_floor = floor;
	state->job_climb = FALSE;
	state->job_blocked = FALSE;
	state->job_age = 0;
	state->job_still = 0;
	state->job_path_fail = 0;
	state->job_grab = 0;
	state->job_anchor = *goal;
	state->job_far = *goal;
	state->job_far_valid = TRUE;
	state->job_far_floor = floor;
	state->job_far_surface = surface;
	state->job_hop = NONE;
	state->route_link = NONE;
	state->route.valid = FALSE;
	state->roam_goal = *goal;
	state->roam_valid = TRUE;
	state->look_ticks = 30 * TICKS_PER_SECOND;
}

static void bot_job_fail(struct bot_state *state, real_point3d const *point)
{
	if (state->job_far_valid)
		bot_ban_point(state, &state->job_far, 20 * TICKS_PER_SECOND);
	else
	{
		bot_ban_point(state, point, 20 * TICKS_PER_SECOND);
		if (state->job == _bot_job_ladder)
			bot_ban_point(state, &state->job_resume, 20 * TICKS_PER_SECOND);
	}
	bot_job_clear(state);
}

/* a climbable face on this floor that still goes up. The base is the walk. */
static boolean bot_job_find_ladder(struct bot_state const *state, real_point3d const *feet,
	real_point3d *stand, real *yaw)
{
	struct collision_bsp *bsp;
	long index;
	real best;
	boolean found;

	bsp = global_collision_bsp_get();
	if (!bsp)
		return FALSE;
	best = 1.0e9f;
	found = FALSE;
	for (index = 0; index < bsp->surfaces.count; index++)
	{
		real_point3d base;
		real top;
		real face;
		real dx;
		real dy;
		real dist;

		if (!bot_ladder_base(bsp, index, &base, &top, &face))
			continue;
		if (top < feet->z + 1.4f)
			continue;
		if (base.z < feet->z - 2.0f || base.z > feet->z + 1.6f)
			continue;
		if (bot_point_banned(state, &base))
			continue;
		dx = base.x - feet->x;
		dy = base.y - feet->y;
		dist = dx * dx + dy * dy;
		if (dist < best)
		{
			best = dist;
			*stand = base;
			*yaw = face;
			found = TRUE;
		}
	}
	return found;
}

static boolean bot_job_climb_toward(struct bot_state *state, real_point3d const *feet)
{
	real_point3d stand;
	real yaw;
	real_point3d resume;
	long surface;
	boolean floor;

	if (state->job_far_valid)
	{
		resume = state->job_far;
		surface = state->job_far_surface;
		floor = state->job_far_floor;
	}
	else
	{
		resume = state->job_goal;
		surface = state->job_surface;
		floor = state->job_floor;
	}
	if (!floor && !bot_different_floor(feet, &resume))
		return FALSE;
	if (!bot_job_find_ladder(state, feet, &stand, &yaw))
		return FALSE;
	bot_job_begin(state, _bot_job_ladder, &stand, NONE, FALSE);
	state->job_far = resume;
	state->job_far_valid = TRUE;
	state->job_far_floor = floor;
	state->job_far_surface = surface;
	state->job_resume = resume;
	state->job_resume_surface = surface;
	state->job_resume_floor = floor;
	state->job_ladder_yaw = yaw;
	state->job_hop = NONE;
	state->job_anchor = *feet;
	return TRUE;
}

/* point the current trip at the next link. The far place stays put. */
static boolean bot_route_apply(struct bot_state *state, real_point3d const *feet,
	long unit_index, short avoid)
{
	short link;
	short kind;
	real_point3d hop;
	real dx;
	real dy;

	if (bot_route_phase != _bot_route_phase_ready || !state->job_far_valid)
		return FALSE;
	if (!bot_route_first(state, feet, &state->job_far, unit_index, avoid, &link, &hop, &kind))
		return FALSE;
	if (avoid != NONE && (link == avoid || link == NONE))
		return FALSE;
	if (link == NONE)
	{
		dx = state->job_goal.x - state->job_far.x;
		dy = state->job_goal.y - state->job_far.y;
		if (state->job_hop != NONE || dx * dx + dy * dy > 0.25f)
			state->route.valid = FALSE;
		state->job_hop = NONE;
		state->job_goal = state->job_far;
		state->job_floor = state->job_far_floor;
		state->job_surface = state->job_far_surface;
		return TRUE;
	}
	/* a walk link is the next node on a short chain. Following that chain
	is the same hall forever. The far room is the trip, and the ground
	path is what goes through the doors between here and there. */
	if (kind == _bot_link_walk)
	{
		if (state->job_hop != NONE || state->route_link != NONE)
			state->route.valid = FALSE;
		state->job_hop = NONE;
		state->route_link = NONE;
		state->job_goal = state->job_far;
		state->job_floor = state->job_far_floor;
		state->job_surface = state->job_far_surface;
		return TRUE;
	}
	if (state->job_hop == link && kind == _bot_link_ladder && state->job == _bot_job_ladder)
		return TRUE;
	if (state->job_hop == link && kind != _bot_link_ladder && state->job != _bot_job_ladder)
		return TRUE;
	if (kind == _bot_link_ladder)
	{
		real_point3d far;
		boolean far_floor;
		long far_surface;
		real yaw;

		far = state->job_far;
		far_floor = state->job_far_floor;
		far_surface = state->job_far_surface;
		yaw = bot_route_links[link].yaw;
		bot_job_begin(state, _bot_job_ladder, &hop, NONE, FALSE);
		state->job_far = far;
		state->job_far_valid = TRUE;
		state->job_far_floor = far_floor;
		state->job_far_surface = far_surface;
		state->job_resume = far;
		state->job_resume_floor = far_floor;
		state->job_resume_surface = far_surface;
		state->job_ladder_yaw = yaw;
		state->job_hop = link;
		state->job_anchor = *feet;
		return TRUE;
	}
	if (state->job == _bot_job_ladder)
	{
		real_point3d far;
		boolean far_floor;
		long far_surface;

		far = state->job_far;
		far_floor = state->job_far_floor;
		far_surface = state->job_far_surface;
		bot_job_begin(state, _bot_job_travel, &hop, NONE, FALSE);
		state->job_far = far;
		state->job_far_valid = TRUE;
		state->job_far_floor = far_floor;
		state->job_far_surface = far_surface;
		state->job_anchor = *feet;
	}
	state->job_hop = link;
	state->job_goal = hop;
	state->job_floor = FALSE;
	state->job_surface = NONE;
	state->route.valid = FALSE;
	state->route_link = NONE;
	return TRUE;
}

static void bot_job_aim(struct bot_state *state, real_point3d const *feet, long unit_index)
{
	real dx;
	real dy;

	if (state->job == _bot_job_none || !state->job_far_valid)
		return;
	if (state->job_grab > 0 || state->gap_hold || state->job_climb)
		return;
	if (state->job == _bot_job_ladder)
	{
		dx = state->job_goal.x - feet->x;
		dy = state->job_goal.y - feet->y;
		if (dx * dx + dy * dy < 3.2f * 3.2f)
			return;
	}
	if (state->job == _bot_job_fetch && !bot_different_floor(feet, &state->job_far))
	{
		dx = state->job_far.x - feet->x;
		dy = state->job_far.y - feet->y;
		if (dx * dx + dy * dy < 8.0f * 8.0f)
		{
			if (state->job_hop != NONE)
			{
				state->job_goal = state->job_far;
				state->job_hop = NONE;
				state->job_floor = FALSE;
				state->job_surface = NONE;
				state->route.valid = FALSE;
			}
			return;
		}
	}
	bot_route_apply(state, feet, unit_index,
		state->job_avoid_ticks > 0 ? state->job_avoid : (short)NONE);
}

/* hold the current trip, or start one. Called only while nobody on this
floor is in sight, so a fight never has to share the body with it. */
static void bot_job_maintain(struct bot_state *state, real_point3d const *feet,
	long unit_index, long player_index, boolean hunting, real_point3d const *hunt)
{
	boolean pick;
	real dx;
	real dy;

	if (state->job_avoid_ticks > 0)
		state->job_avoid_ticks--;
	pick = state->job == _bot_job_none;
	if (!pick && !state->gap_hold && (state->job_blocked || state->job_path_fail > 20))
	{
		short avoid;

		state->job_blocked = FALSE;
		state->job_path_fail = 0;
		avoid = state->job_hop;
		if (avoid != NONE)
		{
			state->job_avoid = avoid;
			state->job_avoid_ticks = 3 * TICKS_PER_SECOND;
		}
		/* a different link is a way around. The same room is not. */
		if (bot_route_apply(state, feet, unit_index, avoid) &&
			(state->job_hop != NONE || avoid != NONE))
		{
			state->job_still = 0;
			state->job_anchor = *feet;
		}
		else if (!bot_job_climb_toward(state, feet))
		{
			bot_job_fail(state, &state->job_goal);
			pick = TRUE;
		}
	}
	if (!pick && state->job == _bot_job_fetch &&
		(player_index < 0 || player_index >= MAXIMUM_NUMBER_OF_PLAYERS ||
			!bot_item_goals[player_index].valid))
	{
		bot_job_clear(state);
		pick = TRUE;
	}
	if (!pick && state->job == _bot_job_assist && !state->assist_valid)
	{
		bot_job_clear(state);
		pick = TRUE;
	}
	if (!pick && state->job == _bot_job_assist && state->assist_valid)
	{
		state->job_far = state->assist_goal;
		state->job_far_valid = TRUE;
		state->job_far_floor = state->assist_floor;
		state->job_far_surface = state->assist_surface;
	}
	if (!pick && state->job == _bot_job_ladder && state->job_climb)
	{
		real yaw;
		boolean on;

		on = bot_support_ladder_yaw(unit_index, &yaw);
		if (on)
			state->job_path_fail = 0;
		else
		{
			state->job_path_fail++;
			if (feet->z > state->job_goal.z + 1.5f)
			{
				real_point3d resume = state->job_resume;
				long surface = state->job_resume_surface;
				boolean floor = state->job_resume_floor;

				bot_job_begin(state, _bot_job_travel, &resume, surface, floor);
				state->job_anchor = *feet;
				bot_job_aim(state, feet, unit_index);
				return;
			}
			if (state->job_path_fail > 3 * TICKS_PER_SECOND)
			{
				bot_job_fail(state, &state->job_goal);
				pick = TRUE;
			}
		}
	}
	if (!pick && state->job != _bot_job_none)
	{
		state->job_age++;
		if (state->job_age == 1)
			state->job_anchor = *feet;
		dx = feet->x - state->job_anchor.x;
		dy = feet->y - state->job_anchor.y;
		if (dx * dx + dy * dy > 0.45f * 0.45f)
		{
			state->job_anchor = *feet;
			state->job_still = 0;
		}
		else if (!state->job_climb && state->job_grab <= 0 && !state->move_thwarted)
		{
			state->job_still++;
			/* a turn finishes well inside this. Longer than that is a stand. */
			if (state->job_still > 2 * TICKS_PER_SECOND + (TICKS_PER_SECOND / 2))
			{
				bot_job_fail(state, &state->job_goal);
				pick = TRUE;
			}
		}
	}
	if (!pick && state->job == _bot_job_fetch &&
		player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
		bot_item_goals[player_index].valid)
	{
		real_point3d item = bot_item_goals[player_index].point;
		real ix = item.x - feet->x;
		real iy = item.y - feet->y;
		real item_dist = square_root(ix * ix + iy * iy);

		if (item_dist < 8.0f && bot_solid_before_point(feet, &item, item_dist, unit_index))
		{
			real hit = bot_first_block(feet, bot_yaw_between(feet, &item), 0.45f,
				item_dist, unit_index);

			if (hit > 0.6f)
			{
				real gx = state->job_goal.x - item.x;
				real gy = state->job_goal.y - item.y;

				if (gx * gx + gy * gy < 1.5f * 1.5f)
				{
					real yaw = bot_yaw_between(feet, &item);

					state->job_goal.x = feet->x + cosine(yaw) * (hit - 0.45f);
					state->job_goal.y = feet->y + sine(yaw) * (hit - 0.45f);
					state->job_goal.z = feet->z;
					state->job_floor = FALSE;
					state->route.valid = FALSE;
				}
			}
		}
	}
	if (!pick && state->job == _bot_job_ladder)
	{
		dx = state->job_goal.x - feet->x;
		dy = state->job_goal.y - feet->y;
		if (dx * dx + dy * dy < 2.2f * 2.2f && !bot_different_floor(feet, &state->job_goal))
			state->job_climb = TRUE;
	}
	else if (!pick && state->job != _bot_job_none && state->job != _bot_job_fetch)
	{
		real_point3d const *done_at;
		boolean done_floor;

		done_at = state->job_far_valid ? &state->job_far : &state->job_goal;
		done_floor = state->job_far_valid ? state->job_far_floor : state->job_floor;
		dx = done_at->x - feet->x;
		dy = done_at->y - feet->y;
		if (dx * dx + dy * dy < 2.2f * 2.2f && !bot_different_floor(feet, done_at))
		{
			if (state->job == _bot_job_travel)
			{
				/* arrived on another floor: learn that one. Same floor
				again, and the next trip is allowed to change floors. */
				if (done_floor)
					state->floor_trips = 0;
				else if (state->floor_trips < 6)
					state->floor_trips++;
				bot_ban_point(state, done_at, 20 * TICKS_PER_SECOND);
			}
			bot_job_clear(state);
			pick = TRUE;
		}
	}
	if (!pick)
	{
		bot_job_aim(state, feet, unit_index);
		return;
	}
	/* where they last saw someone, if that spot is a walk on this floor.
	otherwise the search clock is not a reason to stand. */
	if (state->search_ticks > 0 &&
		!bot_different_floor(feet, &state->search_position) &&
		!bot_point_banned(state, &state->search_position))
	{
		real yaw = bot_yaw_between(feet, &state->search_position);

		if (!bot_drop_ahead(feet, yaw))
		{
			bot_job_begin(state, _bot_job_travel, &state->search_position, NONE, FALSE);
			state->job_anchor = *feet;
			state->search_ticks = 0;
			bot_job_aim(state, feet, unit_index);
			return;
		}
	}
	if (player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
		bot_item_goals[player_index].valid &&
		!bot_point_banned(state, &bot_item_goals[player_index].point))
	{
		real_point3d goal = bot_item_goals[player_index].point;
		boolean floor = FALSE;
		long surface = NONE;

		if (bot_different_floor(feet, &goal))
		{
			real_point3d ground;

			if (bot_ground_at(&goal, &ground, &surface))
			{
				floor = TRUE;
				goal = ground;
			}
		}
		bot_job_begin(state, _bot_job_fetch, &goal, surface, floor);
		state->job_anchor = *feet;
		bot_job_aim(state, feet, unit_index);
		return;
	}
	if (hunting && hunt && !bot_point_banned(state, hunt))
	{
		boolean floor = bot_different_floor(feet, hunt);

		bot_job_begin(state, _bot_job_travel, hunt, floor ? state->route_surface : NONE, floor);
		state->job_anchor = *feet;
		bot_job_aim(state, feet, unit_index);
		return;
	}
	if (state->assist_valid && !bot_point_banned(state, &state->assist_goal))
	{
		bot_job_begin(state, _bot_job_assist, &state->assist_goal,
			state->assist_surface, state->assist_floor);
		state->job_anchor = *feet;
		bot_job_aim(state, feet, unit_index);
		return;
	}
	if (bot_choose_tour(state, feet, unit_index))
	{
		real_point3d goal = state->roam_goal;
		boolean floor = FALSE;
		long surface = NONE;

		if (bot_different_floor(feet, &goal))
		{
			real_point3d ground;

			if (bot_ground_at(&goal, &ground, &surface))
			{
				floor = TRUE;
				goal = ground;
			}
		}
		if (!bot_point_banned(state, &goal))
		{
			bot_job_begin(state, _bot_job_travel, &goal, surface, floor);
			state->job_anchor = *feet;
			bot_job_aim(state, feet, unit_index);
			return;
		}
	}
	bot_choose_roam(state, feet, unit_index);
	if (!bot_drop_ahead(feet, state->wander_yaw) &&
		!bot_point_banned(state, &state->roam_goal))
	{
		bot_job_begin(state, _bot_job_travel, &state->roam_goal, NONE, FALSE);
		state->job_anchor = *feet;
	}
	bot_job_aim(state, feet, unit_index);
}

/* a point on the ring around the target, at the gun's range, off to the
open side. The bot looks at the target and walks that ring instead of
standing on the body. */
static void bot_fight_goal(struct bot_state *state, real_point3d const *feet,
	real_point3d const *target, real near_range, real far_range, long unit_index,
	long skill, real_point3d *goal)
{
	real standoff;
	real bearing;
	real orbit;
	real left_clear;
	real right_clear;
	real side_yaw;
	real other;

	standoff = (near_range + far_range) * 0.5f;
	if (standoff < near_range + 1.0f)
		standoff = near_range + 1.0f;
	if (standoff > far_range)
		standoff = far_range;
	if (standoff < 2.5f)
		standoff = 2.5f;
	if (state->strafe == 0.0f || state->strafe_ticks <= 0)
	{
		left_clear = bot_open_distance(feet, state->facing_yaw + _pi * 0.5f, unit_index);
		right_clear = bot_open_distance(feet, state->facing_yaw - _pi * 0.5f, unit_index);
		if (left_clear > right_clear + 0.6f)
			state->strafe = -1.0f;
		else if (right_clear > left_clear + 0.6f)
			state->strafe = 1.0f;
		else if (state->strafe == 0.0f)
			state->strafe = ((bot_random() >> 8) & 1) ? 1.0f : -1.0f;
		else
			state->strafe = -state->strafe;
		/* a short step and back. A long hold walks them out of the shots.
		the hop is rolled once for that step, not on every tick of it. */
		state->strafe_ticks = 8 + (long)((bot_random() >> 8) % 5);
		state->strafe_jump = (long)((bot_random() >> 8) % 100) < bot_jump_chance[skill];
	}
	side_yaw = state->facing_yaw - state->strafe * (_pi * 0.5f);
	if (bot_open_distance(feet, side_yaw, unit_index) < 1.1f ||
		bot_heading_is_corner(feet, side_yaw, unit_index))
	{
		other = -state->strafe;
		side_yaw = state->facing_yaw - other * (_pi * 0.5f);
		if (bot_open_distance(feet, side_yaw, unit_index) >= 1.1f &&
			!bot_heading_is_corner(feet, side_yaw, unit_index))
		{
			state->strafe = other;
			state->strafe_ticks = 20;
			state->strafe_jump = (long)((bot_random() >> 8) % 100) < bot_jump_chance[skill];
		}
	}
	bearing = bot_yaw_between(target, feet);
	orbit = bearing - state->strafe * 1.15f;
	goal->x = target->x + cosine(orbit) * standoff;
	goal->y = target->y + sine(orbit) * standoff;
	goal->z = feet->z;
}

/* the closest living enemy, seen or not. With nobody to fight and no
pickup to fetch, every skill walks toward a shot. A teammate is not one. */
static boolean bot_nearest_enemy(long self_index, long team, boolean teams,
	real_point3d const *feet, real_point3d *enemy, long *unit_index)
{
	long index;
	real best;
	boolean found;

	best = 1.0e9f;
	found = FALSE;
	if (unit_index)
		*unit_index = NONE;
	for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
	{
		struct player_datum *other;
		struct unit_datum *unit;
		real_point3d point;
		real dx;
		real dy;
		real dist;

		if (index == self_index)
			continue;
		other = player_try_and_get(index);
		if (!other || other->unit_index == NONE)
			continue;
		if (teams && other->team_index == team)
			continue;
		unit = unit_try_and_get(other->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		object_get_origin(other->unit_index, &point);
		dx = point.x - feet->x;
		dy = point.y - feet->y;
		dist = dx * dx + dy * dy;
		if (dist < best)
		{
			best = dist;
			*enemy = point;
			found = TRUE;
			if (unit_index)
				*unit_index = other->unit_index;
		}
	}
	return found;
}

/* an enemy close enough to this body that the fight is here, not somewhere
the bot has only guessed. */
static boolean bot_fight_at(real_point3d const *where, long self_index, long team, boolean teams)
{
	long index;

	for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
	{
		struct player_datum *other;
		struct unit_datum *unit;
		real_point3d point;
		real dx;
		real dy;
		real dz;

		if (index == self_index)
			continue;
		other = player_try_and_get(index);
		if (!other || other->unit_index == NONE)
			continue;
		if (teams && other->team_index == team)
			continue;
		unit = unit_try_and_get(other->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		object_get_origin(other->unit_index, &point);
		dx = point.x - where->x;
		dy = point.y - where->y;
		dz = point.z - where->z;
		if (dz < 0.0f)
			dz = -dz;
		if (dx * dx + dy * dy < 18.0f * 18.0f && dz < 5.0f)
			return TRUE;
	}
	return FALSE;
}

/* a teammate who is in a fight, far enough away that walking there leaves
the room. The point is the teammate, which is where that fight is. */
static boolean bot_teammate_fight(long self_index, long team, boolean teams,
	real_point3d const *feet, real_point3d *goal, long *unit_index, boolean *other_floor, long *surface)
{
	long index;
	real best;
	boolean found;

	*surface = NONE;
	*other_floor = FALSE;
	*unit_index = NONE;
	if (!teams)
		return FALSE;
	best = 1.0e9f;
	found = FALSE;
	for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
	{
		struct player_datum *other;
		struct unit_datum *unit;
		real_point3d point;
		real dx;
		real dy;
		real dist;
		boolean engaged;

		if (index == self_index)
			continue;
		other = player_try_and_get(index);
		if (!other || other->unit_index == NONE || other->team_index != team)
			continue;
		unit = unit_try_and_get(other->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		object_get_origin(other->unit_index, &point);
		engaged = bot_states[index].started && bot_states[index].target_visible;
		if (!engaged)
			engaged = bot_fight_at(&point, self_index, team, TRUE);
		if (!engaged)
			continue;
		dx = point.x - feet->x;
		dy = point.y - feet->y;
		dist = dx * dx + dy * dy;
		/* the same closet is not a way out of it */
		if (dist < 8.0f * 8.0f && !bot_different_floor(feet, &point))
			continue;
		if (dist < best)
		{
			best = dist;
			*goal = point;
			*unit_index = other->unit_index;
			found = TRUE;
		}
	}
	if (!found)
		return FALSE;
	if (bot_different_floor(feet, goal))
	{
		real_point3d floor_point;
		long floor_surface;

		*other_floor = TRUE;
		floor_point = *goal;
		if (bot_enemy_floor(*unit_index, &floor_point, &floor_surface))
		{
			*goal = floor_point;
			*surface = floor_surface;
		}
	}
	return TRUE;
}

/* an enemy a teammate can see right now. The point is that body, and it
stops being known the moment no teammate still has the sight line. */
static boolean bot_called_enemy(long self_index, long team, boolean teams,
	real_point3d const *feet, long *enemy_index, real_point3d *where)
{
	long mate;
	real best;
	boolean found;

	*enemy_index = NONE;
	if (!teams)
		return FALSE;
	best = 1.0e9f;
	found = FALSE;
	for (mate = 0; mate < MAXIMUM_NUMBER_OF_PLAYERS; mate++)
	{
		struct player_datum *other;
		struct unit_datum *unit;
		real_point3d mate_eye;
		long foe;

		if (mate == self_index)
			continue;
		other = player_try_and_get(mate);
		if (!other || other->unit_index == NONE || other->team_index != team)
			continue;
		if (!network_player_is_valid(&other->network_player_data))
			continue;
		unit = unit_try_and_get(other->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		object_get_origin(other->unit_index, &mate_eye);
		mate_eye.z += 0.62f;
		for (foe = 0; foe < MAXIMUM_NUMBER_OF_PLAYERS; foe++)
		{
			struct player_datum *enemy;
			struct unit_datum *enemy_unit;
			real_point3d point;
			real_point3d eye;
			real dx;
			real dy;
			real dist;

			if (foe == self_index || foe == mate)
				continue;
			enemy = player_try_and_get(foe);
			if (!enemy || enemy->unit_index == NONE || enemy->team_index == team)
				continue;
			if (!network_player_is_valid(&enemy->network_player_data))
				continue;
			enemy_unit = unit_try_and_get(enemy->unit_index);
			if (!enemy_unit || TEST_FLAG(enemy_unit->object.damage_flags, _object_dead_bit))
				continue;
			object_get_origin(enemy->unit_index, &point);
			eye = point;
			eye.z += 0.62f;
			if (!bot_can_see(&mate_eye, &eye, other->unit_index))
				continue;
			dx = point.x - feet->x;
			dy = point.y - feet->y;
			dist = dx * dx + dy * dy;
			if (dist < best)
			{
				best = dist;
				*enemy_index = foe;
				*where = point;
				found = TRUE;
			}
		}
	}
	return found;
}

/* a place the bot can walk to and see the enemy from. The enemy's own
point is the last resort, and the bot faces the walk, not the wall. */
static void bot_los_goal(real_point3d const *feet, real_point3d const *enemy,
	long unit_index, real_point3d *goal)
{
	static real const angles[] = {
		0.0f, 0.8f, -0.8f, 1.6f, -1.6f, 2.4f, -2.4f, 3.14159274f
	};
	static real const radii[] = { 8.0f, 14.0f };
	real bearing;
	real best;
	long radius_index;
	long angle_index;
	real_point3d enemy_eye;

	bearing = bot_yaw_between(enemy, feet);
	enemy_eye = *enemy;
	enemy_eye.z += 0.62f;
	best = 1.0e9f;
	*goal = *enemy;
	for (radius_index = 0; radius_index < (long)NUMBEROF(radii); radius_index++)
	{
		for (angle_index = 0; angle_index < (long)NUMBEROF(angles); angle_index++)
		{
			real yaw;
			real_point3d point;
			real_point3d eye;
			real dx;
			real dy;
			real dist;

			yaw = bearing + angles[angle_index];
			point.x = enemy->x + cosine(yaw) * radii[radius_index];
			point.y = enemy->y + sine(yaw) * radii[radius_index];
			point.z = feet->z;
			eye = point;
			eye.z += 0.62f;
			if (!bot_can_see(&eye, &enemy_eye, unit_index))
				continue;
			if (bot_open_distance(&point, bot_yaw_between(&point, enemy), unit_index) < 1.2f)
				continue;
			dx = point.x - feet->x;
			dy = point.y - feet->y;
			dist = dx * dx + dy * dy;
			if (dist < 4.0f)
				continue;
			if (dist < best)
			{
				best = dist;
				*goal = point;
			}
		}
	}
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

/* the goal could not be walked. Leave it alone, or the next tick sends
the bot straight back. away is a marker inside a solid: a long block,
so the lap around that object does not resume as soon as it expires. */
static void bot_give_up(struct bot_state *state, long player_index, real_point3d const *feet,
	boolean away)
{
	if (player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
		bot_item_goals[player_index].valid)
	{
		state->avoid_point = bot_item_goals[player_index].point;
		state->avoid_ticks = 150;
		{
			long slot = 0;
			long index;

			for (index = 1; index < 4; index++)
			{
				if (state->given_up_ticks[index] < state->given_up_ticks[slot])
					slot = index;
			}
			state->given_up[slot] = bot_item_goals[player_index].point;
			/* long enough to cross the map. A short block and the next
			tick's pickup score pulls them back onto the same pad.
			a solid marker stays blocked for a whole respawn. */
			state->given_up_ticks[slot] = (away ? 45 : 20) * TICKS_PER_SECOND;
			if (bot_item_goals[player_index].wait > state->given_up_ticks[slot])
				state->given_up_ticks[slot] = away ?
					bot_item_goals[player_index].wait :
					bot_item_goals[player_index].wait - 4 * TICKS_PER_SECOND;
		}
		bot_item_goals[player_index].valid = FALSE;
	}
	state->roam_valid = FALSE;
	state->route.valid = FALSE;
	if (state->job == _bot_job_fetch)
		bot_job_clear(state);
	state->hunt_valid = FALSE;
	state->hunt_ticks = 0;
	state->search_ticks = 0;
	state->search_arrived = FALSE;
	state->step_hold = 0;
	state->commit_ticks = 0;
	state->look_ticks = 0;
	state->wander_yaw = state->facing_yaw;
	state->stuck_ticks = 0;
	state->idle_ticks = 0;
	state->wait_ticks = 0;
	state->fetch_fail = 0;
	state->fight_idle = 0;
}

static long bot_name_score(char const *name)
{
	long index;

	if (!name)
		return 0;
	if (strstr(name, "overshield") || strstr(name, "over shield"))
		return 16;
	if (strstr(name, "camouflage") || strstr(name, "camo") || strstr(name, "invisibility"))
		return 15;
	if (strstr(name, "health"))
		return 12;
	if (bot_power_gun(name) == 2)
		return 9;
	if (bot_power_gun(name) == 1)
		return 8;
	for (index = 0; index < (long)NUMBEROF(bot_weapon_kinds); index++)
	{
		if (strstr(name, bot_weapon_kinds[index].name))
			return bot_weapon_kinds[index].score;
	}
	return 3;
}

/* seconds on the placement, or on the collection, then the same period the
game uses to put the item back (game_engine_update_item_spawn) */
static long bot_item_period(struct scenario_netgame_equipment *equipment)
{
	struct item_collection_definition *collection;
	long period = 30 * TICKS_PER_SECOND;
	short spawn = equipment->spawn_time;

	if (equipment->item_collection.index == NONE)
		return 0;
	if (spawn == 0)
	{
		collection = (struct item_collection_definition *)tag_get(
			ITEM_COLLECTION_DEFINITION_TAG, equipment->item_collection.index);
		if (collection && collection->spawn_time != 0)
			period = (long)collection->spawn_time * TICKS_PER_SECOND;
	}
	else
		period = (long)spawn * TICKS_PER_SECOND;
	return hardcore_rules_item_period(period, spawn);
}

/* overshield, camouflage, and a health pack. The tag name is not reliable:
active camouflage does not contain "camo". */
static long bot_equipment_score(long definition_index, short *kind)
{
	struct equipment_definition const *definition;

	if (kind)
		*kind = 0;
	if (definition_index == NONE)
		return 0;
	definition = equipment_definition_get(definition_index);
	if (!definition)
		return 0;
	switch (definition->equipment.powerup_type)
	{
	case _equipment_powerup_overshield:
		if (kind)
			*kind = 1;
		return 16;
	case _equipment_powerup_active_camouflage:
		if (kind)
			*kind = 2;
		return 15;
	case _equipment_powerup_health:
		if (kind)
			*kind = 3;
		return 12;
	case _equipment_powerup_double_speed:
	case _equipment_powerup_full_spectrum_vision:
		if (kind)
			*kind = 4;
		return 8;
	default:
		return 0;
	}
}

static long bot_collection_score(long collection_index, boolean *is_weapon, short *powerup_kind)
{
	struct item_collection_definition *collection;
	long index;
	long best = 0;

	if (is_weapon)
		*is_weapon = FALSE;
	if (powerup_kind)
		*powerup_kind = 0;
	if (collection_index == NONE)
		return 0;
	collection = (struct item_collection_definition *)tag_get(
		ITEM_COLLECTION_DEFINITION_TAG, collection_index);
	if (!collection)
		return 0;
	for (index = 0; index < collection->permutations.count; index++)
	{
		struct item_permutation_definition *perm;
		char const *name;
		long score;
		short kind;

		perm = TAG_BLOCK_GET_ELEMENT(&collection->permutations, index,
			struct item_permutation_definition);
		if (!perm)
			continue;
		if (perm->item.group_tag == 'weap' && is_weapon)
			*is_weapon = TRUE;
		kind = 0;
		score = 0;
		if (perm->item.group_tag == EQUIPMENT_DEFINITION_TAG)
			score = bot_equipment_score(perm->item.index, &kind);
		name = perm->item.name;
		if (perm->item.index != NONE)
		{
			char const *tag_name = tag_get_name(perm->item.index);
			long tag_score = bot_name_score(tag_name);

			if (!name || tag_score > bot_name_score(name))
				name = tag_name;
		}
		if (bot_name_score(name) > score)
		{
			score = bot_name_score(name);
			if (score >= 16)
				kind = 1;
			else if (score >= 15)
				kind = 2;
			else if (score >= 12 && kind == 0)
				kind = 3;
		}
		if (score > best)
		{
			best = score;
			if (powerup_kind && kind != 0)
				*powerup_kind = kind;
		}
	}
	return best;
}

/* a sniper or a rocket in this collection, from the tag path */
static short bot_collection_power_gun(long collection_index)
{
	struct item_collection_definition *collection;
	long index;
	short gun;

	gun = 0;
	if (collection_index == NONE)
		return 0;
	collection = (struct item_collection_definition *)tag_get(
		ITEM_COLLECTION_DEFINITION_TAG, collection_index);
	if (!collection)
		return 0;
	for (index = 0; index < collection->permutations.count; index++)
	{
		struct item_permutation_definition *perm;
		long which;

		perm = TAG_BLOCK_GET_ELEMENT(&collection->permutations, index,
			struct item_permutation_definition);
		if (!perm)
			continue;
		which = bot_power_gun(perm->item.name);
		if (which == 0 && perm->item.index != NONE)
			which = bot_power_gun(tag_get_name(perm->item.index));
		if (which > gun)
			gun = (short)which;
	}
	return gun;
}

static boolean bot_has_power_gun(long unit_index, short which)
{
	short slot;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		struct bot_weapon_kind const *kind;
		long weapon_index;

		weapon_index = unit_inventory_get_weapon(unit_index, slot);
		if (weapon_index == NONE)
			continue;
		kind = bot_weapon_kind(weapon_index);
		if (kind && bot_power_gun(kind->name) == which)
			return TRUE;
	}
	return FALSE;
}

/* a powerup or a power gun already within arm's reach. The marker inside a
solid becomes the face in front of it. A drop, a ban, or one they already
hold is left on the path. */
static boolean bot_close_powerup(struct bot_state *state, struct player_datum *player,
	long unit_index, real_point3d const *feet, real_point3d *walk, real_point3d *item)
{
	struct unit_datum *unit;
	boolean has_os;
	boolean has_camo;
	long index;
	real best;
	boolean found;

	if (!object_header_data || !object_header_data->valid)
		return FALSE;
	unit = unit_try_and_get(unit_index);
	has_os = unit && (unit->object.shield_vitality > 1.05f ||
		TEST_FLAG(unit->object.damage_flags, _object_shield_over_charging_bit));
	has_camo = player->powerup_durations[_player_powerup_active_camouflage] > 0;
	best = 8.0f * 8.0f;
	found = FALSE;
	for (index = data_next_index(object_header_data, NONE);
		index != NONE;
		index = data_next_index(object_header_data, index))
	{
		struct object_datum *object = object_try_and_get(index);
		real dx;
		real dy;
		real dz;
		real dist2;
		real dist;
		real yaw;
		real_point3d point;
		real_point3d approach;
		short kind;

		if (!object || object->object.parent_object_index != NONE)
			continue;
		if (object->object.type != _object_type_equipment &&
			object->object.type != _object_type_weapon)
			continue;
		point = object->object.position;
		dx = point.x - feet->x;
		dy = point.y - feet->y;
		dz = point.z - feet->z;
		if (dz < 0.0f)
			dz = -dz;
		if (dz > 1.2f)
			continue;
		dist2 = dx * dx + dy * dy;
		if (dist2 > best)
			continue;
		kind = 0;
		if (object->object.type == _object_type_equipment)
		{
			bot_equipment_score(object->definition_index, &kind);
			if (kind == 0)
				continue;
			if (kind == 1 && has_os)
				continue;
			if (kind == 2 && has_camo)
				continue;
		}
		else
		{
			short gun = (short)bot_power_gun(tag_get_name(object->definition_index));

			if (gun == 0 || bot_has_power_gun(unit_index, gun))
				continue;
		}
		if (bot_point_banned(state, &point))
			continue;
		dist = square_root(dist2);
		yaw = bot_yaw_between(feet, &point);
		if (dist > 1.05f && bot_drop_ahead(feet, yaw) &&
			!bot_jump_across(feet, yaw, unit_index, NULL, NULL))
			continue;
		approach = point;
		approach.z = feet->z;
		if (dist > 0.45f && bot_solid_before_point(feet, &point, dist, unit_index))
		{
			real hit = bot_first_block(feet, yaw, 0.45f, dist, unit_index);

			if (hit < 0.35f)
				continue;
			approach.x = feet->x + cosine(yaw) * (hit - 0.35f);
			approach.y = feet->y + sine(yaw) * (hit - 0.35f);
		}
		best = dist2;
		*walk = approach;
		*item = point;
		found = TRUE;
	}
	return found;
}

/* the closest free bot takes each weapon or powerup that is on the ground
or about to come back. A live overshield, camouflage, or health pack is
worth leaving a long fight. A sniper or a rocket is too, until they are holding one. */
static void bot_assign_items(void)
{
	struct scenario *scenario;
	struct game_variant *variant;
	struct game_variant_options const *options;
	long engine;
	long now;
	long spot_count;
	long bot_count;
	long round;
	boolean no_weapons;
	struct
	{
		real_point3d point;
		long worth;
		long wait;
		boolean present;
		/* 1 overshield, 2 camouflage, 3 health, 4 another powerup */
		short kind;
		/* 1 sniper, 2 rocket. Either is a trip even with a pistol in hand. */
		short power_gun;
	} spots[48];
	struct
	{
		long player;
		real_point3d feet;
		long carried;
		boolean busy;
		boolean has_os;
		boolean has_camo;
		boolean hurt;
		long unit;
	} bots[MAXIMUM_NUMBER_OF_PLAYERS];
	boolean spot_taken[48];
	boolean bot_taken[MAXIMUM_NUMBER_OF_PLAYERS];

	csmemset(bot_item_goals, 0, sizeof(bot_item_goals));
	scenario = global_scenario_get();
	if (!scenario || !object_header_data || !object_header_data->valid)
		return;
	variant = game_engine_get_variant();
	engine = variant ? variant->game_engine_index : 0;
	options = game_variant_options_get();
	no_weapons = options && options->no_map_weapons;
	now = game_time_get();
	spot_count = 0;
	for (round = 0; round < scenario->netgame_equipment.count && spot_count < 48; round++)
	{
		struct scenario_netgame_equipment *equipment;
		boolean weapon = FALSE;
		short kind = 0;
		long worth;
		long period;
		long index;
		boolean present = FALSE;
		long live = 0;
		short gun = 0;

		equipment = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, round,
			struct scenario_netgame_equipment);
		if (!equipment || equipment->item_collection.index == NONE)
			continue;
		if (!match_game_type(engine, 4, equipment->game_type))
			continue;
		worth = bot_collection_score(equipment->item_collection.index, &weapon, &kind);
		gun = bot_collection_power_gun(equipment->item_collection.index);
		if (weapon && no_weapons)
			continue;
		period = bot_item_period(equipment);
		if (period <= 0)
			continue;
		for (index = data_next_index(object_header_data, NONE);
			index != NONE;
			index = data_next_index(object_header_data, index))
		{
			struct object_datum *object = object_try_and_get(index);
			real dx;
			real dy;
			long score;
			short live_kind;

			if (!object)
				continue;
			if (object->object.type != _object_type_weapon &&
				object->object.type != _object_type_equipment)
				continue;
			dx = object->object.position.x - equipment->position.x;
			dy = object->object.position.y - equipment->position.y;
			if (dx * dx + dy * dy > 6.0f * 6.0f ||
				object->object.position.z - equipment->position.z > 3.0f ||
				equipment->position.z - object->object.position.z > 3.0f)
				continue;
			present = TRUE;
			live_kind = 0;
			if (object->object.type == _object_type_equipment)
				score = bot_equipment_score(object->definition_index, &live_kind);
			else
				score = bot_name_score(tag_get_name(object->definition_index));
			if (score > live)
			{
				live = score;
				if (live_kind != 0)
					kind = live_kind;
			}
			if (object->object.type == _object_type_weapon)
			{
				short live_gun = (short)bot_power_gun(tag_get_name(object->definition_index));

				if (live_gun != 0)
					gun = live_gun;
			}
		}
		if (present && live > worth)
			worth = live;
		spots[spot_count].point = equipment->position;
		spots[spot_count].worth = worth;
		spots[spot_count].present = present;
		spots[spot_count].kind = kind;
		spots[spot_count].power_gun = gun;
		spots[spot_count].wait = present ? 0 : period - (now % period);
		if (spots[spot_count].wait == period)
			spots[spot_count].wait = 0;
		/* a spawn that is not back yet, and not due soon, is not a trip.
		a sniper, a rocket, or a powerup is worth the walk a bit earlier. */
		{
			long lead = (gun != 0 || kind != 0) ? 25 : 10;

			if (!present && spots[spot_count].wait > lead * TICKS_PER_SECOND)
				continue;
		}
		spot_count++;
	}

	bot_count = 0;
	for (round = 0; round < MAXIMUM_NUMBER_OF_PLAYERS; round++)
	{
		struct player_datum *player;
		struct unit_datum *unit;
		short slot;
		long carried;

		player = player_try_and_get(round);
		if (!player || player->unit_index == NONE)
			continue;
		if (!bot_controller_is_bot(player->network_player_data.machine_index,
				player->network_player_data.controller_index) &&
			ustrncmp(player->name, L"Bot", 3) != 0)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		carried = 0;
		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		{
			long score = bot_slot_score(player->unit_index, slot);

			if (bot_slot_usable(player->unit_index, slot) && score > carried)
				carried = score;
		}
		bots[bot_count].player = round;
		object_get_origin(player->unit_index, &bots[bot_count].feet);
		bots[bot_count].carried = carried;
		/* in a fight they can see. A player out of sight is not a place they know */
		bots[bot_count].busy = bot_states[round].target_visible;
		/* the charging bit clears once the shield hits full overcharge.
		the extra shield is what they are actually holding. */
		bots[bot_count].has_os = unit->object.shield_vitality > 1.05f ||
			TEST_FLAG(unit->object.damage_flags, _object_shield_over_charging_bit);
		bots[bot_count].has_camo = player->powerup_durations[_player_powerup_active_camouflage] > 0;
		bots[bot_count].hurt = unit->object.body_vitality < 0.99f;
		bots[bot_count].unit = player->unit_index;
		/* a trip already underway is not replaced by this tick's best pad. */
		bot_taken[bot_count] = bot_states[round].job != _bot_job_none;
		bot_count++;
	}
	for (round = 0; round < spot_count; round++)
		spot_taken[round] = FALSE;

	for (round = 0; round < bot_count && round < spot_count; round++)
	{
		long best_bot = NONE;
		long best_spot = NONE;
		real best_value = 12.0f;
		long bot_index;
		long spot_index;

		for (bot_index = 0; bot_index < bot_count; bot_index++)
		{
			if (bot_taken[bot_index])
				continue;
			for (spot_index = 0; spot_index < spot_count; spot_index++)
			{
				real dx;
				real dy;
				real dist;
				real value;
				real ax;
				real ay;
				long interest;

				if (spot_taken[spot_index])
					continue;
				if (bot_states[bots[bot_index].player].avoid_ticks > 0)
				{
					ax = spots[spot_index].point.x - bot_states[bots[bot_index].player].avoid_point.x;
					ay = spots[spot_index].point.y - bot_states[bots[bot_index].player].avoid_point.y;
					if (ax * ax + ay * ay < 9.0f)
						continue;
				}
				{
					long abandoned;
					boolean skipped = FALSE;

					for (abandoned = 0; abandoned < 4; abandoned++)
					{
						if (bot_states[bots[bot_index].player].given_up_ticks[abandoned] <= 0)
							continue;
						ax = spots[spot_index].point.x -
							bot_states[bots[bot_index].player].given_up[abandoned].x;
						ay = spots[spot_index].point.y -
							bot_states[bots[bot_index].player].given_up[abandoned].y;
						if (ax * ax + ay * ay < 9.0f)
							skipped = TRUE;
					}
					if (skipped)
						continue;
				}
				dx = spots[spot_index].point.x - bots[bot_index].feet.x;
				dy = spots[spot_index].point.y - bots[bot_index].feet.y;
				/* a story below is a drop off the platform. The tour
				stays up. The pickup is taken when they are on its floor. */
				if (spots[spot_index].point.z < bots[bot_index].feet.z - 2.0f)
					continue;
				/* a pistol can wait while they shoot. A sniper, a rocket,
				or a powerup is the trip, including across the map. */
				if (bots[bot_index].busy && spots[spot_index].kind == 0 &&
					spots[spot_index].power_gun == 0)
				{
					if (!spots[spot_index].present &&
						spots[spot_index].wait > 4 * TICKS_PER_SECOND)
						continue;
					if (dx * dx + dy * dy > 18.0f * 18.0f)
						continue;
				}
				interest = spots[spot_index].worth;
				/* already holding it. Another trip to that pad is the camp. */
				if (spots[spot_index].kind == 1 && bots[bot_index].has_os)
					continue;
				if (spots[spot_index].kind == 2 && bots[bot_index].has_camo)
					continue;
				if (spots[spot_index].kind == 3)
					interest = bots[bot_index].hurt ? 16 : 5;
				else if (spots[spot_index].power_gun != 0 &&
					bot_has_power_gun(bots[bot_index].unit, spots[spot_index].power_gun))
					interest = 3;
				else if (spots[spot_index].kind == 0 && spots[spot_index].power_gun == 0 &&
					interest < 12 && bots[bot_index].carried >= interest)
					interest -= 6;
				if (interest < 1)
					continue;
				dist = square_root(dx * dx + dy * dy);
				value = (real)interest * 6.0f - dist - (real)spots[spot_index].wait / (real)TICKS_PER_SECOND * 2.0f;
				if (spots[spot_index].present)
					value += 8.0f;
				if (spots[spot_index].kind >= 1 && spots[spot_index].kind <= 3 && interest >= 12)
					value += 16.0f;
				if (spots[spot_index].power_gun != 0 && interest >= 8)
					value += 24.0f;
				if (value > best_value)
				{
					best_value = value;
					best_bot = bot_index;
					best_spot = spot_index;
				}
			}
		}
		if (best_bot == NONE || best_spot == NONE)
			break;
		bot_taken[best_bot] = TRUE;
		spot_taken[best_spot] = TRUE;
		bot_item_goals[bots[best_bot].player].valid = TRUE;
		bot_item_goals[bots[best_bot].player].present = spots[best_spot].present;
		bot_item_goals[bots[best_bot].player].wait = spots[best_spot].wait;
		bot_item_goals[bots[best_bot].player].point = spots[best_spot].point;
	}
	/* a fetch keeps its pad. Refresh whether the item is still there
	without letting a different pad win the slot. */
	for (round = 0; round < MAXIMUM_NUMBER_OF_PLAYERS; round++)
	{
		long nearest = NONE;
		real nearest_dist = 8.0f * 8.0f;
		long index;

		if (bot_states[round].job != _bot_job_fetch)
			continue;
		for (index = 0; index < spot_count; index++)
		{
			real dx = spots[index].point.x - bot_states[round].job_goal.x;
			real dy = spots[index].point.y - bot_states[round].job_goal.y;
			real dist = dx * dx + dy * dy;

			if (dist < nearest_dist)
			{
				nearest_dist = dist;
				nearest = index;
			}
		}
		if (nearest == NONE)
		{
			bot_item_goals[round].valid = FALSE;
			continue;
		}
		bot_item_goals[round].valid = TRUE;
		bot_item_goals[round].present = spots[nearest].present;
		bot_item_goals[round].wait = spots[nearest].wait;
		bot_item_goals[round].point = spots[nearest].point;
	}
}

/* the host fills in a player whose name starts with Bot. Skill 0 to 4
is easy through impossible */
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
	bot_routes_tick();
	bot_assign_items();

	config_skill = config_integer("network.bot_skill");
	if (config_skill < 0)
		config_skill = 0;
	if (config_skill > BOT_SKILL_IMPOSSIBLE)
		config_skill = BOT_SKILL_IMPOSSIBLE;
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
		long called_index;
		long engage;
		long old_target;
		long threat_index;
		real seen_distance;
		real threat_distance;
		real committed_distance;
		boolean committed_visible;
		boolean called;
		real_point3d called_point;
		boolean holding;
		boolean visible;
		long weapon_index;
		struct bot_weapon_kind const *kind;
		short chosen;
		long style;
		long target_player_index;
		boolean planted;

		planted = FALSE;
		holding = FALSE;

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
		if (hardcore_rules_opening_hold())
		{
			csmemset(&action, 0, sizeof(action));
			action.desired_weapon_index = NONE;
			action.desired_grenade_index = NONE;
			action.desired_zoom_level = NONE;
			queue->current_action = action;
			continue;
		}

		state = &bot_states[player_index];
		if (!state->started || state->life_unit != player->unit_index)
		{
			csmemset(state, 0, sizeof(*state));
			state->started = TRUE;
			state->life_unit = player->unit_index;
			state->target_index = NONE;
			state->held_slot = NONE;
			state->assist_player = NONE;
			state->job_hop = NONE;
			state->job_avoid = NONE;
			state->route_link = NONE;
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
		if (state->nade_hold > 0)
			state->nade_hold--;
		/* hold the aim through the wind-up. Idle for a few ticks is the
		action not consumed yet, not a finished throw. */
		if (unit->unit.grenade_throw_state == _unit_grenade_throw_wind_up ||
			unit->unit.grenade_throw_state == _unit_grenade_throw_in_hand)
			state->grenade_aim = TRUE;
		else if (unit->unit.grenade_throw_state == _unit_grenade_throw_ending)
			state->grenade_aim = FALSE;
		else if (state->grenade_aim && state->grenade_ticks <= 172)
			state->grenade_aim = FALSE;
		if (state->switch_ticks > 0)
			state->switch_ticks--;
		if (state->burst_pause > 0)
			state->burst_pause--;
		if (state->avoid_ticks > 0)
			state->avoid_ticks--;
		{
			long slot;

			for (slot = 0; slot < 4; slot++)
			{
				if (state->given_up_ticks[slot] > 0)
					state->given_up_ticks[slot]--;
			}
		}
		if (state->pad_cool > 0)
			state->pad_cool--;
		if (state->assist_ticks > 0)
			state->assist_ticks--;

		csmemset(&action, 0, sizeof(action));
		action.desired_weapon_index = NONE;
		action.desired_grenade_index = NONE;
		action.desired_zoom_level = NONE;
		object_get_origin(player->unit_index, &feet);
		eye = feet;
		eye.z += 0.62f;
		moved = distance3d(&state->last_position, &feet);
		state->last_position = feet;
		/* a teleporter drops them across the map. The route and the
		facing still belong to the side they left. */
		if (moved > 6.0f)
		{
			state->route.valid = FALSE;
			state->route_link = NONE;
			state->job_hop = NONE;
			state->roam_valid = FALSE;
			state->pad_valid = FALSE;
			state->hunt_valid = FALSE;
			state->search_ticks = 0;
			state->search_arrived = FALSE;
			state->step_hold = 0;
			state->commit_ticks = 0;
			state->vault = 0;
			state->gap_hold = FALSE;
			state->gap_ticks = 0;
			state->look_ticks = 0;
			state->facing_yaw = arctangent(unit->object.forward.j, unit->object.forward.i);
			state->facing_pitch = 0.0f;
		}
		aim = unit->object.forward;
		variant = game_engine_get_variant();

		seen_index = NONE;
		called_index = NONE;
		called = FALSE;
		if (variant && variant->universal_variant.teams)
			called = bot_called_enemy(player_index, player->team_index, TRUE,
				&feet, &called_index, &called_point);
		seen_distance = 1.0e9f;
		threat_index = NONE;
		threat_distance = 1.0e9f;
		committed_visible = FALSE;
		committed_distance = 1.0e9f;
		for (target_player_index = 0; target_player_index < MAXIMUM_NUMBER_OF_PLAYERS; target_player_index++)
		{
			struct player_datum *target_player;
			struct unit_datum *target_unit;
			real_point3d point;
			real_point3d point_eye;
			real dx;
			real dy;
			real flat;
			real score_dist;
			real separation;
			boolean seen;
			boolean heard;

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
			flat = square_root(dx * dx + dy * dy);
			/* the enemy already being fought scores as three metres closer */
			score_dist = flat;
			if (target_player_index == state->target_index)
				score_dist -= 3.0f;
			separation = distance3d(&eye, &point_eye);
			seen = bot_can_see(&eye, &point_eye, player->unit_index);
			heard = bot_know_hidden_enemies && bot_within_hearing(&feet, &point);
			/* their floor is not a fight. A sight line on this floor is.
			tracking the body through the ceiling is how the circle starts. */
			if (bot_different_floor(&feet, &point))
				seen = FALSE;
			if (seen)
			{
				if (target_player_index == state->target_index)
				{
					committed_visible = TRUE;
					committed_distance = separation;
				}
				/* someone close enough to punch. A far fight can wait. */
				if (separation < 1.8f && separation < threat_distance)
				{
					threat_distance = separation;
					threat_index = target_player_index;
				}
				if (seen_index == NONE || score_dist <= seen_distance)
				{
					seen_distance = score_dist;
					seen_index = target_player_index;
				}
			}
			else if (bot_know_hidden_enemies && !bot_different_floor(&feet, &point) &&
				flat < 50.0f && score_dist < seen_distance &&
				(heard || (target_player_index == state->target_index && state->unknown_ticks < 60)))
			{
				/* a heard enemy only wins when nobody in sight is closer */
				seen_distance = score_dist;
				seen_index = target_player_index;
			}
		}

		/* a body this bot can see wins. Otherwise a teammate's sight is
		the location, and only while that sight still holds. One missed
		ray is not the end of it. */
		if (seen_index != NONE)
			state->call_hold = 0;
		else if (called)
			seen_index = called_index;
		else if (state->call_hold > 0 && state->target_index != NONE)
		{
			seen_index = state->target_index;
			state->call_hold--;
		}

		old_target = state->target_index;
		/* the current enemy already scored three metres closer. A much
		closer body, seen or heard, takes the fight. */
		engage = seen_index;
		if (threat_index != NONE && threat_index != engage &&
			(!committed_visible || committed_distance > 4.0f))
			engage = threat_index;

		if (engage != old_target)
		{
			/* the spot they were actually seen, not where they are now */
			if (engage == NONE && !bot_know_hidden_enemies && state->last_seen_valid)
			{
				state->search_position = state->last_seen;
				state->search_ticks = 210;
				state->search_arrived = FALSE;
			}
			state->target_index = engage;
			state->target_visible = FALSE;
			state->unseen_ticks = engage == NONE ? 0 : 22;
			state->unknown_ticks = 0;
			state->sight_ticks = 0;
			state->last_seen_valid = FALSE;
			state->reaction_ticks = 0;
			state->yaw_error = bot_random_real() * bot_aim_error[skill];
			state->pitch_error = bot_random_real() * bot_aim_error[skill];
			state->burst_ticks = 0;
			state->burst_pause = 0;
			state->powerup_ready = engage != NONE;
			if (engage == NONE)
			{
				state->look_ticks = 0;
				state->call_hold = 0;
			}
		}

		visible = FALSE;
		distance = 12.0f;
		aim_point = eye;
		if (engage != NONE)
		{
			struct player_datum *target_player;
			struct unit_datum *target_unit;
			real_point3d point;
			real_point3d point_eye;

			target_player = player_try_and_get(engage);
			target_unit = NULL;
			if (target_player && target_player->unit_index != NONE)
				target_unit = unit_try_and_get(target_player->unit_index);
			if (!target_unit || TEST_FLAG(target_unit->object.damage_flags, _object_dead_bit))
			{
				engage = NONE;
				state->target_index = NONE;
				state->target_visible = FALSE;
				state->last_seen_valid = FALSE;
				state->call_hold = 0;
			}
			else
			{
				object_get_origin(target_player->unit_index, &point);
				point_eye = point;
				point_eye.z += 0.62f;
				/* shots aim at the chest. The sight ray stays at the eyes. */
				aim_point = point;
				aim_point.z += 0.50f;
				distance = distance3d(&eye, &aim_point);
				visible = bot_can_see(&eye, &point_eye, player->unit_index);
				/* the ray that arms a reaction is every five ticks. Losing
				them still counts every tick, up to 22, which is what makes
				the next look wait. */
				state->sight_ticks--;
				if (state->sight_ticks <= 0)
				{
					state->sight_ticks = 5;
					state->target_visible = visible;
					if (visible && state->unseen_ticks >= 22)
						state->reaction_ticks = bot_reaction_time(skill);
				}
				if (state->target_visible)
				{
					state->unseen_ticks = 0;
					state->unknown_ticks = 0;
					state->last_seen = aim_point;
					state->last_seen_valid = TRUE;
				}
				else
				{
					if (state->unseen_ticks <= 21)
						state->unseen_ticks++;
					if (bot_know_hidden_enemies && bot_within_hearing(&feet, &point))
					{
						/* still heard: the search point follows them,
						and stepping back into earshot clears the forget clock */
						state->unknown_ticks = 0;
						state->last_seen = aim_point;
						state->last_seen_valid = TRUE;
					}
					else if (called && engage == called_index)
					{
						/* the teammate can see them. The point follows the
						body, and it is dropped when that sight ends. */
						state->call_hold = 8;
						state->unknown_ticks = 0;
						state->last_seen = called_point;
						state->last_seen.z += 0.50f;
						state->last_seen_valid = TRUE;
					}
					else
					{
						if (!state->last_seen_valid)
						{
							state->last_seen = aim_point;
							state->last_seen_valid = TRUE;
						}
						state->unknown_ticks++;
						if (state->unknown_ticks > 59)
						{
							/* forgotten. Walk to where they were, and only
							then look around. The scan is that arrival,
							not the walk toward the wall in front of it. */
							if (state->last_seen_valid)
							{
								state->search_position = state->last_seen;
								state->search_ticks = 210;
								state->search_arrived = FALSE;
								state->look_timer = 0;
								state->look_valid = FALSE;
							}
							engage = NONE;
							state->target_index = NONE;
							state->target_visible = FALSE;
							state->last_seen_valid = FALSE;
							visible = FALSE;
						}
					}
				}
			}
		}

		weapon_index = unit_inventory_get_weapon(player->unit_index, unit->unit.current_weapon_index);
		kind = bot_weapon_kind(weapon_index);
		if (kind)
		{
			near_range = kind->near_range;
			far_range = kind->far_range;
			style = kind->style;
		}
		else if (weapon_index == NONE)
		{
			near_range = 0.0f;
			far_range = 1.5f;
			style = 0;
		}
		else
		{
			near_range = 3.0f;
			far_range = 12.0f;
			style = 0;
		}
		if (state->switch_ticks <= 0 || !bot_slot_usable(player->unit_index, unit->unit.current_weapon_index))
		{
			short current_slot = unit->unit.current_weapon_index;
			boolean current_usable = bot_slot_usable(player->unit_index, current_slot);
			long current_weapon = unit_inventory_get_weapon(player->unit_index, current_slot);

			chosen = bot_choose_slot(player->unit_index, engage == NONE ? 12.0f : distance);
			/* nothing carried can fire: do not swap onto an empty slot */
			if (chosen == NONE)
				chosen = current_slot;
			/* a flag or an oddball is not a gun to put away */
			else if (current_weapon != NONE && weapon_must_be_readied(current_weapon))
				chosen = current_slot;
			/* keep the gun in hand unless it is empty or the other one scores higher */
			else if (current_usable && chosen != current_slot &&
				bot_slot_score(player->unit_index, chosen) <= bot_slot_score(player->unit_index, current_slot))
				chosen = current_slot;
			if (chosen != current_slot)
			{
				state->held_slot = chosen;
				state->switch_ticks = 150;
			}
			else
				state->held_slot = NONE;
		}
		if (state->held_slot != NONE && state->held_slot != unit->unit.current_weapon_index)
			action.desired_weapon_index = state->held_slot;
		else if (state->reload_ticks <= 0 && bot_should_reload(weapon_index))
		{
			action.control_flags |= FLAG(_unit_control_weapon_reload_bit);
			state->reload_ticks = 30;
		}

		{
			real_point3d move_goal;
			real_point3d look;
			real goal_yaw;
			real goal_pitch;
			real limit;
			real pitch_limit;
			boolean walking;
			boolean fetching;
			boolean on_item;
			boolean waiting;
			boolean hunting;
			boolean stepped;
			boolean in_hall;
			real step_yaw;
			real hall_yaw;
			real_point3d hunt_point;

			move_goal = feet;
			goal_yaw = state->facing_yaw;
			goal_pitch = 0.0f;
			fetching = FALSE;
			on_item = FALSE;
			waiting = FALSE;
			hunting = FALSE;
			stepped = FALSE;
			in_hall = FALSE;
			hall_yaw = state->facing_yaw;
			if (player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
				bot_item_goals[player_index].valid)
			{
				real dx = bot_item_goals[player_index].point.x - feet.x;
				real dy = bot_item_goals[player_index].point.y - feet.y;
				real item_dist = square_root(dx * dx + dy * dy);

				/* an enemy in arm's reach, and the pickup is the long way.
				anything else, including a powerup across the fight, is a trip.
				a metre short is not on the pickup. Once it is gone, or it
				is not back yet, another lap of the marker is the camp. */
				if (visible && distance < 4.0f && distance + 0.5f < item_dist)
					;
				else if (!bot_item_goals[player_index].present && item_dist < 6.0f)
				{
					if (bot_item_goals[player_index].wait > 4 * TICKS_PER_SECOND)
						bot_give_up(state, player_index, &feet, FALSE);
					else
						waiting = TRUE;
				}
				else if (item_dist > 0.4f)
					fetching = TRUE;
				else if (!bot_item_goals[player_index].present)
					waiting = TRUE;
				else
				{
					fetching = TRUE;
					on_item = TRUE;
				}
			}
			if (waiting || on_item)
				state->wait_ticks++;
			else
				state->wait_ticks = 0;
			/* standing on a weapon that will not be carried */
			if (on_item && state->wait_ticks > 45)
			{
				bot_give_up(state, player_index, &feet, FALSE);
				fetching = FALSE;
				on_item = FALSE;
			}
			/* close to a pickup that is still there, and not standing on it.
			the path around a marker inside a solid never arrives. */
			if (fetching && !on_item &&
				player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
				bot_item_goals[player_index].valid && bot_item_goals[player_index].present)
			{
				real dx = bot_item_goals[player_index].point.x - feet.x;
				real dy = bot_item_goals[player_index].point.y - feet.y;
				real item_dist = square_root(dx * dx + dy * dy);
				boolean solid = bot_solid_before_point(&feet,
					&bot_item_goals[player_index].point, item_dist, player->unit_index);

				/* once they are on the marker, drifting past 5m is the
				lap around the object, not a fresh approach. */
				if (item_dist < 6.0f || state->look_timer > 30)
				{
					long limit = solid ? 3 * TICKS_PER_SECOND : 4 * TICKS_PER_SECOND;

					state->look_timer++;
					if (state->look_timer > limit)
					{
						bot_give_up(state, player_index, &feet, solid);
						fetching = FALSE;
						state->look_timer = 0;
					}
				}
				else
					state->look_timer = 0;
			}
			else if (!on_item)
				state->look_timer = 0;
			/* a pad that does not produce the item is not a place to live */
			if (waiting && state->wait_ticks > 4 * TICKS_PER_SECOND)
			{
				bot_give_up(state, player_index, &feet, FALSE);
				waiting = FALSE;
			}
			if (engage != NONE)
			{
				state->search_ticks = 0;
				state->search_arrived = FALSE;
			}
			else if (state->search_ticks > 0)
			{
				real dx;
				real dy;
				real dz;

				/* walk to where they were. Getting there, or finding that
				the spot is another floor, is the end of it. Standing and
				glancing is not a search, and it is not a way upstairs. */
				state->search_ticks--;
				dx = state->search_position.x - feet.x;
				dy = state->search_position.y - feet.y;
				dz = state->search_position.z - feet.z;
				if (dz < 0.0f)
					dz = -dz;
				if (state->search_ticks <= 0 ||
					bot_different_floor(&feet, &state->search_position) ||
					(dx * dx + dy * dy < 4.0f && dz < 2.5f))
				{
					state->search_ticks = 0;
					state->search_arrived = FALSE;
				}
			}
			state->route_floor = FALSE;
			state->route_surface = NONE;
			state->route_tour = FALSE;
			if (engage == NONE && !fetching && !waiting && state->search_ticks <= 0)
			{
				real_point3d enemy;
				long enemy_unit;

				if (bot_know_hidden_enemies &&
					bot_nearest_enemy(player_index, player->team_index,
						variant && variant->universal_variant.teams, &feet, &enemy, &enemy_unit))
				{
					/* their floor. Walk the ground path to it. A peek on
					this floor looks up through the ceiling and the two of
					them circle the column. */
					if (bot_different_floor(&feet, &enemy))
					{
						real_point3d floor_point;
						long surface;

						floor_point = enemy;
						if (bot_enemy_floor(enemy_unit, &floor_point, &surface))
						{
							state->route_floor = TRUE;
							state->route_surface = surface;
							state->hunt_goal = floor_point;
						}
						else
							state->hunt_goal = enemy;
						state->hunt_valid = TRUE;
						state->hunt_ticks = 45;
						hunt_point = state->hunt_goal;
						hunting = TRUE;
					}
					/* nobody in sight on this floor. A ring around the
					nearest body keeps them in one spot. The tour walks
					the map until someone is actually there. */
				}
			}
			else
			{
				state->hunt_valid = FALSE;
				state->hunt_ticks = 0;
			}
			{
				real dx = feet.x - state->area_anchor.x;
				real dy = feet.y - state->area_anchor.y;
				boolean roaming = engage == NONE && !fetching && !waiting &&
					state->search_ticks <= 0 && !hunting;

				/* a live trip is not indecision. The stand is a bot with no job. */
				if (!state->area_valid || !roaming || state->job != _bot_job_none ||
					dx * dx + dy * dy > 6.0f * 6.0f)
				{
					state->area_anchor = feet;
					state->area_valid = TRUE;
					state->area_ticks = 0;
				}
				else if (state->area_ticks < 2 * TICKS_PER_SECOND)
					state->area_ticks++;
				if (engage != NONE)
				{
					state->assist_valid = FALSE;
					state->assist_ticks = 0;
				}
				else if (state->assist_valid)
				{
					struct player_datum *mate;
					real_point3d point;
					real ax;
					real ay;

					mate = state->assist_player == NONE ? NULL :
						player_try_and_get(state->assist_player);
					if (mate && mate->unit_index != NONE)
					{
						object_get_origin(mate->unit_index, &point);
						state->assist_goal = point;
						state->assist_floor = FALSE;
						state->assist_surface = NONE;
						if (bot_different_floor(&feet, &point))
						{
							real_point3d floor_point = point;
							long surface = NONE;

							state->assist_floor = TRUE;
							if (bot_enemy_floor(mate->unit_index, &floor_point, &surface))
							{
								state->assist_goal = floor_point;
								state->assist_surface = surface;
							}
						}
					}
					ax = state->assist_goal.x - feet.x;
					ay = state->assist_goal.y - feet.y;
					if (state->assist_ticks <= 0 ||
						(ax * ax + ay * ay < 8.0f * 8.0f &&
							!bot_different_floor(&feet, &state->assist_goal)))
					{
						state->assist_valid = FALSE;
						state->area_anchor = feet;
						state->area_ticks = 0;
					}
				}
				else if (roaming && state->job == _bot_job_none &&
					state->area_ticks >= 2 * TICKS_PER_SECOND)
				{
					real_point3d fight;
					long fight_unit;
					long fight_surface;
					boolean other_floor;

					if (bot_teammate_fight(player_index, player->team_index,
							variant && variant->universal_variant.teams, &feet,
							&fight, &fight_unit, &other_floor, &fight_surface))
					{
						state->assist_valid = TRUE;
						state->assist_goal = fight;
						state->assist_floor = other_floor;
						state->assist_surface = fight_surface;
						state->assist_ticks = 20 * TICKS_PER_SECOND;
						state->assist_player = NONE;
						{
							long index;

							for (index = 0; index < MAXIMUM_NUMBER_OF_PLAYERS; index++)
							{
								struct player_datum *other = player_try_and_get(index);

								if (!other || other->unit_index != fight_unit)
									continue;
								state->assist_player = index;
								break;
							}
						}
						state->route.valid = FALSE;
						state->roam_valid = FALSE;
					}
				}
			}
			if (holding && state->last_seen_valid &&
				!bot_different_floor(&feet, &state->last_seen))
			{
				real_vector3d direction;
				real_euler_angles2d angles;

				/* still their target. Look where they were, do not pick
				someone else and do not wander off. */
				vector_from_points3d(&eye, &state->last_seen, &direction);
				if (normalize3d(&direction) != 0.0f)
				{
					euler_angles2d_from_vector3d(&angles, &direction);
					goal_yaw = angles.yaw;
					goal_pitch = PIN(angles.pitch, -1.0f, 1.0f);
				}
			}
			else if (engage == NONE)
			{
				real dx;
				real dy;

				bot_job_maintain(state, &feet, player->unit_index, player_index,
					hunting, &hunt_point);
				if (state->job != _bot_job_none)
				{
					move_goal = state->job_goal;
					goal_yaw = bot_yaw_between(&feet, &move_goal);
					if (state->job_floor && state->job_surface != NONE)
					{
						state->route_floor = TRUE;
						state->route_surface = state->job_surface;
					}
					state->route_tour = TRUE;
					/* already beside a powerup. Step onto it, then the same trip. */
					if (state->job != _bot_job_ladder && !state->job_climb)
					{
						real_point3d walk;
						real_point3d item;

						if (bot_close_powerup(state, player, player->unit_index, &feet,
								&walk, &item))
						{
							if (state->job_grab <= 0)
							{
								real dx = item.x - feet.x;
								real dy = item.y - feet.y;
								real dist = square_root(dx * dx + dy * dy);
								long ticks = (long)(dist / 2.25f * (real)TICKS_PER_SECOND) +
									TICKS_PER_SECOND + TICKS_PER_SECOND / 2;

								if (ticks < 2 * TICKS_PER_SECOND)
									ticks = 2 * TICKS_PER_SECOND;
								if (ticks > 6 * TICKS_PER_SECOND)
									ticks = 6 * TICKS_PER_SECOND;
								state->job_grab = (short)ticks;
								state->job_grab_point = item;
								state->job_still = 0;
								state->job_anchor = feet;
								state->route.valid = FALSE;
							}
							if (state->job_grab > 0)
							{
								state->job_grab--;
								if (state->job_grab == 0)
								{
									bot_ban_point(state, &state->job_grab_point,
										8 * TICKS_PER_SECOND);
									state->route.valid = FALSE;
								}
								else
								{
									move_goal = walk;
									state->route_floor = FALSE;
									goal_yaw = bot_yaw_between(&feet, &move_goal);
								}
							}
						}
						else if (state->job_grab > 0)
						{
							state->job_grab = 0;
							state->route.valid = FALSE;
						}
					}
				}
				else if (fetching)
				{
					move_goal = bot_item_goals[player_index].point;
					goal_yaw = bot_yaw_between(&feet, &move_goal);
				}
				else if (state->search_ticks > 0)
				{
					move_goal = state->search_position;
					goal_yaw = bot_yaw_between(&feet, &move_goal);
				}
				else if (hunting)
				{
					real desired;
					real walk;

					/* face the open way toward a corner that can see them.
					the bearing through a wall is not a look. A floor change
					faces the path step instead, not the column under them. */
					move_goal = hunt_point;
					if (!state->route_floor)
					{
						desired = bot_yaw_between(&feet, &move_goal);
						if (bot_open_distance(&feet, desired, player->unit_index) >= 1.6f &&
							!bot_heading_is_corner(&feet, desired, player->unit_index))
							goal_yaw = desired;
						else if (bot_clear_yaw(&feet, desired, player->unit_index, &walk))
							goal_yaw = walk;
					}
				}
				else if (state->assist_valid)
				{
					/* ten seconds in one spot. The teammate's fight is
					out of it, so the path there replaces the local loop. */
					move_goal = state->assist_goal;
					goal_yaw = bot_yaw_between(&feet, &move_goal);
					if (state->assist_floor)
					{
						state->route_floor = TRUE;
						state->route_surface = state->assist_surface;
					}
					state->route_tour = TRUE;
				}
				else if (!waiting)
				{
					/* nothing to shoot and no spawn to take: walk to a
					place across the map, and stay on that path until it
					is done. A new heading every few seconds is the
					back-and-forth. */
					dx = state->roam_goal.x - feet.x;
					dy = state->roam_goal.y - feet.y;
					if (!state->roam_valid || state->look_ticks <= 0 ||
						square_root(dx * dx + dy * dy) < 3.5f)
					{
						if (!bot_choose_tour(state, &feet, player->unit_index))
							bot_choose_roam(state, &feet, player->unit_index);
					}
					move_goal = state->roam_goal;
					goal_yaw = bot_yaw_between(&feet, &move_goal);
					/* the high ground is another floor. The path is the
					stairs, not a line across this one. */
					if (bot_different_floor(&feet, &move_goal))
					{
						real_point3d ground;
						long surface;

						if (bot_ground_at(&move_goal, &ground, &surface))
						{
							state->route_floor = TRUE;
							state->route_surface = surface;
							move_goal = ground;
							state->roam_goal = ground;
						}
						else
						{
							move_goal.z = feet.z;
							state->roam_goal.z = feet.z;
						}
					}
					state->route_tour = TRUE;
				}
			}
			else if (!visible)
			{
				real_vector3d direction;
				real_euler_angles2d angles;
				real_point3d search;
				real dx;
				real dy;
				real dist;
				boolean walled;

				/* lost the sightline. Walk to where they were. A wall
				between them is a path around it, not a stand against it. */
				if (fetching)
					move_goal = bot_item_goals[player_index].point;
				else if (state->last_seen_valid)
					move_goal = state->last_seen;
				else
					move_goal = aim_point;
				search = state->last_seen_valid ? state->last_seen : aim_point;
				dx = search.x - feet.x;
				dy = search.y - feet.y;
				dist = square_root(dx * dx + dy * dy);
				walled = dist > 1.2f && !bot_different_floor(&feet, &search) &&
					bot_solid_before_point(&feet, &search, dist, player->unit_index);
				/* a wall, or their floor. Walk the ground path. The bearing
				through the wall is the look that never arrives. */
				if (walled || bot_different_floor(&feet, &search))
				{
					struct player_datum *target_player = player_try_and_get(engage);
					real_point3d floor_point;
					long surface = NONE;

					if (target_player && target_player->unit_index != NONE &&
						bot_enemy_floor(target_player->unit_index, &floor_point, &surface))
					{
						move_goal = floor_point;
						state->route_floor = TRUE;
						state->route_surface = surface;
					}
					state->route_tour = TRUE;
					goal_pitch = 0.0f;
				}
				else if (!bot_different_floor(&feet, &search))
				{
					/* a body on another floor is not a look through the ceiling */
					vector_from_points3d(&eye, &search, &direction);
					if (normalize3d(&direction) != 0.0f)
					{
						euler_angles2d_from_vector3d(&angles, &direction);
						goal_pitch = PIN(angles.pitch, -1.0f, 1.0f);
					}
				}
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
				if (fetching)
					move_goal = bot_item_goals[player_index].point;
				else
					bot_fight_goal(state, &feet, &aim_point, near_range, far_range,
						player->unit_index, skill, &move_goal);
			}
			/* a corridor is one committed direction. Recomputing it from
			the nose turns a dead end into a walk back down the same hall. */
			if (engage == NONE && !holding && !fetching && !waiting &&
				state->search_ticks <= 0 && !state->route_floor && !state->route_tour &&
				unit->object.parent_object_index == NONE &&
				bot_hall_heading(&feet, state->facing_yaw, bot_yaw_between(&feet, &move_goal),
					state->wander_yaw, player->unit_index, &hall_yaw))
			{
				real reach;

				/* hold the direction only while it still has room.
				a shortening hall is the moment to take the doorway,
				not two seconds later at the wall. */
				if (state->hall_lock > 0 &&
					bot_open_distance(&feet, state->wander_yaw, player->unit_index) >= 3.0f)
				{
					hall_yaw = state->wander_yaw;
					state->hall_lock--;
				}
				else
					state->hall_lock = 18;
				state->wander_yaw = hall_yaw;
				state->commit_yaw = hall_yaw;
				state->commit_ticks = 15;
				reach = bot_open_distance(&feet, hall_yaw, player->unit_index);
				if (reach < 2.0f)
					reach = 2.0f;
				if (reach > 6.0f)
					reach = 6.0f;
				move_goal.x = feet.x + cosine(hall_yaw) * reach;
				move_goal.y = feet.y + sine(hall_yaw) * reach;
				move_goal.z = feet.z;
				goal_yaw = hall_yaw;
				state->roam_goal = move_goal;
				state->roam_valid = TRUE;
				state->look_ticks = 30;
				in_hall = TRUE;
			}
			else
				state->hall_lock = 0;
			/* a teleporter is a route when its exit is nearer the goal
			than this side of the map. A fight already in sight stays.
			a link the graph already chose is the walk, including its pad. */
			if (engage == NONE && !holding && !waiting && unit->object.parent_object_index == NONE &&
				!bot_hop_owned(state))
			{
				real_point3d destination;
				real_point3d pad;
				real_point3d enemy;

				destination = move_goal;
				if (hunting && bot_nearest_enemy(player_index, player->team_index,
						variant && variant->universal_variant.teams, &feet, &enemy, NULL))
					destination = enemy;
				if (state->pad_cool <= 0 &&
					bot_teleporter_pad(&feet, &destination, player->unit_index,
						player->teleporter_index, &pad,
						state->pad_valid || state->job_path_fail > 10))
				{
					state->pad_point = pad;
					state->pad_valid = TRUE;
					move_goal = pad;
					/* the pad is on this floor. The route to their floor
					starts again on the other side. */
					state->route_floor = FALSE;
				}
				else
					state->pad_valid = FALSE;
			}
			else
				state->pad_valid = FALSE;
			/* face the step the walk will take. The point itself can sit
			around a corner, and turning toward that point never lines up
			with the step, so the bot stays planted. */
			if ((engage == NONE || !visible) && !holding && !waiting && !in_hall &&
				!(state->search_ticks > 0 && state->search_arrived))
				stepped = bot_step_yaw(state, player->unit_index, &feet, &move_goal, &step_yaw);
			if (stepped)
			{
				real flip;
				real magnitude;
				real side;

				state->step_hold = 0;
				flip = bot_angle_delta(state->commit_yaw, step_yaw);
				magnitude = flip < 0.0f ? -flip : flip;

				/* a step straight back is the hallway bounce. Take an
				opening off to the side before turning around. A floor
				change is allowed to turn around: that is the stairs. */
				if (!state->route_floor && !state->route_tour &&
					state->commit_ticks > 0 && magnitude > 2.2f)
				{
					if (bot_side_path(&feet, state->commit_yaw, player->unit_index, &side))
						step_yaw = side;
					else if (bot_open_distance(&feet, state->commit_yaw, player->unit_index) >= 1.4f)
						step_yaw = state->commit_yaw;
					flip = bot_angle_delta(state->commit_yaw, step_yaw);
					magnitude = flip < 0.0f ? -flip : flip;
				}
				if (state->commit_ticks <= 0 || magnitude <= 1.2f)
				{
					state->commit_yaw = step_yaw;
					state->commit_ticks = 50;
				}
				state->step_yaw = step_yaw;
				if (state->commit_ticks > 0)
					state->commit_ticks--;
				goal_yaw = step_yaw;
			}
			else if ((engage == NONE || !visible) &&
				(state->route_tour || state->route_floor))
			{
				/* no step yet. The far point is around a corner, and
				looking at it is the scan. */
				goal_yaw = state->facing_yaw;
				goal_pitch = 0.0f;
			}
			/* the next step is a gap, and a landing on this heading is closer
			to that step. A landing closer to a far flag is how they leave
			the catwalk. A stored link jumps from its own lip. */
			if (engage == NONE && !state->gap_hold && state->gap_ticks == 0 &&
				state->vault == 0 && !state->job_climb && !waiting && !in_hall)
			{
				real_point3d jump_goal;
				real toward;

				jump_goal = move_goal;
				if (bot_jump_choice(&feet, player->unit_index, &jump_goal, &toward))
				{
					step_yaw = toward;
					stepped = TRUE;
					goal_yaw = toward;
					state->commit_yaw = toward;
					state->commit_ticks = 20;
					state->gap_ticks = 50;
					state->gap_yaw = toward;
				}
			}
			if (state->pad_valid)
			{
				real dx = state->pad_point.x - feet.x;
				real dy = state->pad_point.y - feet.y;

				/* the last steps are into the closet. The path step stops
				short of the back wall and never reaches the flag. */
				if (dx * dx + dy * dy < 2.4f * 2.4f)
					goal_yaw = bot_yaw_between(&feet, &state->pad_point);
			}
			if (engage == NONE && state->job != _bot_job_none && state->route_tour && !state->job_climb)
			{
				/* a missing path that still produced a step is a walk, not a stall */
				if (!state->route.valid && !stepped)
					state->job_path_fail++;
				else
					state->job_path_fail = 0;
			}
			if (state->job == _bot_job_none && fetching && bot_item_goals[player_index].present)
			{
				real dx = bot_item_goals[player_index].point.x - feet.x;
				real dy = bot_item_goals[player_index].point.y - feet.y;
				real dist2 = dx * dx + dy * dy;
				real dist = square_root(dist2);
				real_point3d item = bot_item_goals[player_index].point;

				/* the path stops a unit short of the wall the powerup
				sits against. Walk onto it when the point is in the open.
				a marker inside a solid keeps one slide. The reverse
				heading is the turn-around on either side of the object. */
				if (!bot_solid_before_point(&feet, &item, dist, player->unit_index))
				{
					if (dist2 < 2.4f * 2.4f)
						goal_yaw = bot_yaw_between(&feet, &item);
				}
				else
				{
					real slide;
					real toward = bot_yaw_between(&feet, &item);

					if (bot_slide_around(&feet, toward, state->facing_yaw,
							player->unit_index, &slide))
					{
						real held = bot_angle_delta(state->facing_yaw, state->commit_yaw);
						real proposed = bot_angle_delta(state->facing_yaw, slide);

						if (held < 0.0f)
							held = -held;
						if (proposed < 0.0f)
							proposed = -proposed;
						if (state->commit_ticks > 0 && held < 0.8f && proposed > 1.2f)
							slide = state->commit_yaw;
						goal_yaw = slide;
						state->commit_yaw = slide;
						state->commit_ticks = 25;
					}
					else
						goal_yaw = toward;
				}
			}
			/* the reaction wait holds the trigger. Facing still turns. */
			limit = bot_look_step(skill, FALSE);
			pitch_limit = bot_look_step(skill, TRUE);
			/* a lip they cannot walk over. Face it and run at it. The
			trench on Hang 'em High ends in that lip, and turning around
			leaves them on the floor. */
			if (state->vault == 0 && !waiting &&
				unit->object.parent_object_index == NONE &&
				unit->object.type == _object_type_biped)
			{
				real lip;
				struct biped_datum *grounded;
				real pad_dx;
				real pad_dy;
				boolean at_pad;

				at_pad = FALSE;
				if (state->pad_valid)
				{
					pad_dx = state->pad_point.x - feet.x;
					pad_dy = state->pad_point.y - feet.y;
					at_pad = pad_dx * pad_dx + pad_dy * pad_dy < 2.4f * 2.4f;
				}
				grounded = biped_try_and_get(player->unit_index);
				/* a trench is a place to leave, unless they are walking to
				a pickup that is still on this floor */
				if (!at_pad && !state->gap_hold && grounded &&
					!TEST_FLAG(grounded->biped.flags, _biped_airborne_bit) &&
					bot_find_ledge(&feet, state->facing_yaw, player->unit_index,
						engage == NONE && !fetching, &lip))
				{
					state->vault = 1;
					state->vault_yaw = lip;
					state->vault_ticks = 0;
					state->vault_run = 0;
					state->vault_air = FALSE;
				}
			}
			if (state->pad_valid)
			{
				real dx = state->pad_point.x - feet.x;
				real dy = state->pad_point.y - feet.y;

				if (dx * dx + dy * dy < 2.4f * 2.4f)
					state->vault = 0;
			}
			if (state->vault != 0)
			{
				goal_yaw = state->vault_yaw;
				limit = bot_look_step(skill, FALSE);
				pitch_limit = bot_look_step(skill, TRUE);
			}
			/* nothing in sight. The walk is level, down the step.
			a jump they have already chosen stays on that diagonal. */
			if (engage == NONE)
				goal_pitch = 0.0f;
			if (engage == NONE && state->gap_ticks > 0 && !state->gap_hold)
			{
				goal_yaw = state->gap_yaw;
				limit *= 3.0f;
			}
			state->facing_yaw = bot_slew(state->facing_yaw, goal_yaw, limit);
			state->facing_pitch = bot_slew(state->facing_pitch, goal_pitch, pitch_limit);
			look.x = eye.x + cosine(state->facing_yaw) * cosine(state->facing_pitch);
			look.y = eye.y + sine(state->facing_yaw) * cosine(state->facing_pitch);
			look.z = eye.z + sine(state->facing_pitch);
			bot_face(player, &action, &eye, &look, 0.0f, 0.0f, &aim);
			if (holding)
			{
				action.throttle.i = 0.0f;
				action.throttle.j = 0.0f;
				walking = TRUE;
			}
			else if (in_hall)
				walking = bot_walk_hall(state, player->unit_index, &feet, hall_yaw, &action);
			else if (waiting && engage == NONE)
			{
				action.throttle.i = 0.0f;
				action.throttle.j = 0.0f;
				walking = TRUE;
			}
			else if (engage != NONE && visible && !fetching)
			{
				real side;
				real side_yaw;
				real forward;

				/* face the target and step across, then back. The weave
				is the dodge. Closing and backing up stay on top of it. */
				side = state->strafe >= 0.0f ? 1.0f : -1.0f;
				side_yaw = state->facing_yaw - side * (_pi * 0.5f);
				if (bot_open_distance(&feet, side_yaw, player->unit_index) < 0.75f)
				{
					side = -side;
					state->strafe = side;
					state->strafe_ticks = 8;
					side_yaw = state->facing_yaw - side * (_pi * 0.5f);
				}
				forward = 0.0f;
				if (distance > far_range)
					forward = 0.8f;
				else if (distance < near_range)
					forward = -0.5f;
				if (bot_open_distance(&feet, side_yaw, player->unit_index) >= 0.55f)
				{
					state->strafe = side;
					action.throttle.i = forward;
					action.throttle.j = side;
					walking = TRUE;
				}
				else
					walking = bot_walk(state, player->unit_index, &feet, &move_goal, &action, TRUE);
			}
			else if (stepped)
				walking = bot_apply_heading(state, player->unit_index, &feet, step_yaw, &action, FALSE,
					(state->route_tour || state->route_floor) && state->route.valid);
			else
				walking = bot_walk(state, player->unit_index, &feet, &move_goal, &action,
					engage != NONE && visible);
			if (fetching && !walking)
			{
				real dx = bot_item_goals[player_index].point.x - feet.x;
				real dy = bot_item_goals[player_index].point.y - feet.y;

				/* the last steps are into the pickup. A blocked probe
				here is the wall behind it, not a failed trip. */
				if (!bot_item_goals[player_index].valid ||
					!bot_item_goals[player_index].present ||
					dx * dx + dy * dy >= 2.4f * 2.4f)
					state->fetch_fail++;
			}
			else if (!fetching || action.throttle.i != 0.0f || action.throttle.j != 0.0f)
				state->fetch_fail = 0;
			if (state->fetch_fail > 25)
			{
				bot_give_up(state, player_index, &feet, FALSE);
				fetching = FALSE;
				waiting = FALSE;
				state->fetch_fail = 0;
			}
			if (!walking && engage == NONE && !holding && !waiting && state->vault == 0 && !in_hall)
			{
				if (!state->roam_valid || state->look_ticks <= 0)
					bot_choose_roam(state, &feet, player->unit_index);
				walking = bot_walk(state, player->unit_index, &feet, &state->roam_goal, &action, FALSE);
			}
			if (!walking)
				state->route.valid = FALSE;
			if (walking && engage != NONE && visible && state->strafe_jump)
			{
				action.control_flags |= FLAG(_unit_control_jump_bit);
				state->strafe_jump = FALSE;
			}
			planted = waiting;
		}
		if (engage != NONE)
		{
			boolean widen;
			boolean on_target;
			boolean shoot;
			short grenade;

			/* the cone widens while the trigger was already down */
			widen = state->trigger_down;
			on_target = visible && bot_on_target(&eye, &aim_point, &aim, skill, widen);
			shoot = FALSE;

			if (visible && distance < 1.6f && state->melee_ticks <= 0 &&
				action.desired_weapon_index == NONE)
			{
				/* melee is the equipment button. The action button reloads
				when there is nothing to pick up or board. A weapon swap
				uses the same tick, so the punch waits. */
				action.control_flags |= FLAG(_unit_control_use_equipment_bit);
				state->melee_ticks = 30;
			}
			else if (state->powerup_ready && unit->unit.equipment_object_index != NONE &&
				state->reaction_ticks <= 0 && action.desired_weapon_index == NONE)
			{
				/* once, when the fight starts. Holding the button would
				keep them from shooting if the powerup did not get used. */
				action.control_flags |= FLAG(_unit_control_use_equipment_bit);
				state->powerup_ready = FALSE;
			}
			else
			{
				boolean threw;

				threw = FALSE;
				grenade = NONE;
				if (state->grenade_ticks <= 0 &&
					action.desired_weapon_index == NONE &&
					state->vault == 0 && !state->gap_hold)
				{
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
				/* one roll out of a thousand, then six seconds before another.
				the throw waits until they are already looking at the enemy
				and the line in front of the hand is open. A spin releases
				into the wall. Nearly dead, a grenade that lands an arm's
				length out can still be the trade, wall or not. */
				if (grenade != NONE)
				{
					struct player_datum *target_player;
					long enemy_unit;
					real enemy_yaw;
					real yaw_off;
					real pitch_off;
					real dx;
					real dy;
					real dz;
					real flat;
					real enemy_pitch;
					real_point3d throw_to;
					boolean aimed;
					boolean last_ditch;

					target_player = player_try_and_get(engage);
					enemy_unit = target_player ? target_player->unit_index : NONE;
					enemy_yaw = bot_yaw_between(&feet, &aim_point);
					yaw_off = bot_angle_delta(state->facing_yaw, enemy_yaw);
					if (yaw_off < 0.0f)
						yaw_off = -yaw_off;
					dx = aim_point.x - eye.x;
					dy = aim_point.y - eye.y;
					dz = aim_point.z - eye.z;
					flat = square_root(dx * dx + dy * dy);
					enemy_pitch = flat > 0.05f ? arctangent(dz, flat) : 0.0f;
					pitch_off = enemy_pitch - state->facing_pitch;
					if (pitch_off < 0.0f)
						pitch_off = -pitch_off;
					throw_to.x = eye.x + cosine(state->facing_yaw) * cosine(state->facing_pitch) * distance;
					throw_to.y = eye.y + sine(state->facing_yaw) * cosine(state->facing_pitch) * distance;
					throw_to.z = eye.z + sine(state->facing_pitch) * distance;
					aimed = visible && distance >= 6.0f && distance <= 18.0f &&
						yaw_off <= 0.22f && pitch_off <= 0.20f &&
						bot_throw_reaches(&eye, &throw_to, player->unit_index, enemy_unit);
					last_ditch = bot_about_to_die(unit) &&
						!bot_different_floor(&feet, &aim_point) &&
						bot_grenade_can_finish(distance, grenade);
					if (last_ditch ||
						(aimed && (long)((bot_random() >> 8) % 1000) < bot_grenade_chance[skill]))
					{
						action.desired_grenade_index = grenade;
						action.control_flags |= FLAG(_unit_control_throw_grenade_bit) |
							FLAG(_unit_control_weapon_secondary_trigger_bit);
						state->grenade_ticks = 180;
						state->grenade_yaw = state->facing_yaw;
						state->grenade_pitch = state->facing_pitch;
						state->grenade_aim = TRUE;
						threw = TRUE;
						if (last_ditch)
							state->nade_trade = TRUE;
					}
				}
				}
				if (!threw && state->reaction_ticks <= 0 && on_target &&
					bot_weapon_ready(weapon_index) &&
					action.desired_weapon_index == NONE &&
					!(style == 2 && distance < 4.0f))
				{
				if (style == 1)
				{
					/* inside the close band the trigger stays down. Past it,
					twelve to fourteen ticks on, then seven to ten off. The
					pause is one longer than that gap because this tick already
					counted it down at the start of the think. */
					if (distance <= near_range * 0.6f)
					{
						shoot = TRUE;
						state->burst_ticks = 0;
						state->burst_pause = 0;
					}
					else if (state->burst_pause <= 0)
					{
						if (state->burst_ticks <= 0)
							state->burst_ticks = 12 + (long)((bot_random() >> 8) % 3);
						shoot = TRUE;
						state->burst_ticks--;
						if (state->burst_ticks <= 0)
							state->burst_pause = 8 + (long)((bot_random() >> 8) & 3);
					}
				}
				else
					shoot = TRUE;
				if (shoot)
				{
					action.primary_trigger = 1.0f;
					action.control_flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
				}
				}
			}
			state->trigger_down = shoot;
		}
		else
			state->trigger_down = FALSE;

		if (moved < 0.03f && (action.throttle.i != 0.0f || action.throttle.j != 0.0f))
			state->stuck_ticks++;
		else
			state->stuck_ticks = 0;
		if (engage == NONE && !planted &&
			action.throttle.i == 0.0f && action.throttle.j == 0.0f &&
			!((state->route_tour || state->route_floor) && state->route.valid))
			state->idle_ticks++;
		else
			state->idle_ticks = 0;
		if (state->vault == 0 && (state->stuck_ticks > 10 || state->idle_ticks > 30))
		{
			real pad_dx;
			real pad_dy;
			boolean at_pad;

			at_pad = FALSE;
			if (state->pad_valid)
			{
				pad_dx = state->pad_point.x - feet.x;
				pad_dy = state->pad_point.y - feet.y;
				at_pad = pad_dx * pad_dx + pad_dy * pad_dy < 3.0f * 3.0f;
			}
			/* the closet reads as a wall. Keep walking onto the flag
			instead of giving up and turning around. */
			if (at_pad && state->stuck_ticks > 90)
			{
				state->pad_valid = FALSE;
				state->pad_cool = 150;
				at_pad = FALSE;
			}
			if (!at_pad &&
				!(player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
					bot_item_goals[player_index].valid && bot_item_goals[player_index].present &&
					(bot_item_goals[player_index].point.x - feet.x) *
						(bot_item_goals[player_index].point.x - feet.x) +
					(bot_item_goals[player_index].point.y - feet.y) *
						(bot_item_goals[player_index].point.y - feet.y) < 2.4f * 2.4f &&
					!bot_against_solid(&feet, &bot_item_goals[player_index].point,
						square_root((bot_item_goals[player_index].point.x - feet.x) *
							(bot_item_goals[player_index].point.x - feet.x) +
							(bot_item_goals[player_index].point.y - feet.y) *
							(bot_item_goals[player_index].point.y - feet.y)),
						player->unit_index)))
			{
				real hall;

				/* a failed step needs an opening off this hall. Another
				heading down the same corridor is not one, and waiting
				out the lock just walks the lap again. */
				if (engage == NONE && !holding &&
					bot_side_path(&feet, state->wander_yaw, player->unit_index, &hall))
				{
					state->wander_yaw = hall;
					state->commit_yaw = hall;
					state->commit_ticks = 15;
					state->hall_lock = 18;
					state->idle_ticks = 0;
					state->stuck_ticks = 0;
				}
				else
				{
					bot_give_up(state, player_index, &feet, FALSE);
					action.throttle.i = 0.0f;
					action.throttle.j = 0.0f;
				}
			}
		}
		if (engage != NONE && action.throttle.i == 0.0f && action.throttle.j == 0.0f)
			state->fight_idle++;
		else
			state->fight_idle = 0;
		/* the path to the fight failed. Step sideways instead of standing. */
		if (state->fight_idle > 10)
		{
			real side = state->strafe >= 0.0f ? 1.0f : -1.0f;
			real side_yaw = state->facing_yaw - side * (_pi * 0.5f);

			if (bot_open_distance(&feet, side_yaw, player->unit_index) < 1.1f ||
				bot_heading_is_corner(&feet, side_yaw, player->unit_index))
			{
				side = -side;
				side_yaw = state->facing_yaw - side * (_pi * 0.5f);
			}
			state->strafe = side;
			state->strafe_ticks = 25;
			if (bot_open_distance(&feet, side_yaw, player->unit_index) >= 1.1f &&
				!bot_heading_is_corner(&feet, side_yaw, player->unit_index) &&
				!bot_drop_ahead(&feet, side_yaw))
			{
				action.throttle.i = 0.2f;
				action.throttle.j = side;
				state->fight_idle = 0;
			}
		}
		if (state->fight_idle > 20)
		{
			real away;

			state->fight_idle = 0;
			if (bot_escape_yaw(&feet, state->facing_yaw + _pi * 0.5f, player->unit_index, &away))
			{
				real rel = bot_angle_delta(state->facing_yaw, away);

				state->commit_yaw = away;
				state->commit_ticks = 20;
				state->route.valid = FALSE;
				state->escape_yaw = away;
				state->escape_ticks = 20;
				action.throttle.i = cosine(rel) > 0.15f ? 1.0f : 0.0f;
				action.throttle.j = -sine(rel);
			}
		}

		{
			struct biped_datum *biped;
			boolean airborne;
			boolean crouch;

			/* a lip: line up, run straight at it, jump, and crouch once
			the jump stops rising so the body clears the edge. A fall that
			would hurt waits until the ground is close, then crouches. */
			biped = NULL;
			airborne = FALSE;
			crouch = FALSE;
			if (unit->object.parent_object_index != NONE)
				state->vault = 0;
			else if (unit->object.type == _object_type_biped)
				biped = biped_try_and_get(player->unit_index);
			if (biped)
				airborne = TEST_FLAG(biped->biped.flags, _biped_airborne_bit);
			if (state->pad_valid && unit->object.parent_object_index == NONE)
			{
				real dx;
				real dy;
				real dist2;

				dx = state->pad_point.x - feet.x;
				dy = state->pad_point.y - feet.y;
				dist2 = dx * dx + dy * dy;
				/* the probes stop a unit short of a wall. The flag is
				inside that, so the last steps ignore the closet. */
				if (dist2 < 2.4f * 2.4f)
				{
					real dist;
					real yaw;
					real rel;

					dist = square_root(dist2);
					yaw = bot_yaw_between(&feet, &state->pad_point);
					rel = bot_angle_delta(state->facing_yaw, yaw);
					state->vault = 0;
					action.throttle.i = 0.0f;
					action.throttle.j = 0.0f;
					if (dist > 0.05f && rel <= 0.45f && rel >= -0.45f)
						action.throttle.i = dist < 0.35f ? 0.45f : 1.0f;
				}
			}
			if (state->job == _bot_job_none &&
				player_index >= 0 && player_index < MAXIMUM_NUMBER_OF_PLAYERS &&
				bot_item_goals[player_index].valid && bot_item_goals[player_index].present &&
				unit->object.parent_object_index == NONE)
			{
				real dx;
				real dy;
				real dist2;

				dx = bot_item_goals[player_index].point.x - feet.x;
				dy = bot_item_goals[player_index].point.y - feet.y;
				dist2 = dx * dx + dy * dy;
				if (dist2 < 2.4f * 2.4f)
				{
					real dist;
					real yaw;
					real rel;
					real slide;
					real_point3d item;

					dist = square_root(dist2);
					item = bot_item_goals[player_index].point;
					yaw = bot_yaw_between(&feet, &item);
					if (bot_against_solid(&feet, &item, dist, player->unit_index))
					{
						/* the pickup is inside the object. Slide along the
						face for the few seconds the fetch is allowed to
						last, then the tour takes them off it. */
						if (bot_slide_around(&feet, yaw, state->facing_yaw,
								player->unit_index, &slide))
						{
							rel = bot_angle_delta(state->facing_yaw, slide);
							action.throttle.i = 0.0f;
							action.throttle.j = 0.0f;
							state->idle_ticks = 0;
							state->stuck_ticks = 0;
							if (rel <= 0.6f && rel >= -0.6f)
								action.throttle.i = 1.0f;
						}
					}
					else
					{
						rel = bot_angle_delta(state->facing_yaw, yaw);
						action.throttle.i = 0.0f;
						action.throttle.j = 0.0f;
						state->idle_ticks = 0;
						state->stuck_ticks = 0;
						if (dist > 0.05f && rel <= 0.5f && rel >= -0.5f)
							action.throttle.i = 1.0f;
					}
				}
			}
			if (state->gap_ticks < 0)
				state->gap_ticks++;
			else if (state->gap_ticks > 0 && state->vault == 0 && biped && !airborne &&
				!state->job_climb && !state->gap_hold)
			{
				real lip;
				real rel = bot_angle_delta(state->facing_yaw, state->gap_yaw);

				if (rel > 0.55f || rel < -0.55f ||
					!bot_jump_across(&feet, state->gap_yaw, player->unit_index, &lip, NULL))
				{
					state->gap_ticks--;
					/* the lineup did not become a jump. Leave the edge
					alone for a moment instead of standing on it. */
					if (state->gap_ticks == 0)
						state->gap_ticks = -30;
				}
				else
				{
					real_point3d landing;
					real forward;
					real span;
					real reach;
					real ldx;
					real ldy;
					real ldist;

					action.throttle.i = 1.0f;
					action.throttle.j = 0.0f;
					forward = biped->object.translational_velocity.i *
							cosine(state->gap_yaw) +
						biped->object.translational_velocity.j *
							sine(state->gap_yaw);
					span = bot_jump_span(player->unit_index);
					reach = span * (forward / 0.075f);
					if (reach > span)
						reach = span;
					ldist = span;
					if (bot_jump_across(&feet, state->gap_yaw, player->unit_index, &lip, &landing))
					{
						ldx = landing.x - feet.x;
						ldy = landing.y - feet.y;
						ldist = square_root(ldx * ldx + ldy * ldy);
					}
					/* full run, at the lip, pointed at the platform.
					jumping early or slow is the miss onto the camo. */
					if (rel <= 0.16f && rel >= -0.16f && lip < 0.30f && lip > 0.02f &&
						forward >= 0.060f && ldist <= reach)
					{
						action.control_flags |= FLAG(_unit_control_jump_bit);
						state->gap_hold = TRUE;
						state->gap_left = FALSE;
						state->gap_ticks = 0;
					}
					else if (lip < 0.16f && (forward < 0.050f || ldist > reach))
					{
						action.throttle.i = 0.0f;
						action.throttle.j = 0.0f;
						state->gap_ticks = -20;
					}
				}
			}
			if (state->vault == 1)
			{
				real rel;

				rel = bot_angle_delta(state->facing_yaw, state->vault_yaw);
				state->vault_ticks++;
				action.throttle.i = 0.0f;
				action.throttle.j = 0.0f;
				if (rel > 0.40f || rel < -0.40f)
					state->vault_run = 0;
				else
				{
					/* run at the lip before leaving the ground. A jump
					from a stand does not carry them onto the edge. */
					action.throttle.i = 1.0f;
					state->vault_run++;
					if (state->vault_run >= 8 &&
						!bot_ray_clear_z(&feet, state->vault_yaw, 0.80f, 0.40f,
							player->unit_index))
					{
						action.control_flags |= FLAG(_unit_control_jump_bit);
						state->vault = 2;
						state->vault_ticks = 0;
						state->vault_air = FALSE;
					}
				}
				if (state->vault == 1 && state->vault_ticks > 70)
					state->vault = 0;
			}
			else if (state->vault == 2)
			{
				state->vault_ticks++;
				action.throttle.i = 1.0f;
				action.throttle.j = 0.0f;
				if (airborne)
					state->vault_air = TRUE;
				if (state->vault_air && airborne &&
					biped->object.translational_velocity.k <= 0.025f)
					crouch = TRUE;
				if ((state->vault_air && !airborne) || state->vault_ticks > 90)
					state->vault = 0;
			}
			if (biped && airborne && biped->object.translational_velocity.k < 0.0f &&
				-biped->object.translational_velocity.k > bot_harmful_fall_speed())
			{
				real drop;
				real_point3d from;
				real_point3d to;

				drop = -biped->object.translational_velocity.k * 6.0f;
				if (drop < 0.45f)
					drop = 0.45f;
				if (drop > 1.1f)
					drop = 1.1f;
				from = feet;
				from.z += 0.05f;
				to = from;
				to.z -= drop;
				if (!bot_structure_clear(&from, &to, player->unit_index))
					crouch = TRUE;
			}
			if (crouch)
				action.control_flags |= FLAG(_unit_control_crouch_modifier_bit);
			/* the light stays off. A bot never switches it on. */
			if (TEST_FLAG(unit->unit.flags, _unit_integrated_light_on_bit))
				unit_set_desired_flashlight_state(player->unit_index, FALSE);
		}

		bot_use_ladder(state, player, player->unit_index, &feet, engage, &action);
		bot_respect_grenades(state, player, player->unit_index, &feet, &action);
		/* the jump is already in the air, or it was just pressed. Keep the
		stick forward along that heading until the landing. A new step
		here is what drops them short of the platform. */
		if (state->gap_hold)
		{
			struct biped_datum *jumping = biped_try_and_get(player->unit_index);
			boolean left_ground = jumping &&
				TEST_FLAG(jumping->biped.flags, _biped_airborne_bit);
			real_point3d ahead;

			if (left_ground)
				state->gap_left = TRUE;
			if (!left_ground && state->gap_left)
			{
				state->gap_hold = FALSE;
				state->gap_left = FALSE;
				state->gap_ticks = 0;
			}
			else if (!left_ground && !state->gap_left &&
				bot_drop_ahead(&feet, state->gap_yaw) &&
				!bot_jump_across(&feet, state->gap_yaw, player->unit_index, NULL, NULL))
			{
				/* still on the ground, and the landing is gone. Holding
				forward here is the run off the map. */
				state->gap_hold = FALSE;
				state->gap_left = FALSE;
				state->gap_ticks = -20;
				action.throttle.i = 0.0f;
				action.throttle.j = 0.0f;
			}
			else if (!left_ground && state->gap_ticks > 8)
			{
				state->gap_hold = FALSE;
				state->gap_ticks = 0;
			}
			else
			{
				if (!left_ground)
					state->gap_ticks++;
				state->facing_yaw = state->gap_yaw;
				state->facing_pitch = 0.0f;
				ahead.x = eye.x + cosine(state->gap_yaw);
				ahead.y = eye.y + sine(state->gap_yaw);
				ahead.z = eye.z;
				bot_face(player, &action, &eye, &ahead, 0.0f, 0.0f, NULL);
				action.throttle.i = 1.0f;
				action.throttle.j = 0.0f;
			}
		}
		if (state->gap_hold)
			state->escape_ticks = 0;
		else if (state->escape_ticks > 0)
		{
			real rel = bot_angle_delta(state->facing_yaw, state->escape_yaw);
			real_point3d ahead;

			state->escape_ticks--;
			state->stall_ticks = 0;
			state->facing_yaw = bot_slew(state->facing_yaw, state->escape_yaw,
				bot_look_step(skill, FALSE) * 2.0f);
			ahead.x = eye.x + cosine(state->facing_yaw);
			ahead.y = eye.y + sine(state->facing_yaw);
			ahead.z = eye.z;
			bot_face(player, &action, &eye, &ahead, 0.0f, 0.0f, NULL);
			action.throttle.i = cosine(rel) > 0.15f ? 1.0f : 0.0f;
			action.throttle.j = -sine(rel);
		}
		if (state->gap_hold || state->vault != 0 || state->job_climb || planted ||
			state->gap_ticks > 0 || state->escape_ticks > 0 || state->move_thwarted)
			state->stall_ticks = 0;
		else if (action.throttle.i == 0.0f && action.throttle.j == 0.0f)
			state->stall_ticks++;
		else
			state->stall_ticks = 0;
		/* a second with no step. Take an open heading on this floor. */
		if (state->stall_ticks >= TICKS_PER_SECOND && !state->gap_hold &&
			state->escape_ticks == 0)
		{
			real away;

			state->stall_ticks = 0;
			if (bot_escape_yaw(&feet, state->facing_yaw + 1.2f, player->unit_index, &away))
			{
				real rel = bot_angle_delta(state->facing_yaw, away);
				real_point3d ahead;

				state->escape_yaw = away;
				state->escape_ticks = 20;
				state->facing_yaw = bot_slew(state->facing_yaw, away, bot_look_step(skill, FALSE) * 2.0f);
				state->commit_yaw = away;
				state->commit_ticks = 25;
				state->route.valid = FALSE;
				state->wander_yaw = away;
				ahead.x = eye.x + cosine(state->facing_yaw);
				ahead.y = eye.y + sine(state->facing_yaw);
				ahead.z = eye.z;
				bot_face(player, &action, &eye, &ahead, 0.0f, 0.0f, NULL);
				action.throttle.i = cosine(rel) > 0.15f ? 1.0f : 0.0f;
				action.throttle.j = -sine(rel);
			}
		}
		/* the grenade leaves along the aim from the tick it was chosen.
		a flee or a new step during the wind-up is the turn as it releases. */
		if (state->grenade_aim)
		{
			real_point3d held;

			state->escape_ticks = 0;
			state->facing_yaw = state->grenade_yaw;
			state->facing_pitch = state->grenade_pitch;
			held.x = eye.x + cosine(state->facing_yaw) * cosine(state->facing_pitch);
			held.y = eye.y + sine(state->facing_yaw) * cosine(state->facing_pitch);
			held.z = eye.z + sine(state->facing_pitch);
			bot_face(player, &action, &eye, &held, 0.0f, 0.0f, NULL);
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

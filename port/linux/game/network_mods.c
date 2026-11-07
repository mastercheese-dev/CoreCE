/* Server Setup's MODS, ALLOWED or NOT ALLOWED (network.mods_allowed).
The host plays the map's own tags when mods are not allowed, and tells
every client. A client answers. A machine that never answers is dropped,
so an older client cannot keep its mods in that game. Allowing mods does
not drop anyone. The network version stays as it is. */
#include "cseries.h"
#include "errors.h"
#include "game/game.h"
#include "mods.h"
#include "network_distributed.h"
#include "network_mods.h"
#include <string.h>

int config_boolean(char const *name);
short network_distributed_server_machines(long *machine_indices, short maximum);
void network_game_server_kick_machine(long machine_index, boolean kept_out);
boolean network_connection_last_read_was_unreliable(void);
unsigned long system_milliseconds(void);
short network_game_client_get_local_machine_index(void);

/* the same bound as HALO_PORT_MAXIMUM_NETWORK_MACHINES */
#define MODS_MACHINES 128
/* ask again, then drop a machine that still has not answered */
#define MODS_RETRY_MILLISECONDS 2000
#define MODS_DROP_MILLISECONDS 5000

struct mods_policy_message
{
	struct distributed_message_header header;
	word allowed;
	word reserved;
};
typedef char mods_policy_message_size_assert[sizeof(struct mods_policy_message) == 12 ? 1 : -1];

/* NONE until this host has sent a choice. 1 allowed, 0 not. */
static short sent_allowed = NONE;
static boolean heard = FALSE;
static boolean host_forbids = FALSE;
/* this match turned mods off, so leaving turns them back on */
static boolean forced_off = FALSE;
static byte acked[MODS_MACHINES];
static unsigned long asked_ms[MODS_MACHINES];
static unsigned long retry_ms[MODS_MACHINES];

static boolean network_mods_forbidden(void)
{
	short connection = game_connection();

	if (connection == _game_connection_network_server)
		return config_boolean("network.mods_allowed") == 0;
	if (connection == _game_connection_network_client && heard)
		return host_forbids;
	return FALSE;
}

static void network_mods_enforce(void)
{
	if (!mods_map_loaded())
		return;
	if (network_mods_forbidden())
	{
		mods_enable(FALSE);
		forced_off = TRUE;
		return;
	}
	if (forced_off)
	{
		mods_enable(TRUE);
		forced_off = FALSE;
	}
}

static void network_mods_left(void)
{
	heard = FALSE;
	host_forbids = FALSE;
	sent_allowed = NONE;
	if (forced_off && mods_map_loaded())
		mods_enable(TRUE);
	forced_off = FALSE;
}

static void network_mods_send(long machine_index, int allowed)
{
	struct mods_policy_message message;
	unsigned long now;

	if (machine_index < 0 || machine_index >= MODS_MACHINES)
		return;
	memset(&message, 0, sizeof(message));
	message.allowed = allowed ? 1 : 0;
	distributed_send_to_machine_reliably(machine_index, &message,
		_distributed_message_mods_policy, 1, sizeof(message));
	now = system_milliseconds();
	if (!asked_ms[machine_index])
		asked_ms[machine_index] = now ? now : 1;
	retry_ms[machine_index] = now;
}

static void network_mods_broadcast(int allowed)
{
	struct mods_policy_message message;

	memset(&message, 0, sizeof(message));
	message.allowed = allowed ? 1 : 0;
	distributed_send(&message, _distributed_message_mods_policy, 1,
		sizeof(message), _distributed_to_clients_reliably);
}

static void network_mods_clear_answers(void)
{
	short index;

	for (index = 0; index < MODS_MACHINES; index++)
	{
		acked[index] = 0;
		asked_ms[index] = 0;
		retry_ms[index] = 0;
	}
}

void network_mods_reset(void)
{
	sent_allowed = NONE;
	network_mods_clear_answers();
}

void network_mods_map_loaded(void)
{
	network_mods_enforce();
}

void network_mods_send_policy(long machine_index)
{
	int allowed;

	if (game_connection() != _game_connection_network_server)
		return;
	allowed = config_boolean("network.mods_allowed") != 0;
	sent_allowed = (short)allowed;
	if (machine_index >= 0 && machine_index < MODS_MACHINES)
	{
		acked[machine_index] = 0;
		asked_ms[machine_index] = 0;
	}
	network_mods_send(machine_index, allowed);
}

void network_mods_update(void)
{
	short connection;
	int allowed;
	long machines[MODS_MACHINES];
	short count;
	short index;
	unsigned long now;

	connection = game_connection();
	if (connection != _game_connection_network_server)
	{
		if (connection != _game_connection_network_client)
			network_mods_left();
		else
			network_mods_enforce();
		return;
	}
	allowed = config_boolean("network.mods_allowed") != 0;
	if (sent_allowed != (short)allowed)
	{
		sent_allowed = (short)allowed;
		network_mods_clear_answers();
		network_mods_broadcast(allowed);
	}
	network_mods_enforce();
	if (allowed)
		return;
	now = system_milliseconds();
	count = network_distributed_server_machines(machines, MODS_MACHINES);
	for (index = 0; index < count; index++)
	{
		long machine = machines[index];

		if (machine < 0 || machine >= MODS_MACHINES || acked[machine] ||
			machine == network_game_client_get_local_machine_index())
			continue;
		if (!asked_ms[machine] || now - retry_ms[machine] >= MODS_RETRY_MILLISECONDS)
			network_mods_send(machine, allowed);
		if (asked_ms[machine] && now - asked_ms[machine] >= MODS_DROP_MILLISECONDS)
		{
			error(_error_log, "machine #%ld did not confirm mods are off: dropped", machine);
			network_game_server_kick_machine(machine, FALSE);
			acked[machine] = 1;
		}
	}
}

boolean network_mods_handles_message(word const *message, word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	memcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_mods_policy ||
		header.type == _distributed_message_mods_ack;
}

void network_mods_handle_message(long machine_index, word const *data, word size)
{
	struct mods_policy_message message;

	if (network_connection_last_read_was_unreliable() || size != sizeof(message))
		return;
	memcpy(&message, data, sizeof(message));
	if (GET_MESSAGE_TYPE(message.header.header) != 2 || message.header.count != 1 ||
		message.allowed > 1 || message.reserved != 0)
		return;
	if (message.header.type == _distributed_message_mods_policy)
	{
		struct mods_policy_message answer;

		/* a client, and only from the host. another machine cannot set it. */
		if (machine_index != NONE || game_connection() != _game_connection_network_client)
			return;
		heard = TRUE;
		host_forbids = message.allowed == 0;
		network_mods_enforce();
		memset(&answer, 0, sizeof(answer));
		answer.allowed = message.allowed;
		distributed_send(&answer, _distributed_message_mods_ack, 1, sizeof(answer),
			_distributed_to_host_reliably);
		return;
	}
	if (message.header.type != _distributed_message_mods_ack ||
		game_connection() != _game_connection_network_server ||
		machine_index < 0 || machine_index >= MODS_MACHINES)
		return;
	if (sent_allowed == 0 || sent_allowed == 1)
	{
		if (message.allowed == (word)sent_allowed)
			acked[machine_index] = 1;
	}
}

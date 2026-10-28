/*
Bots added from the lobby. A seat is a controller index on a machine, not a
pad: the player who added the bot keeps theirs. Indices 0 to 3 stay the
machine's own players. A bot uses 4 and up, which still fits in the signed
char the player record stores.
*/

#ifndef __BOTS_H
#define __BOTS_H
#pragma once

#define BOT_CONTROLLER_LIMIT 128

void bot_controller_set(
	short machine_index,
	short controller_index,
	long skill);
void bot_controller_clear(
	short machine_index,
	short controller_index);
boolean bot_controller_is_bot(
	short machine_index,
	short controller_index);
/* 0 easy, 1 normal, 2 heroic, 3 legendary, 4 impossible.
NONE when that seat is not a bot */
#define BOT_SKILL_IMPOSSIBLE 4
#define BOT_SKILL_COUNT 5
long bot_controller_skill(
	short machine_index,
	short controller_index);
/* kept for the pickup check. This build's bots take every gun, including a needler. */
boolean bot_refuses_weapon(
	long unit_index,
	long weapon_index);

#endif

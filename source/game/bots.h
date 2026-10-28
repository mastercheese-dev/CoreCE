/*
Bots added from the lobby. A seat is a controller index on a machine, not a
pad: the player who added the bot keeps theirs.
*/

#ifndef __BOTS_H
#define __BOTS_H
#pragma once

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
/* 0 to 3, or NONE when that seat is not a bot */
long bot_controller_skill(
	short machine_index,
	short controller_index);

#endif

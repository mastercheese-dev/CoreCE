#ifndef NETWORK_MODS_H
#define NETWORK_MODS_H
#include "cseries.h"
/* The host's MODS choice on Server Setup (network.mods_allowed). A game that
does not allow mods plays the map's own tags on every machine. */
void network_mods_reset(void);
void network_mods_update(void);
void network_mods_send_policy(long machine_index);
/* a map's tags just loaded: apply the choice before anything draws them */
void network_mods_map_loaded(void);
boolean network_mods_handles_message(word const *message, word size);
void network_mods_handle_message(long machine_index, word const *message, word size);
#endif

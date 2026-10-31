/*
MATCHMAKING.H

The matchmaking service (port/linux/src/matchmaking.c). A game it sets up
listens on the port it names. Ordinary games stay on the usual port.
*/

#ifndef __HALO_LINUX_MATCHMAKING_H
#define __HALO_LINUX_MATCHMAKING_H

enum
{
	MATCHMAKING_ROLE_NONE = 0,
	MATCHMAKING_ROLE_WAIT,
	MATCHMAKING_ROLE_SERVER,
	MATCHMAKING_ROLE_HOST,
	MATCHMAKING_ROLE_JOIN,
	MATCHMAKING_ROLE_ERROR,
	MATCHMAKING_MODE_SLAYER = 0,
	MATCHMAKING_MODE_ODDBALL,
	MATCHMAKING_MODE_KING,
	MATCHMAKING_MODE_CTF
};

/* the offer's rules, already filled with this match's values */
struct matchmaking_rules
{
	int mode;
	long respawn_ticks;
	long respawn_growth_ticks;
	long suicide_penalty_ticks;
	int motion_sensor;
	long goal_radar;
	/* the game's radar_players values */
	int radar_players;
	/* the game's loadout weapon values */
	int primary;
	int secondary;
	long time_limit_ticks;
	long score;
	int no_death_bonus;
	int no_kill_penalty;
	int teams;
	int moving_hill;
	long ball_spawn_count;
};

struct matchmaking_offer
{
	int role;
	unsigned short port;
	char invite[256];
	char map[64];
	char name[32];
	char playlist[16];
	int players;
	int teams;
	int team_size;
	/* the network version the service accepts, when it refuses this build */
	int network;
	struct matchmaking_rules rules;
};

/* one catalog row. the menu is these, not a list kept in the game. */
struct matchmaking_playlist
{
	char id[16];
	char name[32];
	int players;
	int teams;
	int team_size;
	unsigned short port;
	char modes[4][16];
	int mode_count;
};

struct matchmaking_catalog
{
	int network;
	int count;
	struct matchmaking_playlist playlist[8];
};

/* the menu is open. subscribe so the catalog can fill the rows. */
void matchmaking_watch(void);
/* 1 when the catalog is this build's network, 0 while it is still arriving,
-1 when the service is on another network (catalog is still filled in). */
int matchmaking_catalog(struct matchmaking_catalog *catalog);
/* start publishing this playlist. name is the gamertag, 11 characters at most.
publishing continues until the player has joined, or leaves. */
void matchmaking_queue(const char *playlist, const char *name);
/* the player left the menu. a match already taken keeps its port. */
void matchmaking_leave(void);
/* back at the main menu: the queue and the port are done */
void matchmaking_reset(void);
/* this machine is the fallback host. publish the invite once it exists. */
void matchmaking_commit_host(const char *playlist, unsigned short port);
/* the invite has been joined. stop the queue and keep the port. */
void matchmaking_commit_join(void);
/* 1 when a new offer is waiting */
int matchmaking_take_offer(struct matchmaking_offer *offer);
/* 1 connected, 0 still trying, -1 the service could not be reached */
int matchmaking_link(void);
/* the port a matchmaking game is using, or 0 for an ordinary game */
unsigned short matchmaking_game_port(void);
void matchmaking_set_game_port(unsigned short port);

#endif

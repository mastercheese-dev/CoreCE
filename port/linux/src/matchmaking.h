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

enum
{
	MATCHMAKING_LOBBY_NAMES = 16,
	MATCHMAKING_LOBBY_NAME_LENGTH = 12
};

enum
{
	MATCHMAKING_PHASE_NONE = 0,
	MATCHMAKING_PHASE_PARTY,
	MATCHMAKING_PHASE_SEARCH
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
	/* how many seats this match has */
	int players;
	/* how many players are in the lobby now. -1 when the offer did not say */
	int joined;
	/* seconds the server will still look for parties. 0 means the roster is
	full. -1 means there is no countdown. */
	int wait_seconds;
	/* party is the private lobby. search is after Find Game. */
	int phase;
	/* local players this machine may add, before the other seats are counted */
	int local_max;
	int name_count;
	char names[MATCHMAKING_LOBBY_NAMES][MATCHMAKING_LOBBY_NAME_LENGTH];
	/* which party each name is in. -1 when that name is not in a party.
	party 0 and party 1 are the two teams. */
	int name_party[MATCHMAKING_LOBBY_NAMES];
	/* how many parties the offer listed. 0 means it did not list any. */
	int party_count;
	/* seats to fill once the roster is set. -1 when the offer did not say.
	0 means leave the seats empty. */
	int bots;
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
	int local_max;
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
ready is false and locals is 1 until the lobby changes them. publishing
continues through the private lobby until the match starts or the player leaves. */
void matchmaking_queue(const char *playlist, const char *name);
/* how many local players are in this party, 1 to 4. ignored once Find Game is pressed. */
void matchmaking_set_locals(int locals);
/* Find Game: ready becomes true and publishing continues. */
void matchmaking_find_game(void);
/* 1 while this machine is publishing a queue */
int matchmaking_queue_active(void);
/* 1 after Find Game */
int matchmaking_is_ready(void);
/* the latest offer, including ones already taken. 1 when there is one. */
int matchmaking_offer_copy(struct matchmaking_offer *offer);
/* the player left the menu. a party already in its lobby keeps publishing. */
void matchmaking_leave(void);
/* back at the main menu, or out of the lobby: the queue and the port are done */
void matchmaking_reset(void);
/* the match has started. stop publishing and clear the retained queue. the port stays. */
void matchmaking_stop(void);
/* this machine is the fallback host. publish the invite once it exists. */
void matchmaking_commit_host(const char *playlist, unsigned short port);
/* the private lobby has been joined. keep publishing and keep the port. */
void matchmaking_commit_join(void);
/* this machine opened its own party lobby. keep publishing and keep the port. */
void matchmaking_commit_party(void);
/* a party opened before the service names a match. catalog row supplies the port. */
void matchmaking_party_offer(struct matchmaking_offer *offer, const struct matchmaking_playlist *row);
/* a join link that is not this party's. a machine already publishing stops,
so a friend on someone else's link does not keep a queue. */
void matchmaking_yield_to_invite(const char *text);
/* 1 when a new offer is waiting */
int matchmaking_take_offer(struct matchmaking_offer *offer);
/* 1 connected, 0 still trying, -1 the service could not be reached */
int matchmaking_link(void);
/* the port a matchmaking game is using, or 0 for an ordinary game */
unsigned short matchmaking_game_port(void);
void matchmaking_set_game_port(unsigned short port);
/* 0 or 1 for a matchmaking team game, -1 otherwise. a party stays together.
without a parties list, every human in this game shares team 0. */
int matchmaking_join_team(const wchar_t *name);
/* how long the lobby waits before it starts, in milliseconds. 0 means start
now. the offer's wait_seconds is that wait. when the offer did not say, this
is the long lobby wait. */
unsigned long matchmaking_countdown_ms(void);
/* how many seats to fill. -1 when this is not a matchmaking team game.
other_team_only is 1 while the lobby is still open, and the count is then
how many of those seats go on team 1. humans is how many people are in now. */
int matchmaking_filler_count(int final, int humans, int *team_size, int *other_team_only);

#endif

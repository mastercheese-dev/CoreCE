/* Hardcore rules that are not fields of the Xbox variant. Classic games
never set the anniversary bit, so none of this runs for them. The bytes ride
a data message an older build ignores. */
#include "cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_globals.h"
#include "game/cheats.h"
#include "game/players.h"
#include "game/players_runtime.h"
#include "scenario/scenario_definitions.h"
#include "game/performance_variant.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "interface/hud_messaging.h"
#include "interface/ui_widget.h"
#include "interface/progress_bar.h"
#include "cseries/cseries_windows.h"
#include "memory/data.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/common/rasterizer_common.h"
#include "render/render.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "units/units.h"
#include "halo_port_limits.h"
#include "performance_options.h"
#include "hardcore_rules.h"
#include "opening_film.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "cache/cache_files.h"
#include "game/bots.h"
#include "game/bot_nav.h"
#include "game/game_engine_race.h"
#include "hardcore_timer.h"
#include "tag_files/files.h"
#include "saved games/saved_game_files.h"
#include "saved games/playlist_profile.h"

#include <stdio.h>
#include <string.h>

struct network_game *network_game_server_get_game(struct network_game_server *server);

#define HARDCORE_RULES_VERSION 3
#define HARDCORE_RULES_MESSAGE_SIZE 35
#define HARDCORE_SKIP_SLOTS HALO_PORT_MAXIMUM_OBJECTS_PER_MAP

#ifdef HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#define HARDCORE_PLAYER_SLOTS HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#else
#define HARDCORE_PLAYER_SLOTS 16
#endif

static struct hardcore_rules hardcore_edit;
static struct hardcore_rules hardcore_live;
static unsigned char hardcore_skip_armed[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_press[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_seen[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_weapon[HARDCORE_SKIP_SLOTS];
static int hardcore_random_percent;
static int hardcore_cheats_written;
/* set when this match is started. cleared when a classic mode is chosen,
   and when the host's match variant is a classic one. the lobby's live
   variant is still the previous map, so the bit alone is not on yet when
   the next map loads. */
static int hardcore_session;
/* A joined client applies the host's sheet only after that message arrives.
   Its own editor must not replace those bytes. */
static int hardcore_from_host;
/* The sheet last loaded for a game type, and Server Setup's copy of it. */
static struct hardcore_rules hardcore_loaded;
static int hardcore_loaded_valid;
static struct hardcore_rules hardcore_setup;
static int hardcore_setup_on;

static void hardcore_rules_kept_only(struct hardcore_rules *rules);
static boolean hardcore_remote_client(void);
/* The opening ring. The clock does not advance while it is up. */
static int opening_hold;
static int opening_release;
static int opening_fading;
static int opening_go;
static int opening_fade_ticks;
static unsigned long opening_started_ms;
static unsigned long opening_played_ms;
static unsigned long opening_last_present_ms;
static unsigned long opening_fade_from_ms;
static long opening_deferred_sound = NONE;

/* The film plays on presented frames, one frame at a time, so a hitch
holds the picture instead of skipping it. A local game fades once the
world is ready and the film has reached its last moment. A network game
fades when the host says the worlds match, and the film's tail plays
across that fade. The fade is 12 ticks, the old 400ms, so every machine
that shares the host's update finishes together. A stuck local spawn
still ends. A network game does not start on that timer. */
#define OPENING_RING_MS 3000UL
#define OPENING_FADE_MS 400UL
#define OPENING_FADE_TICKS 12
#define OPENING_READY_WAIT_MS 60000UL

static int hardcore_anniversary(void)
{
	struct game_variant const *variant = game_engine_get_variant();

	return variant && TEST_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit);
}

void hardcore_rules_defaults(struct hardcore_rules *rules)
{
	hardcore_rules_stock(rules);
	/* The hardcore sheet's counters. A normal game type stays at stock. */
	rules->team_scores = 1;
}

void hardcore_rules_stock(struct hardcore_rules *rules)
{
	csmemset(rules, 0, sizeof(*rules));
	rules->hill_move_s = 60;
	rules->movement_sounds = 1;
	/* fading is the retail drain. the byte stays so its place does not move. */
	rules->os_type = 1;
}

boolean hardcore_rules_is_stock(struct hardcore_rules const *rules)
{
	struct hardcore_rules stock;

	hardcore_rules_stock(&stock);
	return rules && csmemcmp(rules, &stock, sizeof(stock)) == 0;
}

void hardcore_rules_note_variant(struct game_variant *variant)
{
	if (!variant)
		return;
	/* A removed row is not a match option, so it must not mark the variant. */
	hardcore_rules_kept_only(&hardcore_edit);
	SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit,
		!hardcore_rules_is_stock(&hardcore_edit));
}

static boolean hardcore_rules_file(long profile_index, char *path)
{
	char directory[MAXIMUM_FILENAME_LENGTH + 1];
	long length;

	path[0] = 0;
	if (!saved_game_file_get_path_to_enclosing_directory(profile_index, directory))
		return FALSE;
	length = csstrlen(directory);
	if (length <= 0 || length + 12 >= MAXIMUM_FILENAME_LENGTH)
		return FALSE;
	csstrncpy(path, directory, MAXIMUM_FILENAME_LENGTH);
	path[MAXIMUM_FILENAME_LENGTH] = 0;
	csstrncat(path, "match.rules", MAXIMUM_FILENAME_LENGTH - length);
	return TRUE;
}

void hardcore_rules_store(long profile_index)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	struct file_reference file;

	if (!hardcore_rules_file(profile_index, path))
		return;
	if (!file_reference_create_from_path(&file, path, FALSE) || !file_create(&file) ||
		!file_open(&file, FLAG(_permission_write_bit)) || !file_set_position(&file, 0))
		return;
	/* The file keeps the six rules and this machine's field of view. A removed row must not come back on load. */
	hardcore_rules_kept_only(&hardcore_edit);
	file_write(&file, sizeof(hardcore_edit), &hardcore_edit);
	file_close(&file);
	hardcore_loaded = hardcore_edit;
	hardcore_loaded_valid = 1;
}

void hardcore_rules_forget(long profile_index)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	struct file_reference file;

	if (!hardcore_rules_file(profile_index, path))
		return;
	if (file_reference_create_from_path(&file, path, FALSE) && file_exists(&file))
		file_delete(&file);
}

void hardcore_rules_choose(long profile_index)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	struct file_reference file;
	struct game_variant variant;
	struct hardcore_rules loaded;
	boolean opened = FALSE;

	hardcore_rules_stock(&hardcore_edit);
	if (hardcore_rules_file(profile_index, path) &&
		file_reference_create_from_path(&file, path, FALSE) && file_exists(&file) &&
		file_open(&file, FLAG(_permission_read_bit)))
	{
		opened = TRUE;
		if (file_read(&file, sizeof(loaded), &loaded))
			hardcore_edit = loaded;
	}
	if (opened)
		file_close(&file);
	if (!opened && playlist_profile_get(profile_index, &variant) &&
		!ustrcmp(variant.human_readable_game_description, L"Comp 2v2"))
		hardcore_edit.team_scores = 1;
	hardcore_rules_kept_only(&hardcore_edit);
	/* A client in someone else's lobby still opens game types. That
	   editor is local. The match keeps the host's sheet. */
	if (!hardcore_remote_client())
		hardcore_live = hardcore_edit;
	hardcore_session = 0;
	if (playlist_profile_get(profile_index, &variant) &&
		TEST_FLAG(variant.universal_variant.flags, _game_variant_anniversary_rules_bit))
		hardcore_session = 1;
	hardcore_loaded = hardcore_edit;
	hardcore_loaded_valid = 1;
}

boolean hardcore_rules_changed(void)
{
	struct hardcore_rules current;
	struct hardcore_rules loaded;

	if (!hardcore_loaded_valid)
		return FALSE;
	current = hardcore_edit;
	loaded = hardcore_loaded;
	hardcore_rules_kept_only(&current);
	hardcore_rules_kept_only(&loaded);
	return csmemcmp(&current, &loaded, sizeof(current)) != 0;
}

void hardcore_rules_setup_begin(void)
{
	if (hardcore_setup_on)
		return;
	hardcore_rules_kept_only(&hardcore_edit);
	hardcore_setup = hardcore_edit;
	hardcore_setup_on = 1;
}

void hardcore_rules_setup_end(void)
{
	hardcore_setup_on = 0;
}

boolean hardcore_rules_setup_active(void)
{
	return hardcore_setup_on ? TRUE : FALSE;
}

struct hardcore_rules *hardcore_rules_setup(void)
{
	return &hardcore_setup;
}

void hardcore_rules_setup_apply(struct game_variant *variant)
{
	if (!hardcore_setup_on)
	{
		hardcore_rules_clear_session();
		if (variant)
			SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
		return;
	}
	hardcore_rules_kept_only(&hardcore_setup);
	hardcore_edit = hardcore_setup;
	if (hardcore_rules_is_stock(&hardcore_edit))
	{
		hardcore_rules_clear_session();
		if (variant)
			SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
		return;
	}
	if (variant)
		hardcore_rules_note_variant(variant);
	hardcore_rules_commit();
}

/* the menu's one switch for the pads and the volumes. an older sheet may
   have set only one of them. the percent is on only while that switch is. */
static void hardcore_training_pair(struct hardcore_rules *rules)
{
	unsigned char on = (rules->spawns_visible || rules->random_areas) ? 1 : 0;

	rules->spawns_visible = on;
	rules->random_areas = on;
	/* The percent is drawn only while training is on, and it must not
	   keep a sheet off retail after training is turned off. */
	rules->random_pct = on ? 1 : 0;
}

/* The rows that left the menu. Their bytes stay in the message so its
   layout does not move, and they are the retail sheet: a zero used to
   mean manual pickup and ordered hills, which a match option must not
   turn on. The six kept rules are the ones a host enforces. The field of
   view stays on this machine. */
static void hardcore_rules_kept_only(struct hardcore_rules *rules)
{
	struct hardcore_rules stock;
	unsigned char movement;
	unsigned char training;
	unsigned char camo;
	unsigned char scores;
	unsigned char pistol;
	unsigned char shotgun;
	unsigned char fov;

	if (!rules)
		return;
	movement = rules->movement_sounds ? 1 : 0;
	training = (rules->spawns_visible || rules->random_areas) ? 1 : 0;
	camo = rules->camo > 2 ? 0 : rules->camo;
	scores = rules->team_scores ? 1 : 0;
	pistol = rules->pistol_spread > 3 ? 0 : rules->pistol_spread;
	shotgun = rules->shotgun_spread > 3 ? 0 : rules->shotgun_spread;
	fov = rules->forced_fov;
	if (fov != 0 && (fov < 80 || fov > 150 || ((fov - 80) % 5) != 0))
		fov = 0;
	hardcore_rules_stock(&stock);
	*rules = stock;
	rules->movement_sounds = movement;
	rules->spawns_visible = training;
	rules->random_areas = training;
	rules->camo = camo;
	rules->team_scores = scores;
	rules->pistol_spread = pistol;
	rules->shotgun_spread = shotgun;
	rules->forced_fov = fov;
	hardcore_training_pair(rules);
}

/* This machine joined someone else's game. The host's own client is not
   one: it keeps the sheet it will send. */
static boolean hardcore_remote_client(void)
{
	return global_network_game_server_get() == NULL &&
		global_network_game_client_get() != NULL;
}

struct hardcore_rules *hardcore_rules_editing(void)
{
	return &hardcore_edit;
}

void hardcore_rules_reset_editing(void)
{
	hardcore_rules_defaults(&hardcore_edit);
}

void hardcore_rules_commit(void)
{
	hardcore_rules_kept_only(&hardcore_edit);
	if (hardcore_remote_client())
		return;
	hardcore_live = hardcore_edit;
	hardcore_session = 1;
}

void hardcore_rules_clear_session(void)
{
	hardcore_session = 0;
}

/* The sheet from a game this machine joined. The editor still has the
   one saved with its game type, and that is the sheet a game it hosts uses. */
void hardcore_rules_restore_local(void)
{
	hardcore_from_host = 0;
	hardcore_rules_kept_only(&hardcore_edit);
	hardcore_live = hardcore_edit;
}

/* port: the lobby's gametype is a classic one. the sheet from the mode
   stops with it, including a session the menu had not cleared. */
void hardcore_rules_host_variant(struct game_variant *variant)
{
	struct game_variant *live;

	if (variant && TEST_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit))
		return;
	hardcore_session = 0;
	live = game_engine_get_variant();
	if (live)
		SET_FLAG(live->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
}

boolean hardcore_rules_active(void)
{
	/* port: the live multiplayer variant's bit. a leftover session must
	   not change a classic game, co-op, or the campaign. a joined client
	   waits for the host's sheet, and does not play its own. */
	if (!game_engine_running() || !hardcore_anniversary())
		return FALSE;
	if (hardcore_remote_client() && !hardcore_from_host)
		return FALSE;
	return TRUE;
}

boolean hardcore_rules_random_areas(void)
{
	return hardcore_rules_active() && hardcore_live.random_areas ? TRUE : FALSE;
}

const struct hardcore_rules *hardcore_rules_live(void)
{
	return &hardcore_live;
}

unsigned char hardcore_rules_local_fov(void)
{
	unsigned char fov;

	if (!hardcore_rules_active() || hardcore_remote_client())
		return 0;
	fov = hardcore_live.forced_fov;
	if (fov != 0 && (fov < 80 || fov > 150 || ((fov - 80) % 5) != 0))
		return 0;
	return fov;
}

static void hardcore_camo_write(int refraction, int falloff, int red, int green, int blue)
{
	real tint_red = (real)red * (1.0f / 255.0f);
	real tint_green = (real)green * (1.0f / 255.0f);
	real tint_blue = (real)blue * (1.0f / 255.0f);

	/* both ends of the fade, or a strong cloak slides back to the tag */
	global_rasterizer_data->active_camouflage_refraction_amount = (real)refraction;
	global_rasterizer_data->active_camouflage_distance_falloff = (real)falloff;
	global_rasterizer_data->active_camouflage_tint_color.red = tint_red;
	global_rasterizer_data->active_camouflage_tint_color.green = tint_green;
	global_rasterizer_data->active_camouflage_tint_color.blue = tint_blue;
	global_rasterizer_data->active_camouflage_hyper_stealth_refraction_amount = (real)refraction;
	global_rasterizer_data->active_camouflage_hyper_stealth_distance_falloff = (real)falloff;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.red = tint_red;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.green = tint_green;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.blue = tint_blue;
}

static void hardcore_apply_camo(void)
{
	if (!global_rasterizer_data || hardcore_live.camo == 0)
		return;
	/* mcc hardcore is 9, 9 and white. custom is the preset 11, 22 and
	   the pale tint. xbox leaves the tag, which is that pale look on a
	   stock map and something else on a map that changed it. */
	if (hardcore_live.camo == 1)
		hardcore_camo_write(9, 9, 255, 255, 255);
	else
		hardcore_camo_write(11, 22, 215, 214, 233);
}

static void hardcore_apply_presentation(void)
{
	unsigned long flags = performance_options_get_flags();

	/* The sheet is the authority. A later match that turns these off has to
	clear the bits the previous one set, on this machine and on a client. */
	if (hardcore_live.spawns_visible)
		flags |= _performance_option_spawn_markers;
	else
		flags &= ~_performance_option_spawn_markers;
	if (!hardcore_live.movement_sounds)
		flags |= _performance_option_silent_movement;
	else
		flags &= ~_performance_option_silent_movement;
	performance_options_apply_host_flags(flags);
	hardcore_apply_camo();
}

/* port: the menu's map list is built again after a match. defined with
   the menus (menu_functions.c). */
void hardcore_list_match_started(void);

void hardcore_rules_begin_map(void)
{
	/* Hosting. The sheet from a game this machine joined is not sent. */
	if (global_network_game_server_get() && hardcore_from_host)
		hardcore_rules_restore_local();
	if (opening_hold)
		progress_bar_end();
	opening_hold = FALSE;
	opening_release = FALSE;
	opening_fading = FALSE;
	opening_go = FALSE;
	opening_fade_ticks = 0;
	opening_started_ms = 0;
	opening_played_ms = 0;
	opening_last_present_ms = 0;
	opening_fade_from_ms = 0;
	opening_deferred_sound = NONE;
	hardcore_list_match_started();
	csmemset(hardcore_skip_armed, 0, sizeof(hardcore_skip_armed));
	csmemset(hardcore_shot_press, 0, sizeof(hardcore_shot_press));
	csmemset(hardcore_shot_seen, 0, sizeof(hardcore_shot_seen));
	csmemset(hardcore_shot_weapon, 0xFF, sizeof(hardcore_shot_weapon));
	hardcore_random_percent = -1;
	if (hardcore_cheats_written)
	{
		cheat.bottomless_clip = FALSE;
		cheat.infinite_ammo = FALSE;
		hardcore_cheats_written = 0;
	}
	/* port: the menu, co-op and the campaign have no multiplayer engine.
	   the last match's bit stays on that variant and would change their
	   shots. a multiplayer map has already copied its own variant. */
	if (!game_engine_running())
	{
		struct game_variant *stale = game_engine_get_variant();

		hardcore_session = 0;
		if (stale)
			SET_FLAG(stale->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
		return;
	}
	/* port: the match was started from this mode. stamp the bit the other
	   checks read, including when the loaded copy never received it.
	   a classic game this machine is hosting does not keep that session. */
	if (hardcore_session)
	{
		struct game_variant *variant = game_engine_get_variant();
		struct network_game_server *server = global_network_game_server_get();

		if (server)
		{
			struct network_game *hosted = network_game_server_get_game(server);

			hardcore_rules_host_variant(hosted ? &hosted->variant : NULL);
		}
		/* port: a client takes the variant it was sent. a leftover session
		   must not mark a classic game. */
		else if (!hardcore_anniversary())
			hardcore_session = 0;
		if (hardcore_session && variant)
			SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit, TRUE);
	}
	if (!hardcore_rules_active())
		return;
	hardcore_apply_presentation();
}

static void opening_finish(void)
{
	long sound = opening_deferred_sound;

	opening_deferred_sound = NONE;
	opening_hold = FALSE;
	opening_release = FALSE;
	opening_fading = FALSE;
	opening_go = FALSE;
	opening_fade_ticks = 0;
	opening_started_ms = 0;
	opening_played_ms = 0;
	opening_last_present_ms = 0;
	opening_fade_from_ms = 0;
	progress_bar_end();
	/* The stinger was stored while the film was up. It plays as control returns. */
	if (sound != NONE && game_in_progress())
		game_engine_play_multiplayer_sound(sound);
}

/* A player counts once they are in the world and the weapon in their
hand has finished its draw. An empty slot is not a player. A loadout
that gives nothing is placed. A client waits until the host's weapon
arrives. */
static boolean opening_player_armed(struct player_datum *player)
{
	struct unit_datum *unit;
	struct weapon_datum *weapon;
	long weapon_index;
	int slot;
	int any = FALSE;

	if (!player || player->quit_out_of_game)
		return TRUE;
	if (player->unit_index == NONE)
		return FALSE;
	unit = unit_try_and_get(player->unit_index);
	if (!unit)
		return FALSE;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (unit->unit.weapon_object_indices[slot] != NONE)
			any = TRUE;
	}
	if (!any)
		return !network_game_distributed_client();
	if (unit->unit.current_weapon_index < 0 ||
		unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT)
		return FALSE;
	weapon_index = unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_try_and_get(weapon_index);
	if (!weapon)
		return FALSE;
	return weapon->weapon.state == _weapon_state_idle;
}

static boolean opening_players_ready(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	int count = 0;

	if (!player_data)
		return FALSE;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->quit_out_of_game)
			continue;
		count++;
		if (!opening_player_armed(player))
			return FALSE;
	}
	return count > 0;
}

void hardcore_rules_hold_pregame(void)
{
	opening_hold = TRUE;
	opening_release = FALSE;
	opening_fading = FALSE;
	opening_go = FALSE;
	opening_fade_ticks = 0;
	opening_played_ms = 0;
	opening_last_present_ms = 0;
	opening_fade_from_ms = 0;
	opening_deferred_sound = NONE;
	opening_started_ms = system_milliseconds();
	progress_bar_begin(TRUE);
}

boolean hardcore_rules_opening_hold(void)
{
	return opening_hold ? TRUE : FALSE;
}

boolean hardcore_rules_opening_fading(void)
{
	return opening_fading ? TRUE : FALSE;
}

static boolean opening_network(void)
{
	short connection = game_connection();

	return connection == _game_connection_network_client ||
		connection == _game_connection_network_server;
}

static boolean opening_has_bots(void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	if (!player_data)
		return FALSE;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->quit_out_of_game)
			continue;
		if (bot_controller_is_bot(player->network_player_data.machine_index,
			player->network_player_data.controller_index))
			return TRUE;
	}
	return FALSE;
}

boolean hardcore_rules_opening_world_ready(void)
{
	short connection;

	if (cache_files_precache_in_progress())
		return FALSE;
	if (!opening_players_ready())
		return FALSE;
	connection = game_connection();
	if (connection == _game_connection_network_client)
	{
		/* The host places the items. This machine is ready once that
		world, and the game type's state, have arrived. */
		if (!network_objects_client_synchronized())
			return FALSE;
		if (!distributed_opening_state_seen())
			return FALSE;
		return TRUE;
	}
	if (!game_engine_opening_items_placed())
		return FALSE;
	if (!race_opening_vehicles_ready())
		return FALSE;
	/* Clients do not path the bots. The host does, and a mesh that is
	still building would start them with nowhere to step. */
	if (opening_has_bots() && !bot_nav_settled())
		return FALSE;
	return TRUE;
}

boolean hardcore_rules_opening_waiting(void)
{
	return opening_hold && !opening_go;
}

boolean hardcore_rules_opening_film_at_fade(void)
{
	unsigned long duration = opening_film_duration_ms();
	unsigned long hold_ms;
	unsigned long elapsed;

	/* No frame has been shown yet. That is the start of the film, not
	its end: the ticks before the first picture must not start the match.
	A host that never draws one waits out the film, then starts. */
	if (opening_last_present_ms == 0)
	{
		elapsed = opening_started_ms != 0 ? system_milliseconds() - opening_started_ms : 0;
		return elapsed >= (duration > 0 ? duration : OPENING_RING_MS);
	}
	if (duration > OPENING_FADE_MS)
		hold_ms = duration - OPENING_FADE_MS;
	else if (duration > 0)
		hold_ms = 0;
	else
		hold_ms = OPENING_RING_MS;
	if (duration > 0)
		return opening_played_ms >= hold_ms;
	return opening_started_ms != 0 &&
		system_milliseconds() - opening_started_ms >= hold_ms;
}

boolean hardcore_rules_opening_expired(void)
{
	return opening_hold && opening_started_ms != 0 &&
		system_milliseconds() - opening_started_ms > OPENING_READY_WAIT_MS;
}

static void opening_begin_fade(int ticks)
{
	if (ticks <= 0)
	{
		opening_finish();
		return;
	}
	opening_go = TRUE;
	opening_fading = TRUE;
	opening_fade_ticks = ticks;
	opening_fade_from_ms = opening_played_ms;
}

void hardcore_rules_opening_go(long update_number, boolean host)
{
	long late = 0;

	if (!opening_hold || opening_go)
		return;
	if (!host)
	{
		struct network_game_client *client = global_network_game_client_get();
		long next = client ? network_game_client_get_next_update_number(client) : 0;

		/* Updates that already arrived are fade ticks already spent. */
		late = next - update_number;
		if (late < 0)
			late = 0;
	}
	opening_begin_fade(OPENING_FADE_TICKS - (int)late);
}

void hardcore_rules_opening_defer_sound(long sound_index)
{
	if (!opening_hold || opening_fading)
	{
		game_engine_play_multiplayer_sound(sound_index);
		return;
	}
	opening_deferred_sound = sound_index;
}

/* A presented frame moves the film by at most one of its frames. */
static unsigned long opening_advance(void)
{
	unsigned long now = system_milliseconds();
	unsigned long step;
	unsigned long cap;

	if (opening_last_present_ms == 0)
	{
		opening_last_present_ms = now;
		return opening_played_ms;
	}
	step = now - opening_last_present_ms;
	opening_last_present_ms = now;
	cap = opening_film_step_ms();
	if (step > cap)
		step = cap;
	opening_played_ms += step;
	return opening_played_ms;
}

void hardcore_rules_opening_before_tick(void)
{
	if (!opening_hold)
		return;
	if (!game_engine_running() || !game_in_progress())
	{
		opening_finish();
		return;
	}
	if (opening_fading)
	{
		opening_fade_ticks--;
		if (opening_fade_ticks <= 0)
			opening_finish();
		return;
	}
	if (opening_release)
		opening_finish();
	/* A network match starts from the host. This only frees a local game
	whose weapon never finishes drawing. */
	else if (!opening_network() && opening_started_ms != 0 &&
		system_milliseconds() - opening_started_ms > OPENING_READY_WAIT_MS)
		opening_finish();
}

void hardcore_rules_opening_frame(void)
{
	unsigned long played;
	unsigned long film_ms;
	unsigned long hold_ms;
	unsigned long ring_ms;
	real progress;

	if (!opening_hold || opening_fading)
		return;
	if (!game_engine_running() || !game_in_progress())
	{
		opening_finish();
		return;
	}
	played = opening_advance();
	/* The film is the original loading animation. Its last frame holds
	while a network world is still arriving. Without the film, the sweep
	is the stand-in ring. */
	film_ms = opening_film_duration_ms();
	if (!opening_film_show(played, 1.f))
	{
		ring_ms = opening_started_ms != 0 ? system_milliseconds() - opening_started_ms : 0;
		progress = (real)ring_ms / (real)OPENING_RING_MS;
		if (progress < 0.02f)
			progress = 0.02f;
		if (progress > 1.f)
			progress = 0.72f;
		progress_bar_show_ring(progress);
		film_ms = 0;
	}
	/* The host names the tick. This machine does not fade because it
	happens to look ready. */
	if (opening_network())
		return;
	if (!hardcore_rules_opening_world_ready())
		return;
	if (film_ms > OPENING_FADE_MS)
		hold_ms = film_ms - OPENING_FADE_MS;
	else if (film_ms > 0)
		hold_ms = 0;
	else
		hold_ms = OPENING_RING_MS;
	if (film_ms > 0)
	{
		if (played < hold_ms)
			return;
	}
	else if (opening_started_ms == 0 ||
		system_milliseconds() - opening_started_ms < hold_ms)
		return;
	opening_begin_fade(OPENING_FADE_TICKS);
}

void hardcore_rules_opening_overlay(void)
{
	unsigned long film_ms;
	unsigned long left;
	unsigned long shown;
	real alpha;
	real done;

	if (!opening_hold || !opening_fading)
		return;
	alpha = opening_fade_ticks > 0 ?
		(real)opening_fade_ticks / (real)OPENING_FADE_TICKS : 0.f;
	/* Stay under a full replace so this draws over the match. */
	if (alpha > 0.998f)
		alpha = 0.998f;
	film_ms = opening_film_duration_ms();
	if (film_ms == 0)
	{
		opening_film_show(opening_fade_from_ms, alpha);
		return;
	}
	/* Whatever of the film is left plays out as the fade does. */
	left = opening_fade_from_ms < film_ms ? film_ms - opening_fade_from_ms : 0;
	done = 1.f - alpha;
	shown = opening_fade_from_ms + (unsigned long)((real)left * done);
	if (shown > film_ms)
		shown = film_ms;
	opening_film_show(shown, alpha);
}

void hardcore_rules_tick(void)
{
	if (!hardcore_rules_active())
	{
		if (hardcore_cheats_written)
		{
			cheat.bottomless_clip = FALSE;
			cheat.infinite_ammo = FALSE;
			hardcore_cheats_written = 0;
		}
		return;
	}
	/* port: a settings update replaces the practice flags with the variant's,
	   which does not carry the spawn markers. put them back each tick. */
	hardcore_apply_presentation();
	cheat.bottomless_clip = hardcore_live.infinite_ammo == 1;
	cheat.infinite_ammo = hardcore_live.infinite_ammo == 2;
	hardcore_cheats_written = 1;
}

boolean hardcore_rules_manual_pickup(long weapon_index)
{
	(void)weapon_index;
	/* Pickup is a retail walk-on. The removed row stored manual as zero,
	   which is also the empty sheet, so another match option must not
	   hold the flag or the ball. */
	return FALSE;
}

boolean hardcore_rules_hill_ordered(void)
{
	/* Retail draws the next hill at random. Ordered was a removed row,
	   and a zero byte is the empty sheet. */
	return FALSE;
}

long hardcore_rules_hill_move_ticks(void)
{
	int seconds = hardcore_live.hill_move_s;

	if (seconds < 5)
		seconds = 60;
	return (long)seconds * TICKS_PER_SECOND;
}

real hardcore_rules_spawn_rating(long player_index, real_point3d const *position, real rating)
{
	struct player_datum *player;
	struct game_variant const *variant;
	long mates[HARDCORE_PLAYER_SLOTS];
	long self;
	short count, index, mine, partner;
	boolean found;

	if (!hardcore_rules_active() || !hardcore_live.fireteams || rating <= 0.0f || !position)
		return rating;
	variant = game_engine_get_variant();
	player = player_try_and_get(player_index);
	if (!variant || !variant->universal_variant.teams || !player || player->team_index < 0)
		return rating;
	count = 0;
	for (index = 0; index < HARDCORE_PLAYER_SLOTS && count < HARDCORE_PLAYER_SLOTS; index++)
	{
		struct player_datum *other = player_try_and_get(index);

		if (other && !other->quit_out_of_game && other->team_index == player->team_index)
			mates[count++] = index;
	}
	if (count <= 2)
		return rating;
	/* mates[] is a slot. player_index carries a salt, so the slot is the low word. */
	mine = 0;
	self = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	found = FALSE;
	for (index = 0; index < count; index++)
		if (mates[index] == self)
		{
			mine = index;
			found = TRUE;
		}
	if (!found)
		return rating;
	partner = (mine ^ 1) < count ? (short)mates[mine ^ 1] : (short)self;
	for (index = 0; index < count; index++)
	{
		struct player_datum *other;
		struct object_datum *unit;
		real dx, dy, dz;

		if (mates[index] == self || mates[index] == partner)
			continue;
		other = player_try_and_get(mates[index]);
		if (!other || other->unit_index == NONE)
			continue;
		unit = object_try_and_get(other->unit_index);
		if (!unit)
			continue;
		dx = unit->object.position.x - position->x;
		dy = unit->object.position.y - position->y;
		dz = unit->object.position.z - position->z;
		if (dx * dx + dy * dy + dz * dz < 36.0f)
			return rating * 0.15f;
	}
	return rating;
}

boolean hardcore_rules_shot_blocked(long weapon_index)
{
	int delay;
	long now;
	unsigned slot;

	delay = (int)hardcore_live.aim_delay + (int)hardcore_live.trig_delay;
	if (!hardcore_rules_active() || delay <= 0 || weapon_index == NONE)
		return FALSE;
	now = game_time_get();
	slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(weapon_index);
	if (slot >= HARDCORE_SKIP_SLOTS)
		return FALSE;
	/* the shot checks again the same tick the hold was noted. a gap is
	   the trigger coming up. the same tick must not start the wait over.
	   a new weapon in a freed slot is not the hold that slot had. */
	if (hardcore_shot_weapon[slot] != weapon_index ||
		(hardcore_shot_seen[slot] != now && hardcore_shot_seen[slot] != now - 1))
		hardcore_shot_press[slot] = now;
	hardcore_shot_weapon[slot] = weapon_index;
	hardcore_shot_seen[slot] = now;
	return now - hardcore_shot_press[slot] < delay;
}

static real hardcore_scale(unsigned char which, real tag_value, real const *table, int count)
{
	if (!hardcore_rules_active() || which == 0 || which >= count)
		return tag_value;
	return tag_value * table[which];
}

/* tag stays the map's number. the named stops are amounts, so full is 1
   and is not another way to leave the tag. strong is past that. */
static real hardcore_amount(unsigned char which, real tag_value)
{
	static real const table[] = { 0.0f, 0.0f, 0.5f, 1.0f, 1.5f };

	if (!hardcore_rules_active() || which == 0 || which >= 5)
		return tag_value;
	return table[which];
}

real hardcore_rules_friction(real tag_value)
{
	return hardcore_amount(hardcore_live.friction, tag_value);
}

real hardcore_rules_adhesion(real tag_value)
{
	return hardcore_amount(hardcore_live.adhesion, tag_value);
}

real hardcore_rules_spread(long weapon_definition_index, real error)
{
	char const *name;
	char const *leaf;
	unsigned char which;
	static real const table[] = { 1.0f, 0.5f, 1.5f, 2.0f };

	if (!hardcore_rules_active() || weapon_definition_index == NONE)
		return error;
	name = tag_get_name(weapon_definition_index);
	if (!name)
		return error;
	/* the leaf, so the plasma pistol is not the pistol */
	leaf = strrchr(name, '\\');
	if (!leaf)
		leaf = strrchr(name, '/');
	leaf = leaf ? leaf + 1 : name;
	which = 0;
	if (!strcmp(leaf, "pistol"))
		which = hardcore_live.pistol_spread;
	else if (!strcmp(leaf, "shotgun"))
		which = hardcore_live.shotgun_spread;
	return hardcore_scale(which, error, table, 4);
}

void hardcore_rules_skip_arm(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= HARDCORE_SKIP_SLOTS)
		return;
	hardcore_skip_armed[slot] = 1;
}

boolean hardcore_rules_skip_consume(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= HARDCORE_SKIP_SLOTS || !hardcore_skip_armed[slot])
		return FALSE;
	hardcore_skip_armed[slot] = 0;
	return hardcore_rules_active() && hardcore_live.skip_draw;
}

void hardcore_rules_on_kill(long killing_player_index, long dead_player_index)
{
	struct player_datum *killer;
	struct unit_datum *killer_unit;

	if (!hardcore_rules_active() || !hardcore_live.kill_restores ||
		killing_player_index == NONE || killing_player_index == dead_player_index)
		return;
	killer = player_try_and_get(killing_player_index);
	killer_unit = killer && killer->unit_index != NONE ? unit_try_and_get(killer->unit_index) : NULL;
	/* a dead killer stays dead. an overshield is the number past full,
	   so the shield is left there. the body still comes back. */
	if (!killer_unit || TEST_FLAG(killer_unit->object.damage_flags, _object_dead_bit))
		return;
	if (killer_unit->object.shield_vitality <= 1.0f)
		killer_unit->object.shield_vitality = 1.0f;
	killer_unit->object.body_vitality = 1.0f;
}

boolean hardcore_rules_overshield_fades(void)
{
	/* Fading is the retail drain, in every match. The constant byte stays
	   in the message so its place does not move, and it is not read. */
	return TRUE;
}

long hardcore_rules_item_period(long period_ticks, long authored_seconds)
{
	(void)authored_seconds;
	if (!hardcore_rules_active() || hardcore_live.item_spawn_s == 0)
		return period_ticks;
	return (long)hardcore_live.item_spawn_s * TICKS_PER_SECOND;
}

/* slot 0 can be empty while another local player is on this machine */
static long hardcore_local_player_index(void)
{
	short slot;

	if (!players_globals)
		return NONE;
	slot = local_player_get_next(NONE);
	if (slot == NONE)
		return NONE;
	return local_player_get_player_index(slot);
}

static void hardcore_random_update(void)
{
	struct player_datum *local;
	struct unit_datum *body;
	real near_weight, total_weight;
	short index, count;
	long player_index;

	hardcore_random_percent = -1;
	/* the percent is its own byte. training mode is the pads and the
	   volumes. a free for all still has a respawn weight. */
	if (!hardcore_live.random_pct || !players_globals ||
		players_globals->local_player_count <= 0)
		return;
	player_index = hardcore_local_player_index();
	local = player_try_and_get(player_index);
	if (!local || local->unit_index == NONE)
		return;
	body = unit_try_and_get(local->unit_index);
	if (!body)
		return;
	near_weight = 0.0f;
	total_weight = 0.0f;
	count = player_get_starting_location_count();
	for (index = 0; index < count; index++)
	{
		struct player_starting_location const *start = player_get_starting_location(index);
		real rating, dx, dy, dz;

		if (!start)
			continue;
		rating = game_engine_get_starting_location_rating(player_index, start);
		if (rating <= 0.0f)
			continue;
		rating = rating * rating;
		total_weight += rating;
		dx = start->position.x - body->object.position.x;
		dy = start->position.y - body->object.position.y;
		dz = start->position.z - body->object.position.z;
		if (dx * dx + dy * dy + dz * dz < 36.0f)
			near_weight += rating;
	}
	if (total_weight <= 0.0f)
		return;
	hardcore_random_percent = (int)(100.0f * (1.0f - near_weight / total_weight) + 0.5f);
	if (hardcore_random_percent < 0)
		hardcore_random_percent = 0;
	if (hardcore_random_percent > 100)
		hardcore_random_percent = 100;
}

static void hardcore_marker_line(real_point3d const *position, real_argb_color const *color)
{
	real_point3d base, top;

	base = *position;
	top = base;
	top.z += 0.8f;
	rasterizer_debug_line(&base, &top, color);
}

static void hardcore_markers(void)
{
	struct player_datum *local;
	real_argb_color const outline = { 1.0f, 0.2f, 0.85f, 1.0f };
	real_argb_color const shield = { 1.0f, 1.0f, 0.85f, 0.2f };
	long index;

	if (ui_widgets_active() || !players_globals || players_globals->local_player_count <= 0)
		return;
	if (!hardcore_live.outlines && !hardcore_live.os_visible)
		return;
	local = player_try_and_get(hardcore_local_player_index());
	for (index = 0; index < HARDCORE_PLAYER_SLOTS; index++)
	{
		struct player_datum *player = player_try_and_get(index);
		struct unit_datum *unit;

		if (!player || player->unit_index == NONE)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		if (hardcore_live.outlines && local && player != local &&
			local->team_index >= 0 && player->team_index == local->team_index)
			hardcore_marker_line(&unit->object.position, &outline);
		if (hardcore_live.os_visible && unit->object.shield_vitality > 1.0f)
			hardcore_marker_line(&unit->object.position, &shield);
	}
}

void hardcore_rules_render(void)
{
	long font;
	real_argb_color color;
	rectangle2d bounds;
	char text[48];
	wchar_t wide[48];
	long index;

	if (!hardcore_rules_active() || !game_in_progress())
		return;
	hardcore_markers();
	/* The percent belongs to training mode. The byte can stay set after
	   training is turned off, and a comp preset used to set it alone. */
	if (!hardcore_live.random_areas && !hardcore_live.spawns_visible)
		hardcore_random_percent = -1;
	else
		hardcore_random_update();
	if (hardcore_random_percent < 0 || ui_widgets_active())
		return;
	/* the clock's font and top, so the percent is on its line */
	font = hud_globals ? hud_globals->messaging.single_player_font.index : NONE;
	if (font == NONE)
		font = hud_get_font_index();
	if (font == NONE)
		return;
	snprintf(text, sizeof(text), "RANDOM %d%%", hardcore_random_percent);
	/* this pane's coordinates. the safe frame ends above the pane, and the
	clock is on the pane, so this row is too. */
	bounds.x0 = (short)(render.camera.window_bounds.x0 - render.camera.viewport_bounds.x0 + 16);
	bounds.x1 = (short)(render.camera.window_bounds.x1 - render.camera.viewport_bounds.x0);
	bounds.y0 = hardcore_timer_top();
	bounds.y1 = (short)(render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0);
	if (bounds.x0 >= bounds.x1 || bounds.y0 >= bounds.y1)
		return;
	for (index = 0; text[index] && index < 47; index++)
		wide[index] = (wchar_t)text[index];
	wide[index] = 0;
	hud_get_text_color(&color);
	draw_string_set_draw_mode(font, NONE, 0, 0, &color);
	rasterizer_text_set_shadow_color(0xC0000000);
	rasterizer_draw_unicode_string(&bounds, &bounds, NULL, 0, wide);
	rasterizer_text_set_shadow_color(0);
}

boolean hardcore_rules_write_message(void *buffer, long buffer_size)
{
	unsigned char *bytes = buffer;
	unsigned long header;

	if (!buffer || buffer_size < HARDCORE_RULES_MESSAGE_SIZE)
		return FALSE;
	csmemset(buffer, 0, HARDCORE_RULES_MESSAGE_SIZE);
	/* Host order until the connection write swaps the header word. */
	header = ((unsigned long)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2);
	bytes[0] = (unsigned char)header;
	bytes[1] = (unsigned char)(header >> 8);
	bytes[2] = 'C';
	bytes[3] = 'R';
	bytes[4] = 'U';
	bytes[5] = 'L';
	bytes[6] = HARDCORE_RULES_VERSION;
	hardcore_rules_kept_only(&hardcore_live);
	{
		struct hardcore_rules sent = hardcore_live;

		/* A joined player keeps display.fov. This machine still uses its own. */
		sent.forced_fov = 0;
		csmemcpy(bytes + 7, &sent, sizeof(sent));
	}
	return TRUE;
}

boolean hardcore_rules_take_message(void const *message, long size)
{
	unsigned char const *bytes = message;

	if (!message || size != HARDCORE_RULES_MESSAGE_SIZE)
		return FALSE;
	if (bytes[2] != 'C' || bytes[3] != 'R' || bytes[4] != 'U' || bytes[5] != 'L' ||
		bytes[6] != HARDCORE_RULES_VERSION)
		return FALSE;
	if (bytes[0] != (unsigned char)(((unsigned)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2)) ||
		bytes[1] != (unsigned char)((((unsigned)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2)) >> 8))
		return FALSE;
	csmemcpy(&hardcore_live, bytes + 7, sizeof(hardcore_live));
	hardcore_rules_kept_only(&hardcore_live);
	hardcore_live.forced_fov = 0;
	/* port: the loaded variant may not have the bit yet. the session is
	   what makes this match apply the sheet, as it does on the host.
	   these bytes are the host's. the local editor does not replace them. */
	hardcore_from_host = 1;
	hardcore_session = 1;
	if (game_in_progress() && hardcore_rules_active())
		hardcore_apply_presentation();
	return TRUE;
}

/* port: a classic variant ends the match this machine joined. the host's
   session belongs to its menu and is not cleared from here. */
void hardcore_rules_observe_match(boolean hardcore)
{
	if (!hardcore)
	{
		hardcore_session = 0;
		hardcore_from_host = 0;
	}
}

#ifndef HARDCORE_VIEWS_H
#define HARDCORE_VIEWS_H

struct observer_result;

/* How many panes the one local player gets this frame: 1, or up to 4 when
teammate views are on. */
short hardcore_views_window_count(void);

/* Local players this screen draws. A hardcore team game draws only the
first local player's team. */
short hardcore_views_local_count(void);

/* The next local player this screen draws after previous, or NONE. */
short hardcore_views_next_local(short previous);

/* The camera for a teammate pane. A living teammate is first person.
A dead one uses the death camera. The local player's pane is this
machine's display.teammate_view_slot. */
boolean hardcore_views_camera(short window_index, struct observer_result *result);

/* Drops a pose from an earlier frame, so a pane that no longer uses it
does not keep that body hidden. */
void hardcore_views_local_pose_reset(void);

/* The local player's pane when its observer is not in the world, or the
slot does not yet control the unit. Fills result and returns FALSE when
the observer should be kept. A living unit is hidden in that pane only. */
boolean hardcore_views_local_pose(
	short window_index,
	short local_player_index,
	struct observer_result const *observer,
	struct observer_result *result);

/* The living unit a spare HUD slot is showing, or NONE. The field of
view uses it, because that slot has no director of its own. */
long hardcore_views_fov_unit(short slot);

/* A pane that shows a teammate, not the local player and not the console. */
boolean hardcore_views_is_teammate_window(short window_index);

/* The pane that is the local player while teammate views split the screen. */
boolean hardcore_views_is_local_window(short window_index);

/* Teammate views are splitting this screen, so messages use the
split-screen font and line spacing. */
boolean hardcore_views_split(void);

/* Multiplayer with one local player, when display.force_split is on.
Campaign stays one pane. A second local player is a real split, and
teammate views keep their own panes. */
boolean hardcore_views_blank_split(void);

/* The pane that is the local player in that two-pane split. */
short hardcore_views_blank_player_slot(void);

/* The other pane. It is cleared and not a camera. */
boolean hardcore_views_is_blank_window(short window_index);

/* Spare local-player slot this teammate pane borrows so the HUD and the
first-person weapon will draw. A slot that already has a player is left
alone. Each pane keeps its own slot, so the weapon animation continues. */
short hardcore_views_hud_slot(short window_index);

/* Once a tick, advance the weapons those panes are holding. */
void hardcore_views_weapons_tick(void);

/* The camera a spare listener uses, so that pane hears the world.
A slot that already has a player is left alone. */
boolean hardcore_views_listener(short slot, struct observer_result *camera);

/* The pane being drawn, so that body is hidden the way a first-person view is. */
void hardcore_views_begin(short window_index);
void hardcore_views_end(void);
boolean hardcore_views_hides_unit(long object_index);

#endif

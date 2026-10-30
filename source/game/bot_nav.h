/*
The walk Recast builds from the loaded collision BSP. Combat still writes
the aim and the trigger. This only answers where the next step is.
*/

#ifndef BOT_NAV_H
#define BOT_NAV_H

#include <stddef.h>

struct bot_nav_identity
{
	unsigned long key;
	int verts;
	int surfaces;
	float marker;
};

struct bot_nav_step
{
	float x;
	float y;
	float z;
	/* 1 when the mesh cannot reach the goal. */
	int partial;
	/* 1 when the next step is a ladder. yaw looks into the face.
	land is the floor the climb comes out on. top is the lip. */
	int ladder;
	int up;
	float yaw;
	float top;
	float land_x;
	float land_y;
	float land_z;
	float face_x;
	float face_y;
	/* 1 when this step leaves the corridor they were walking. */
	int switched;
};

struct bot_nav_ladder
{
	float stand_x;
	float stand_y;
	float stand_z;
	float land_x;
	float land_y;
	float land_z;
	float yaw;
	float top;
	float face_x;
	float face_y;
};

#ifdef __cplusplus
extern "C" {
#endif

int bot_nav_bsp_identity(struct bot_nav_identity *id);
void *bot_nav_alloc(size_t size);
void *bot_nav_realloc(void *pointer, size_t size);
void bot_nav_free(void *pointer);
int bot_nav_collect_geometry(float **verts, int *vert_count, int **tris, int *tri_count);
int bot_nav_collect_ladders(struct bot_nav_ladder *out, int capacity);
void bot_nav_begin_tick(void);
int bot_nav_ready(void);
/* agent is the player index. A negative agent is one shortest path with
no memory. blocked is 1 when that body has not been moving. A few
corridors are kept for the goal. A short one is the usual trip. Another
is taken when this one has stopped, or when the body is already on it. */
int bot_nav_steer(float fx, float fy, float fz, float gx, float gy, float gz,
	int agent, int blocked, struct bot_nav_step *step);
/* up to limit places the mesh can reach, spread around the bot.
the furthest tile is one stop on that circuit, not the only one.
points is x,y,z triples. returns how many were written. */
int bot_nav_explore(float x, float y, float z, int salt, float *points, int limit);

#ifdef __cplusplus
}
#endif

#endif

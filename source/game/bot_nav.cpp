/*
Walk mesh for the bots. Recast is Y-up and Halo is Z-up, so a point
(x, y, z) goes in as (x, z, y) and comes back out as (x, z, y). The
swap mirrors the mesh, so every triangle's winding is flipped to put
floors back on the up axis.

The guest has no C++ library. Allocation is malloc, and nothing here throws.
Tiles are built a few at a time so the first seconds of a match still use
the walker that was already there. Once the mesh is ready, a bot keeps a
few corridors to its goal and walks the next corner of one of them.
*/

#include "bot_nav.h"

#include "Recast.h"
#include "RecastAlloc.h"
#include "DetourAlloc.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" void platform_log(const char *format, ...);

/* Recast and the triangle soup share one block from the game heap.
   Individual frees stay inside that block. Returning a smashed chunk to
   musl executes a breakpoint and the match dies, which is what happened
   when the finished mesh released its vertices. A pointer this heap did
   not hand out is ignored. */
static const size_t NAV_HEAP_BYTES = 64u * 1024u * 1024u;
static const unsigned NAV_LIVE = 0x4e415631u;
static const unsigned NAV_DEAD = 0x4e415646u;

struct nav_block
{
	unsigned magic;
	unsigned size;
	struct nav_block *next;
	unsigned pad;
};

static unsigned char *nav_heap = 0;
static size_t nav_heap_used = 0;
static struct nav_block *nav_heap_free = 0;

static int nav_heap_init(void)
{
	if (nav_heap)
		return 1;
	nav_heap = (unsigned char *)malloc(NAV_HEAP_BYTES);
	nav_heap_used = 0;
	nav_heap_free = 0;
	return nav_heap != 0;
}

static void nav_heap_reset(void)
{
	nav_heap_used = 0;
	nav_heap_free = 0;
}

static int nav_heap_contains(void const *pointer)
{
	unsigned char const *p;

	p = (unsigned char const *)pointer;
	return nav_heap && p >= nav_heap && p < nav_heap + NAV_HEAP_BYTES;
}

static void *nav_heap_alloc(size_t size)
{
	struct nav_block *block;
	struct nav_block **link;
	size_t aligned;
	size_t bytes;
	int seen;

	if (!nav_heap_init())
		return 0;
	if (size < 1)
		size = 1;
	aligned = (size + 15u) & ~(size_t)15u;
	if (aligned < size)
		return 0;
	seen = 0;
	for (link = &nav_heap_free; *link && seen < 100000; link = &(*link)->next, seen++)
	{
		struct nav_block *rest;

		block = *link;
		if (!nav_heap_contains(block) || !nav_heap_contains((unsigned char *)(block + 1) + block->size - 1))
			break;
		if (block->magic != NAV_DEAD || block->size < aligned)
			continue;
		if (block->size >= aligned + sizeof(struct nav_block) + 16u)
		{
			rest = (struct nav_block *)((unsigned char *)(block + 1) + aligned);
			rest->magic = NAV_DEAD;
			rest->size = block->size - (unsigned)aligned - (unsigned)sizeof(struct nav_block);
			rest->next = block->next;
			rest->pad = 0;
			block->size = (unsigned)aligned;
			*link = rest;
		}
		else
			*link = block->next;
		block->magic = NAV_LIVE;
		block->next = 0;
		return block + 1;
	}
	bytes = sizeof(struct nav_block) + aligned;
	if (bytes < aligned || nav_heap_used > NAV_HEAP_BYTES - bytes)
		return 0;
	block = (struct nav_block *)(nav_heap + nav_heap_used);
	nav_heap_used += bytes;
	block->magic = NAV_LIVE;
	block->size = (unsigned)aligned;
	block->next = 0;
	block->pad = 0;
	return block + 1;
}

static void nav_heap_release(void *pointer)
{
	struct nav_block *block;
	struct nav_block **link;
	unsigned char *block_end;
	int seen;

	if (!pointer || !nav_heap_contains(pointer))
		return;
	block = ((struct nav_block *)pointer) - 1;
	if (!nav_heap_contains(block) || block->magic != NAV_LIVE)
		return;
	block_end = (unsigned char *)(block + 1) + block->size;
	if (block_end < (unsigned char *)(block + 1) || block_end > nav_heap + nav_heap_used)
		return;
	block->magic = NAV_DEAD;
	block->next = 0;
	link = &nav_heap_free;
	seen = 0;
	while (*link && seen < 100000)
	{
		struct nav_block *other;
		unsigned char *other_end;

		seen++;
		other = *link;
		if (!nav_heap_contains(other) || other->size > NAV_HEAP_BYTES)
			break;
		other_end = (unsigned char *)(other + 1) + other->size;
		if (!nav_heap_contains(other_end - 1))
			break;
		if (other->magic == NAV_DEAD && other_end == (unsigned char *)block)
		{
			*link = other->next;
			other->size += (unsigned)sizeof(struct nav_block) + block->size;
			block = other;
			block_end = (unsigned char *)(block + 1) + block->size;
			continue;
		}
		if (other->magic == NAV_DEAD && block_end == (unsigned char *)other)
		{
			*link = other->next;
			block->size += (unsigned)sizeof(struct nav_block) + other->size;
			block_end = (unsigned char *)(block + 1) + block->size;
			continue;
		}
		link = &other->next;
	}
	block->next = nav_heap_free;
	nav_heap_free = block;
}

static void *nav_heap_realloc(void *pointer, size_t size)
{
	struct nav_block *block;
	void *grown;
	size_t old;

	if (!pointer)
		return nav_heap_alloc(size);
	if (!size)
	{
		nav_heap_release(pointer);
		return 0;
	}
	if (!nav_heap_contains(pointer))
		return 0;
	block = ((struct nav_block *)pointer) - 1;
	if (block->magic != NAV_LIVE)
		return 0;
	if (block->size >= size)
		return pointer;
	grown = nav_heap_alloc(size);
	if (!grown)
		return 0;
	old = block->size;
	memcpy(grown, pointer, old < size ? old : size);
	nav_heap_release(pointer);
	return grown;
}

extern "C" void *bot_nav_alloc(size_t size)
{
	return nav_heap_alloc(size);
}

extern "C" void bot_nav_free(void *pointer)
{
	nav_heap_release(pointer);
}

extern "C" void *bot_nav_realloc(void *pointer, size_t size)
{
	return nav_heap_realloc(pointer, size);
}

void *operator new(__SIZE_TYPE__ size)
{
	return nav_heap_alloc((size_t)size);
}

void *operator new[](__SIZE_TYPE__ size)
{
	return nav_heap_alloc((size_t)size);
}

void operator delete(void *pointer) noexcept
{
	nav_heap_release(pointer);
}

void operator delete[](void *pointer) noexcept
{
	nav_heap_release(pointer);
}

void operator delete(void *pointer, __SIZE_TYPE__) noexcept
{
	nav_heap_release(pointer);
}

void operator delete[](void *pointer, __SIZE_TYPE__) noexcept
{
	nav_heap_release(pointer);
}

extern "C" void __cxa_pure_virtual(void)
{
}

extern "C" int __cxa_guard_acquire(unsigned long long *guard)
{
	if (!guard || *guard)
		return 0;
	*guard = 1;
	return 1;
}

extern "C" void __cxa_guard_release(unsigned long long *)
{
}

extern "C" void __cxa_guard_abort(unsigned long long *guard)
{
	if (guard)
		*guard = 0;
}

static const float CELL_SIZE = 0.20f;
static const float CELL_HEIGHT = 0.10f;
static const float AGENT_HEIGHT = 0.70f;
static const float AGENT_RADIUS = 0.30f;
static const float AGENT_CLIMB = 0.40f;
static const float WALKABLE_SLOPE = 50.0f;
static const int TILE_CELLS = 48;
static const int MAX_TILES = 4096;

enum
{
	NAV_EMPTY = 0,
	NAV_BUILDING = 1,
	NAV_READY = 2,
	NAV_FAILED = 3
};

struct tile_span
{
	int *ids;
	int count;
	int cap;
};

static int nav_state = NAV_EMPTY;
static struct bot_nav_identity nav_id;
static float *nav_verts = 0;
static int *nav_tris = 0;
static unsigned char *nav_areas = 0;
static int nav_vert_count = 0;
static int nav_tri_count = 0;
static float nav_origin[3];
static float nav_y_max = 0.0f;
static int nav_tiles_x = 0;
static int nav_tiles_z = 0;
static int nav_next_tile = 0;
static int nav_tiles_added = 0;
static struct tile_span *nav_spans = 0;
static dtNavMesh *nav_mesh = 0;
static dtNavMeshQuery *nav_query = 0;
static int nav_alloc_ready = 0;
static int nav_poly_limit = 256;

static const int NAV_LADDERS = 64;
struct nav_ladder
{
	float stand[3];
	float land[3];
	float yaw;
	float top;
	float base_z;
	float face_x;
	float face_y;
};
static struct nav_ladder nav_ladders[NAV_LADDERS];
static int nav_ladder_count = 0;
static unsigned char nav_seen[4096];
static int nav_seen_count = 0;

/* One slot per player. A corridor is the polygon strip from a search.
Three fit: the short walk, and two others that leave its middle. */
static const int NAV_AGENTS = 128;
static const int NAV_ROUTES = 3;
static const int NAV_ROUTE_POLYS = 192;
static const unsigned char NAV_AREA_SLOW = 2;
static const float NAV_SLOW_COST = 12.0f;
static const int NAV_HOLD_TRY = 15;
static const int NAV_HOLD_SWITCH = 45;

struct nav_route
{
	dtPolyRef polys[NAV_ROUTE_POLYS];
	int count;
	int complete;
	float length;
};

struct nav_agent
{
	int ready;
	int goal_valid;
	int lost;
	float goal[3];
	dtPolyRef anchor;
	int route_count;
	int active;
	int hold;
	int switches;
	float best_dist;
	struct nav_route routes[NAV_ROUTES];
};

static struct nav_agent nav_agents[NAV_AGENTS];

static void *nav_rc_alloc(size_t size, rcAllocHint)
{
	return nav_heap_alloc(size);
}

static void nav_rc_free(void *pointer)
{
	nav_heap_release(pointer);
}

static void *nav_dt_alloc(size_t size, dtAllocHint)
{
	return nav_heap_alloc(size);
}

static void nav_dt_free(void *pointer)
{
	nav_heap_release(pointer);
}

static void nav_use_alloc(void)
{
	if (nav_alloc_ready)
		return;
	rcAllocSetCustom(nav_rc_alloc, nav_rc_free);
	dtAllocSetCustom(nav_dt_alloc, nav_dt_free);
	nav_alloc_ready = 1;
}

static int nav_pow2(int value)
{
	int power;

	power = 1;
	while (power < value && power < 0x40000000)
		power <<= 1;
	return power;
}

static int nav_log2(int power)
{
	int bits;

	bits = 0;
	while (power > 1)
	{
		power >>= 1;
		bits++;
	}
	return bits;
}

static void nav_free_spans(void)
{
	int index;

	if (!nav_spans)
		return;
	for (index = 0; index < nav_tiles_x * nav_tiles_z; index++)
		nav_heap_release(nav_spans[index].ids);
	nav_heap_release(nav_spans);
	nav_spans = 0;
}

static void nav_free_geometry(void)
{
	nav_heap_release(nav_verts);
	nav_heap_release(nav_tris);
	nav_heap_release(nav_areas);
	nav_verts = 0;
	nav_tris = 0;
	nav_areas = 0;
	nav_vert_count = 0;
	nav_tri_count = 0;
	nav_free_spans();
}

static void nav_release(void)
{
	if (nav_query)
	{
		dtFreeNavMeshQuery(nav_query);
		nav_query = 0;
	}
	if (nav_mesh)
	{
		dtFreeNavMesh(nav_mesh);
		nav_mesh = 0;
	}
	nav_free_geometry();
	nav_tiles_x = 0;
	nav_tiles_z = 0;
	nav_next_tile = 0;
	nav_tiles_added = 0;
	nav_ladder_count = 0;
	nav_seen_count = 0;
	memset(nav_seen, 0, sizeof(nav_seen));
	memset(nav_agents, 0, sizeof(nav_agents));
	nav_state = NAV_EMPTY;
	memset(&nav_id, 0, sizeof(nav_id));
	nav_heap_reset();
}

static int nav_same_map(struct bot_nav_identity const *id)
{
	return id->key == nav_id.key && id->verts == nav_id.verts &&
		id->surfaces == nav_id.surfaces && id->marker == nav_id.marker;
}

static void nav_to_recast(float *point)
{
	float halo_y;

	halo_y = point[1];
	point[1] = point[2];
	point[2] = halo_y;
}

static void nav_to_halo(float const *recast, float *x, float *y, float *z)
{
	*x = recast[0];
	*y = recast[2];
	*z = recast[1];
}

static int nav_bin_coord(float value, int limit)
{
	if (!isfinite(value))
		return -2;
	if (value < 0.0f)
		return -1;
	if (value >= (float)limit)
		return limit;
	return (int)value;
}

static int nav_span_add(struct tile_span *span, int triangle)
{
	int next;
	int *grown;

	if (span->count >= span->cap)
	{
		next = span->cap > 0 ? span->cap * 2 : 8;
		grown = (int *)nav_heap_realloc(span->ids, (size_t)next * sizeof(int));
		if (!grown)
			return 0;
		span->ids = grown;
		span->cap = next;
	}
	span->ids[span->count++] = triangle;
	return 1;
}

static int nav_prepare(void)
{
	rcContext context;
	int walkable_height;
	int walkable_climb;
	int walkable_radius;
	int border;
	float tile_world;
	float minimum[3];
	float maximum[3];
	int grid_x;
	int grid_z;
	int triangle;
	int slots;
	int tile_bits;
	int poly_bits;
	dtNavMeshParams params;

	nav_use_alloc();
	if (!bot_nav_collect_geometry(&nav_verts, &nav_vert_count, &nav_tris, &nav_tri_count))
		return 0;
	for (triangle = 0; triangle < nav_vert_count; triangle++)
	{
		float *vert;

		vert = nav_verts + triangle * 3;
		if (!isfinite(vert[0]) || !isfinite(vert[1]) || !isfinite(vert[2]))
			return 0;
		nav_to_recast(vert);
	}
	/* the axis swap mirrors every face. Flip them so a floor's normal points up. */
	for (triangle = 0; triangle < nav_tri_count; triangle++)
	{
		int swap;

		swap = nav_tris[triangle * 3 + 1];
		nav_tris[triangle * 3 + 1] = nav_tris[triangle * 3 + 2];
		nav_tris[triangle * 3 + 2] = swap;
	}

	nav_areas = (unsigned char *)nav_heap_alloc((size_t)nav_tri_count);
	if (!nav_areas)
		return 0;
	memset(nav_areas, 0, (size_t)nav_tri_count);
	rcMarkWalkableTriangles(&context, WALKABLE_SLOPE, nav_verts, nav_vert_count,
		nav_tris, nav_tri_count, nav_areas);

	rcCalcBounds(nav_verts, nav_vert_count, minimum, maximum);
	minimum[0] -= 1.0f;
	minimum[1] -= 1.0f;
	minimum[2] -= 1.0f;
	maximum[0] += 1.0f;
	maximum[1] += 1.0f;
	maximum[2] += 1.0f;
	rcVcopy(nav_origin, minimum);
	nav_y_max = maximum[1];

	walkable_height = (int)ceilf(AGENT_HEIGHT / CELL_HEIGHT);
	walkable_climb = (int)floorf(AGENT_CLIMB / CELL_HEIGHT);
	walkable_radius = (int)ceilf(AGENT_RADIUS / CELL_SIZE);
	if (walkable_height < 3)
		walkable_height = 3;
	if (walkable_climb < 1)
		walkable_climb = 1;
	if (walkable_radius < 1)
		walkable_radius = 1;
	border = walkable_radius + 3;
	tile_world = TILE_CELLS * CELL_SIZE;

	rcCalcGridSize(minimum, maximum, CELL_SIZE, &grid_x, &grid_z);
	nav_tiles_x = (grid_x + TILE_CELLS - 1) / TILE_CELLS;
	nav_tiles_z = (grid_z + TILE_CELLS - 1) / TILE_CELLS;
	if (nav_tiles_x < 1)
		nav_tiles_x = 1;
	if (nav_tiles_z < 1)
		nav_tiles_z = 1;
	if (nav_tiles_x > MAX_TILES || nav_tiles_z > MAX_TILES || nav_tiles_z > MAX_TILES / nav_tiles_x)
		return 0;

	nav_spans = (struct tile_span *)nav_heap_alloc((size_t)nav_tiles_x * (size_t)nav_tiles_z * sizeof(struct tile_span));
	if (nav_spans)
		memset(nav_spans, 0, (size_t)nav_tiles_x * (size_t)nav_tiles_z * sizeof(struct tile_span));
	if (!nav_spans)
		return 0;
	for (triangle = 0; triangle < nav_tri_count; triangle++)
	{
		int const *tri;
		float min_x;
		float max_x;
		float min_z;
		float max_z;
		float pad;
		int x0;
		int x1;
		int z0;
		int z1;
		int x;
		int z;
		int corner;

		tri = nav_tris + triangle * 3;
		if (tri[0] < 0 || tri[1] < 0 || tri[2] < 0 ||
			tri[0] >= nav_vert_count || tri[1] >= nav_vert_count || tri[2] >= nav_vert_count)
			continue;
		min_x = max_x = nav_verts[tri[0] * 3];
		min_z = max_z = nav_verts[tri[0] * 3 + 2];
		for (corner = 1; corner < 3; corner++)
		{
			float vx;
			float vz;

			vx = nav_verts[tri[corner] * 3];
			vz = nav_verts[tri[corner] * 3 + 2];
			if (vx < min_x)
				min_x = vx;
			if (vx > max_x)
				max_x = vx;
			if (vz < min_z)
				min_z = vz;
			if (vz > max_z)
				max_z = vz;
		}
		pad = (float)(border + 2) * CELL_SIZE;
		x0 = nav_bin_coord((min_x - pad - nav_origin[0]) / tile_world, nav_tiles_x);
		x1 = nav_bin_coord((max_x + pad - nav_origin[0]) / tile_world, nav_tiles_x);
		z0 = nav_bin_coord((min_z - pad - nav_origin[2]) / tile_world, nav_tiles_z);
		z1 = nav_bin_coord((max_z + pad - nav_origin[2]) / tile_world, nav_tiles_z);
		if (x0 == -2 || x1 == -2 || z0 == -2 || z1 == -2)
			continue;
		if (x1 < 0 || z1 < 0 || x0 >= nav_tiles_x || z0 >= nav_tiles_z)
			continue;
		if (x0 < 0)
			x0 = 0;
		if (z0 < 0)
			z0 = 0;
		if (x1 >= nav_tiles_x)
			x1 = nav_tiles_x - 1;
		if (z1 >= nav_tiles_z)
			z1 = nav_tiles_z - 1;
		for (z = z0; z <= z1; z++)
		{
			for (x = x0; x <= x1; x++)
			{
				if (!nav_span_add(&nav_spans[z * nav_tiles_x + x], triangle))
					return 0;
			}
		}
	}

	slots = nav_pow2(nav_tiles_x * nav_tiles_z);
	if (slots < 16)
		slots = 16;
	tile_bits = nav_log2(slots);
	poly_bits = 22 - tile_bits;
	if (poly_bits > 14)
		poly_bits = 14;
	if (poly_bits < 8)
		return 0;
	nav_poly_limit = 1 << poly_bits;

	nav_mesh = dtAllocNavMesh();
	if (!nav_mesh)
		return 0;
	memset(&params, 0, sizeof(params));
	rcVcopy(params.orig, nav_origin);
	params.tileWidth = tile_world;
	params.tileHeight = tile_world;
	params.maxTiles = slots;
	params.maxPolys = 1 << poly_bits;
	if (dtStatusFailed(nav_mesh->init(&params)))
		return 0;

	nav_query = dtAllocNavMeshQuery();
	if (!nav_query || dtStatusFailed(nav_query->init(nav_mesh, 4096)))
		return 0;

	nav_next_tile = 0;
	nav_tiles_added = 0;
	nav_ladder_count = 0;
	nav_seen_count = 0;
	memset(nav_seen, 0, sizeof(nav_seen));
	{
		struct bot_nav_ladder raw[NAV_LADDERS];
		int found;
		int index;

		found = bot_nav_collect_ladders(raw, NAV_LADDERS);
		for (index = 0; index < found; index++)
		{
			struct nav_ladder *ladder;

			ladder = &nav_ladders[nav_ladder_count];
			ladder->stand[0] = raw[index].stand_x;
			ladder->stand[1] = raw[index].stand_z;
			ladder->stand[2] = raw[index].stand_y;
			ladder->land[0] = raw[index].land_x;
			ladder->land[1] = raw[index].land_z;
			ladder->land[2] = raw[index].land_y;
			ladder->yaw = raw[index].yaw;
			ladder->top = raw[index].top;
			ladder->base_z = raw[index].stand_z;
			ladder->face_x = raw[index].face_x;
			ladder->face_y = raw[index].face_y;
			platform_log(
				"nav ladder %d stand %.2f %.2f %.2f face %.2f %.2f yaw %.2f top %.2f land %.2f %.2f %.2f",
				nav_ladder_count,
				raw[index].stand_x, raw[index].stand_y, raw[index].stand_z,
				raw[index].face_x, raw[index].face_y, raw[index].yaw, raw[index].top,
				raw[index].land_x, raw[index].land_y, raw[index].land_z);
			nav_ladder_count++;
		}
		platform_log("nav ladders %d", nav_ladder_count);
	}
	return 1;
}

static int nav_build_tile(int tile_x, int tile_z)
{
	rcContext context;
	rcConfig config;
	rcHeightfield *solid;
	rcCompactHeightfield *compact;
	rcContourSet *contours;
	rcPolyMesh *mesh;
	rcPolyMeshDetail *detail;
	struct tile_span *span;
	int *local_tris;
	unsigned char *local_areas;
	int local_count;
	int index;
	int walkable_height;
	int walkable_climb;
	int walkable_radius;
	float tile_world;
	unsigned char *nav_data;
	int nav_size;
	int flagged;
	dtNavMeshCreateParams params;

	span = &nav_spans[tile_z * nav_tiles_x + tile_x];
	if (span->count <= 0)
		return 1;

	solid = 0;
	compact = 0;
	contours = 0;
	mesh = 0;
	detail = 0;
	local_tris = 0;
	local_areas = 0;
	nav_data = 0;
	local_count = span->count;
	local_tris = (int *)nav_heap_alloc((size_t)local_count * 3 * sizeof(int));
	local_areas = (unsigned char *)nav_heap_alloc((size_t)local_count);
	if (!local_tris || !local_areas)
	{
		nav_heap_release(local_tris);
		nav_heap_release(local_areas);
		return 0;
	}
	for (index = 0; index < local_count; index++)
	{
		int triangle;

		triangle = span->ids[index];
		local_tris[index * 3] = nav_tris[triangle * 3];
		local_tris[index * 3 + 1] = nav_tris[triangle * 3 + 1];
		local_tris[index * 3 + 2] = nav_tris[triangle * 3 + 2];
		local_areas[index] = nav_areas[triangle];
	}

	walkable_height = (int)ceilf(AGENT_HEIGHT / CELL_HEIGHT);
	walkable_climb = (int)floorf(AGENT_CLIMB / CELL_HEIGHT);
	walkable_radius = (int)ceilf(AGENT_RADIUS / CELL_SIZE);
	if (walkable_height < 3)
		walkable_height = 3;
	if (walkable_climb < 1)
		walkable_climb = 1;
	if (walkable_radius < 1)
		walkable_radius = 1;
	tile_world = TILE_CELLS * CELL_SIZE;

	memset(&config, 0, sizeof(config));
	config.cs = CELL_SIZE;
	config.ch = CELL_HEIGHT;
	config.walkableSlopeAngle = WALKABLE_SLOPE;
	config.walkableHeight = walkable_height;
	config.walkableClimb = walkable_climb;
	config.walkableRadius = walkable_radius;
	config.maxEdgeLen = (int)(12.0f / CELL_SIZE);
	config.maxSimplificationError = 1.3f;
	config.minRegionArea = 16;
	config.mergeRegionArea = 40;
	config.maxVertsPerPoly = 6;
	config.tileSize = TILE_CELLS;
	config.borderSize = walkable_radius + 3;
	config.detailSampleDist = CELL_SIZE * 6.0f;
	config.detailSampleMaxError = CELL_HEIGHT;
	config.bmin[0] = nav_origin[0] + (float)tile_x * tile_world;
	config.bmin[1] = nav_origin[1];
	config.bmin[2] = nav_origin[2] + (float)tile_z * tile_world;
	config.bmax[0] = config.bmin[0] + tile_world;
	config.bmax[1] = nav_y_max;
	config.bmax[2] = config.bmin[2] + tile_world;
	config.bmin[0] -= (float)config.borderSize * config.cs;
	config.bmin[2] -= (float)config.borderSize * config.cs;
	config.bmax[0] += (float)config.borderSize * config.cs;
	config.bmax[2] += (float)config.borderSize * config.cs;
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

	solid = rcAllocHeightfield();
	if (!solid || !rcCreateHeightfield(&context, *solid, config.width, config.height,
			config.bmin, config.bmax, config.cs, config.ch))
		goto skip;
	if (!rcRasterizeTriangles(&context, nav_verts, nav_vert_count, local_tris, local_areas,
			local_count, *solid, config.walkableClimb))
		goto skip;
	rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *solid);
	rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *solid);
	rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *solid);

	compact = rcAllocCompactHeightfield();
	if (!compact || !rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb,
			*solid, *compact))
		goto skip;
	rcFreeHeightField(solid);
	solid = 0;
	if (!rcErodeWalkableArea(&context, config.walkableRadius, *compact))
		goto skip;
	if (!rcBuildDistanceField(&context, *compact))
		goto skip;
	if (!rcBuildRegions(&context, *compact, config.borderSize, config.minRegionArea, config.mergeRegionArea))
		goto skip;

	contours = rcAllocContourSet();
	if (!contours || !rcBuildContours(&context, *compact, config.maxSimplificationError,
			config.maxEdgeLen, *contours))
		goto skip;
	mesh = rcAllocPolyMesh();
	if (!mesh || !rcBuildPolyMesh(&context, *contours, config.maxVertsPerPoly, *mesh))
		goto skip;
	detail = rcAllocPolyMeshDetail();
	if (!detail || !rcBuildPolyMeshDetail(&context, *mesh, *compact, config.detailSampleDist,
			config.detailSampleMaxError, *detail))
		goto skip;
	rcFreeCompactHeightfield(compact);
	compact = 0;
	rcFreeContourSet(contours);
	contours = 0;

	if (mesh->nverts <= 0 || mesh->npolys <= 0 || mesh->nverts >= 0xffff)
		goto skip;
	flagged = 0;
	for (index = 0; index < mesh->npolys; index++)
	{
		if (mesh->areas[index] == RC_WALKABLE_AREA)
		{
			mesh->flags[index] = 1;
			flagged++;
		}
		else
			mesh->flags[index] = 0;
	}
	if (!flagged)
		goto skip;

	{
		float con_verts[32 * 6];
		float con_rad[32];
		unsigned short con_flags[32];
		unsigned char con_area[32];
		unsigned char con_dir[32];
		unsigned int con_id[32];
		int con_count;
		int ladder_index;
		float core_min_x;
		float core_max_x;
		float core_min_z;
		float core_max_z;

		con_count = 0;
		core_min_x = nav_origin[0] + (float)tile_x * tile_world;
		core_max_x = core_min_x + tile_world;
		core_min_z = nav_origin[2] + (float)tile_z * tile_world;
		core_max_z = core_min_z + tile_world;
		for (ladder_index = 0; ladder_index < nav_ladder_count && con_count < 32; ladder_index++)
		{
			struct nav_ladder *ladder;
			int base;

			ladder = &nav_ladders[ladder_index];
			if (ladder->stand[0] < core_min_x || ladder->stand[0] >= core_max_x ||
				ladder->stand[2] < core_min_z || ladder->stand[2] >= core_max_z)
				continue;
			if (mesh->npolys + con_count + 1 > nav_poly_limit)
				break;
			base = con_count * 6;
			con_verts[base + 0] = ladder->stand[0];
			con_verts[base + 1] = ladder->stand[1];
			con_verts[base + 2] = ladder->stand[2];
			con_verts[base + 3] = ladder->land[0];
			con_verts[base + 4] = ladder->land[1];
			con_verts[base + 5] = ladder->land[2];
			/* wide enough that a point just inside the eroded lip still
			lands on a polygon. Vertical snap stays the step height. */
			con_rad[con_count] = 1.80f;
			con_flags[con_count] = 1;
			con_area[con_count] = RC_WALKABLE_AREA;
			con_dir[con_count] = (unsigned char)DT_OFFMESH_CON_BIDIR;
			con_id[con_count] = (unsigned int)ladder_index;
			con_count++;
		}
		memset(&params, 0, sizeof(params));
		params.offMeshConVerts = con_count ? con_verts : 0;
		params.offMeshConRad = con_count ? con_rad : 0;
		params.offMeshConFlags = con_count ? con_flags : 0;
		params.offMeshConAreas = con_count ? con_area : 0;
		params.offMeshConDir = con_count ? con_dir : 0;
		params.offMeshConUserID = con_count ? con_id : 0;
		params.offMeshConCount = con_count;
	params.verts = mesh->verts;
	params.vertCount = mesh->nverts;
	params.polys = mesh->polys;
	params.polyAreas = mesh->areas;
	params.polyFlags = mesh->flags;
	params.polyCount = mesh->npolys;
	params.nvp = mesh->nvp;
	params.detailMeshes = detail->meshes;
	params.detailVerts = detail->verts;
	params.detailVertsCount = detail->nverts;
	params.detailTris = detail->tris;
	params.detailTriCount = detail->ntris;
	params.walkableHeight = AGENT_HEIGHT;
	params.walkableRadius = AGENT_RADIUS;
	params.walkableClimb = AGENT_CLIMB;
	params.tileX = tile_x;
	params.tileY = tile_z;
	params.tileLayer = 0;
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true;
	rcVcopy(params.bmin, mesh->bmin);
	rcVcopy(params.bmax, mesh->bmax);
	if (!dtCreateNavMeshData(&params, &nav_data, &nav_size))
		goto skip;
	}
	if (dtStatusFailed(nav_mesh->addTile(nav_data, nav_size, DT_TILE_FREE_DATA, 0, 0)))
	{
		dtFree(nav_data);
		nav_data = 0;
		goto skip;
	}
	nav_data = 0;
	nav_tiles_added++;

skip:
	nav_heap_release(local_tris);
	nav_heap_release(local_areas);
	if (solid)
		rcFreeHeightField(solid);
	if (compact)
		rcFreeCompactHeightfield(compact);
	if (contours)
		rcFreeContourSet(contours);
	if (mesh)
		rcFreePolyMesh(mesh);
	if (detail)
		rcFreePolyMeshDetail(detail);
	return 1;
}

static void nav_build_some(void)
{
	int solid_built;
	int visited;
	int total;

	solid_built = 0;
	visited = 0;
	total = nav_tiles_x * nav_tiles_z;
	while (nav_next_tile < total && visited < 48 && solid_built < 2)
	{
		int tile_x;
		int tile_z;
		int had_triangles;

		tile_x = nav_next_tile % nav_tiles_x;
		tile_z = nav_next_tile / nav_tiles_x;
		had_triangles = nav_spans[nav_next_tile].count > 0;
		if (had_triangles && solid_built >= 2)
			break;
		if (!nav_build_tile(tile_x, tile_z))
		{
			struct bot_nav_identity kept;

			kept = nav_id;
			nav_release();
			nav_state = NAV_FAILED;
			nav_id = kept;
			return;
		}
		nav_next_tile++;
		visited++;
		if (had_triangles)
			solid_built++;
	}
	if (nav_next_tile >= total)
	{
		nav_free_geometry();
		if (nav_tiles_added > 0)
		{
			nav_state = NAV_READY;
			platform_log("nav ready tiles %d", nav_tiles_added);
		}
		else
		{
			nav_release();
			nav_state = NAV_FAILED;
		}
	}
}

static int nav_nearest(dtQueryFilter const *filter, float const *position, float horizontal, float vertical,
	dtPolyRef *ref, float *nearest)
{
	float extents[3];
	dtStatus status;

	extents[0] = horizontal;
	extents[1] = vertical;
	extents[2] = horizontal;
	status = nav_query->findNearestPoly(position, extents, filter, ref, nearest);
	return dtStatusSucceed(status) && *ref != 0;
}

extern "C" void bot_nav_begin_tick(void)
{
	struct bot_nav_identity id;

	nav_use_alloc();
	if (!bot_nav_bsp_identity(&id))
	{
		if (nav_state != NAV_EMPTY)
			nav_release();
		return;
	}
	if (nav_state != NAV_EMPTY && !nav_same_map(&id))
		nav_release();
	if (nav_state == NAV_EMPTY)
	{
		nav_id = id;
		if (!nav_prepare())
		{
			nav_release();
			nav_state = NAV_FAILED;
			nav_id = id;
			return;
		}
		nav_state = NAV_BUILDING;
	}
	if (nav_state == NAV_BUILDING)
		nav_build_some();
}

extern "C" int bot_nav_ready(void)
{
	struct bot_nav_identity id;

	if (nav_state != NAV_READY)
		return 0;
	if (!bot_nav_bsp_identity(&id) || !nav_same_map(&id))
		return 0;
	return 1;
}

extern "C" int bot_nav_settled(void)
{
	struct bot_nav_identity id;

	if (!bot_nav_bsp_identity(&id))
		return 1;
	if (nav_state == NAV_FAILED && nav_same_map(&id))
		return 1;
	return bot_nav_ready();
}

static void nav_note(float x, float z)
{
	int tx;
	int tz;
	int slot;
	float tile_world;

	if (nav_state != NAV_READY || nav_tiles_x < 1 || nav_tiles_z < 1)
		return;
	tile_world = TILE_CELLS * CELL_SIZE;
	tx = (int)((x - nav_origin[0]) / tile_world);
	tz = (int)((z - nav_origin[2]) / tile_world);
	if (tx < 0 || tz < 0 || tx >= nav_tiles_x || tz >= nav_tiles_z)
		return;
	slot = tz * nav_tiles_x + tx;
	if (slot < 0 || slot >= 4096 || nav_seen[slot])
		return;
	nav_seen[slot] = 1;
	nav_seen_count++;
}

struct nav_pick
{
	float dist;
	float nearest[3];
	dtPolyRef ref;
	int slot;
};

static void nav_keep_pick(struct nav_pick *slot, int *count, int limit, float dist,
	dtPolyRef ref, float const *nearest, int tile_slot, int farthest)
{
	int place;
	int extreme;
	int n;

	if (*count < limit)
		place = (*count)++;
	else
	{
		extreme = 0;
		for (n = 1; n < limit; n++)
		{
			if (farthest)
			{
				if (slot[n].dist < slot[extreme].dist)
					extreme = n;
			}
			else if (slot[n].dist > slot[extreme].dist)
				extreme = n;
		}
		if (farthest)
		{
			if (dist <= slot[extreme].dist)
				return;
		}
		else if (dist >= slot[extreme].dist)
			return;
		place = extreme;
	}
	slot[place].dist = dist;
	slot[place].ref = ref;
	slot[place].nearest[0] = nearest[0];
	slot[place].nearest[1] = nearest[1];
	slot[place].nearest[2] = nearest[2];
	slot[place].slot = tile_slot;
}

static int nav_complete_picks(dtQueryFilter *filter, dtPolyRef start_ref, float const *start_point,
	struct nav_pick const *cands, int cand_count, struct nav_pick *ok)
{
	static dtPolyRef path[2048];
	int index;
	int ok_count;

	ok_count = 0;
	for (index = 0; index < cand_count; index++)
	{
		int path_count;
		dtStatus status;

		status = nav_query->findPath(start_ref, cands[index].ref, start_point, cands[index].nearest,
			filter, path, &path_count, 2048);
		if (dtStatusFailed(status) || path_count <= 0)
			continue;
		if (dtStatusDetail(status, DT_PARTIAL_RESULT) || dtStatusDetail(status, DT_BUFFER_TOO_SMALL) ||
			path[path_count - 1] != cands[index].ref)
			continue;
		ok[ok_count] = cands[index];
		ok_count++;
	}
	return ok_count;
}

/* the first stop is whichever reachable place this bot was dealt.
the rest sit as far from those stops as the mesh allows, then the
list is walked in heading order. the far edge of the map is one
stop, not the whole trip. */
static void nav_pick_tour(struct nav_pick const *pool, int pool_count,
	float const *from, int salt, int limit, int *out_index, int *out_count)
{
	int used[24];
	int chosen[8];
	int count;
	int index;
	int seed;
	float base;

	*out_count = 0;
	if (!pool || pool_count <= 0 || limit <= 0)
		return;
	if (pool_count > 24)
		pool_count = 24;
	if (limit > 8)
		limit = 8;
	if (limit > pool_count)
		limit = pool_count;
	for (index = 0; index < pool_count; index++)
		used[index] = 0;
	seed = salt % pool_count;
	if (seed < 0)
		seed = -seed;
	chosen[0] = seed;
	used[seed] = 1;
	count = 1;
	base = atan2f(pool[seed].nearest[2] - from[2], pool[seed].nearest[0] - from[0]);
	while (count < limit)
	{
		int best;
		float best_score;
		int item;

		best = -1;
		best_score = -1.0f;
		for (item = 0; item < pool_count; item++)
		{
			float score;
			int prev;

			if (used[item])
				continue;
			score = 1.0e9f;
			for (prev = 0; prev < count; prev++)
			{
				float dx;
				float dz;
				float dist;

				dx = pool[item].nearest[0] - pool[chosen[prev]].nearest[0];
				dz = pool[item].nearest[2] - pool[chosen[prev]].nearest[2];
				dist = dx * dx + dz * dz;
				if (dist < score)
					score = dist;
			}
			if (score > best_score)
			{
				best_score = score;
				best = item;
			}
		}
		if (best < 0)
			break;
		chosen[count] = best;
		used[best] = 1;
		count++;
	}
	for (index = 1; index < count; index++)
	{
		int item;
		float item_turn;
		int place;

		item = chosen[index];
		item_turn = atan2f(pool[item].nearest[2] - from[2],
			pool[item].nearest[0] - from[0]) - base;
		if (item_turn < 0.0f)
			item_turn += 6.2831853f;
		place = index;
		while (place > 0)
		{
			float prev_turn;
			int prev;

			prev = chosen[place - 1];
			prev_turn = atan2f(pool[prev].nearest[2] - from[2],
				pool[prev].nearest[0] - from[0]) - base;
			if (prev_turn < 0.0f)
				prev_turn += 6.2831853f;
			if (prev_turn <= item_turn)
				break;
			chosen[place] = prev;
			place--;
		}
		chosen[place] = item;
	}
	for (index = 0; index < count; index++)
		out_index[index] = chosen[index];
	*out_count = count;
}

extern "C" int bot_nav_explore(float x, float y, float z, int salt, float *points, int limit)
{
	dtQueryFilter filter;
	float start[3];
	float start_point[3];
	dtPolyRef start_ref;
	int pass;

	if (!points || limit <= 0 || nav_state != NAV_READY || !nav_mesh || !nav_query)
		return 0;
	if (x != x || y != y || z != z)
		return 0;
	nav_note(x, z);
	start[0] = x;
	start[1] = y;
	start[2] = z;
	nav_to_recast(start);
	filter.setIncludeFlags(0x1);
	filter.setExcludeFlags(0);
	if (!nav_nearest(&filter, start, 3.0f, 2.5f, &start_ref, start_point))
		return 0;
	for (pass = 0; pass < 2; pass++)
	{
		struct nav_pick far[12];
		struct nav_pick near[12];
		struct nav_pick ok[24];
		int far_count;
		int near_count;
		int ok_count;
		int tile_index;
		int tiles;

		far_count = 0;
		near_count = 0;
		ok_count = 0;
		tiles = nav_mesh->getMaxTiles();
		for (tile_index = 0; tile_index < tiles; tile_index++)
		{
			const dtMeshTile *tile;
			int tx;
			int ty;
			int slot;
			float point[3];
			float extents[3];
			float nearest[3];
			dtPolyRef ref;
			float dx;
			float dz;
			float dist;

			tile = static_cast<dtNavMesh const *>(nav_mesh)->getTile(tile_index);
			if (!tile || !tile->header)
				continue;
			if (tile->header->polyCount <= tile->header->offMeshConCount)
				continue;
			tx = tile->header->x;
			ty = tile->header->y;
			if (tx < 0 || ty < 0 || tx >= nav_tiles_x || ty >= nav_tiles_z)
				continue;
			slot = ty * nav_tiles_x + tx;
			if (slot < 0 || slot >= 4096 || nav_seen[slot])
				continue;
			point[0] = (tile->header->bmin[0] + tile->header->bmax[0]) * 0.5f;
			point[1] = (tile->header->bmin[1] + tile->header->bmax[1]) * 0.5f;
			point[2] = (tile->header->bmin[2] + tile->header->bmax[2]) * 0.5f;
			extents[0] = TILE_CELLS * CELL_SIZE;
			extents[2] = extents[0];
			extents[1] = (tile->header->bmax[1] - tile->header->bmin[1]) * 0.5f + 2.0f;
			if (extents[1] < 4.0f)
				extents[1] = 4.0f;
			if (!dtStatusSucceed(nav_query->findNearestPoly(point, extents, &filter, &ref, nearest)) || !ref)
				continue;
			dx = nearest[0] - start_point[0];
			dz = nearest[2] - start_point[2];
			dist = dx * dx + dz * dz;
			nav_keep_pick(far, &far_count, 12, dist, ref, nearest, slot, 1);
			nav_keep_pick(near, &near_count, 12, dist, ref, nearest, slot, 0);
		}
		ok_count = nav_complete_picks(&filter, start_ref, start_point, far, far_count, ok);
		{
			struct nav_pick more[12];
			int more_count;
			int extra;

			/* a nearer tile is a real stop. the far list alone is the
			same edge of the map every time. */
			more_count = nav_complete_picks(&filter, start_ref, start_point,
				near, near_count, more);
			for (extra = 0; extra < more_count && ok_count < 24; extra++)
			{
				int have;
				int item;

				have = 0;
				for (item = 0; item < ok_count; item++)
				{
					if (ok[item].slot == more[extra].slot)
						have = 1;
				}
				if (!have)
					ok[ok_count++] = more[extra];
			}
		}
		if (ok_count > 0)
		{
			int tour[8];
			int tour_count;
			int stop;

			nav_pick_tour(ok, ok_count, start_point, salt, limit, tour, &tour_count);
			for (stop = 0; stop < tour_count; stop++)
			{
				int seen_slot;

				nav_to_halo(ok[tour[stop]].nearest,
					&points[stop * 3], &points[stop * 3 + 1], &points[stop * 3 + 2]);
				seen_slot = ok[tour[stop]].slot;
				if (seen_slot >= 0 && seen_slot < 4096 && !nav_seen[seen_slot])
				{
					nav_seen[seen_slot] = 1;
					nav_seen_count++;
				}
			}
			platform_log("nav tour p%d n%d", salt, tour_count);
			return tour_count;
		}
		if (pass == 0)
		{
			memset(nav_seen, 0, sizeof(nav_seen));
			nav_seen_count = 0;
			nav_note(x, z);
		}
	}
	return 0;
}

/* A goal can be reached by more than one corridor. The short ones are
split across the bots, so a match does not file through a single hall.
A body that has stopped, or that is already standing on another
corridor, changes to that one. A much longer walk is kept for that
moment, and is not the trip they start. */

static int nav_poly_center(dtPolyRef ref, float *center)
{
	const dtMeshTile *tile;
	const dtPoly *poly;
	int vert;

	if (dtStatusFailed(nav_mesh->getTileAndPolyByRef(ref, &tile, &poly)) || !poly ||
		poly->vertCount <= 0 || !tile || !tile->verts)
		return 0;
	center[0] = 0.0f;
	center[1] = 0.0f;
	center[2] = 0.0f;
	for (vert = 0; vert < poly->vertCount; vert++)
	{
		float const *point;

		point = &tile->verts[poly->verts[vert] * 3];
		center[0] += point[0];
		center[1] += point[1];
		center[2] += point[2];
	}
	center[0] /= (float)poly->vertCount;
	center[1] /= (float)poly->vertCount;
	center[2] /= (float)poly->vertCount;
	return 1;
}

static float nav_path_length(dtPolyRef const *path, int count,
	float const *from, float const *goal)
{
	float prev[3];
	float total;
	int index;

	prev[0] = from[0];
	prev[1] = from[1];
	prev[2] = from[2];
	total = 0.0f;
	for (index = 0; index < count; index++)
	{
		float center[3];
		float dx;
		float dy;
		float dz;

		if (!nav_poly_center(path[index], center))
			continue;
		dx = center[0] - prev[0];
		dy = center[1] - prev[1];
		dz = center[2] - prev[2];
		total += sqrtf(dx * dx + dy * dy + dz * dz);
		prev[0] = center[0];
		prev[1] = center[1];
		prev[2] = center[2];
	}
	{
		float dx;
		float dy;
		float dz;

		dx = goal[0] - prev[0];
		dy = goal[1] - prev[1];
		dz = goal[2] - prev[2];
		total += sqrtf(dx * dx + dy * dy + dz * dz);
	}
	return total;
}

static float nav_span(float const *from, float const *goal)
{
	float dx;
	float dy;
	float dz;

	dx = goal[0] - from[0];
	dy = goal[1] - from[1];
	dz = goal[2] - from[2];
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

static int nav_route_contains(struct nav_route const *route, dtPolyRef ref)
{
	int index;

	if (!ref)
		return 0;
	for (index = 0; index < route->count; index++)
	{
		if (route->polys[index] == ref)
			return 1;
	}
	return 0;
}

static int nav_exact(struct nav_route const *route, dtPolyRef ref)
{
	int index;
	int found;

	found = -1;
	for (index = 0; index < route->count; index++)
	{
		if (route->polys[index] == ref)
			found = index;
	}
	return found;
}

/* the polygon under them, or the nearest one on this corridor that is
not back toward the way they came. -1 when this corridor is elsewhere. */
static int nav_locate(struct nav_route const *route, dtPolyRef start_ref,
	float const *from, float const *goal)
{
	int index;
	int found;
	float from_goal;
	float best;
	float dx;
	float dy;
	float dz;

	found = nav_exact(route, start_ref);
	if (found >= 0)
		return found;
	dx = goal[0] - from[0];
	dy = goal[1] - from[1];
	dz = goal[2] - from[2];
	from_goal = dx * dx + dy * dy + dz * dz;
	best = 6.0f * 6.0f;
	found = -1;
	for (index = 0; index < route->count; index++)
	{
		float center[3];
		float dist;
		float goal_dist;

		if (!nav_poly_center(route->polys[index], center))
			continue;
		dx = center[0] - from[0];
		dy = center[1] - from[1];
		dz = center[2] - from[2];
		dist = dx * dx + dy * dy + dz * dz;
		if (dist >= best)
			continue;
		dx = center[0] - goal[0];
		dy = center[1] - goal[1];
		dz = center[2] - goal[2];
		goal_dist = dx * dx + dy * dy + dz * dz;
		if (goal_dist > from_goal + 2.5f * 2.5f && dist > 1.5f * 1.5f)
			continue;
		best = dist;
		found = index;
	}
	return found;
}

static int nav_poly_shared(struct nav_agent const *agent, int index, dtPolyRef ref)
{
	int other;

	for (other = 0; other < agent->route_count; other++)
	{
		if (other == index)
			continue;
		if (nav_route_contains(&agent->routes[other], ref))
			return 1;
	}
	return 0;
}

/* a polygon that belongs to this corridor and not the others, so a later
search can tell the halls apart. The shared doorway is not that. */
static dtPolyRef nav_anchor_of(struct nav_agent const *agent, int index)
{
	struct nav_route const *route;
	int mid;
	int at;

	route = &agent->routes[index];
	if (route->count <= 0)
		return 0;
	mid = route->count / 2;
	for (at = mid; at >= 2; at--)
	{
		if (!nav_poly_shared(agent, index, route->polys[at]))
			return route->polys[at];
	}
	for (at = mid + 1; at <= route->count - 3; at++)
	{
		if (!nav_poly_shared(agent, index, route->polys[at]))
			return route->polys[at];
	}
	return route->polys[mid];
}

/* 1 when the new strip is the same hall. A doorway that rejoins counts
as another hall only when a couple of its polygons are actually new. */
static int nav_same_corridor(struct nav_agent const *agent, dtPolyRef const *path, int count)
{
	int begin;
	int end;
	int index;
	int interior;
	int shared;

	begin = count / 7;
	end = count - count / 7;
	if (begin < 1)
		begin = 1;
	if (end > count - 1)
		end = count - 1;
	if (end - begin < 2)
		return 1;
	interior = 0;
	shared = 0;
	for (index = begin; index < end; index++)
	{
		int route;
		int found;

		interior++;
		found = 0;
		for (route = 0; route < agent->route_count && !found; route++)
		{
			if (nav_route_contains(&agent->routes[route], path[index]))
				found = 1;
		}
		if (found)
			shared++;
	}
	if (interior - shared < 2)
		return 1;
	if (shared * 100 >= interior * 68)
		return 1;
	return 0;
}

/* 1 when the middle of this strip is still in a hall we already kept.
a one-step jog around a post is not another way across the map. */
static int nav_same_place(struct nav_agent const *agent, dtPolyRef const *path, int count)
{
	float center[3];
	int route;

	if (count < 3 || !nav_poly_center(path[count / 2], center))
		return 0;
	for (route = 0; route < agent->route_count; route++)
	{
		struct nav_route const *item;
		float other[3];
		float dx;
		float dy;
		float dz;

		item = &agent->routes[route];
		if (item->count < 3 || !nav_poly_center(item->polys[item->count / 2], other))
			continue;
		dx = other[0] - center[0];
		dy = other[1] - center[1];
		dz = other[2] - center[2];
		if (dx * dx + dy * dy + dz * dz < 5.0f * 5.0f)
			return 1;
	}
	return 0;
}

static int nav_search(dtQueryFilter *filter, dtPolyRef start_ref, dtPolyRef goal_ref,
	float const *start_point, float const *goal_point,
	dtPolyRef *path, int *path_count, int *complete)
{
	dtStatus status;

	*path_count = 0;
	*complete = 0;
	status = nav_query->findPath(start_ref, goal_ref, start_point, goal_point, filter,
		path, path_count, 2048);
	if (dtStatusFailed(status) || *path_count <= 0)
		return 0;
	*complete = path[*path_count - 1] == goal_ref &&
		!dtStatusDetail(status, DT_PARTIAL_RESULT) &&
		!dtStatusDetail(status, DT_BUFFER_TOO_SMALL);
	return 1;
}

struct nav_area_edit
{
	dtPoly *poly;
	unsigned char saved;
};

static struct nav_area_edit nav_edits[NAV_ROUTE_POLYS * NAV_ROUTES];
static int nav_edit_count;

static void nav_slow_clear(void)
{
	int index;

	for (index = 0; index < nav_edit_count; index++)
	{
		if (nav_edits[index].poly)
			nav_edits[index].poly->areaAndtype = nav_edits[index].saved;
		nav_edits[index].poly = 0;
	}
	nav_edit_count = 0;
}

static void nav_slow_mark(dtPolyRef ref)
{
	const dtMeshTile *tile;
	const dtPoly *poly;
	dtPoly *editable;
	int index;

	if (nav_edit_count >= NAV_ROUTE_POLYS * NAV_ROUTES)
		return;
	if (dtStatusFailed(nav_mesh->getTileAndPolyByRef(ref, &tile, &poly)) || !poly)
		return;
	editable = (dtPoly *)poly;
	for (index = 0; index < nav_edit_count; index++)
	{
		if (nav_edits[index].poly == editable)
			return;
	}
	nav_edits[nav_edit_count].poly = editable;
	nav_edits[nav_edit_count].saved = editable->areaAndtype;
	nav_edit_count++;
	editable->setArea(NAV_AREA_SLOW);
}

/* the middle of each corridor costs more for the next search, so that
search has to leave the hall. The ends stay cheap: every route still
has to start here and finish at the goal. Areas are put back before
anything else asks the mesh. */
static void nav_slow_apply(struct nav_agent const *agent, dtPolyRef start_ref, dtPolyRef goal_ref)
{
	int route;

	nav_edit_count = 0;
	for (route = 0; route < agent->route_count; route++)
	{
		struct nav_route const *item;
		int begin;
		int end;
		int index;

		item = &agent->routes[route];
		if (item->count < 6)
			continue;
		begin = item->count / 7;
		end = item->count - item->count / 7;
		if (begin < 1)
			begin = 1;
		if (end > item->count - 1)
			end = item->count - 1;
		for (index = begin; index < end; index++)
		{
			if (item->polys[index] == start_ref || item->polys[index] == goal_ref)
				continue;
			nav_slow_mark(item->polys[index]);
		}
	}
}

static void nav_store(struct nav_route *route, dtPolyRef const *path, int count,
	int complete, float length)
{
	int index;

	for (index = 0; index < count; index++)
		route->polys[index] = path[index];
	route->count = count;
	route->complete = complete;
	route->length = length;
}

/* 1 when the corridor they already had is still one of these. Otherwise
the short corridors are dealt out by player, and a long one waits. */
static int nav_assign(struct nav_agent *agent, int agent_index, dtPolyRef kept)
{
	int preferred[NAV_ROUTES];
	int count;
	int index;
	float best;
	int keep_at;

	count = 0;
	best = 1.0e9f;
	for (index = 0; index < agent->route_count; index++)
	{
		if (!agent->routes[index].complete)
			continue;
		if (agent->routes[index].length < best)
			best = agent->routes[index].length;
	}
	for (index = 0; index < agent->route_count; index++)
	{
		if (!agent->routes[index].complete)
			continue;
		/* a flank that is only somewhat longer is still a path to
		hand out. The much longer one stays for when this one fails. */
		if (agent->routes[index].length <= best * 1.65f + 12.0f)
			preferred[count++] = index;
	}
	for (index = 1; index < count; index++)
	{
		int item;
		int slot;

		item = preferred[index];
		slot = index;
		while (slot > 0 &&
			agent->routes[preferred[slot - 1]].length > agent->routes[item].length)
		{
			preferred[slot] = preferred[slot - 1];
			slot--;
		}
		preferred[slot] = item;
	}
	keep_at = -1;
	if (kept)
	{
		float keep_len;
		int hits;

		keep_len = 1.0e9f;
		hits = 0;
		for (index = 0; index < agent->route_count; index++)
		{
			if (!agent->routes[index].complete)
				continue;
			if (!nav_route_contains(&agent->routes[index], kept))
				continue;
			hits++;
			if (agent->routes[index].length >= keep_len)
				continue;
			keep_len = agent->routes[index].length;
			keep_at = index;
		}
		/* the doorway every route uses is not a commitment, and
		neither is the short hall. Keeping that one put every bot
		back on the normal path. A flank they already took stays. */
		if (hits != 1 || keep_at == 0)
			keep_at = -1;
	}
	if (keep_at >= 0)
		agent->active = keep_at;
	else if (count > 0)
		agent->active = preferred[agent_index % count];
	else
		agent->active = 0;
	agent->anchor = nav_anchor_of(agent, agent->active);
	if (keep_at < 0)
	{
		agent->hold = NAV_HOLD_TRY;
		agent->switches = 0;
	}
	return keep_at >= 0;
}

static int nav_goal_close(struct nav_agent const *agent, float const *goal)
{
	float dx;
	float dy;
	float dz;

	if (!agent->goal_valid)
		return 0;
	dx = goal[0] - agent->goal[0];
	dy = goal[1] - agent->goal[1];
	dz = goal[2] - agent->goal[2];
	/* a target that only sidesteps is still this trip. Replanning
	every few steps put every bot back on the short hall. */
	if (dx * dx + dz * dz > 10.0f * 10.0f)
		return 0;
	if (dy * dy > 2.0f * 2.0f)
		return 0;
	return 1;
}

static int nav_fresh(struct nav_agent const *agent, float const *goal)
{
	struct nav_route const *route;

	if (!agent->ready || agent->route_count < 1 || agent->active < 0 ||
		agent->active >= agent->route_count)
		return 0;
	if (!nav_goal_close(agent, goal))
		return 0;
	route = &agent->routes[agent->active];
	if (route->count <= 0 || !nav_mesh->isValidPolyRef(route->polys[0]))
		return 0;
	return 1;
}

static void nav_remember_goal(struct nav_agent *agent, float const *goal)
{
	agent->goal[0] = goal[0];
	agent->goal[1] = goal[1];
	agent->goal[2] = goal[2];
	agent->goal_valid = 1;
}

/* 1 stored, 0 no walk, -1 the walk is too long to keep. */
static int nav_build(struct nav_agent *agent, int agent_index,
	dtPolyRef start_ref, float const *start_point,
	dtPolyRef goal_ref, float const *goal_point)
{
	dtQueryFilter filter;
	static dtPolyRef path[2048];
	int path_count;
	int complete;
	float length;
	float best_complete;
	dtPolyRef kept;
	int pass;
	int kept_route;
	char const *alt_stop;

	kept = 0;
	alt_stop = "ok";
	if (agent->ready)
		kept = agent->anchor;
	agent->ready = 0;
	agent->route_count = 0;
	agent->lost = 0;
	nav_remember_goal(agent, goal_point);
	filter.setIncludeFlags(0x1);
	filter.setExcludeFlags(0);
	if (!nav_search(&filter, start_ref, goal_ref, start_point, goal_point,
			path, &path_count, &complete))
		return 0;
	if (path_count > NAV_ROUTE_POLYS)
		return -1;
	length = nav_path_length(path, path_count, start_point, goal_point);
	if (length < 1.0f)
		length = 1.0f;
	nav_store(&agent->routes[0], path, path_count, complete, length);
	agent->route_count = 1;
	best_complete = complete ? length : 1.0e9f;
	{
		float slow_cost;
		int same_tries;

		slow_cost = NAV_SLOW_COST;
		same_tries = 0;
		alt_stop = "ok";
		filter.setAreaCost(NAV_AREA_SLOW, slow_cost);
		for (pass = 1; pass < NAV_ROUTES; )
		{
			int alt_count;
			int alt_complete;
			float alt_length;

			if (agent->routes[0].count < 6)
			{
				alt_stop = "short";
				break;
			}
			nav_slow_apply(agent, start_ref, goal_ref);
			if (nav_edit_count < 2)
			{
				nav_slow_clear();
				alt_stop = "mark";
				break;
			}
			if (!nav_search(&filter, start_ref, goal_ref, start_point, goal_point,
					path, &alt_count, &alt_complete))
			{
				nav_slow_clear();
				alt_stop = "search";
				break;
			}
			nav_slow_clear();
			if (!alt_complete || alt_count < 2 || alt_count > NAV_ROUTE_POLYS)
			{
				alt_stop = "partial";
				break;
			}
			alt_length = nav_path_length(path, alt_count, start_point, goal_point);
			if (alt_length < 1.0f)
				alt_length = 1.0f;
			if (best_complete < 1.0e8f && alt_length > best_complete * 1.85f + 14.0f)
			{
				alt_stop = "long";
				break;
			}
			/* the cheap penalty often steps around one post and calls
			it another hall. A harder penalty has to leave the hall. */
			if (nav_same_corridor(agent, path, alt_count) ||
				nav_same_place(agent, path, alt_count))
			{
				same_tries++;
				if (same_tries >= 2)
				{
					alt_stop = "same";
					break;
				}
				slow_cost *= 3.0f;
				filter.setAreaCost(NAV_AREA_SLOW, slow_cost);
				continue;
			}
			same_tries = 0;
			slow_cost = NAV_SLOW_COST;
			filter.setAreaCost(NAV_AREA_SLOW, slow_cost);
			nav_store(&agent->routes[agent->route_count], path, alt_count, 1, alt_length);
			agent->route_count++;
			pass++;
			if (alt_length < best_complete)
				best_complete = alt_length;
		}
	}
	kept_route = nav_assign(agent, agent_index, kept);
	if (!kept_route)
		agent->best_dist = nav_span(start_point, goal_point);
	agent->ready = 1;
	platform_log("nav routes p%d n%d a%d %.0f %.0f %.0f %s",
		agent_index, agent->route_count, agent->active,
		agent->route_count > 0 ? agent->routes[0].length : 0.0f,
		agent->route_count > 1 ? agent->routes[1].length : 0.0f,
		agent->route_count > 2 ? agent->routes[2].length : 0.0f,
		alt_stop);
	return 1;
}

/* 1 a step, 0 the goal is already underfoot, -1 the strip did not walk. */
static int nav_emit(float const *start_point, float const *goal_point,
	dtPolyRef const *path, int path_count, int incomplete,
	float fx, float fy, float fz, struct bot_nav_step *step)
{
	static float straight[256 * 3];
	static unsigned char straight_flags[256];
	static dtPolyRef straight_refs[256];
	int straight_count;
	dtStatus status;
	int partial;
	int index;
	int chosen;
	float chosen_point[3];

	/* height is already in the strip. The corner test is the ground plane. */
	(void)fz;
	if (!path || path_count <= 0)
		return -1;
	partial = incomplete;
	status = nav_query->findStraightPath(start_point, goal_point, path, path_count,
		straight, straight_flags, straight_refs, &straight_count, 256, 0);
	if (dtStatusFailed(status) || straight_count <= 0)
		return -1;
	{
		float dx;
		float dy;
		float dz;

		dx = straight[(straight_count - 1) * 3] - goal_point[0];
		dy = straight[(straight_count - 1) * 3 + 1] - goal_point[1];
		dz = straight[(straight_count - 1) * 3 + 2] - goal_point[2];
		if (dx * dx + dy * dy + dz * dz > 2.25f)
			partial = 1;
	}
	step->ladder = 0;
	step->up = 0;
	step->yaw = 0.0f;
	step->top = 0.0f;
	step->land_x = 0.0f;
	step->land_y = 0.0f;
	step->land_z = 0.0f;
	step->face_x = 0.0f;
	step->face_y = 0.0f;
	chosen = -1;
	for (index = 0; index < straight_count; index++)
	{
		float hx;
		float hy;
		float hz;
		float dx;
		float dy;
		float dist2;

		nav_to_halo(straight + index * 3, &hx, &hy, &hz);
		dx = hx - fx;
		dy = hy - fy;
		dist2 = dx * dx + dy * dy;
		if (straight_flags[index] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			const dtOffMeshConnection *con;

			con = 0;
			/* far enough out to square up to the face before the body
			touches it. Inside this, the step is the climb. */
			if (dist2 <= 3.2f * 3.2f)
				con = nav_mesh->getOffMeshConnectionByRef(straight_refs[index]);
			if (con)
			{
				float ax;
				float ay;
				float az;
				float bx;
				float by;
				float bz;
				float exit_x;
				float exit_y;
				float exit_z;
				unsigned int id;

				nav_to_halo(con->pos, &ax, &ay, &az);
				nav_to_halo(con->pos + 3, &bx, &by, &bz);
				if ((hx - ax) * (hx - ax) + (hy - ay) * (hy - ay) <=
					(hx - bx) * (hx - bx) + (hy - by) * (hy - by))
				{
					exit_x = bx;
					exit_y = by;
					exit_z = bz;
				}
				else
				{
					exit_x = ax;
					exit_y = ay;
					exit_z = az;
				}
				id = con->userId;
				/* beside the shaft, a straight line at the face is the wall.
				the mesh still has to walk them to the stand. The climb
				starts once the body is in front of the rungs. */
				if (id < (unsigned int)nav_ladder_count)
				{
					float face_dx;
					float face_dy;
					float ahead;
					float side;
					float yaw;

					yaw = nav_ladders[id].yaw;
					face_dx = nav_ladders[id].face_x - fx;
					face_dy = nav_ladders[id].face_y - fy;
					ahead = cosf(yaw) * face_dx + sinf(yaw) * face_dy;
					side = -sinf(yaw) * face_dx + cosf(yaw) * face_dy;
					if (side < 0.0f)
						side = -side;
					if (side > 0.50f || ahead > 1.90f || ahead < 0.02f)
					{
						/* beside the rungs. The stand is down the face,
						and the straight line to it meets the rail. Step
						onto the centerline first. */
						if (side > 0.50f && ahead > 0.05f && ahead < 2.2f)
						{
							float out;

							out = ahead;
							if (out < 1.15f)
								out = 1.15f;
							chosen = index;
							chosen_point[0] = nav_ladders[id].face_x - cosf(yaw) * out;
							chosen_point[1] = nav_ladders[id].face_y - sinf(yaw) * out;
							chosen_point[2] = hz;
							break;
						}
						con = 0;
					}
				}
				if (!con)
				{
					chosen = index;
					chosen_point[0] = hx;
					chosen_point[1] = hy;
					chosen_point[2] = hz;
					break;
				}
				step->x = exit_x;
				step->y = exit_y;
				step->z = exit_z;
				step->land_x = exit_x;
				step->land_y = exit_y;
				step->land_z = exit_z;
				step->ladder = 1;
				step->up = exit_z > hz + 0.35f;
				step->partial = partial;
				if (id < (unsigned int)nav_ladder_count)
				{
					step->yaw = nav_ladders[id].yaw;
					step->top = nav_ladders[id].top;
					step->face_x = nav_ladders[id].face_x;
					step->face_y = nav_ladders[id].face_y;
				}
				else
				{
					step->yaw = atan2f(hy - fy, hx - fx);
					step->top = exit_z;
					step->face_x = hx;
					step->face_y = hy;
				}
				return 1;
			}
			chosen = index;
			chosen_point[0] = hx;
			chosen_point[1] = hy;
			chosen_point[2] = hz;
			break;
		}
		if (dist2 > 0.75f * 0.75f)
		{
			chosen = index;
			chosen_point[0] = hx;
			chosen_point[1] = hy;
			chosen_point[2] = hz;
			break;
		}
	}
	if (chosen < 0)
	{
		if (!partial)
			return 0;
		nav_to_halo(straight + (straight_count - 1) * 3,
			&chosen_point[0], &chosen_point[1], &chosen_point[2]);
	}
	step->x = chosen_point[0];
	step->y = chosen_point[1];
	step->z = chosen_point[2];
	step->partial = partial;
	return 1;
}

static int nav_emit_slice(struct nav_route const *route, int index,
	float const *start_point, float const *goal_point,
	float fx, float fy, float fz, struct bot_nav_step *step)
{
	float use_goal[3];
	int partial;

	if (index < 0 || index >= route->count)
		return -1;
	use_goal[0] = goal_point[0];
	use_goal[1] = goal_point[1];
	use_goal[2] = goal_point[2];
	partial = !route->complete;
	if (partial)
	{
		float clamped[3];

		if (dtStatusSucceed(nav_query->closestPointOnPoly(
				route->polys[route->count - 1], goal_point, clamped, 0)))
		{
			use_goal[0] = clamped[0];
			use_goal[1] = clamped[1];
			use_goal[2] = clamped[2];
		}
	}
	return nav_emit(start_point, use_goal, route->polys + index, route->count - index,
		partial, fx, fy, fz, step);
}

static int nav_direct(float const *start_point, float const *goal_point,
	dtPolyRef start_ref, dtPolyRef goal_ref,
	float fx, float fy, float fz, struct bot_nav_step *step)
{
	dtQueryFilter filter;
	static dtPolyRef path[2048];
	int path_count;
	int complete;
	int result;
	float use_goal[3];

	filter.setIncludeFlags(0x1);
	filter.setExcludeFlags(0);
	if (!nav_search(&filter, start_ref, goal_ref, start_point, goal_point,
			path, &path_count, &complete))
		return 0;
	use_goal[0] = goal_point[0];
	use_goal[1] = goal_point[1];
	use_goal[2] = goal_point[2];
	if (!complete)
	{
		float clamped[3];

		if (dtStatusSucceed(nav_query->closestPointOnPoly(
				path[path_count - 1], goal_point, clamped, 0)))
		{
			use_goal[0] = clamped[0];
			use_goal[1] = clamped[1];
			use_goal[2] = clamped[2];
		}
	}
	result = nav_emit(start_point, use_goal, path, path_count, !complete, fx, fy, fz, step);
	return result == 1;
}

static void nav_take(struct nav_agent *agent, int which)
{
	agent->active = which;
	agent->anchor = nav_anchor_of(agent, which);
	agent->hold = NAV_HOLD_SWITCH;
	agent->switches++;
}

static void nav_consider_switch(struct nav_agent *agent, dtPolyRef start_ref,
	float const *start_point, float const *goal_point,
	float fx, float fy, float fz, int blocked, int *switched)
{
	int index;
	int on;
	struct bot_nav_step current;
	struct bot_nav_step other;

	if (agent->hold > 0)
		agent->hold--;
	if (agent->hold > 0 || agent->route_count < 2)
		return;
	if (agent->switches >= agent->route_count)
		return;
	on = nav_exact(&agent->routes[agent->active], start_ref);
	if (on < 0)
	{
		for (index = 0; index < agent->route_count; index++)
		{
			int at;

			if (index == agent->active)
				continue;
			if (agent->routes[agent->active].complete && !agent->routes[index].complete)
				continue;
			at = nav_exact(&agent->routes[index], start_ref);
			if (at < 0)
				continue;
			if (nav_emit_slice(&agent->routes[index], at, start_point, goal_point,
					fx, fy, fz, &other) != 1)
				continue;
			nav_take(agent, index);
			*switched = 1;
			return;
		}
	}
	if (!blocked)
		return;
	on = nav_locate(&agent->routes[agent->active], start_ref, start_point, goal_point);
	if (on < 0)
		return;
	if (nav_emit_slice(&agent->routes[agent->active], on, start_point, goal_point,
			fx, fy, fz, &current) != 1)
		return;
	for (index = 1; index < agent->route_count; index++)
	{
		int which;
		int at;
		float dx;
		float dy;
		float dz;

		which = (agent->active + index) % agent->route_count;
		if (agent->routes[agent->active].complete && !agent->routes[which].complete)
			continue;
		at = nav_locate(&agent->routes[which], start_ref, start_point, goal_point);
		if (at < 0)
			continue;
		if (nav_emit_slice(&agent->routes[which], at, start_point, goal_point,
				fx, fy, fz, &other) != 1)
			continue;
		dx = other.x - current.x;
		dy = other.y - current.y;
		dz = other.z - current.z;
		if (dx * dx + dy * dy + dz * dz <= 2.2f * 2.2f)
			continue;
		nav_take(agent, which);
		*switched = 1;
		return;
	}
}

static int nav_follow(struct nav_agent *agent, dtPolyRef start_ref,
	float const *start_point, float const *goal_point,
	float fx, float fy, float fz, struct bot_nav_step *step)
{
	struct nav_route const *route;
	int at;

	if (agent->active < 0 || agent->active >= agent->route_count)
		return -1;
	route = &agent->routes[agent->active];
	at = nav_locate(route, start_ref, start_point, goal_point);
	if (at < 0)
		return -1;
	return nav_emit_slice(route, at, start_point, goal_point, fx, fy, fz, step);
}

static void nav_note_progress(struct nav_agent *agent, float const *from, float const *goal)
{
	float dist;

	dist = nav_span(from, goal);
	if (agent->best_dist <= 0.0f)
		agent->best_dist = dist;
	else if (dist < agent->best_dist - 0.75f)
	{
		agent->best_dist = dist;
		agent->switches = 0;
	}
}

extern "C" int bot_nav_steer(float fx, float fy, float fz, float gx, float gy, float gz,
	int agent_index, int blocked, struct bot_nav_step *step)
{
	dtQueryFilter filter;
	float start[3];
	float goal[3];
	float start_point[3];
	float goal_point[3];
	dtPolyRef start_ref;
	dtPolyRef goal_ref;
	struct nav_agent *agent;
	int switched;
	int followed;
	int built;
	dtPolyRef before;

	if (!step || nav_state != NAV_READY || !nav_mesh || !nav_query)
		return 0;
	if (fx != fx || fy != fy || fz != fz || gx != gx || gy != gy || gz != gz)
		return 0;
	step->switched = 0;
	nav_note(fx, fz);
	start[0] = fx;
	start[1] = fy;
	start[2] = fz;
	goal[0] = gx;
	goal[1] = gy;
	goal[2] = gz;
	nav_to_recast(start);
	nav_to_recast(goal);
	filter.setIncludeFlags(0x1);
	filter.setExcludeFlags(0);
	if (!nav_nearest(&filter, start, 2.0f, 1.5f, &start_ref, start_point) &&
		!nav_nearest(&filter, start, 3.0f, 2.5f, &start_ref, start_point))
		return 0;
	if (!nav_nearest(&filter, goal, 2.0f, 1.5f, &goal_ref, goal_point) &&
		!nav_nearest(&filter, goal, 4.0f, 4.0f, &goal_ref, goal_point))
		return 0;
	if (agent_index < 0 || agent_index >= NAV_AGENTS)
		return nav_direct(start_point, goal_point, start_ref, goal_ref, fx, fy, fz, step);
	agent = &nav_agents[agent_index];
	if (agent->lost > 0 && nav_goal_close(agent, goal_point))
	{
		agent->lost--;
		if (agent->lost > 0)
			return nav_direct(start_point, goal_point, start_ref, goal_ref, fx, fy, fz, step);
	}
	switched = 0;
	if (!nav_fresh(agent, goal_point))
	{
		built = nav_build(agent, agent_index, start_ref, start_point, goal_ref, goal_point);
		if (built == 0)
			return 0;
		if (built < 0)
		{
			agent->lost = 8;
			return nav_direct(start_point, goal_point, start_ref, goal_ref, fx, fy, fz, step);
		}
	}
	else
		nav_consider_switch(agent, start_ref, start_point, goal_point,
			fx, fy, fz, blocked, &switched);
	followed = nav_follow(agent, start_ref, start_point, goal_point, fx, fy, fz, step);
	if (followed == 1)
	{
		nav_note_progress(agent, start_point, goal_point);
		step->switched = switched;
		return 1;
	}
	if (followed == 0)
		return 0;
	before = agent->anchor;
	built = nav_build(agent, agent_index, start_ref, start_point, goal_ref, goal_point);
	if (built == 1)
	{
		followed = nav_follow(agent, start_ref, start_point, goal_point, fx, fy, fz, step);
		if (followed == 1)
		{
			nav_note_progress(agent, start_point, goal_point);
			step->switched = agent->anchor != before;
			return 1;
		}
		if (followed == 0)
			return 0;
	}
	agent->lost = 8;
	if (built < 0)
		return nav_direct(start_point, goal_point, start_ref, goal_ref, fx, fy, fz, step);
	return 0;
}

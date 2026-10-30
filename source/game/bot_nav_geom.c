/*
Triangles from the loaded collision BSP. Floors, walls, and ceilings all
go in. Recast decides which of those a person can stand on.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "game/bot_nav.h"

static int bot_nav_push_triangle(int **tris, int *count, int *capacity, int a, int b, int c)
{
	int next;
	int *grown;

	if (a == b || b == c || a == c)
		return 1;
	if (*count >= *capacity)
	{
		next = *capacity > 0 ? *capacity * 2 : 1024;
		grown = (int *)bot_nav_realloc(*tris, (size_t)next * 3 * sizeof(int));
		if (!grown)
			return 0;
		*tris = grown;
		*capacity = next;
	}
	(*tris)[*count * 3] = a;
	(*tris)[*count * 3 + 1] = b;
	(*tris)[*count * 3 + 2] = c;
	(*count)++;
	return 1;
}

int bot_nav_bsp_identity(struct bot_nav_identity *id)
{
	struct collision_bsp *bsp;
	struct collision_vertex *vertex;

	if (!id)
		return 0;
	id->key = 0;
	id->verts = 0;
	id->surfaces = 0;
	id->marker = 0.0f;
	bsp = global_collision_bsp_get();
	if (!bsp || bsp->vertices.count < 3 || bsp->surfaces.count < 1)
		return 0;
	id->key = (unsigned long)bsp;
	id->verts = bsp->vertices.count;
	id->surfaces = bsp->surfaces.count;
	vertex = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, 0, struct collision_vertex);
	if (vertex)
		id->marker = vertex->point.x + vertex->point.y + vertex->point.z;
	return 1;
}

int bot_nav_collect_geometry(float **verts_out, int *vert_count, int **tris_out, int *tri_count)
{
	struct collision_bsp *bsp;
	float *verts;
	int *tris;
	int count;
	int capacity;
	int vert_index;
	long surface_index;

	if (!verts_out || !vert_count || !tris_out || !tri_count)
		return 0;
	*verts_out = NULL;
	*vert_count = 0;
	*tris_out = NULL;
	*tri_count = 0;
	bsp = global_collision_bsp_get();
	if (!bsp || bsp->vertices.count < 3 || bsp->surfaces.count < 1)
		return 0;
	if (bsp->vertices.count > MAXIMUM_VERTICES_PER_COLLISION_BSP ||
		bsp->surfaces.count > MAXIMUM_SURFACES_PER_COLLISION_BSP)
		return 0;

	verts = (float *)bot_nav_alloc((size_t)bsp->vertices.count * 3 * sizeof(float));
	if (!verts)
		return 0;
	for (vert_index = 0; vert_index < bsp->vertices.count; vert_index++)
	{
		struct collision_vertex *vertex;

		vertex = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, vert_index, struct collision_vertex);
		verts[vert_index * 3] = vertex->point.x;
		verts[vert_index * 3 + 1] = vertex->point.y;
		verts[vert_index * 3 + 2] = vertex->point.z;
	}

	tris = NULL;
	count = 0;
	capacity = 0;
	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		struct collision_surface *surface;
		real_plane3d plane;
		long plane_index;
		long first_edge;
		long edge_index;
		int indices[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
		int point_count;
		int fan;

		if (!collision_bsp_valid_surface_index(bsp, surface_index))
			continue;
		surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
		plane_index = surface->plane_designator & LONG_MAX;
		if (plane_index < 0 || plane_index >= bsp->bsp3d.planes.count)
			continue;
		bsp3d_get_plane_from_designator(&bsp->bsp3d, surface->plane_designator, &plane);

		first_edge = surface->first_edge_index;
		edge_index = first_edge;
		point_count = 0;
		if (!collision_surface_edge_ring_continues(bsp, edge_index, 0))
			continue;
		do
		{
			struct collision_edge *edge;
			boolean reverse;
			long vertex_index;

			if (point_count >= MAXIMUM_VERTICES_PER_COLLISION_SURFACE)
				break;
			edge = TAG_BLOCK_GET_ELEMENT(&bsp->edges, edge_index, struct collision_edge);
			reverse = edge->surface_indices[1] == surface_index;
			vertex_index = edge->vertex_indices[reverse];
			if (vertex_index < 0 || vertex_index >= bsp->vertices.count)
			{
				point_count = 0;
				break;
			}
			indices[point_count++] = (int)vertex_index;
			edge_index = edge->edge_indices[reverse];
		}
		while (edge_index != first_edge &&
			collision_surface_edge_ring_continues(bsp, edge_index, (short)point_count));

		if (point_count < 3)
			continue;
		for (fan = 1; fan < point_count - 1; fan++)
		{
			int a;
			int b;
			int c;
			float ax;
			float ay;
			float az;
			float bx;
			float by;
			float bz;
			float nx;
			float ny;
			float nz;
			float dot;

			a = indices[0];
			b = indices[fan];
			c = indices[fan + 1];
			ax = verts[b * 3] - verts[a * 3];
			ay = verts[b * 3 + 1] - verts[a * 3 + 1];
			az = verts[b * 3 + 2] - verts[a * 3 + 2];
			bx = verts[c * 3] - verts[a * 3];
			by = verts[c * 3 + 1] - verts[a * 3 + 1];
			bz = verts[c * 3 + 2] - verts[a * 3 + 2];
			nx = ay * bz - az * by;
			ny = az * bx - ax * bz;
			nz = ax * by - ay * bx;
			if (nx * nx + ny * ny + nz * nz < 0.0000001f)
				continue;
			dot = nx * plane.n.i + ny * plane.n.j + nz * plane.n.k;
			if (dot < 0.0f)
			{
				int swap;

				swap = b;
				b = c;
				c = swap;
			}
			if (!bot_nav_push_triangle(&tris, &count, &capacity, a, b, c))
			{
				bot_nav_free(verts);
				bot_nav_free(tris);
				return 0;
			}
		}
	}

	if (count < 1)
	{
		bot_nav_free(verts);
		bot_nav_free(tris);
		return 0;
	}
	*verts_out = verts;
	*vert_count = bsp->vertices.count;
	*tris_out = tris;
	*tri_count = count;
	return 1;
}

/* climbable faces. Bit 2 of the surface flags. */
enum
{
	BOT_NAV_CLIMBABLE = 2
};

static int bot_nav_ground(float x, float y, float z, float reach, float *gx, float *gy, float *gz)
{
	struct collision_bsp *bsp;
	struct collision_bsp_test_vector_result hit;
	real_point3d origin;
	real_vector3d down;

	bsp = global_collision_bsp_get();
	if (!bsp || !global_down3d || reach <= 0.0f)
		return 0;
	/* the sample is often inside the floor brush. A ray that starts
	there never sees the top of it, and the ladder is dropped. */
	origin.x = x;
	origin.y = y;
	origin.z = z + 0.6f;
	scale_vector3d(global_down3d, reach + 0.6f, &down);
	if (!collision_bsp_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit),
			bsp, 0, NULL, &origin, &down, 1.0f, &hit) ||
		hit.surface_index == NONE)
		return 0;
	*gx = origin.x + down.i * hit.t;
	*gy = origin.y + down.j * hit.t;
	*gz = origin.z + down.k * hit.t;
	if (z - *gz > reach || *gz > z + 0.45f)
		return 0;
	return 1;
}

/* neighbors on the same deck. A point on the lip has air beside it, and
the navmesh erodes that lip, so the link would never snap to a polygon. */
static int bot_nav_floor_open(float x, float y, float z)
{
	static float const ang[] = { 0.0f, 1.5707963f, 3.14159265f, 4.712389f };
	int index;
	int open;

	open = 0;
	for (index = 0; index < 4; index++)
	{
		float gx;
		float gy;
		float gz;
		float dz;

		if (!bot_nav_ground(x + cosine(ang[index]) * 0.50f, y + sine(ang[index]) * 0.50f,
				z + 0.45f, 1.3f, &gx, &gy, &gz))
			continue;
		dz = gz - z;
		if (dz < 0.0f)
			dz = -dz;
		if (dz < 0.35f)
			open++;
	}
	return open;
}

/* a floor the climb comes out on. The exit is often beside the face or
past it, and sometimes well below the highest vertex when one tall face
runs past the deck. Keep a point on open floor, off the eroded lip. */
static int bot_nav_ladder_land(float face_x, float face_y, float yaw, float lowest, float highest,
	float *lx, float *ly, float *lz)
{
	static float const turn[] = {
		0.0f, 0.55f, -0.55f, 1.15f, -1.15f,
		1.5707963f, -1.5707963f, 2.2f, -2.2f, 2.7f, -2.7f, 3.14159265f
	};
	static float const reach[] = { 1.15f, 1.70f, 2.30f };
	int sample_index;
	float sample;
	int found;
	int best_tier;
	int best_open;
	int best_away;
	float best_z;
	float best_reach;

	found = 0;
	best_tier = -1;
	best_open = -1;
	best_away = 1;
	best_z = -1.0e9f;
	best_reach = 0.0f;
	sample_index = 0;
	for (sample = highest; sample >= lowest + 1.05f && sample_index < 24; sample -= 0.65f, sample_index++)
	{
		int reach_index;

		for (reach_index = 0; reach_index < (int)(sizeof(reach) / sizeof(reach[0])); reach_index++)
		{
			int turn_index;

			for (turn_index = 0; turn_index < (int)(sizeof(turn) / sizeof(turn[0])); turn_index++)
			{
				float heading;
				float x;
				float y;
				float gx;
				float gy;
				float gz;
				float fall;
				float prefer;
				int open;
				int tier;
				int better;
				int away;
				float forward;

				heading = yaw + turn[turn_index];
				x = face_x + cosine(heading) * reach[reach_index];
				y = face_y + sine(heading) * reach[reach_index];
				if (!bot_nav_ground(x, y, sample + 0.30f, 1.8f, &gx, &gy, &gz))
					continue;
				if (gz < lowest + 0.70f || gz > highest + 0.55f)
					continue;
				fall = sample + 0.30f - gz;
				if (fall > 1.15f)
					continue;
				/* yaw looks into the face. Past that plane is the back. */
				forward = cosine(yaw) * (gx - face_x) + sine(yaw) * (gy - face_y);
				away = forward > 0.35f;
				open = bot_nav_floor_open(gx, gy, gz);
				tier = open >= 3 ? 1 : 0;
				prefer = reach[reach_index] - 1.70f;
				if (prefer < 0.0f)
					prefer = -prefer;
				better = 0;
				/* a floor through the face is the wrong side. Keep it
				only when the climb side has no floor up there. */
				if (away && found && best_away == 0)
					better = 0;
				else if (!away && best_away == 1)
					better = 1;
				else if (!found || gz > best_z + 0.30f)
					better = 1;
				else if (gz < best_z - 0.30f)
					better = 0;
				else if (tier > best_tier)
					better = 1;
				else if (tier == best_tier && gz > best_z + 0.30f)
					better = 1;
				else if (tier == best_tier && gz >= best_z - 0.30f && open > best_open)
					better = 1;
				else if (tier == best_tier && gz >= best_z - 0.30f && open == best_open)
				{
					float had;

					had = best_reach - 1.70f;
					if (had < 0.0f)
						had = -had;
					if (prefer < had)
						better = 1;
				}
				if (!better)
					continue;
				best_away = away;
				best_tier = tier;
				best_open = open;
				best_z = gz;
				best_reach = reach[reach_index];
				*lx = gx;
				*ly = gy;
				*lz = gz;
				found = 1;
			}
		}
	}
	return found;
}

int bot_nav_collect_ladders(struct bot_nav_ladder *out, int capacity)
{
	struct collision_bsp *bsp;
	long surface_index;
	int count;
	int stand_score[128];
	int climb_count;
	int tall_count;
	int stand_count;
	int land_count;

	if (!out || capacity < 1)
		return 0;
	bsp = global_collision_bsp_get();
	if (!bsp)
		return 0;
	count = 0;
	climb_count = 0;
	tall_count = 0;
	stand_count = 0;
	land_count = 0;
	for (surface_index = 0; surface_index < bsp->surfaces.count && count < capacity; surface_index++)
	{
		struct collision_surface *surface;
		real_plane3d plane;
		real_point3d points[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
		short verts;
		short index;
		real_point3d mid;
		float lowest;
		float highest;
		float stand_x;
		float stand_y;
		float stand_z;
		float yaw;
		float land_x;
		float land_y;
		float land_z;
		int facing;
		int best_score;
		float sign;
		int kept;
		int previous;

		if (!collision_bsp_valid_surface_index(bsp, surface_index))
			continue;
		surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
		if (!surface || !TEST_FLAG(surface->flags, BOT_NAV_CLIMBABLE))
			continue;
		climb_count++;
		bsp3d_get_plane_from_designator(&bsp->bsp3d, surface->plane_designator, &plane);
		if (plane.n.k > 0.40f || plane.n.k < -0.40f)
			continue;
		verts = collision_surface_polygon(bsp, surface_index, points);
		if (verts < 3)
			continue;
		mid = points[0];
		lowest = highest = points[0].z;
		for (index = 1; index < verts; index++)
		{
			mid.x += points[index].x;
			mid.y += points[index].y;
			mid.z += points[index].z;
			if (points[index].z < lowest)
				lowest = points[index].z;
			if (points[index].z > highest)
				highest = points[index].z;
		}
		mid.x /= (real)verts;
		mid.y /= (real)verts;
		mid.z /= (real)verts;
		if (highest - lowest < 1.15f)
			continue;
		tall_count++;
		facing = 0;
		stand_x = mid.x;
		stand_y = mid.y;
		stand_z = lowest;
		yaw = 0.0f;
		{
			/* the side the plane normal points at is the side a body
			can grab. The floor behind a thin ladder is often the more
			open one, and standing there never attaches. */
			static float const stand_reach[] = { 1.55f, 1.15f, 0.75f };
			int reach_index;

			best_score = -1;
			for (sign = 1.0f; sign > -1.5f; sign -= 2.0f)
			{
				for (reach_index = 0; reach_index < 3; reach_index++)
				{
					float gx;
					float gy;
					float gz;
					float sx;
					float sy;
					real_point3d origin;
					real_vector3d into;
					struct collision_bsp_test_vector_result hit;
					real_plane3d faced;
					float align;
					float toward;
					float above;
					float hx;
					float hy;
					int open;
					int front_hit;
					int score;
					long flags;

					sx = mid.x + plane.n.i * stand_reach[reach_index] * sign;
					sy = mid.y + plane.n.j * stand_reach[reach_index] * sign;
					/* from the top of the face, so the ray starts in the
					air beside it instead of in the floor. */
					if (!bot_nav_ground(sx, sy, highest + 0.4f,
							(highest - lowest) + 2.5f, &gx, &gy, &gz) ||
						lowest - gz > 2.2f || gz > lowest + 1.4f)
						continue;
					origin.x = gx;
					origin.y = gy;
					origin.z = gz + 0.85f;
					if (origin.z > highest - 0.30f)
						origin.z = (lowest + highest) * 0.5f;
					if (origin.z < lowest + 0.30f)
						origin.z = lowest + 0.45f;
					/* through the middle of the polygon, and past it.
					a segment that ends on the face is not a hit. */
					into.i = mid.x - origin.x;
					into.j = mid.y - origin.y;
					into.k = mid.z - origin.z;
					{
						float span;

						span = into.i * into.i + into.j * into.j + into.k * into.k;
						if (span > 0.0001f)
						{
							span = 0.80f / square_root(span);
							into.i += into.i * span;
							into.j += into.j * span;
							into.k += into.k * span;
						}
					}
					front_hit = 0;
					/* one test from either side. A close face is still
					the ladder; requiring the hit to be far out dropped
					every one of them. */
					flags = FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_back_facing_surfaces_bit);
					if (!collision_bsp_test_vector(flags, bsp, 0, NULL, &origin, &into, 1.0f, &hit) ||
						hit.t < 0.05f || !TEST_FLAG(hit.flags, BOT_NAV_CLIMBABLE))
						continue;
					front_hit = 1;
					bsp3d_get_plane_from_designator(&bsp->bsp3d, hit.plane_designator, &faced);
					align = faced.n.i * plane.n.i + faced.n.j * plane.n.j;
					if (align < 0.0f)
						align = -align;
					/* a wall beside the ladder is a different plane. */
					if (align < 0.75f)
						continue;
					hx = origin.x + into.i * hit.t;
					hy = origin.y + into.j * hit.t;
					toward = faced.n.i * (origin.x - hx) + faced.n.j * (origin.y - hy);
					open = bot_nav_floor_open(gx, gy, gz);
					/* the floor up at the lip is the far deck. Starting
					there walks them around to the wrong side. */
					above = gz - lowest;
					if (above < 0.0f)
						above = 0.0f;
					score = 2000 - (int)(above * 100.0f) + open;
					if (sign > 0.0f)
						score += 40;
					if (front_hit && toward > 0.15f)
						score += 40;
					if (facing && score < best_score)
						continue;
					if (facing && score == best_score && stand_reach[reach_index] <= 1.15f)
						continue;
					best_score = score;
					stand_x = gx;
					stand_y = gy;
					stand_z = gz;
					yaw = arctangent(mid.y - stand_y, mid.x - stand_x);
					facing = 1;
				}
			}
		}
		if (!facing)
		{
			extern void platform_log(char const *format, ...);
			float gx;
			float gy;
			float gz;
			int grounded;
			real_point3d origin;
			real_vector3d into;
			struct collision_bsp_test_vector_result hit;
			int ray;

			gx = mid.x + plane.n.i * 1.15f;
			gy = mid.y + plane.n.j * 1.15f;
			grounded = bot_nav_ground(gx, gy, highest + 0.4f,
				(highest - lowest) + 2.5f, &gx, &gy, &gz);
			ray = 0;
			hit.t = 0.0f;
			hit.flags = 0;
			if (grounded)
			{
				origin.x = gx;
				origin.y = gy;
				origin.z = gz + 0.85f;
				into.i = mid.x - origin.x;
				into.j = mid.y - origin.y;
				into.k = 0.0f;
				ray = collision_bsp_test_vector(
					FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_back_facing_surfaces_bit),
					bsp, 0, NULL, &origin, &into, 1.0f, &hit) ? 1 : 0;
			}
			platform_log(
				"nav ladder miss face %.2f %.2f %.2f h %.2f..%.2f n %.2f %.2f ground %d gz %.2f ray %d t %.2f flags %d",
				mid.x, mid.y, mid.z, lowest, highest, plane.n.i, plane.n.j,
				grounded, grounded ? gz : 0.0f, ray, hit.t, (int)hit.flags);
			continue;
		}
		stand_count++;
		if (!bot_nav_ladder_land(mid.x, mid.y, yaw, lowest, highest, &land_x, &land_y, &land_z))
			continue;
		land_count++;
		kept = 1;
		for (previous = 0; previous < count; previous++)
		{
			float dx;
			float dy;
			float dz;

			/* the other polygon of this same ladder. A more open floor
			on the back used to replace the side the normal points at,
			and the path then walked around to it. */
			dx = out[previous].face_x - mid.x;
			dy = out[previous].face_y - mid.y;
			dz = out[previous].top - highest;
			if (dz < 0.0f)
				dz = -dz;
			if (dx * dx + dy * dy >= 1.2f * 1.2f || dz >= 1.2f)
				continue;
			if (previous >= 128 || best_score <= stand_score[previous])
			{
				kept = 0;
				break;
			}
			stand_score[previous] = best_score;
			out[previous].stand_x = stand_x;
			out[previous].stand_y = stand_y;
			out[previous].stand_z = stand_z;
			out[previous].land_x = land_x;
			out[previous].land_y = land_y;
			out[previous].land_z = land_z;
			out[previous].yaw = yaw;
			out[previous].top = highest;
			out[previous].face_x = mid.x;
			out[previous].face_y = mid.y;
			kept = 0;
			break;
		}
		if (!kept)
			continue;
		out[count].stand_x = stand_x;
		out[count].stand_y = stand_y;
		out[count].stand_z = stand_z;
		out[count].land_x = land_x;
		out[count].land_y = land_y;
		out[count].land_z = land_z;
		out[count].yaw = yaw;
		out[count].top = highest;
		out[count].face_x = mid.x;
		out[count].face_y = mid.y;
		if (count < 128)
			stand_score[count] = best_score;
		count++;
	}
	{
		extern void platform_log(char const *format, ...);

		platform_log("nav ladder scan surfaces %ld climb %d tall %d stand %d land %d kept %d",
			bsp->surfaces.count, climb_count, tall_count, stand_count, land_count, count);
	}
	return count;
}

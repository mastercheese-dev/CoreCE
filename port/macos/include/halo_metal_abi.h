/* Fixed-width native renderer wire ABI. No GL types or host pointers.
 * This is a transport for the existing Xbox renderer, not a new game API.
 * All packet offsets are relative to its first byte. Guest addresses passed
 * to imports are explicit uint32_t offsets in the existing rebased arena.
 */
#ifndef HALO_METAL_ABI_H
#define HALO_METAL_ABI_H
#include <stdint.h>
#include <stddef.h>

#define HALO_METAL_MAGIC UINT32_C(0x4c544d48)
#define HALO_METAL_ABI_VERSION 1u
#define HALO_METAL_MAX_PACKET (64u * 1024u * 1024u)

enum halo_metal_status {
    HALO_METAL_OK = 0, HALO_METAL_INVALID = -1,
    HALO_METAL_UNSUPPORTED = -2, HALO_METAL_STALE_RESOURCE = -3,
    HALO_METAL_MEMORY = -4, HALO_METAL_GPU_ERROR = -5,
    HALO_METAL_NOT_INITIALIZED = -6, HALO_METAL_UNDEFINED_CONTENT = -7
};
enum halo_metal_capability {
    HALO_METAL_CAP_TARGETS = 1u, HALO_METAL_CAP_UPLOAD = 2u,
    HALO_METAL_CAP_CLEAR = 4u, HALO_METAL_CAP_COPY = 8u,
    HALO_METAL_CAP_READBACK = 16u, HALO_METAL_CAP_PRESENT_EXACT = 32u,
    HALO_METAL_CAP_DRAW = 64u, HALO_METAL_CAP_VISIBILITY = 128u,
    HALO_METAL_CAP_PRESENT_SCALED = 256u, HALO_METAL_CAP_CLEAR_CHANNELS = 512u,
    HALO_METAL_CAP_BLACK_BORDER = 1024u, HALO_METAL_CAP_ALPHA_BORDER = 2048u,
    HALO_METAL_CAP_COPY_SUBRESOURCE = 4096u,
    HALO_METAL_CAP_VOLUME = 8192u, HALO_METAL_CAP_VOLUME_BORDER = 16384u,
    HALO_METAL_CAP_FXAA = 32768u,
    HALO_METAL_CAP_COMPACT_VERTICES = 65536u
};
enum halo_metal_opcode {
    HALO_METAL_CREATE_TEXTURE = 1, HALO_METAL_DELETE_TEXTURE = 2,
    HALO_METAL_UPLOAD = 3, HALO_METAL_CLEAR = 4, HALO_METAL_COPY = 5,
    HALO_METAL_PRESENT = 6, HALO_METAL_CREATE_PROGRAM = 7,
    HALO_METAL_DELETE_PROGRAM = 8, HALO_METAL_DRAW = 9,
    HALO_METAL_CREATE_TEXTURE_EX = 10, HALO_METAL_UPLOAD_EX = 11,
    HALO_METAL_CREATE_VISIBILITY = 12, HALO_METAL_DELETE_VISIBILITY = 13,
    HALO_METAL_BEGIN_VISIBILITY = 14, HALO_METAL_END_VISIBILITY = 15,
    HALO_METAL_PRESENT_SCALED = 16, HALO_METAL_CLEAR_CHANNELS = 17,
    HALO_METAL_DISPLAY_SETTINGS = 18, HALO_METAL_DRAW_ALPHA_BORDER = 19,
    HALO_METAL_COPY_SUBRESOURCE = 20, HALO_METAL_DRAW_VOLUME_BORDER = 21,
    HALO_METAL_FXAA = 22, HALO_METAL_DRAW_COMPACT = 23
};
enum halo_metal_format {
    HALO_METAL_RGBA8 = 1, HALO_METAL_BGRA8 = 2, HALO_METAL_DEPTH32_STENCIL8 = 3,
    HALO_METAL_BC1 = 4, HALO_METAL_BC2 = 5, HALO_METAL_BC3 = 6
};
enum halo_metal_texture_type {
    HALO_METAL_TEXTURE_2D = 1, HALO_METAL_TEXTURE_CUBE = 2,
    /* Normalized, uncompressed shader-read volumes. CREATE_EX supplies XYZ
       dimensions; UPLOAD_EX supplies one mip's XYZ box and image pitch.
       No generated mips, volume render targets or volume copies. */
    HALO_METAL_TEXTURE_3D = 3
};
enum halo_metal_texture_usage { HALO_METAL_SHADER_READ = 1, HALO_METAL_RENDER_TARGET = 2 };
enum halo_metal_plane {
    HALO_METAL_COLOR = 1, HALO_METAL_DEPTH = 2, HALO_METAL_STENCIL = 4,
    HALO_METAL_VISIBILITY = 8
};
enum halo_metal_visibility_mode {
    HALO_METAL_VISIBILITY_BOOLEAN = 1, HALO_METAL_VISIBILITY_COUNTING = 2
};
/* Optional capabilities are advertised only to callers that opt in. This
 * keeps the original ABI-v1 guest's strict capability mask compatible. */
enum { HALO_METAL_OFFSCREEN = 1u, HALO_METAL_ENABLE_FXAA = 2u,
    HALO_METAL_ENABLE_COMPACT_VERTICES = 4u };

struct halo_metal_ref { uint32_t id, generation; };
struct halo_metal_packet {
    uint32_t magic, abi_version, byte_size, command_count;
    uint64_t frame_sequence;
};
struct halo_metal_command { uint32_t opcode, byte_size; };
struct halo_metal_create {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t format, width, height, reserved;
};
struct halo_metal_delete {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
};
struct halo_metal_create_visibility {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t mode, reserved;
};
struct halo_metal_visibility {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    /* BEGIN resets; END seals and increments content_version. Readback plane8
       returns exactly one raw uint64_t after END and completed submission.
       BOOLEAN combines draws with OR; COUNTING sums their samples. This wire
       contract does not implement the game frontend's cached polling policy. */
};
struct halo_metal_upload {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t x, y, width, height, bytes_per_row, data_offset, data_size, reserved;
    /* Data lives inside this command's extent, with packet-relative offset.
       The host copies the entire packet before encoding any GPU operation. */
};
struct halo_metal_create_ex {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t format, width, height, depth, type, mip_levels, usage, reserved;
};
struct halo_metal_upload_ex {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t mip, slice, x, y, z, width, height, depth;
    uint32_t bytes_per_row, bytes_per_image, data_offset, data_size, plane, reserved;
    /* Immutable original subresource bytes, inside this command. Depth/stencil
       planes seed original initial attachment content before ordered draws. */
};
struct halo_metal_clear {
    struct halo_metal_command command;
    struct halo_metal_ref color, depth_stencil;
    uint32_t planes, x, y, width, height, stencil;
    float rgba[4], depth;
    uint32_t reserved;
};
struct halo_metal_copy {
    struct halo_metal_command command;
    struct halo_metal_ref source, destination;
    uint32_t source_x, source_y, destination_x, destination_y, width, height;
};
/* GPU-only copies of the original rendered mip surfaces into their sampled
 * composite. Matching RGBA8/BGRA8 texture2D formats only, COLOR plane, slice0,
 * distinct resources, no conversion or generated mip levels. An initialized
 * source mip is required; a full destination mip copy initializes that mip.
 * Partial copies require the destination mip to be initialized already.
 * Every successful copy advances the destination resource content version.
 * Coordinates and extents are pixels of the explicitly selected mip levels.
 * Legacy COPY keeps its level-zero render-target contract unchanged. */
struct halo_metal_copy_subresource {
    struct halo_metal_command command;
    struct halo_metal_ref source, destination;
    uint32_t source_mip, source_slice, destination_mip, destination_slice;
    uint32_t source_x, source_y, destination_x, destination_y, width, height;
    uint32_t planes, reserved;
};
struct halo_metal_present {
    struct halo_metal_command command;
    struct halo_metal_ref source;
};
/* Optional extensions retain the legacy exact-present/whole-channel records.
 * Scaled presentation uses the drawable's current dimensions, opaque black
 * letterboxing, linear filtering and logical top-left source rows. */
enum { HALO_METAL_DISPLAY_VSYNC = 1u };
struct halo_metal_present_scaled {
    struct halo_metal_command command;
    struct halo_metal_ref source;
    uint32_t display_flags, reserved;
};
struct halo_metal_display_settings {
    struct halo_metal_command command;
    uint32_t display_flags, reserved;
};
/* Optional pre-HUD, in-place RGB edge smoothing. Source must be an initialized
 * shader-readable RGBA8/BGRA8 render target. Coordinates are storage pixels.
 * The host snapshots immutable input before writing this rectangle; samples
 * clamp to its pixel centers. Alpha, pixels outside it, depth/stencil and
 * visibility results remain unchanged. No active visibility query is allowed.
 * Each successful command advances the source's content version once. */
struct halo_metal_fxaa {
    struct halo_metal_command command;
    struct halo_metal_ref source;
    uint32_t x, y, width, height;
};
struct halo_metal_clear_channels {
    struct halo_metal_clear clear;
    uint32_t color_write_mask, reserved; /* logical RGBA bits 0..3 */
};
/* Comparisons: 1 never, 2 less, 3 equal, 4 less_equal, 5 greater,
 * 6 not_equal, 7 greater_equal, 8 always.
 * Stencil operations: 1 keep, 2 zero, 3 replace, 4 increment_clamp,
 * 5 decrement_clamp, 6 invert, 7 increment_wrap, 8 decrement_wrap.
 * Blend factors: 1 zero, 2 one, 3 source_color, 4 inverse_source_color,
 * 5 source_alpha, 6 inverse_source_alpha, 7 destination_alpha,
 * 8 inverse_destination_alpha, 9 destination_color, 10 inverse_destination_color,
 * 11 source_alpha_saturated, 12 blend_color, 13 inverse_blend_color,
 * 14 blend_alpha, 15 inverse_blend_alpha. Operations: add/subtract/reverse/min/max=1..5.
 * Front winding: clockwise=0, counterclockwise=1. Cull: none/front/back=0..2.
 * Depth clip: clip=0, clamp=1. Fill: solid=0, wireframe=1.
 * Viewport/scissor use logical top-left pixels.
 */
struct halo_metal_draw_state {
    uint32_t color_write_mask, blend_enabled, blend_source, blend_destination, blend_operation;
    uint32_t depth_enabled, depth_write, depth_compare;
    uint32_t stencil_enabled, stencil_compare, stencil_read_mask, stencil_write_mask;
    uint32_t stencil_fail, stencil_depth_fail, stencil_pass, stencil_reference;
    uint32_t front_winding, cull_mode, depth_clip_mode, fill_mode;
    float viewport[6]; /* x,y,width,height,znear,zfar */
    uint32_t scissor[4]; /* x,y,width,height */
    float depth_bias, slope_depth_bias, depth_bias_clamp, reserved_float;
    float blend_color[4];
};
struct halo_metal_sampler {
    uint32_t min_filter, mag_filter, mip_filter; /* nearest/linear=0/1; mip none/nearest/linear=0/1/2 */
    uint32_t address_u, address_v, address_w; /* repeat/mirror/edge=0/1/2;
       optional cap1024: transparent-black hardware border=3 for texture2D U/V.
       cap8192 additionally permits RGBA0 volume XYZ border with either point
       min/mip and linear mag, or linear min/mag/mip; anisotropy1 in both cases.
       Cube border is unsupported.
       Other authored border colors remain explicitly shader-managed. */
    uint32_t max_anisotropy, reserved;
    float lod_min, lod_max;
};
struct halo_metal_program {
    struct halo_metal_command command;
    struct halo_metal_ref resource;
    uint32_t vertex_compiler_contract; /* 0 invariant safe, 1 pinned ANGLE invariant fast vertex v1 */
    uint32_t vertex_source_offset, vertex_source_size, fragment_source_offset, fragment_source_size;
    uint32_t fragment_compiler_contract; /* 0 invariant safe (legacy zero), 1 pinned ANGLE fast fragment v1, no invariance */
};
struct halo_metal_draw {
    struct halo_metal_command command;
    struct halo_metal_ref program, color, depth_stencil, textures[4];
    struct halo_metal_sampler samplers[4];
    struct halo_metal_draw_state state;
    uint32_t vertex_count, index_count, packed_mask, primitive; /* points/lines/line_strip/triangles/triangle_strip=0..4 */
    uint32_t vertices_offset, indices_offset, vertex_uniforms_offset, pixel_uniforms_offset;
    uint32_t reserved[2];
    /* Payload: immutable 16 expanded original input registers per vertex
       (256B), uint32 indices, VS3120B and PS608B. No replacement scene vertices.
       Original packed registers retain their instruction-visible raw bits. */
};
/* Optional alpha-only authored border reconstruction. The base draw binds
 * transparent-black U/V border samplers. Each selected original stage also
 * receives an identical opaque-black sampler at 4+stage, and the production
 * shader reconstructs only alpha before original combiner operations. Narrow
 * contract: normalized 2D, linear min/mag, nearest mip, anisotropy 1, black RGB.
 * No reserved legacy field changes meaning; payload begins after this record. */
struct halo_metal_draw_alpha_border {
    struct halo_metal_draw draw;
    uint32_t stage_mask, reserved;
};
/* Optional normalized 3D authored RGBA border reconstruction. Selected volume
 * stages receive an opaque-white companion sampler at 4+stage, otherwise
 * identical to their transparent-black base sampler. Volume and 2D alpha
 * masks are disjoint; alpha stages retain the opcode19 opaque-black contract.
 * Volumes require more than one authored mip, anisotropy1, linear mag and
 * either point min/mip or linear min/mip. Existing DRAW/opcode19 are unchanged. */
struct halo_metal_draw_volume_border {
    struct halo_metal_draw draw;
    uint32_t volume_stage_mask, alpha_stage_mask;
};
/* Opt-in original compact stream snapshots, decoded in the vertex shader.
 * vertices_offset addresses a halo_metal_vertex_input prefix followed by raw
 * stream bytes, all inside this command's immutable extent. Indices/uniforms
 * and border contracts retain their existing meanings. Legacy reserved fields
 * remain zero; old expanded draw opcodes and recorded packets are unchanged. */
struct halo_metal_draw_compact {
    struct halo_metal_draw draw;
    uint32_t vertex_bytes, volume_stage_mask, alpha_stage_mask, reserved;
};
struct halo_metal_reply {
    uint32_t abi_version;
    int32_t status;
    uint32_t failed_command, capabilities;
    uint64_t submitted_sequence, completed_sequence;
    uint32_t live_resources, content_version, byte_size, reserved;
};

#if defined(__cplusplus)
#define HALO_METAL_ASSERT static_assert
extern "C" {
#else
#define HALO_METAL_ASSERT _Static_assert
#endif
HALO_METAL_ASSERT(sizeof(struct halo_metal_packet) == 24, "packet ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_packet, frame_sequence) == 16, "sequence ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_reply) == 48, "reply ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_clear) == 72, "clear ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_clear_channels) == 80, "channel clear ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_present_scaled) == 24, "scaled present ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_display_settings) == 16, "display settings ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_fxaa) == 32, "FXAA ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_upload) == 48, "upload ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_create_ex) == 48, "extended texture ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_upload_ex) == 72, "extended upload ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_copy_subresource) == 72, "subresource copy ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_copy_subresource, source_mip) == 24, "copy source mip ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_copy_subresource, planes) == 64, "copy planes ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_create_visibility) == 24, "visibility creation ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_visibility) == 16, "visibility lifecycle ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_draw_state) == 152, "draw state ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_sampler) == 40, "sampler ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_program) == 40, "program ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_program, fragment_compiler_contract) == 36, "fragment compiler ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_draw) == 416, "draw ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_draw_alpha_border) == 424, "alpha border draw ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_draw_volume_border) == 424, "volume border draw ABI");
HALO_METAL_ASSERT(sizeof(struct halo_metal_draw_compact) == 432, "compact vertex draw ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_draw_volume_border, volume_stage_mask) == 416, "volume border mask ABI");
HALO_METAL_ASSERT(offsetof(struct halo_metal_draw_volume_border, alpha_stage_mask) == 420, "volume alpha border mask ABI");

int host_metal_initialize(uint32_t window, uint32_t flags, uint32_t reply, uint32_t reply_size);
int host_metal_submit(uint32_t packet, uint32_t size, uint32_t reply, uint32_t reply_size);
int host_metal_readback(uint32_t id, uint32_t generation, uint32_t plane,
                        uint32_t destination, uint32_t size, uint32_t reply, uint32_t reply_size);
void host_metal_shutdown(void);
/* Stop writing the Metal warmup file. An update deletes that file, and a
later save would put the old one back. */
void host_metal_forget_warmup(void);
/* Block until the next display-locked 60 Hz tick. Falls back to a plain
 * 1/60 s sleep before the display link is running. */
void host_metal_pace_vblank(void);
#if defined(__cplusplus)
}
#endif
#undef HALO_METAL_ASSERT
#endif

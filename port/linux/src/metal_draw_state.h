/* CPU-only packing of the existing Xbox draw state for the native renderer.
 * Inputs are original D3D state, not translated GL state. No graphics calls. */
#ifndef HALO_METAL_DRAW_STATE_H
#define HALO_METAL_DRAW_STATE_H
#include <stdint.h>
#include "../../macos/include/halo_metal_abi.h"

#define METAL_DRAW_RENDER_STATES 144u
#define METAL_DRAW_TEXTURE_STATES 32u
#define METAL_DRAW_STAGES 4u
#define METAL_DRAW_CONSTANTS 192u
struct nv2a_pixel_shader_key;
enum metal_draw_depth_contract {
    METAL_DRAW_DEPTH_NONE = 0,
    METAL_DRAW_DEPTH_RAW_D24 = 1
};

struct metal_draw_texture {
    /* sampler_type uses _xgpu_sampler_* (0 none, 1 2D, 2 3D, 3 cube).
     * Dimensions/levels describe the effective bound resource, including
     * rendered aliases and optional hires replacements, not its old header. */
    uint32_t present, sampler_type, width, height, levels;
    uint32_t linear, hires, hires_coverage, hires_point_threshold, ce_meter_channels;
    /* 1 when the bound resource is a render target or a composite of one
     * (water ripples, shadows). Those keep the game's own filtering. */
    uint32_t rendered;
};
struct metal_draw_state_input {
    const uint32_t *render_state;                 /* [144] */
    const uint32_t (*texture_state)[32];          /* [4][32] */
    const float (*constants)[4];                 /* [192][4], hardware order */
    const float *viewport_scale, *viewport_offset; /* [4] each */
    /* Original viewport after target_pixel rounding/target scaling; logical
     * Xbox top rows, x/y/width/height/MinZ/MaxZ. Scissor defaults to this. */
    float viewport[6];
    uint32_t target_width, target_height, has_depth, sample_count;
    int32_t screen_offset;                       /* original (GLint)UI_OFFSET */
    struct metal_draw_texture textures[4];
    /* Explicit opt-in only after the host reports transparent-black border
     * sampler support. Uses wire address3 for the otherwise unsupported
     * mipped/anisotropic 2D footprint, and only for literal border ARGB0.
     * Existing single-level shader emulation and the default0 stay intact. */
    uint32_t native_black_border;
    /* Separate explicit host capability for a same-texture opaque-black
     * companion sampler. Initially only normalized mipped texture2D,
     * black RGB, linear min/mag, nearest mip and original aniso1. */
    uint32_t native_alpha_border;
    /* Appended metadata keeps all earlier field offsets intact. Zero/one is
     * accepted for old 2D/cube inputs; a volume requires its actual depth. */
    uint32_t texture_depth[4];
    /* Explicit cap8192 opt-in. Volumes are normalized, uncompressed resources
     * with their original authored mips. Literal transparent-black volume
     * border requires point min/mip with linear mag, or linear min/mag/mip;
     * both footprints require anisotropy1 and literal transparent black. */
    uint32_t native_volume;
    /* Explicit original target-depth units for DOT_ZW. Default0 preserves
     * ordinary packing. RAW_D24 requires the original presentation request,
     * current original surface Format and actual rasterizer floating-Z byte;
     * a native host attachment's storage format is not that provenance.
     * Viewport scale/offset Z must match the original D24 range mapping. */
    uint32_t native_depth_contract;
    uint32_t original_auto_depth_format, depth_surface_format_word;
    uint32_t floating_point_zbuffer;
    /* Separate cap16384 opt-in for normalized mipped 3D authored RGBA border
     * reconstruction with an otherwise identical opaque-white companion.
     * Only the two cap8192 sampling footprints and anisotropy1 are supported. */
    uint32_t native_volume_border;
    /* display.anisotropic_filtering, already clamped to 1..16. 1 keeps the
     * game's own filtering. Appended so earlier field offsets stay put. */
    uint32_t anisotropic_level;
};
struct metal_draw_vertex_uniforms {
    float c[192][4], viewport_scale[4], viewport_offset[4];
    float point_size, screen_offset, padding[2];
};
struct metal_draw_pixel_uniforms {
    float ps_c0[8][4], ps_c1[8][4], ps_final_c0[4], ps_final_c1[4];
    float fog_color[4], fog_parameters[4], alpha_reference;
    float depth_scale, depth_clip_min, depth_clip_max;
    float bump_matrix[4][4], bump_luminance[4][4];
    float texture_scale[4][4], texture_border_color[4][4], texture_lod_bias[4];
};
struct metal_draw_state_output {
    struct metal_draw_vertex_uniforms vertex;
    struct metal_draw_pixel_uniforms pixel;
    struct halo_metal_draw_state state;
    struct halo_metal_sampler samplers[4];
    uint32_t active_texture_mask;
    uint32_t native_alpha_border_mask;
    uint32_t depth_contract;
    uint32_t native_volume_border_mask;
};
struct metal_draw_state_error {
    /* stage==UINT32_MAX means render/global input. Message is static. */
    uint32_t stage, state, value;
    const char *message;
};

/* Atomic: on any error, key and output are unchanged. Error may be NULL.
 * Native visibility is provided by the draw encoder, never a PS counter.
 * Pixel depth replacement requires the explicit original integer-D24
 * contract above; unknown, floating or missing units remain unsupported.
 * Success confirms CPU packing, not general shader support: callers must
 * still reject a NULL result from the direct NV2A Metal shader emitters. */
int metal_draw_state_pack(const struct metal_draw_state_input *input,
    struct nv2a_pixel_shader_key *key, struct metal_draw_state_output *output,
    struct metal_draw_state_error *error);

_Static_assert(sizeof(struct metal_draw_vertex_uniforms) == 3120, "vertex uniforms ABI");
_Static_assert(sizeof(struct metal_draw_pixel_uniforms) == 608, "pixel uniforms ABI");
_Static_assert(offsetof(struct metal_draw_vertex_uniforms, point_size) == 3104, "vertex scalar offset");
_Static_assert(offsetof(struct metal_draw_pixel_uniforms, bump_matrix) == 336, "pixel matrix offset");
_Static_assert(offsetof(struct metal_draw_pixel_uniforms, depth_scale) == 324, "pixel depth scale offset");
_Static_assert(offsetof(struct metal_draw_state_output, depth_contract) == 4048, "preserved output prefix");
_Static_assert(offsetof(struct metal_draw_pixel_uniforms, texture_lod_bias) == 592, "pixel LOD offset");
#endif

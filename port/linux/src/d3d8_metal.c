/* Native Metal implementation of the Xbox D3D8 CPU frontend.
 * Original XDK state, declarations, streams and instructions remain the inputs.
 * This development frontend fails on unsupported GPU operations; it never
 * silently drops a draw or substitutes scene-viewer geometry.
 */
#if defined(HALO_MACOS_NATIVE_METAL) && HALO_MACOS_NATIVE_METAL
#include "xgpu.h"
#include "xgpu_msl.h"
#include "sdl_platform.h"
#include "halo_ui_pointer.h"
#include "port_config.h"
#include "metal_guest_transport.h"
#include "metal_packet_room.h"
#include "metal_vertex_fetch.h"
#include "metal_draw_state.h"
#include "metal_fixed_function.h"
#include "metal_mip_composite.h"
#include "metal_hud_mipmap.h"
#include "metal_render_scale.h"
#include "hud_hires.h"
#include "text_hires.h"
#include "menu_files.h"
#include "halo_frame_pacing.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>

void d3d8_surface_initialize(D3DSurface *, D3DFORMAT, unsigned long, unsigned long);
void d3d8_surface_resize(D3DSurface *, D3DFORMAT, unsigned long, unsigned long);
struct xgpu_capabilities xgpu_capabilities;
_Static_assert(sizeof(DWORD) == sizeof(uint32_t), "native guest DWORD ABI");
#define SCREEN_HEIGHT 480
#define SCREEN_MAXIMUM_WIDTH 1920

/* the width the game draws, 0 until first asked, and how many pixels a
render target the size of the screen has per unit of it */
static long screen_width;
static float screen_scale[2] = { 1.0f, 1.0f };
static long ui_offset;
/* Accepted choices are committed only after a completed Present. Storage
 * scale stays fixed for every command in a frame. */
static uint32_t render_height;
static uint32_t native_storage_width, native_storage_height;
static BOOL render_settings_initialized;
static BOOL gpu_vertex_decode;
static uint32_t native_render_height(void) {
    if (!render_settings_initialized) {
        long requested=config_integer("display.render_height");
        render_height=(requested==0 || requested==720 || requested==1080 || requested==1440 || requested==2160) ? requested:480;
        render_settings_initialized=TRUE;
    }
    return render_height;
}
#define UI_OFFSET ((long)ui_offset)

static void screen_mode_choose(long *width, float scale[2])
{
#ifdef HALO_ANDROID
	/* display.screen_width, or 0 for the display's shape, which the app
	passes (port/android/host/host_main.c) */
	const char *display = getenv("HALO_DISPLAY_WIDTH");

	*width = config_integer("display.screen_width");
	if (*width <= 0)
		*width = display ? atol(display) : 640;
	if (*width < 640)
		*width = 640;
	if (*width > 1600)
		*width = 1600;
	*width &= ~1L;
	uint32_t height=native_render_height();
	/* Native backing is not knowable until SDL has created and synchronized
	   its window. This early selection chooses logical game units only. */
	if (!height && native_storage_width && native_storage_height) {
		scale[0]=(float)native_storage_width/(float)*width;
		scale[1]=(float)native_storage_height/SCREEN_HEIGHT;
	} else scale[0] = scale[1] = height ? (float)height / SCREEN_HEIGHT:1.0f;
#else
	long display_width, display_height;

	*width = 640;
	scale[0] = scale[1] = 1.0f;
	if (platform_screen_mode(&display_width, &display_height) && display_width > 0 && display_height > 0)
	{
		long wanted = (SCREEN_HEIGHT * display_width + display_height / 2) / display_height;

		*width = wanted < 640 ? 640 : wanted > SCREEN_MAXIMUM_WIDTH ? SCREEN_MAXIMUM_WIDTH : wanted & ~1L;
		scale[0] = (float)display_width / (float)*width;
		scale[1] = (float)display_height / (float)SCREEN_HEIGHT;
		/* a display narrower or wider than the game can be: the picture
		keeps its shape and the display blit letterboxes it */
		if (*width != wanted && *width != (wanted & ~1L))
			scale[0] = scale[1] = scale[0] < scale[1] ? scale[0] : scale[1];
	}
#endif
}

long halo_screen_width(void)
{
	if (!screen_width)
	{
		screen_mode_choose(&screen_width, screen_scale);
		if (native_render_height())
			platform_log("screen: %ldx%d drawn at %.0fx%.0f", screen_width, SCREEN_HEIGHT,
				screen_width * screen_scale[0], SCREEN_HEIGHT * screen_scale[1]);
		else platform_log("screen: %ldx%d logical; native backing awaits the SDL drawable",
			screen_width, SCREEN_HEIGHT);
	}
	return screen_width;
}

/* The atlas rasterizes at the same physical pixels per logical line as the
 * native attachment. Fixed-height presets and native drawable mode both keep
 * the game's text layout in its original 480-line coordinates. */
float halo_screen_pixel_scale(void)
{
	(void)halo_screen_width();
	return screen_scale[1];
}

float halo_screen_scale(void)
{
	(void)halo_screen_width();
	return screen_scale[0] > screen_scale[1] ? screen_scale[0] : screen_scale[1];
}

/* display.model_detail, the same choice the GL frontend makes. The Mac
guest does not link that file. Stock keeps the Xbox's 480-line measure.
Native and highest measure in this display's lines. */
enum
{
	_model_detail_stock,
	_model_detail_native,
	_model_detail_highest,
};

static int model_detail = -1;
static unsigned long model_detail_read_at;

static int model_detail_setting(void)
{
	if (model_detail < 0 || model_detail_read_at != config_changes())
	{
		const char *setting = config_string("display.model_detail");
		int value = !strcmp(setting, "native") ? _model_detail_native :
			!strcmp(setting, "highest") ? _model_detail_highest : _model_detail_stock;

		model_detail_read_at = config_changes();
		if (value != model_detail)
		{
			if (value == _model_detail_stock && strcmp(setting, "stock"))
				platform_log("model detail: \"%s\" is unknown, so stock", setting);
			else
				platform_log("model detail: %s", setting);
		}
		model_detail = value;
	}
	return model_detail;
}

int halo_model_detail_stock(void)
{
	return model_detail_setting() == _model_detail_stock;
}

float halo_model_detail_scale(int level)
{
	float display_scale;

	model_detail_setting();
	if (model_detail == _model_detail_stock)
		return 1.0f;
	if (model_detail == _model_detail_highest && level)
		return 1.0e6f;
	(void)halo_screen_width();
	display_scale = screen_scale[1];
	return display_scale > 1.0f ? display_scale : 1.0f;
}

#define SHADOW_MAP_SIZE 128
#define SHADOW_MAP_MAXIMUM_SCALE 8
static long shadow_scale;

long halo_shadow_map_scale(void)
{
	long resolution, scale;

	if (shadow_scale)
		return shadow_scale;
	resolution = config_integer("display.shadow_resolution");
	scale = 1;
	while (scale < SHADOW_MAP_MAXIMUM_SCALE && SHADOW_MAP_SIZE * scale * 2 <= resolution)
		scale *= 2;
	shadow_scale = scale;
	platform_log("shadow maps: %ldx%ld", SHADOW_MAP_SIZE * shadow_scale, SHADOW_MAP_SIZE * shadow_scale);
	return shadow_scale;
}

void halo_vertex_shader_lighting(unsigned long handle)
{
	(void)handle;
}

void halo_screen_ui_offset(unsigned char centered)
{
	ui_offset = centered ? (halo_screen_width() - 640) / 2 : 0;
}

/* ---------- state the XDK header's inline functions read and write */

DWORD D3D__RenderState[D3DRS_MAX];
DWORD D3D__TextureState[D3DTSS_MAXSTAGES][D3DTSS_MAX];
WORD *D3D__IndexData;
BYTE D3D__StateBlockDirty[1024];


#define VERTEX_SHADER_SIGNATURE 0x76736864UL
#define VERTEX_PROGRAM_SLOTS 136
#define VISIBILITY_TEST_SLOTS 4096
#define VISIBILITY_ALL_SAMPLES 1000000
struct vertex_shader_object {
    struct vertex_shader_object *next;
    unsigned long signature, id;
    DWORD *instructions;
    unsigned long instruction_count;
    struct metal_vertex_declaration declaration;
};
struct native_resource {
    struct native_resource *next;
    struct native_resource *hud_override;
    struct halo_metal_ref ref;
    DWORD data, format_word, size_word;
    struct xgpu_texture_description description;
    unsigned long generation, palette_hash, hud_generation;
    long hud_asset;
    uint64_t last_rendered, text_revision;
    /* Only rendered-mip destinations allocate this bounded, per-level cache. */
    struct halo_metal_mip_composite_copy *mip_copy_cache;
    uint32_t format, usage, storage_width, storage_height, scale_mode;
    BOOL mip_composite, volume, hud_checked, text_atlas, menu_art;
    /* Same-address resources only. A frame's model draws must not scan every
       bitmap the map has loaded. */
    struct native_resource *bucket_next;
};
struct native_program {
    struct native_program *next;
    struct native_program *bucket_next;
    uint32_t lookup_hash;
    struct halo_metal_ref ref;
    struct vertex_shader_object *vertex;
    uint32_t packed_mask, depth_contract, alpha_border_mask, volume_border_mask, compact_vertices;
    struct nv2a_pixel_shader_key key;
};
struct native_device {
    D3DPRESENT_PARAMETERS presentation;
    D3DSurface back_buffer, history_buffer, depth_buffer;
    D3DSurface *render_target, *depth_stencil;
    D3DVIEWPORT8 viewport;
    D3DMATRIX transforms[D3DTS_MAX];
    D3DBaseTexture *textures[4];
    D3DPalette *palettes[4];
    D3DSHADERCONSTANTMODE shader_constant_mode;
    struct vertex_shader_object *vertex_shader, *program_slots[VERTEX_PROGRAM_SLOTS];
    struct vertex_shader_object *vertex_shader_objects;
    unsigned long program_address, next_vertex_shader_id;
    float constants[192][4], viewport_scale[4], viewport_offset[4], attributes[16][4];
    struct {DWORD data; UINT stride;} streams[16];
    UINT base_vertex_index;
    BOOL immediate_active, ready, created, visibility_test_active, fixed_function_selected;
    D3DPRIMITIVETYPE immediate_type;
    float *immediate_vertices;
    unsigned long immediate_count, immediate_capacity, frame, draws;
    uint64_t resource_serial;
    uint64_t antialias_passes;
    BOOL antialiasing_enabled;
    struct halo_metal_ref query_scratch, query_slots[VISIBILITY_TEST_SLOTS];
    UINT query_results[VISIBILITY_TEST_SLOTS];
    BOOL query_pending[VISIBILITY_TEST_SLOTS];
    uint64_t query_packet[VISIBILITY_TEST_SLOTS];
};
static struct native_device device;
/* Distinct cache identity; no original programmable slot is overwritten. */
static struct vertex_shader_object fixed_function_vertex;
static struct halo_metal_guest_transport transport;
static uint64_t sequence;
static uint32_t next_resource_id = 1, next_program_id = 1, next_query_id = 1;
static struct native_resource *resources;
#define NATIVE_RESOURCE_BUCKETS 4096u
static struct native_resource *resource_buckets[NATIVE_RESOURCE_BUCKETS];
#define NATIVE_PROGRAM_BUCKETS 1024u
static struct native_program *program_buckets[NATIVE_PROGRAM_BUCKETS];
/* Keep the last completed indexed frame sampled at its original resolution
 * during the first resized frame. Its replacement is copied at Present. */
static struct native_resource *history_sample_override;
static struct native_resource *text_atlas_resource;
static struct native_program *programs;
static unsigned native_resource_bucket(DWORD data) {
    uint32_t mixed=(uint32_t)data;
    mixed^=mixed>>16;mixed*=0x7feb352du;mixed^=mixed>>15;
    return (unsigned)(mixed&(NATIVE_RESOURCE_BUCKETS-1));
}
static void resource_publish(struct native_resource *entry) {
    unsigned bucket=native_resource_bucket(entry->data);
    entry->next=resources;resources=entry;
    entry->bucket_next=resource_buckets[bucket];resource_buckets[bucket]=entry;
}
static void resource_unindex(struct native_resource *entry) {
    unsigned bucket=native_resource_bucket(entry->data);
    struct native_resource **link=&resource_buckets[bucket];
    while (*link && *link!=entry) link=&(*link)->bucket_next;
    if (*link) *link=entry->bucket_next;
    entry->bucket_next=NULL;
}
static unsigned program_lookup_hash(struct vertex_shader_object *vertex,uint32_t packed,
    const struct nv2a_pixel_shader_key *key,uint32_t alpha_border_mask,uint32_t volume_border_mask,
    uint32_t depth_contract,uint32_t compact_vertices) {
    const unsigned char *bytes=(const unsigned char *)key;
    unsigned i;uint32_t hash=2166136261u;
    hash^=(uint32_t)(unsigned long)vertex;hash*=16777619u;
    hash^=packed;hash*=16777619u;
    hash^=alpha_border_mask;hash*=16777619u;
    hash^=volume_border_mask;hash*=16777619u;
    hash^=depth_contract;hash*=16777619u;
    hash^=compact_vertices;hash*=16777619u;
    for (i=0;i<sizeof(*key);i++) { hash^=bytes[i];hash*=16777619u; }
    return hash;
}
/* Original rasterizer.h verifies floating_point_zbuffer at byte0x3c in the
   linked ILP32 global. Character access reads that object's representation. */
extern struct rasterizer_globals_definition rasterizer_globals;
static uint32_t original_float_z_value(void) {
    return ((const unsigned char *)&rasterizer_globals)[0x3c];
}
static D3DDevice *device_pointer(void) { return (D3DDevice *)&device; }
Direct3D *WINAPI Direct3DCreate8(UINT sdk_version) {
    (void)sdk_version; return (Direct3D *)1;
}
static void constants_store(unsigned long first, const void *data, unsigned long count) {
    memcpy(device.constants[first],data,count * sizeof(device.constants[0]));
}
static void native_fail(const char *operation, int status) {
    platform_log("Native Metal: %s failed (%d), frame %lu, command %u: %s",
        operation,status,device.frame,transport.reply.failed_command,
        transport.error ? transport.error : "unsupported original renderer state");
    abort();
}
static void require_status(const char *operation, int status) {
    if (status) native_fail(operation,status);
}
/* Synchronous host submissions contain complete original commands in order.
 * The soft limit bounds transient per-frame storage; one larger valid command
 * may use the existing 64MiB hard capacity and is submitted when complete. */
#define NATIVE_BATCH_SOFT_BYTES (16u * 1024u * 1024u)
static uint32_t packet_expected_end;
static uint64_t submitted_batches, submitted_commands, submitted_bytes, submit_wall_ns;
static uint64_t statistics_batches, statistics_commands, statistics_bytes, statistics_wall_ns;
static uint32_t largest_statistics_batch;
static BOOL draw_diagnostics;
struct native_draw_profile {
    uint64_t draws, expansion_ns, textures_ns, state_ns, program_ns, emit_ns, submit_ns;
    uint64_t compact_draws, compact_bytes, expanded_bytes, vertex_bytes;
};
static struct native_draw_profile draw_profile;
enum native_flush_reason {
    NATIVE_FLUSH_SOFT_BATCH_LIMIT,
    NATIVE_FLUSH_HARD_CAPACITY,
    NATIVE_FLUSH_COMMAND_LIMIT,
    NATIVE_FLUSH_VISIBILITY_COLLECT,
    NATIVE_FLUSH_KICK_PUSH_BUFFER,
    NATIVE_FLUSH_INSERT_CALLBACK,
    NATIVE_FLUSH_PRESENT,
    NATIVE_FLUSH_RESIZE,
    NATIVE_FLUSH_READBACK,
    NATIVE_FLUSH_REASON_COUNT
};
struct native_flush_statistics {
    uint64_t packets, commands, bytes, wall_ns;
};
static struct native_flush_statistics flush_statistics[NATIVE_FLUSH_REASON_COUNT];
static const char *native_flush_reason_name(enum native_flush_reason reason) {
    static const char *const names[NATIVE_FLUSH_REASON_COUNT]={
        "SoftBatchLimit", "HardCapacity", "CommandLimit", "VisibilityCollect",
        "KickPushBuffer", "InsertCallback", "Present", "Resize",
        "Readback"
    };
    return names[reason];
}
static void native_flush_statistics_log(unsigned long first,unsigned long last) {
    for (unsigned i=0;i<NATIVE_FLUSH_REASON_COUNT;i++) {
        const struct native_flush_statistics *s=&flush_statistics[i];
        if (s->packets)
            platform_log("Native flush totals: frames %lu-%lu, reason %s, packets %llu, commands %llu, bytes %llu, submit %llu ns",
                first,last,native_flush_reason_name((enum native_flush_reason)i),
                (unsigned long long)s->packets,(unsigned long long)s->commands,
                (unsigned long long)s->bytes,(unsigned long long)s->wall_ns);
    }
    memset(flush_statistics,0,sizeof(flush_statistics));
}
static uint64_t monotonic_ns(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*UINT64_C(1000000000)+(uint64_t)t.tv_nsec;
}
/* Guest-only draw phases exclude synchronous host submissions nested inside
   preparation/packet emission. GPU and completion timings remain separate. */
static void native_draw_phase(uint64_t *started,uint64_t *submitted,uint64_t *total) {
    uint64_t now=monotonic_ns(),wait=submit_wall_ns-*submitted;
    uint64_t elapsed=now>=*started ? now-*started:0;
    *total+=elapsed>wait ? elapsed-wait:0;
    draw_profile.submit_ns+=wait;
    *started=now;*submitted=submit_wall_ns;
}
/* Preserve rejected inputs only when the existing local diagnostic directory
   is configured. No command is retried or changed after rejection. */
static void native_failure_dump(const char *name,const void *data,size_t size) {
    const char *directory=config_string("debug.texture_dump_directory");
    if (!*directory || !data || !size) return;
    char path[1024];
    int length=snprintf(path,sizeof(path),"%s/native-failure-frame%lu-%s.bin",directory,device.frame,name);
    if (length<=0 || (size_t)length>=sizeof(path)) return;
    FILE *file=fopen(path,"wb");
    if (!file) return;
    size_t written=fwrite(data,1,size,file);
    int closed=fclose(file);
    platform_log("Native rejected input dump: %s bytes %lu/%lu close %d",path,
        (unsigned long)written,(unsigned long)size,closed);
}
static void native_failed_packet(void) {
    platform_log("Native rejected packet: sequence %llu commands %u bytes %u failed-index %u",
        (unsigned long long)transport.sequence,transport.command_count,transport.size,transport.reply.failed_command);
    uint32_t offset=sizeof(struct halo_metal_packet);
    for (uint32_t i=0;i<transport.command_count && i<=transport.reply.failed_command;i++) {
        if (offset>transport.size || transport.size-offset<sizeof(struct halo_metal_command)) break;
        struct halo_metal_command c;
        memcpy(&c,transport.bytes+offset,sizeof(c));
        platform_log("Native rejected packet record: index %u opcode %u offset %u extent %u",
            i,c.opcode,offset,c.byte_size);
        if (c.byte_size<sizeof(c) || c.byte_size>transport.size-offset) break;
        offset+=c.byte_size;
    }
    if (transport.bytes && transport.size<=transport.capacity)
        native_failure_dump("packet",transport.bytes,transport.size);
}
static void packet_flush(enum native_flush_reason reason) {
    if (!transport.recording) return;
    if (!transport.command_count || transport.size != packet_expected_end)
        native_fail("incomplete queued command",HALO_METAL_INVALID);
    uint32_t commands=transport.command_count,bytes=transport.size;
    uint64_t packet_sequence=transport.sequence;
    uint64_t started=monotonic_ns();
    int status=halo_metal_guest_submit(&transport);
    if (status) native_failed_packet();
    require_status("submit original commands",status);
    uint64_t ended=monotonic_ns(),duration=ended-started;
    submit_wall_ns+=duration;
    submitted_batches++;submitted_commands+=commands;submitted_bytes+=bytes;
    if (bytes>largest_statistics_batch) largest_statistics_batch=bytes;
    packet_expected_end=0;
    if (config_boolean("debug.gpu_stats")) {
        struct native_flush_statistics *s=&flush_statistics[reason];
        s->packets++;s->commands+=commands;s->bytes+=bytes;s->wall_ns+=duration;
        // Zero-based submission frame precedes Present's frame increment.
        // Native timing records the completed frame after that increment;
        // sequence joins this reason/timing with the host's packet-stage log.
        platform_log("Native flush: frame %lu, sequence %llu, reason %s, commands %u, bytes %u, start %llu ns, end %llu ns, submit %llu ns",
            device.frame,(unsigned long long)packet_sequence,native_flush_reason_name(reason),commands,bytes,
            (unsigned long long)started,(unsigned long long)ended,(unsigned long long)duration);
    }
}
static void packet_begin(uint32_t fixed,const uint32_t *payloads,uint32_t count) {
    uint32_t empty_end=0,end=0;
    /* Validate an entire empty-packet command before submitting prior work. */
    require_status("reserve complete original command",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,fixed,payloads,count,&empty_end));
    if (transport.recording) {
        if (transport.size != packet_expected_end) native_fail("unsealed queued command",HALO_METAL_INVALID);
        int room=halo_metal_packet_room(transport.size,transport.capacity,fixed,payloads,count,&end);
        if (room || end>NATIVE_BATCH_SOFT_BYTES || transport.command_count>=65536u)
            packet_flush(room ? NATIVE_FLUSH_HARD_CAPACITY:
                transport.command_count>=65536u ? NATIVE_FLUSH_COMMAND_LIMIT:NATIVE_FLUSH_SOFT_BATCH_LIMIT);
    }
    if (!transport.recording) {
        if (sequence==UINT64_MAX) native_fail("packet sequence overflow",HALO_METAL_MEMORY);
        require_status("begin packet",halo_metal_guest_begin(&transport,++sequence));
        end=empty_end;
    }
    packet_expected_end=end;
}
static uint32_t command_append(const void *record,uint32_t size) {
    uint32_t offset=0;
    require_status("append command",halo_metal_guest_append_command(&transport,record,size,&offset));
    return offset;
}
static void payload_append(uint32_t command,uint32_t field,const void *bytes,uint32_t size) {
    uint32_t offset=0;
    require_status("copy payload",halo_metal_guest_append_payload(&transport,bytes,size,&offset));
    require_status("patch payload",halo_metal_guest_patch_u32(&transport,command,field,offset));
}
static void packet_finish(void) {
    if (!transport.recording || transport.size!=packet_expected_end)
        native_fail("complete original command reservation",HALO_METAL_INVALID);
    if (transport.size>=NATIVE_BATCH_SOFT_BYTES || transport.command_count>=65536u)
        packet_flush(transport.command_count>=65536u ? NATIVE_FLUSH_COMMAND_LIMIT:NATIVE_FLUSH_SOFT_BATCH_LIMIT);
}
static void color_to_vec4(D3DCOLOR color, float out[4]) {
    out[0]=((color>>16)&255)/255.0f; out[1]=((color>>8)&255)/255.0f;
    out[2]=(color&255)/255.0f; out[3]=((color>>24)&255)/255.0f;
}
#define VERTICAL_BLANK_NANOSECONDS (1000000000L / 60)

static pthread_mutex_t vertical_blank_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertical_blank_condition = PTHREAD_COND_INITIALIZER;
static D3DCALLBACK vertical_blank_callback;
static unsigned long vertical_blank_count;
static volatile unsigned int flip_count;
static unsigned long pending_flips;
static BOOL vertical_blank_thread_started = FALSE;

static void *vertical_blank_thread(void *unused)
{
	(void)unused;
	for (;;)
	{
		D3DCALLBACK callback;

		/* The display link owns the 60 Hz phase. A free-running sleep drifts
		   against that refresh and the presented frames bunch. */
		host_metal_pace_vblank();

		pthread_mutex_lock(&vertical_blank_lock);
		vertical_blank_count++;
		/* a presented frame becomes visible at the next vertical blank */
		if (pending_flips)
		{
			pending_flips--;
			flip_count++;
		}
		callback = vertical_blank_callback;
		pthread_cond_broadcast(&vertical_blank_condition);
		pthread_mutex_unlock(&vertical_blank_lock);

		if (callback)
			callback(0);
	}
	return NULL;
}

static void vertical_blank_start(void)
{
	pthread_mutex_lock(&vertical_blank_lock);
	if (!vertical_blank_thread_started)
	{
		pthread_t thread;

		if (pthread_create(&thread, NULL, vertical_blank_thread, NULL) == 0)
		{
			pthread_detach(thread);
			vertical_blank_thread_started = TRUE;
		}
		else
		{
			platform_log("cannot start the vertical blank thread");
		}
	}
	pthread_mutex_unlock(&vertical_blank_lock);
}

/* replaces main/d3d_intimacy.cpp, which reads the counter out of the Xbox
Direct3D runtime's private device structure */
volatile unsigned int *d3d_find_flipcount(void)
{
	return &flip_count;
}

void WINAPI D3DDevice_SetVerticalBlankCallback(D3DCALLBACK callback)
{
	pthread_mutex_lock(&vertical_blank_lock);
	vertical_blank_callback = callback;
	pthread_mutex_unlock(&vertical_blank_lock);
	vertical_blank_start();
}

void WINAPI D3DDevice_BlockUntilVerticalBlank(void)
{
	unsigned long count;

	vertical_blank_start();
	pthread_mutex_lock(&vertical_blank_lock);
	count = vertical_blank_count;
	while (vertical_blank_count == count)
		pthread_cond_wait(&vertical_blank_condition, &vertical_blank_lock);
	pthread_mutex_unlock(&vertical_blank_lock);
}

static void viewport_update_constants(void)
{
	/* Direct3D's reserved constants c[-38] and c[-37] map clip space to
	the screen; zscale is the depth buffer's range */
	float zscale = 16777215.0f;
	unsigned long width, height;
	BOOL depth;

	if (device.depth_stencil)
	{
		struct xgpu_texture_description description;

		xgpu_texture_describe(device.depth_stencil->Format, device.depth_stencil->Size, &description);
		if (description.format == D3DFMT_D16 || description.format == D3DFMT_LIN_D16 ||
			description.format == D3DFMT_F16 || description.format == D3DFMT_LIN_F16)
		{
			zscale = 65535.0f;
		}
	}
	(void)width; (void)height; (void)depth;
	device.viewport_scale[0] = device.viewport.Width * 0.5f;
	device.viewport_scale[1] = -(float)device.viewport.Height * 0.5f;
	device.viewport_scale[2] = zscale * (device.viewport.MaxZ - device.viewport.MinZ);
	device.viewport_scale[3] = 0.0f;
	device.viewport_offset[0] = device.viewport.X + device.viewport.Width * 0.5f;
	device.viewport_offset[1] = device.viewport.Y + device.viewport.Height * 0.5f;
	device.viewport_offset[2] = zscale * device.viewport.MinZ;
	device.viewport_offset[3] = 0.0f;
	if (!(device.shader_constant_mode & D3DSCM_NORESERVEDCONSTANTS))
	{
		constants_store(XGPU_VERTEX_CONSTANT_BIAS - 38, device.viewport_scale, 1);
		constants_store(XGPU_VERTEX_CONSTANT_BIAS - 37, device.viewport_offset, 1);
	}
}
static void surface_dimensions(const D3DSurface *surface, unsigned long *width, unsigned long *height, BOOL *depth)
{
	struct xgpu_texture_description description;
	DWORD format;

	xgpu_texture_describe(surface->Format, surface->Size, &description);
	*width = description.width;
	*height = description.height;
	format = description.format;
	*depth = format == D3DFMT_D24S8 || format == D3DFMT_F24S8 || format == D3DFMT_D16 || format == D3DFMT_F16 ||
		format == D3DFMT_LIN_D24S8 || format == D3DFMT_LIN_F24S8 || format == D3DFMT_LIN_D16 || format == D3DFMT_LIN_F16;
}

static int native_storage_dimensions(uint32_t logical_width,
    struct halo_metal_render_dimensions *dimensions) {
    int pixel_width=0,pixel_height=0;
    platform_video_drawable_size(&pixel_width,&pixel_height);
    if (pixel_width<=0 || pixel_height<=0 ||
        pixel_width>(int)HALO_METAL_RENDER_MAX_DIMENSION ||
        pixel_height>(int)HALO_METAL_RENDER_MAX_DIMENSION)
        return HALO_METAL_INVALID;
    uint32_t width=(uint32_t)pixel_width,height=(uint32_t)pixel_height;
    uint64_t drawable_logical_width=((uint64_t)SCREEN_HEIGHT*width/height)&~UINT64_C(1);
    /* Explicit4:3/aspect requests and displays beyond the supported logical
       canvas keep their shape. Auto aspect on the startup display instead
       maps its even logical columns to the exact physical drawable edges. */
    if (config_integer("display.screen_width")>0 || drawable_logical_width!=logical_width) {
        struct halo_metal_render_dimensions logical={logical_width,SCREEN_HEIGHT,
            logical_width,SCREEN_HEIGHT,HALO_METAL_RENDER_SCALE_UNIFORM};
        struct halo_metal_render_rectangle box;
        int status=halo_metal_render_presentation_box(&logical,width,height,&box);
        if (status) return status;
        width=box.width;height=box.height;
    }
    return halo_metal_render_native_target_dimensions(
        logical_width,SCREEN_HEIGHT,logical_width,width,height,dimensions);
}
static void native_storage_initialize(void) {
    if (native_render_height() || (native_storage_width && native_storage_height)) return;
    uint32_t logical_width=(uint32_t)halo_screen_width();
    struct halo_metal_render_dimensions dimensions;
    require_status("native drawable dimensions",native_storage_dimensions(logical_width,&dimensions));
    native_storage_width=dimensions.storage_width;native_storage_height=dimensions.storage_height;
    screen_scale[0]=(float)native_storage_width/logical_width;
    screen_scale[1]=(float)native_storage_height/SCREEN_HEIGHT;
    int pixel_width=0,pixel_height=0;
    platform_video_drawable_size(&pixel_width,&pixel_height);
    platform_log("Native resolution: %ux%u storage, %ux%d logical, %dx%d drawable, startup aspect %s",
        native_storage_width,native_storage_height,logical_width,SCREEN_HEIGHT,pixel_width,pixel_height,
        native_storage_width==(uint32_t)pixel_width && native_storage_height==(uint32_t)pixel_height ? "native":"fitted");
}
static void native_initialize(void) {
    void *storage = aligned_alloc(16, HALO_METAL_MAX_PACKET);
    if (!storage) native_fail("allocate packet",HALO_METAL_MEMORY);
    require_status("setup transport",halo_metal_guest_setup(&transport,storage,HALO_METAL_MAX_PACKET));
    int fxaa=!strcmp(config_string("display.anti_aliasing"),"fxaa");
    gpu_vertex_decode=config_boolean("debug.metal_gpu_vertex_decode");
    require_status("initialize native window",halo_metal_guest_initialize(&transport,
        platform_video_native_window(),(fxaa ? HALO_METAL_ENABLE_FXAA:0) |
        (gpu_vertex_decode ? HALO_METAL_ENABLE_COMPACT_VERTICES:0),HALO_METAL_CAP_TARGETS | HALO_METAL_CAP_UPLOAD |
        HALO_METAL_CAP_CLEAR | HALO_METAL_CAP_DRAW | HALO_METAL_CAP_READBACK |
        HALO_METAL_CAP_VISIBILITY | (fxaa ? HALO_METAL_CAP_FXAA:0) |
        (gpu_vertex_decode ? HALO_METAL_CAP_COMPACT_VERTICES:0)));
    /* platform_video_initialize has already synchronized the Retina window.
       Never resolve native pixels before that point or from display points. */
    native_storage_initialize();
    draw_diagnostics=config_boolean("debug.gpu_stats");
    for (unsigned i=0;i<16;i++) device.attributes[i][3]=1.0f;
    memory_watch_initialize();
    device.antialiasing_enabled=fxaa;
    device.ready=TRUE;
}
void WINAPI Direct3D_SetPushBufferSize(DWORD size,DWORD segments) { (void)size; (void)segments; }
static int backbuffer_presentation_supported(const D3DPRESENT_PARAMETERS *parameters) {
    /* The original title zeroes BackBufferCount and MultiSampleType and uses
       DISCARD. This implements its indexed0/-1 loading route, not a general
       Xbox physical swap chain or extra positive back-buffer indices. */
    return parameters && parameters->BackBufferCount==0 &&
        parameters->SwapEffect==D3DSWAPEFFECT_DISCARD &&
        (parameters->MultiSampleType==0 || parameters->MultiSampleType==D3DMULTISAMPLE_NONE);
}
static void backbuffer_surfaces_initialize(unsigned long width,unsigned long height) {
#ifdef HALO_ANDROID
    d3d8_surface_initialize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_initialize(&device.history_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_initialize(&device.depth_buffer,D3DFMT_LIN_D24S8,width,height);
#else
    /* Keep both indexed headers and their Data aliases stable when F11
       changes metadata. The device owns each distinct allocation. */
    d3d8_surface_initialize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,SCREEN_MAXIMUM_WIDTH,height);
    d3d8_surface_initialize(&device.history_buffer,D3DFMT_LIN_A8R8G8B8,SCREEN_MAXIMUM_WIDTH,height);
    d3d8_surface_initialize(&device.depth_buffer,D3DFMT_LIN_D24S8,SCREEN_MAXIMUM_WIDTH,height);
    d3d8_surface_resize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_resize(&device.history_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_resize(&device.depth_buffer,D3DFMT_LIN_D24S8,width,height);
#endif
    if (!device.back_buffer.Data || !device.history_buffer.Data || !device.depth_buffer.Data)
        native_fail("allocate indexed back buffers",HALO_METAL_MEMORY);
}
HRESULT WINAPI Direct3D_CreateDevice(UINT adapter, D3DDEVTYPE device_type, void *unused, DWORD behavior_flags,
	D3DPRESENT_PARAMETERS *presentation_parameters, D3DDevice **returned_device)
{
	unsigned long width, height;
	int index;

	(void)adapter;
	(void)device_type;
	(void)unused;
	(void)behavior_flags;
	if (!device.created)
	{
		if (!backbuffer_presentation_supported(presentation_parameters))
		{
			platform_log("Native Metal: unsupported original back-buffer count, swap effect or multisampling");
			return E_FAIL;
		}
		memset(&device, 0, sizeof(device));
		if (presentation_parameters)
			device.presentation = *presentation_parameters;
		width = device.presentation.BackBufferWidth ? device.presentation.BackBufferWidth : 640;
		height = device.presentation.BackBufferHeight ? device.presentation.BackBufferHeight : 480;
		backbuffer_surfaces_initialize(width,height);
		device.render_target = &device.back_buffer;
		device.depth_stencil = &device.depth_buffer;
		for (index = 0; index < D3DTS_MAX; index++)
		{
			device.transforms[index]._11 = 1.0f;
			device.transforms[index]._22 = 1.0f;
			device.transforms[index]._33 = 1.0f;
			device.transforms[index]._44 = 1.0f;
		}
		device.viewport.Width = width;
		device.viewport.Height = height;
		device.viewport.MaxZ = 1.0f;
		device.next_vertex_shader_id = 1;
		D3D__RenderState[D3DRS_ZENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZWRITEENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
		D3D__RenderState[D3DRS_COLORWRITEENABLE] = D3DCOLORWRITEENABLE_ALL;
		D3D__RenderState[D3DRS_SRCBLEND] = D3DBLEND_ONE;
		D3D__RenderState[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
		D3D__RenderState[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
		D3D__RenderState[D3DRS_CULLMODE] = D3DCULL_CCW;
		D3D__RenderState[D3DRS_FRONTFACE] = D3DFRONT_CW;
		D3D__RenderState[D3DRS_FILLMODE] = D3DFILL_SOLID;
		D3D__RenderState[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILWRITEMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILZFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
		for (index = 0; index < D3DTSS_MAXSTAGES; index++)
		{
			D3D__TextureState[index][D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSW] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_MAGFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MINFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MAXANISOTROPY] = 1;
		}
		viewport_update_constants();

		if (!config_boolean("debug.null_renderer") && platform_video_initialize(width, height))
			native_initialize();
		else
			return E_FAIL;
		device.created = TRUE;
	}
	*returned_device = device_pointer();
	return S_OK;
}
ULONG WINAPI D3DDevice_Release(void)
{
	return 1;
}
void WINAPI D3DDevice_GetDeviceCaps(D3DCAPS8 *caps)
{
	memset(caps, 0, sizeof(*caps));
	caps->DeviceType = D3DDEVTYPE_HAL;
	caps->MaxTextureWidth = 4096;
	caps->MaxTextureHeight = 4096;
	caps->MaxVolumeExtent = 512;
	caps->MaxTextureRepeat = 8192;
	caps->MaxTextureAspectRatio = 4096;
	caps->MaxAnisotropy = 4;
	caps->MaxTextureBlendStages = 4;
	caps->MaxSimultaneousTextures = 4;
	caps->MaxActiveLights = 8;
	caps->MaxVertexBlendMatrices = 4;
	caps->MaxPointSize = 64.0f;
	caps->MaxPrimitiveCount = 0xfffff;
	caps->MaxVertexIndex = 0xffff;
	caps->MaxStreams = 16;
	caps->MaxStreamStride = 255;
	caps->VertexShaderVersion = D3DVS_VERSION(1, 1);
	caps->MaxVertexShaderConst = 192;
	caps->PixelShaderVersion = D3DPS_VERSION(1, 1);
	caps->MaxPixelShaderValue = 1.0f;
}
void WINAPI D3DDevice_GetBackBuffer(INT back_buffer, D3DBACKBUFFER_TYPE type, D3DSurface **result)
{
	if (!result) native_fail("indexed back buffer result",HALO_METAL_INVALID);
	if (type!=D3DBACKBUFFER_TYPE_MONO || (back_buffer!=0 && back_buffer!=-1))
		native_fail("indexed back buffer index or type",HALO_METAL_UNSUPPORTED);
	D3DSurface *surface=back_buffer==0 ? &device.back_buffer:&device.history_buffer;
	/* like Direct3D, the caller gets a reference it must release */
	surface->Common++;
	*result = surface;
}
HRESULT WINAPI D3DDevice_GetDepthStencilSurface(D3DSurface **result)
{
	*result = device.depth_stencil;
	if (!*result)
		return D3DERR_NOTFOUND;
	(*result)->Common++;
	return S_OK;
}
void WINAPI D3DDevice_SetViewport(CONST D3DVIEWPORT8 *viewport)
{
	device.viewport = *viewport;
	viewport_update_constants();
}
void WINAPI D3DDevice_SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		device.transforms[state] = *matrix;
}
void WINAPI D3DDevice_GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		*matrix = device.transforms[state];
}
void WINAPI D3DDevice_SetShaderConstantMode(D3DSHADERCONSTANTMODE mode)
{
	device.shader_constant_mode = mode;
	viewport_update_constants();
}

void WINAPI D3DDevice_SetRenderTarget(D3DSurface *color,D3DSurface *depth) {
    if (color) device.render_target=color;
    device.depth_stencil=depth;
    if (device.render_target) {
        unsigned long w,h; BOOL is_depth;
        surface_dimensions(device.render_target,&w,&h,&is_depth);
        device.viewport.X=device.viewport.Y=0;
        device.viewport.Width=w; device.viewport.Height=h;
        device.viewport.MinZ=0; device.viewport.MaxZ=1;
    }
    viewport_update_constants();
}
void WINAPI D3DDevice_SetFlickerFilter(DWORD v) { (void)v; }
void WINAPI D3DDevice_SetSoftDisplayFilter(BOOL v) { (void)v; }
void D3DFASTCALL D3DDevice_SetRenderState_Simple(DWORD method, DWORD value)
{
	/* callers also store the value in D3D__RenderState themselves */
	(void)method;
	(void)value;
}

void D3DFASTCALL D3DDevice_SetRenderState_Deferred(D3DRENDERSTATETYPE state, DWORD value)
{
	if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value);

void WINAPI D3DDevice_SetRenderStateNotInline(D3DRENDERSTATETYPE state, DWORD value)
{
	if (state == D3DRS_ZBIAS)
		D3DDevice_SetRenderState_ZBias(value);
	else if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

/* As the Xbox's D3D8 does it: a z bias is a polygon offset of -bias depth
units plus -bias/4 times the polygon's depth slope, enabled for every fill
mode. Without the slope term, decals (biased by 8) fight with the surface
under them wherever it is seen at an angle. */
void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value)
{
	float offset = -(float)value;
	float slope = offset * 0.25f;
	DWORD enable = value != 0;

	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE], &slope, sizeof(slope));
	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET], &offset, sizeof(offset));
	D3D__RenderState[D3DRS_POINTOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_WIREFRAMEOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_ZBIAS] = value;
}

#define COMPLEX_RENDER_STATE(name, state) \
	void WINAPI D3DDevice_SetRenderState_##name(DWORD value) { D3D__RenderState[state] = value; }

COMPLEX_RENDER_STATE(PSTextureModes, D3DRS_PSTEXTUREMODES)
COMPLEX_RENDER_STATE(VertexBlend, D3DRS_VERTEXBLEND)
COMPLEX_RENDER_STATE(FogColor, D3DRS_FOGCOLOR)
COMPLEX_RENDER_STATE(FillMode, D3DRS_FILLMODE)
COMPLEX_RENDER_STATE(BackFillMode, D3DRS_BACKFILLMODE)
COMPLEX_RENDER_STATE(TwoSidedLighting, D3DRS_TWOSIDEDLIGHTING)
COMPLEX_RENDER_STATE(NormalizeNormals, D3DRS_NORMALIZENORMALS)
COMPLEX_RENDER_STATE(ZEnable, D3DRS_ZENABLE)
COMPLEX_RENDER_STATE(StencilEnable, D3DRS_STENCILENABLE)
COMPLEX_RENDER_STATE(StencilFail, D3DRS_STENCILFAIL)
COMPLEX_RENDER_STATE(FrontFace, D3DRS_FRONTFACE)
COMPLEX_RENDER_STATE(CullMode, D3DRS_CULLMODE)
COMPLEX_RENDER_STATE(TextureFactor, D3DRS_TEXTUREFACTOR)
COMPLEX_RENDER_STATE(LogicOp, D3DRS_LOGICOP)
COMPLEX_RENDER_STATE(EdgeAntiAlias, D3DRS_EDGEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleAntiAlias, D3DRS_MULTISAMPLEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleMask, D3DRS_MULTISAMPLEMASK)
COMPLEX_RENDER_STATE(MultiSampleType, D3DRS_MULTISAMPLETYPE)
COMPLEX_RENDER_STATE(ShadowFunc, D3DRS_SHADOWFUNC)
COMPLEX_RENDER_STATE(LineWidth, D3DRS_LINEWIDTH)
COMPLEX_RENDER_STATE(Dxt1NoiseEnable, D3DRS_DXT1NOISEENABLE)
COMPLEX_RENDER_STATE(YuvEnable, D3DRS_YUVENABLE)
COMPLEX_RENDER_STATE(OcclusionCullEnable, D3DRS_OCCLUSIONCULLENABLE)
COMPLEX_RENDER_STATE(StencilCullEnable, D3DRS_STENCILCULLENABLE)
COMPLEX_RENDER_STATE(RopZCmpAlwaysRead, D3DRS_ROPZCMPALWAYSREAD)
COMPLEX_RENDER_STATE(RopZRead, D3DRS_ROPZREAD)
COMPLEX_RENDER_STATE(DoNotCullUncompressed, D3DRS_DONOTCULLUNCOMPRESSED)

void D3DFASTCALL D3DDevice_SetTextureState_Deferred(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTextureState_TexCoordIndex(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_TEXCOORDINDEX] = value;
}

void WINAPI D3DDevice_SetTextureState_BorderColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_BORDERCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_ColorKeyColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_COLORKEYCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_BumpEnv(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTexture(DWORD stage, D3DBaseTexture *texture)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.textures[stage] = texture;
}

void WINAPI D3DDevice_SetPalette(DWORD stage, D3DPalette *palette)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.palettes[stage] = palette;
}

void WINAPI D3DDevice_SetPixelShaderProgram(D3DPIXELSHADERDEF *definition)
{
	/* the definition's members are the pixel shader render states */
	if (!definition)
		return;
	memcpy(&D3D__RenderState[D3DRS_PSALPHAINPUTS0], definition->PSAlphaInputs, sizeof(definition->PSAlphaInputs));
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSABCD] = definition->PSFinalCombinerInputsABCD;
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSEFG] = definition->PSFinalCombinerInputsEFG;
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT0_0], definition->PSConstant0, sizeof(definition->PSConstant0));
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT1_0], definition->PSConstant1, sizeof(definition->PSConstant1));
	memcpy(&D3D__RenderState[D3DRS_PSALPHAOUTPUTS0], definition->PSAlphaOutputs, sizeof(definition->PSAlphaOutputs));
	memcpy(&D3D__RenderState[D3DRS_PSRGBINPUTS0], definition->PSRGBInputs, sizeof(definition->PSRGBInputs));
	D3D__RenderState[D3DRS_PSCOMPAREMODE] = definition->PSCompareMode;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0] = definition->PSFinalCombinerConstant0;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1] = definition->PSFinalCombinerConstant1;
	memcpy(&D3D__RenderState[D3DRS_PSRGBOUTPUTS0], definition->PSRGBOutputs, sizeof(definition->PSRGBOutputs));
	D3D__RenderState[D3DRS_PSCOMBINERCOUNT] = definition->PSCombinerCount;
	D3D__RenderState[D3DRS_PSTEXTUREMODES] = definition->PSTextureModes;
	D3D__RenderState[D3DRS_PSDOTMAPPING] = definition->PSDotMapping;
	D3D__RenderState[D3DRS_PSINPUTTEXTURE] = definition->PSInputTexture;
}


static struct vertex_shader_object *vertex_shader_from_handle(DWORD handle) {
    /* Literal zero is the original fixed pipeline selector. Unknown values
       (including FVF codes) must never be dereferenced as guest pointers. */
    for (struct vertex_shader_object *object=device.vertex_shader_objects;object;object=object->next)
        if ((DWORD)object==handle && object->signature==VERTEX_SHADER_SIGNATURE) return object;
    return NULL;
}
HRESULT WINAPI D3DDevice_CreateVertexShader(const DWORD *tokens,const DWORD *function,DWORD *handle,DWORD usage) {
    struct vertex_shader_object *object=calloc(1,sizeof(*object));
    size_t count=0;
    (void)usage;
    if (!object) return E_OUTOFMEMORY;
    if (!handle || !tokens) { free(object); return E_INVALIDARG; }
    /* Original declarations are terminated. Bound token and inline-constant
       counts before passing them to the backend-neutral parser. */
    while (count<1024) {
        DWORD token=tokens[count++];
        unsigned type=(token&D3DVSD_TOKENTYPEMASK)>>D3DVSD_TOKENTYPESHIFT;
        if (token==D3DVSD_END()) break;
        size_t skip=type==D3DVSD_TOKEN_CONSTMEM ? ((token&D3DVSD_CONSTCOUNTMASK)>>D3DVSD_CONSTCOUNTSHIFT)*4 :
            type==D3DVSD_TOKEN_EXT ? (token&D3DVSD_EXTCOUNTMASK)>>D3DVSD_EXTCOUNTSHIFT : 0;
        if (skip>1024-count) { free(object); return E_INVALIDARG; }
        count+=skip;
    }
    if (!count || tokens[count-1]!=D3DVSD_END() ||
        metal_vertex_declaration_parse((const uint32_t *)tokens,count,&object->declaration)) { free(object); return E_INVALIDARG; }
    object->signature=VERTEX_SHADER_SIGNATURE;
    object->id=device.next_vertex_shader_id++;
    if (function) {
        object->instruction_count=function[0]>>16;
        if (!object->instruction_count || object->instruction_count>136) { free(object); return E_INVALIDARG; }
        size_t bytes=object->instruction_count*4*sizeof(DWORD);
        object->instructions=malloc(bytes);
        if (!object->instructions) { free(object); return E_OUTOFMEMORY; }
        memcpy(object->instructions,function+1,bytes);
    }
    object->next=device.vertex_shader_objects;device.vertex_shader_objects=object;
    *handle=(DWORD)object; return S_OK;
}
void WINAPI D3DDevice_DeleteVertexShader(DWORD handle)
{
	/* programs stay cached; the object is small */
	(void)handle;
}
void WINAPI D3DDevice_SetVertexShader(DWORD handle)
{
	if (!handle) {
		device.fixed_function_selected=TRUE;
		return;
	}
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);
	if (!object || !object->instructions)
		native_fail("unsupported original FVF or declaration-only vertex shader",HALO_METAL_UNSUPPORTED);
	device.vertex_shader = object;
	device.program_address = 0;
	device.program_slots[0] = object;
	device.fixed_function_selected=FALSE;
}
void WINAPI D3DDevice_LoadVertexShader(DWORD handle, DWORD address)
{
	if (address < VERTEX_PROGRAM_SLOTS)
		device.program_slots[address] = vertex_shader_from_handle(handle);
}
void WINAPI D3DDevice_SelectVertexShader(DWORD handle, DWORD address)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);
	if (handle && (!object || !object->instructions))
		native_fail("unsupported original vertex shader selection",HALO_METAL_UNSUPPORTED);
	if (object)
		device.vertex_shader = object;
	if (address < VERTEX_PROGRAM_SLOTS) {
		device.program_address = address;
		/* Original runtime reselects loaded slots with handle0 (runtime.c:281).
		 * This API's zero is not SetVertexShader(0)'s fixed-function selector. */
		device.fixed_function_selected=FALSE;
	}
}
void WINAPI D3DDevice_GetVertexShaderSize(DWORD handle, UINT *size)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	*size = object ? object->instruction_count : 0;
}
void WINAPI D3DDevice_SetVertexShaderConstant(INT reg, CONST void *constant_data, DWORD constant_count)
{
	long first = reg + XGPU_VERTEX_CONSTANT_BIAS;

	if (first < 0 || first >= XGPU_VERTEX_CONSTANT_COUNT)
		return;
	if (first + (long)constant_count > XGPU_VERTEX_CONSTANT_COUNT)
		constant_count = XGPU_VERTEX_CONSTANT_COUNT - first;
	constants_store((unsigned long)first, constant_data, constant_count);
}
static struct vertex_shader_object *current_program(void)
{
	struct vertex_shader_object *program = device.program_slots[device.program_address];

	return program ? program : device.vertex_shader;
}
void WINAPI D3DDevice_SetStreamSource(UINT stream_number, D3DVertexBuffer *stream_data, UINT stride)
{
	if (stream_number >= 16)
		return;
	device.streams[stream_number].data = stream_data ? stream_data->Data : 0;
	device.streams[stream_number].stride = stride;
}
void WINAPI D3DDevice_SetIndices(D3DIndexBuffer *index_data, UINT base_vertex_index)
{
	device.base_vertex_index = base_vertex_index;
	D3D__IndexData = index_data ? (WORD *)index_data->Data : NULL;
}
void WINAPI D3DDevice_Begin(D3DPRIMITIVETYPE primitive_type)
{
	device.immediate_active = TRUE;
	device.immediate_type = primitive_type;
	device.immediate_count = 0;
}
static void immediate_emit(void);
static void set_attribute(INT reg, float a, float b, float c, float d)
{
	BOOL emit = FALSE;

	if (reg == D3DVSDE_VERTEX)
	{
		reg = 0;
		emit = TRUE;
	}
	if (reg < 0 || reg >= XGPU_VERTEX_ATTRIBUTE_COUNT)
		return;
	device.attributes[reg][0] = a;
	device.attributes[reg][1] = b;
	device.attributes[reg][2] = c;
	device.attributes[reg][3] = d;
	/* like the hardware, writing register 0 completes a vertex */
	if (device.immediate_active && (emit || reg == 0))
		immediate_emit();
}
void WINAPI D3DDevice_SetVertexData2f(INT reg, FLOAT a, FLOAT b)
{
	set_attribute(reg, a, b, 0.0f, 1.0f);
}
void WINAPI D3DDevice_SetVertexData4f(INT reg, FLOAT a, FLOAT b, FLOAT c, FLOAT d)
{
	set_attribute(reg, a, b, c, d);
}
void WINAPI D3DDevice_SetVertexData2s(INT reg, SHORT a, SHORT b)
{
	set_attribute(reg, (float)a, (float)b, 0.0f, 1.0f);
}
void WINAPI D3DDevice_SetVertexData4ub(INT reg, BYTE a, BYTE b, BYTE c, BYTE d)
{
	set_attribute(reg, a / 255.0f, b / 255.0f, c / 255.0f, d / 255.0f);
}
void WINAPI D3DDevice_SetVertexDataColor(INT reg, D3DCOLOR color)
{
	float value[4];

	color_to_vec4(color, value);
	set_attribute(reg, value[0], value[1], value[2], value[3]);
}

/* Resource, draw and presentation implementation follows. */

static void physical_span(DWORD data, unsigned long bytes) {
    if (!data || data >= PLATFORM_CONTIGUOUS_SIZE || bytes > PLATFORM_CONTIGUOUS_SIZE-data)
        native_fail("guest physical resource range",HALO_METAL_INVALID);
}
static uint32_t texture_format(enum xgpu_texture_copy_format format) {
    switch (format) {
    case _xgpu_texture_copy_rgba8:return HALO_METAL_RGBA8;
    case _xgpu_texture_copy_bgra8:return HALO_METAL_BGRA8;
    case _xgpu_texture_copy_bc1:return HALO_METAL_BC1;
    case _xgpu_texture_copy_bc2:return HALO_METAL_BC2;
    case _xgpu_texture_copy_bc3:return HALO_METAL_BC3;
    default:native_fail("original texture format",HALO_METAL_UNSUPPORTED);return 0;
    }
}
static struct halo_metal_render_dimensions resource_dimensions(const struct native_resource *entry) {
    return (struct halo_metal_render_dimensions){entry->description.width,entry->description.height,
        entry->storage_width,entry->storage_height,entry->scale_mode};
}
static void native_raster_scale(const struct native_resource *target,struct halo_metal_draw_state *state) {
    struct halo_metal_render_dimensions dimensions=resource_dimensions(target);
    struct halo_metal_render_edges edges={(int64_t)state->viewport[0],(int64_t)state->viewport[1],
        (int64_t)state->viewport[0]+(int64_t)state->viewport[2],
        (int64_t)state->viewport[1]+(int64_t)state->viewport[3]};
    struct halo_metal_render_rectangle rectangle;
    require_status("scale native viewport",halo_metal_render_scale_rectangle(&dimensions,&edges,&rectangle));
    state->viewport[0]=rectangle.x;state->viewport[1]=rectangle.y;
    state->viewport[2]=rectangle.width;state->viewport[3]=rectangle.height;
    edges=(struct halo_metal_render_edges){state->scissor[0],state->scissor[1],
        (int64_t)state->scissor[0]+state->scissor[2],(int64_t)state->scissor[1]+state->scissor[3]};
    require_status("scale native scissor",halo_metal_render_scale_rectangle(&dimensions,&edges,&rectangle));
    state->scissor[0]=rectangle.x;state->scissor[1]=rectangle.y;
    state->scissor[2]=rectangle.width;state->scissor[3]=rectangle.height;
}
static void resource_create(struct native_resource *entry) {
    struct halo_metal_create_ex command={0};
    if (!next_resource_id) native_fail("resource ID overflow",HALO_METAL_MEMORY);
    entry->ref.id=next_resource_id++; entry->ref.generation=1;
    command.command.opcode=HALO_METAL_CREATE_TEXTURE_EX;
    command.resource=entry->ref; command.format=entry->format;
    if (!entry->storage_width) entry->storage_width=entry->description.width;
    if (!entry->storage_height) entry->storage_height=entry->description.height;
    command.width=entry->storage_width; command.height=entry->storage_height;
    command.depth=entry->volume ? entry->description.depth:1;
    command.type=entry->volume ? HALO_METAL_TEXTURE_3D:
        entry->description.cube_map ? HALO_METAL_TEXTURE_CUBE:HALO_METAL_TEXTURE_2D;
    command.mip_levels=entry->description.levels;
    command.usage=entry->usage;
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
}
static void resource_delete(struct native_resource *entry) {
    struct halo_metal_delete command={0};
    command.command.opcode=HALO_METAL_DELETE_TEXTURE;command.resource=entry->ref;
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
}
static void target_storage_initialize(struct native_resource *entry) {
    /* Establish every plane before a subsequent partial write or load. */
    struct halo_metal_clear clear={0};
    clear.command.opcode=HALO_METAL_CLEAR;clear.width=entry->storage_width;clear.height=entry->storage_height;
    if (entry->format==HALO_METAL_DEPTH32_STENCIL8) {
        clear.depth_stencil=entry->ref;clear.planes=HALO_METAL_DEPTH|HALO_METAL_STENCIL;
    } else { clear.color=entry->ref;clear.planes=HALO_METAL_COLOR; }
    packet_begin(sizeof(clear),NULL,0);command_append(&clear,sizeof(clear));packet_finish();
}
static struct native_resource *target_get(const D3DSurface *surface) {
    struct native_resource *entry;
    unsigned long width,height; BOOL depth;
    if (!surface || !surface->Data) native_fail("missing original render target",HALO_METAL_INVALID);
    surface_dimensions(surface,&width,&height,&depth);
    struct halo_metal_render_dimensions dimensions;
    if (native_render_height()) {
        require_status("native target dimensions",halo_metal_render_target_dimensions(width,height,
            halo_screen_width(),native_render_height(),&dimensions));
    } else {
        if (!native_storage_width || !native_storage_height)
            native_fail("native backing not initialized",HALO_METAL_INVALID);
        require_status("native target dimensions",halo_metal_render_native_target_dimensions(width,height,
            halo_screen_width(),native_storage_width,native_storage_height,&dimensions));
    }
    for (entry=resource_buckets[native_resource_bucket(surface->Data)];entry;entry=entry->bucket_next)
        if ((entry->usage&HALO_METAL_RENDER_TARGET) && entry->data==surface->Data &&
            entry->format_word==surface->Format && entry->size_word==surface->Size &&
            entry->storage_width==dimensions.storage_width && entry->storage_height==dimensions.storage_height &&
            entry->scale_mode==dimensions.scale_mode) return entry;
    entry=calloc(1,sizeof(*entry));
    if (!entry) native_fail("allocate render target",HALO_METAL_MEMORY);
    entry->data=surface->Data;entry->format_word=surface->Format;entry->size_word=surface->Size;
    xgpu_texture_describe(surface->Format,surface->Size,&entry->description);
    if (entry->description.cube_map || entry->description.depth!=1)
        native_fail("render target type",HALO_METAL_UNSUPPORTED);
    entry->description.levels=1;
    entry->storage_width=dimensions.storage_width;entry->storage_height=dimensions.storage_height;
    entry->scale_mode=dimensions.scale_mode;
    entry->format=depth ? HALO_METAL_DEPTH32_STENCIL8 : HALO_METAL_BGRA8;
    entry->usage=HALO_METAL_SHADER_READ|HALO_METAL_RENDER_TARGET;
    resource_create(entry);
    resource_publish(entry);
    /* Fresh D3D attachment storage is undefined. Establish a deterministic
       zero allocation so untouched planes can safely be loaded by Metal.
       This uses no reference framebuffer or captured after-draw pixels. */
    target_storage_initialize(entry);
    return entry;
}
static struct native_resource *rendered_alias(DWORD data) {
    struct native_resource *entry,*best=NULL;
    for (entry=resource_buckets[native_resource_bucket(data)];entry;entry=entry->bucket_next)
        if (entry->data==data && (entry->usage&HALO_METAL_RENDER_TARGET) &&
            entry->format!=HALO_METAL_DEPTH32_STENCIL8 && (!best || entry->last_rendered>best->last_rendered)) best=entry;
    return best;
}
static uint64_t rendered_serial_next(void) {
    if (device.resource_serial==UINT64_MAX) native_fail("render content serial overflow",HALO_METAL_MEMORY);
    return ++device.resource_serial;
}
/* Presentation enhancement only: retain original draw state, visibility
   results, depth and stencil. The host filters an immutable copy of this
   player's world viewport; subsequent original HUD draws remain unfiltered. */
void halo_metal_antialias_before_hud(long left,long top,long right,long bottom) {
    if (!device.antialiasing_enabled || !device.ready) return;
    if (!(transport.reply.capabilities&HALO_METAL_CAP_FXAA))
        native_fail("pre-HUD anti-aliasing capability",HALO_METAL_UNSUPPORTED);
    if (device.visibility_test_active || !device.render_target)
        native_fail("pre-HUD anti-aliasing target state",HALO_METAL_INVALID);
    struct native_resource *target=target_get(device.render_target);
    if (target->format!=HALO_METAL_BGRA8 || left<0 || top<0 || right<left || bottom<top ||
        (uint64_t)right>target->description.width || (uint64_t)bottom>target->description.height)
        native_fail("pre-HUD anti-aliasing viewport",HALO_METAL_INVALID);
    if (left==right || top==bottom) return;
    struct halo_metal_render_dimensions dimensions=resource_dimensions(target);
    struct halo_metal_render_edges edges={left,top,right,bottom};
    struct halo_metal_render_rectangle rectangle;
    require_status("scale pre-HUD anti-aliasing viewport",
        halo_metal_render_scale_rectangle(&dimensions,&edges,&rectangle));
    if (!rectangle.width || !rectangle.height) return;
    if (device.antialias_passes==UINT64_MAX || device.resource_serial==UINT64_MAX)
        native_fail("pre-HUD anti-aliasing counter overflow",HALO_METAL_MEMORY);
    struct halo_metal_fxaa command={0};command.command.opcode=HALO_METAL_FXAA;
    command.source=target->ref;command.x=rectangle.x;command.y=rectangle.y;
    command.width=rectangle.width;command.height=rectangle.height;
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    target->last_rendered=rendered_serial_next();device.antialias_passes++;
}
void halo_screen_anti_alias(short x0, short y0, short x1, short y1)
{
	halo_metal_antialias_before_hud(x0, y0, x1, y1);
}
static void backbuffer_color_copy(struct native_resource *source,
    struct native_resource *destination,uint32_t width,uint32_t height) {
    if (!(transport.reply.capabilities&HALO_METAL_CAP_COPY_SUBRESOURCE))
        native_fail("indexed back-buffer copy capability",HALO_METAL_UNSUPPORTED);
    if (!source || !destination || source->ref.id==destination->ref.id ||
        source->format!=HALO_METAL_BGRA8 || destination->format!=source->format ||
        !width || !height || width>source->storage_width || height>source->storage_height ||
        width>destination->storage_width || height>destination->storage_height)
        native_fail("indexed back-buffer copy extent",HALO_METAL_INVALID);
    struct halo_metal_copy_subresource command={0};
    command.command.opcode=HALO_METAL_COPY_SUBRESOURCE;
    command.source=source->ref;command.destination=destination->ref;
    command.width=width;command.height=height;command.planes=HALO_METAL_COLOR;
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    destination->last_rendered=rendered_serial_next();
}
static void backbuffer_history_advance(struct native_resource *back) {
    if (!(transport.reply.capabilities&HALO_METAL_CAP_COPY_SUBRESOURCE))
        native_fail("indexed back-buffer history capability",HALO_METAL_UNSUPPORTED);
    struct native_resource *history=target_get(&device.history_buffer);
    if (back->description.width!=history->description.width ||
        back->description.height!=history->description.height ||
        back->storage_width!=history->storage_width || back->storage_height!=history->storage_height ||
        back->scale_mode!=history->scale_mode)
        native_fail("indexed back-buffer history dimensions",HALO_METAL_INVALID);
    /* The source loading screen clears0 before requesting-1. History must
       therefore advance at every Present, including before its first use.
       No physical Xbox buffer rotation or display callback is inferred. */
    backbuffer_color_copy(back,history,back->storage_width,back->storage_height);
}
static void native_history_sample_retire(void) {
    if (!history_sample_override) return;
    struct native_resource **link=&resources;
    while (*link && *link!=history_sample_override) link=&(*link)->next;
    if (!*link) native_fail("previous resolution history ownership",HALO_METAL_INVALID);
    struct native_resource *previous=*link;
    resource_unindex(previous);
    resource_delete(previous);
    packet_flush(NATIVE_FLUSH_RESIZE);
    *link=previous->next;history_sample_override=NULL;
    free(previous->mip_copy_cache);free(previous);
}
static void native_resolution_commit(void) {
    if (!device.ready || device.immediate_active || device.visibility_test_active || history_sample_override) return;
    long requested=config_integer("display.render_height");
    uint32_t height=(requested==0 || requested==720 || requested==1080 ||
        requested==1440 || requested==2160) ? (uint32_t)requested:480;
    uint32_t logical_width=(uint32_t)halo_screen_width();
    struct halo_metal_render_dimensions dimensions;
    int status=height ? halo_metal_render_target_dimensions(logical_width,SCREEN_HEIGHT,
        logical_width,height,&dimensions):native_storage_dimensions(logical_width,&dimensions);
    /* A minimized drawable cannot provide Native backing yet. Keep rendering
       with the accepted previous storage until valid dimensions are available. */
    if (status) return;
    struct native_resource *back=target_get(&device.back_buffer);
    BOOL replace=back->storage_width!=dimensions.storage_width ||
        back->storage_height!=dimensions.storage_height || back->scale_mode!=dimensions.scale_mode;
    if (!replace && height==native_render_height()) return;
    if (replace) {
        packet_flush(NATIVE_FLUSH_RESIZE);
        history_sample_override=target_get(&device.history_buffer);
        for (struct native_resource *entry=resources;entry;entry=entry->next) {
            if (!(entry->usage&HALO_METAL_RENDER_TARGET) || entry==history_sample_override ||
                entry->description.width!=logical_width || entry->description.height!=SCREEN_HEIGHT) continue;
            /* Retain every original alias signature, including color views of
               the depth-buffer address. Textures resolve before attachments. */
            resource_delete(entry);
            entry->storage_width=dimensions.storage_width;entry->storage_height=dimensions.storage_height;
            entry->scale_mode=dimensions.scale_mode;entry->last_rendered=0;
            resource_create(entry);target_storage_initialize(entry);
        }
        packet_flush(NATIVE_FLUSH_RESIZE);
    }
    render_height=height;render_settings_initialized=TRUE;
    native_storage_width=height ? 0:dimensions.storage_width;
    native_storage_height=height ? 0:dimensions.storage_height;
    screen_scale[0]=(float)dimensions.storage_width/logical_width;
    screen_scale[1]=(float)dimensions.storage_height/SCREEN_HEIGHT;
    platform_log("Native resolution applied: %ux%u storage, %ux%d logical, height %u, frame %lu",
        dimensions.storage_width,dimensions.storage_height,logical_width,SCREEN_HEIGHT,height,device.frame);
}
#ifndef HALO_ANDROID
static void backbuffer_surfaces_resize(unsigned long width,unsigned long height) {
    unsigned long old_width,old_height;BOOL depth;
    surface_dimensions(&device.history_buffer,&old_width,&old_height,&depth);
    if (width==old_width && height==old_height) return;
    if (!width || width>SCREEN_MAXIMUM_WIDTH || height!=SCREEN_HEIGHT)
        native_fail("indexed back-buffer resize dimensions",HALO_METAL_UNSUPPORTED);
    if (!(transport.reply.capabilities&HALO_METAL_CAP_COPY_SUBRESOURCE))
        native_fail("indexed back-buffer resize capability",HALO_METAL_UNSUPPORTED);
    /* Finish old-size writes before changing metadata. Keep header/Data
       identities stable; transfer the exact common pixel rectangle only. */
    packet_flush(NATIVE_FLUSH_RESIZE);
    struct native_resource *previous=target_get(&device.history_buffer);
    d3d8_surface_resize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_resize(&device.history_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_resize(&device.depth_buffer,D3DFMT_LIN_D24S8,width,height);
    struct native_resource *history=target_get(&device.history_buffer);
    /* A cached backing for a previously used shape can contain old pixels.
       Explicit zero allocation for the resized image prevents exposing
       those bytes when growing again; no stretch or filtered copy occurs. */
    struct halo_metal_clear clear={0};
    clear.command.opcode=HALO_METAL_CLEAR;clear.color=history->ref;
    clear.planes=HALO_METAL_COLOR;clear.width=history->storage_width;clear.height=history->storage_height;
    packet_begin(sizeof(clear),NULL,0);command_append(&clear,sizeof(clear));packet_finish();
    history->last_rendered=rendered_serial_next();
    if ((uint64_t)previous->storage_height*history->description.height !=
        (uint64_t)history->storage_height*previous->description.height)
        native_fail("indexed back-buffer resize scale",HALO_METAL_UNSUPPORTED);
    struct halo_metal_render_dimensions dimensions=resource_dimensions(previous);
    struct halo_metal_render_edges edges={0,0,width<old_width ? width:old_width,
        height<old_height ? height:old_height};
    struct halo_metal_render_rectangle rectangle;
    require_status("indexed back-buffer resize rectangle",halo_metal_render_scale_rectangle(&dimensions,&edges,&rectangle));
    backbuffer_color_copy(previous,history,rectangle.width,rectangle.height);
}
#endif
static BOOL rendered_mip_copy_matches(const struct halo_metal_mip_composite_copy *cached,
    const struct halo_metal_mip_composite_copy *copy) {
    /* Compare fields, not padding. A resource/address reuse or one-level
       rewrite must copy again even when every other level is unchanged. */
    return cached->source.id==copy->source.id && cached->source.generation==copy->source.generation &&
        cached->source_content_version==copy->source_content_version &&
        cached->source_physical_data==copy->source_physical_data &&
        cached->source_mip==copy->source_mip && cached->source_slice==copy->source_slice &&
        cached->destination_mip==copy->destination_mip && cached->destination_slice==copy->destination_slice &&
        cached->width==copy->width && cached->height==copy->height;
}
static struct native_resource *rendered_mip_composite(DWORD data,
    const struct xgpu_texture_description *original) {
    struct halo_metal_mip_composite_request request={0};
    struct halo_metal_mip_composite_plan plan;
    unsigned long target_count=0;
    if (!(transport.reply.capabilities&HALO_METAL_CAP_COPY_SUBRESOURCE))
        native_fail("rendered mip copy capability",HALO_METAL_UNSUPPORTED);
    if (original->levels>HALO_METAL_MIP_COMPOSITE_MAX_LEVELS)
        native_fail("rendered mip level count",HALO_METAL_UNSUPPORTED);
    request.key=(struct halo_metal_mip_composite_key){data,original->width,original->height,original->levels};
    request.storage_format=HALO_METAL_BGRA8;
    for (unsigned long mip=0;mip<=original->levels;mip++)
        request.level_offsets[mip]=xgpu_texture_level_offset(original,mip);
    for (struct native_resource *entry=resources;entry;entry=entry->next)
        if ((entry->usage&HALO_METAL_RENDER_TARGET) && entry->format!=HALO_METAL_DEPTH32_STENCIL8) target_count++;
    if (target_count>HALO_METAL_MIP_COMPOSITE_MAX_TARGETS)
        native_fail("rendered mip target count",HALO_METAL_UNSUPPORTED);
    struct halo_metal_mip_composite_target *targets=calloc(target_count ? target_count:1,sizeof(*targets));
    if (!targets) native_fail("allocate rendered mip plan",HALO_METAL_MEMORY);
    unsigned long index=0;
    for (struct native_resource *entry=resources;entry;entry=entry->next) {
        if (!(entry->usage&HALO_METAL_RENDER_TARGET) || entry->format==HALO_METAL_DEPTH32_STENCIL8) continue;
        const struct xgpu_texture_description *d=&entry->description;
        targets[index++]=(struct halo_metal_mip_composite_target){entry->ref,entry->data,
            d->width,d->height,entry->storage_width,entry->storage_height,d->levels,HALO_METAL_TEXTURE_2D,entry->format,entry->usage,
            1,entry->last_rendered,entry->last_rendered};
    }
    int status=halo_metal_mip_composite_plan(&request,targets,target_count,&plan);
    free(targets);
    require_status("plan original rendered mip levels",status);
    struct native_resource *composite;
    for (composite=resources;composite;composite=composite->next)
        if (composite->mip_composite && composite->data==data &&
            composite->description.width==original->width && composite->description.height==original->height &&
            composite->description.levels==original->levels) break;
    if (!composite) {
        composite=calloc(1,sizeof(*composite));
        if (!composite) native_fail("allocate rendered mip composite",HALO_METAL_MEMORY);
        composite->mip_copy_cache=calloc(plan.copy_count,sizeof(*composite->mip_copy_cache));
        if (!composite->mip_copy_cache) native_fail("allocate rendered mip copy cache",HALO_METAL_MEMORY);
        composite->data=data;composite->description=*original;
        composite->format=plan.storage_format;composite->usage=HALO_METAL_SHADER_READ;composite->mip_composite=TRUE;
        resource_create(composite);resource_publish(composite);
        platform_log("Native rendered mip composite: Data %08lx %lux%lu levels %lu, independent original targets",
            data,original->width,original->height,original->levels);
    }
    /* The complete planner above still validates every original level, even
       on a cache hit. Copy changed authored levels in their original order
       before attachment binding; unchanged destination texels remain exact.
       A full-chain mip generator would overwrite independently rendered data. */
    for (uint32_t mip=0;mip<plan.copy_count;mip++) {
        const struct halo_metal_mip_composite_copy *copy=&plan.copies[mip];
        if (rendered_mip_copy_matches(&composite->mip_copy_cache[mip],copy)) continue;
        struct halo_metal_copy_subresource command={0};
        command.command.opcode=HALO_METAL_COPY_SUBRESOURCE;
        command.source=copy->source;command.destination=composite->ref;
        command.source_mip=copy->source_mip;command.source_slice=copy->source_slice;
        command.destination_mip=copy->destination_mip;command.destination_slice=copy->destination_slice;
        command.width=copy->width;command.height=copy->height;command.planes=HALO_METAL_COLOR;
        packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    }
    /* Publish freshness only after the ordered copy commands have all been
       appended. Synchronous packet failures terminate rendering; subsequent
       draws/clears change source versions before another binding is resolved. */
    memcpy(composite->mip_copy_cache,plan.copies,plan.copy_count*sizeof(*composite->mip_copy_cache));
    return composite;
}
static unsigned long palette_hash(const D3DCOLOR *palette) {
    unsigned long hash=2166136261UL;
    if (palette) for (unsigned i=0;i<256;i++) hash=(hash^palette[i])*16777619UL;
    return palette ? hash : 0;
}
/* One immutable native GPU texture per embedded HUD sheet, independent of the
 * map's source address. Each source still has to pass its tag/size/CRC check.
 * Only an enabled, matching HUD bitmap reaches this allocation/upload path. */
static struct native_resource *hud_texture_get(long asset) {
    struct native_resource *entry;
    for (entry=resources;entry;entry=entry->next)
        if (entry->description.hires && !entry->text_atlas && entry->hud_asset==asset) return entry;
    unsigned long width=0,height=0;
    unsigned char *pixels=hud_hires_override_pixels(asset,&width,&height);
    if (!pixels) return NULL;
    if (!width || !height || width>8192 || height>8192 ||
        (width&(width-1)) || (height&(height-1))) {
        platform_log("Native high-res HUD: invalid dimensions %lux%lu for asset %ld; original bitmap used",
            width,height,asset);
        free(pixels);return NULL;
    }
    uint32_t bytes=(uint32_t)(width*height*4),upload_end,payloads[]={bytes};
    require_status("largest high-res HUD mip reservation",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,sizeof(struct halo_metal_upload_ex),
        payloads,1,&upload_end));
    entry=calloc(1,sizeof(*entry));
    if (!entry) native_fail("allocate high-res HUD cache",HALO_METAL_MEMORY);
    entry->hud_asset=asset;
    entry->description=(struct xgpu_texture_description){.format=D3DFMT_A8R8G8B8,
        .width=width,.height=height,.depth=1,.levels=1,.hires=TRUE,
        .hires_coverage=hud_hires_override_coverage(asset),
        .hires_point_threshold=hud_hires_override_point_threshold(asset)};
    for (unsigned long largest=width>height ? width:height;largest>1;largest>>=1)
        entry->description.levels++;
    entry->format=HALO_METAL_RGBA8;entry->usage=HALO_METAL_SHADER_READ;
    resource_create(entry);
    uint32_t mip_width=(uint32_t)width,mip_height=(uint32_t)height;
    for (uint32_t mip=0;mip<entry->description.levels;mip++) {
        struct halo_metal_upload_ex upload={0};
        upload.command.opcode=HALO_METAL_UPLOAD_EX;upload.resource=entry->ref;
        upload.mip=mip;upload.width=mip_width;upload.height=mip_height;upload.depth=1;
        upload.bytes_per_row=mip_width*4;upload.bytes_per_image=mip_width*mip_height*4;
        upload.data_size=upload.bytes_per_image;upload.plane=HALO_METAL_COLOR;
        uint32_t mip_payloads[]={upload.data_size};
        packet_begin(sizeof(upload),mip_payloads,1);uint32_t offset=command_append(&upload,sizeof(upload));
        payload_append(offset,offsetof(struct halo_metal_upload_ex,data_offset),pixels,upload.data_size);
        packet_finish();
        if (mip+1<entry->description.levels) {
            metal_hud_mip_reduce_rgba(pixels,mip_width,mip_height);
            mip_width=mip_width>1 ? mip_width/2:1;mip_height=mip_height>1 ? mip_height/2:1;
        }
    }
    free(pixels);resource_publish(entry);
    platform_log("Native high-res HUD: asset %ld uploaded %lux%lu, %lu mip levels",
        asset,width,height,entry->description.levels);
    return entry;
}
/* The registered text placeholder carries the game's logical atlas UVs, not
 * rasterized glyph pixels. Resolve it before any authored-memory access. Each
 * revision becomes immutable packet bytes before the draw that samples it;
 * ordered upload/draw commands also preserve earlier draws if the CPU atlas
 * is reset or gains more glyphs later in this same frame. */
/* A menu bitmap's D3D texture is a 4x4 placeholder. Its picture is the PNG
 * registered for that texture, drawn with the widget's 0..1 coordinates.
 * The picture is cached by that texture's physical address. Loading a map
 * frees the main menu's textures and the pause menu's are built in the
 * same addresses, so a cache hit would draw the main menu's picture (the
 * Mods title, over the hosted pause box) until these are dropped. */
static void menu_art_release(struct native_resource *entry) {
    struct native_resource **link=&resources;
    resource_unindex(entry);
    if (device.ready) resource_delete(entry);
    while (*link && *link!=entry) link=&(*link)->next;
    if (*link) *link=entry->next;
    free(entry->mip_copy_cache);free(entry);
}
void halo_metal_menu_art_drop(unsigned long data) {
    struct native_resource *entry=resource_buckets[native_resource_bucket(data)];
    while (entry) {
        struct native_resource *next=entry->bucket_next;
        if (entry->menu_art && entry->data==data) menu_art_release(entry);
        entry=next;
    }
}
void halo_metal_menu_art_forget(void) {
    struct native_resource *entry=resources;
    while (entry) {
        struct native_resource *next=entry->next;
        if (entry->menu_art) menu_art_release(entry);
        entry=next;
    }
}
static struct native_resource *menu_texture_get(unsigned long data) {
    struct native_resource *entry;
    unsigned char *pixels=NULL;
    unsigned long width=0,height=0;
    for (entry=resource_buckets[native_resource_bucket(data)];entry;entry=entry->bucket_next)
        if (entry->menu_art && entry->data==data) return entry;
    if (!menu_art_pixels(data,&pixels,&width,&height)) return NULL;
    uint32_t bytes=(uint32_t)(width*height*4),upload_end,payloads[]={bytes};
    require_status("menu art reservation",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,sizeof(struct halo_metal_upload_ex),
        payloads,1,&upload_end));
    entry=calloc(1,sizeof(*entry));
    if (!entry) native_fail("allocate menu art cache",HALO_METAL_MEMORY);
    entry->data=data;entry->menu_art=TRUE;entry->hud_asset=-1;
    entry->description=(struct xgpu_texture_description){.format=D3DFMT_A8R8G8B8,
        .width=width,.height=height,.depth=1,.levels=1,.hires=TRUE};
    entry->format=HALO_METAL_RGBA8;entry->usage=HALO_METAL_SHADER_READ;
    resource_create(entry);resource_publish(entry);
    struct halo_metal_upload_ex upload={0};
    upload.command.opcode=HALO_METAL_UPLOAD_EX;upload.resource=entry->ref;
    upload.width=(uint32_t)width;upload.height=(uint32_t)height;upload.depth=1;
    upload.bytes_per_row=upload.width*4;upload.bytes_per_image=bytes;
    upload.data_size=bytes;upload.plane=HALO_METAL_COLOR;
    packet_begin(sizeof(upload),payloads,1);uint32_t offset=command_append(&upload,sizeof(upload));
    payload_append(offset,offsetof(struct halo_metal_upload_ex,data_offset),pixels,bytes);
    packet_finish();
    platform_log("Native menu art: %lux%lu uploaded",width,height);
    return entry;
}
static struct native_resource *text_texture_get(unsigned long data) {
    struct native_resource *entry=text_atlas_resource;
    struct text_hires_atlas_pixels atlas;
    if (!text_hires_atlas_pixels_since(data,entry ? entry->text_revision:0,&atlas)) return NULL;
    if (!atlas.rgba || !atlas.width || !atlas.height || !atlas.revision ||
        atlas.width>8192 || atlas.height>8192 || atlas.dirty_top>atlas.height ||
        atlas.dirty_bottom>atlas.height || (atlas.dirty_bottom && atlas.dirty_top>=atlas.dirty_bottom))
        native_fail("high-res text atlas metadata",HALO_METAL_INVALID);
    BOOL allocate=!entry || entry->description.width!=atlas.width || entry->description.height!=atlas.height;
    if (!allocate && entry->text_revision==atlas.revision) return entry;
    /* The first upload initializes the whole GPU image. Later uploads copy
     * only rows changed since this resource's revision, without consuming a
     * second backend's dirty state. Atlas resets stamp every row as changed. */
    uint32_t top=allocate ? 0:(uint32_t)atlas.dirty_top;
    uint32_t bottom=allocate ? (uint32_t)atlas.height:(uint32_t)atlas.dirty_bottom;
    uint32_t size=bottom>top ? (uint32_t)(atlas.width*(bottom-top)*4):0;
    uint32_t end,payloads[]={size};
    if (size) require_status("high-res text atlas reservation",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,sizeof(struct halo_metal_upload_ex),payloads,1,&end));
    if (allocate) {
        entry=calloc(1,sizeof(*entry));
        if (!entry) native_fail("allocate high-res text cache",HALO_METAL_MEMORY);
        entry->text_atlas=TRUE;entry->hud_asset=-1;
        entry->description=(struct xgpu_texture_description){.format=D3DFMT_A8R8G8B8,
            .width=atlas.width,.height=atlas.height,.depth=1,.levels=1,.hires=TRUE};
        /* Coverage is already the RGBA alpha channel, as with stock glyphs.
         * No HUD meter coverage transform or texture-channel swizzle applies. */
        entry->format=HALO_METAL_RGBA8;entry->usage=HALO_METAL_SHADER_READ;
        resource_create(entry);resource_publish(entry);
        text_atlas_resource=entry;
    }
    if (size) {
        struct halo_metal_upload_ex upload={0};
        upload.command.opcode=HALO_METAL_UPLOAD_EX;upload.resource=entry->ref;
        upload.y=top;upload.width=(uint32_t)atlas.width;upload.height=bottom-top;upload.depth=1;
        upload.bytes_per_row=upload.width*4;upload.bytes_per_image=size;
        upload.data_size=size;upload.plane=HALO_METAL_COLOR;
        packet_begin(sizeof(upload),payloads,1);uint32_t offset=command_append(&upload,sizeof(upload));
        payload_append(offset,offsetof(struct halo_metal_upload_ex,data_offset),
            atlas.rgba+(size_t)top*upload.bytes_per_row,size);
        packet_finish();
    }
    entry->text_revision=atlas.revision;
    if (allocate) platform_log("Native high-res text: atlas %lux%lu uploaded, %.3f pixels per logical unit",
        atlas.width,atlas.height,halo_screen_pixel_scale());
    return entry;
}
static struct native_resource *texture_get(D3DBaseTexture *texture,D3DPalette *palette,unsigned stage) {
    struct native_resource *text=text_texture_get(texture->Data);
    if (text) return text;
    struct native_resource *menu=menu_texture_get(texture->Data);
    if (menu) return menu;
    struct native_resource *entry=rendered_alias(texture->Data);
    /* Device-owned indexed history is GPU storage even before its first
       Present. Resolve/initialize it without uploading undefined CPU bytes
       or taking a new snapshot of the current drawing surface. */
    if (texture->Data==device.history_buffer.Data)
        entry=history_sample_override ? history_sample_override:target_get(&device.history_buffer);
    struct xgpu_texture_description original;
    xgpu_texture_describe(texture->Format,texture->Size,&original);
    if (entry) {
        if (!original.linear && !original.cube_map && original.depth==1 && original.levels>1 &&
            original.width==entry->description.width && original.height==entry->description.height)
            return rendered_mip_composite(texture->Data,&original);
        if (original.depth!=1) native_fail("rendered volume alias",HALO_METAL_UNSUPPORTED);
        return entry;
    }
    for (entry=resource_buckets[native_resource_bucket(texture->Data)];entry;entry=entry->bucket_next)
        if (!(entry->usage&HALO_METAL_RENDER_TARGET) && !entry->mip_composite && !entry->description.hires && entry->data==texture->Data &&
            entry->format_word==texture->Format && entry->size_word==texture->Size) break;
    struct xgpu_texture_mip_layout layout;
    struct xgpu_texture_volume_mip_layout volume_layout;
    BOOL volume=((texture->Format&D3DFORMAT_DIMENSION_MASK)>>D3DFORMAT_DIMENSION_SHIFT)==3;
    int layout_status;
    if (volume) {
        if (!(transport.reply.capabilities&HALO_METAL_CAP_VOLUME))
            native_fail("authored volume capability",HALO_METAL_UNSUPPORTED);
        layout_status=xgpu_texture_volume_mip_layout(texture->Format,texture->Size,0,0,
            _xgpu_texture_copy_bgra,&volume_layout);
        layout=volume_layout.mip;
    } else layout_status=xgpu_texture_mip_layout(texture->Format,texture->Size,0,0,
        _xgpu_texture_copy_bgra,&layout);
    if (layout_status) {
        platform_log("Native unsupported texture: stage %u Data %08lx Format %08lx Size %08lx; format %lu dimensions %lux%lux%lu levels %lu cube %lu linear %lu; PS modes %08lx",
            stage,texture->Data,texture->Format,texture->Size,original.format,
            original.width,original.height,original.depth,original.levels,
            (unsigned long)original.cube_map,(unsigned long)original.linear,
            D3D__RenderState[D3DRS_PSTEXTUREMODES]);
        const DWORD *state=D3D__TextureState[stage];
        platform_log("Native unsupported texture sampler: address %lu/%lu/%lu border %08lx filters %lu/%lu/%lu aniso %lu",
            state[D3DTSS_ADDRESSU],state[D3DTSS_ADDRESSV],state[D3DTSS_ADDRESSW],state[D3DTSS_BORDERCOLOR],
            state[D3DTSS_MINFILTER],state[D3DTSS_MAGFILTER],state[D3DTSS_MIPFILTER],state[D3DTSS_MAXANISOTROPY]);
        /* Preserve the actual failed volume bytes for diagnosis using the
           existing texture-dump setting. This is never a GPU fallback. */
        const char *dump_directory=config_string("debug.texture_dump_directory");
        if (*dump_directory && original.depth>1 && original.width<=512 && original.height<=512 &&
            original.depth<=512 && original.levels<=10 && !original.cube_map && !original.linear && !original.compressed) {
            unsigned long dump_bytes=xgpu_texture_face_size(&original);
            physical_span(texture->Data,dump_bytes);
            char dump_path[1024];
            int length=snprintf(dump_path,sizeof(dump_path),"%s/native-volume-%08lx-%08lx.bin",
                dump_directory,texture->Data,texture->Format);
            if (length>0 && (size_t)length<sizeof(dump_path)) {
                FILE *dump=fopen(dump_path,"wb");
                if (dump) {
                    size_t written=fwrite(PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data),1,dump_bytes,dump);
                    int close_status=fclose(dump);
                    platform_log("Native failed volume dump: %s bytes %lu/%lu close %d",dump_path,
                        (unsigned long)written,dump_bytes,close_status);
                }
            }
        }
        native_fail("original authored mip layout",layout_status);
    }
    uint32_t upload_end,largest_payload[]={layout.output_size};
    require_status("largest original mip reservation",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,sizeof(struct halo_metal_upload_ex),
        largest_payload,1,&upload_end));
    physical_span(texture->Data,layout.source_total_size);
    const D3DCOLOR *colors=NULL;
    if (original.format==D3DFMT_P8 && palette && palette->Data) {
        physical_span(palette->Data,256*sizeof(D3DCOLOR));
        colors=PLATFORM_PHYSICAL_TO_VIRTUAL(palette->Data);
    }
    unsigned long hash=palette_hash(colors);
    unsigned long watched_address=(unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data);
    unsigned long generation=memory_watch_generation(watched_address,layout.source_total_size);
    BOOL fresh=entry==NULL;
    if (!entry) {
        entry=calloc(1,sizeof(*entry));
        if (!entry) native_fail("allocate texture cache",HALO_METAL_MEMORY);
        entry->data=texture->Data;entry->format_word=texture->Format;entry->size_word=texture->Size;
        entry->description=original;
        entry->volume=volume;
        entry->format=texture_format(layout.format);entry->usage=HALO_METAL_SHADER_READ;
        resource_create(entry);resource_publish(entry);
    }
    /* Protect before CRC validation as well as original uploads, so a later
     * source write invalidates this association on the next draw. A changed
     * or localized bitmap falls back to its authored pixels. */
    if (!entry->hud_checked || generation!=entry->hud_generation) {
        memory_watch_protect(watched_address,layout.source_total_size);
        generation=memory_watch_generation(watched_address,layout.source_total_size);
        entry->hud_override=NULL;
        if (!volume && !original.cube_map && original.format!=D3DFMT_P8) {
            unsigned long level0_size=original.levels>1 ?
                xgpu_texture_level_offset(&original,1):xgpu_texture_face_size(&original);
            long asset=hud_hires_override_find(watched_address,original.width,original.height,level0_size);
            if (asset>=0) entry->hud_override=hud_texture_get(asset);
        }
        entry->hud_generation=generation;entry->hud_checked=TRUE;
    }
    if (entry->hud_override) return entry->hud_override;
    if (!fresh && generation==entry->generation && hash==entry->palette_hash) return entry;
    /* Protect before copying; a racing guest write increments the generation,
       forcing another upload at the next use rather than being lost. */
    memory_watch_protect(watched_address,layout.source_total_size);
    generation=memory_watch_generation(watched_address,layout.source_total_size);
    for (unsigned long face=0;face<layout.faces;face++) for (unsigned long mip=0;mip<original.levels;mip++) {
        if (volume) {
            require_status("authored volume mip layout",xgpu_texture_volume_mip_layout(texture->Format,texture->Size,
                face,mip,_xgpu_texture_copy_bgra,&volume_layout));
            layout=volume_layout.mip;
        } else require_status("authored mip layout",xgpu_texture_mip_layout(texture->Format,texture->Size,face,mip,
            _xgpu_texture_copy_bgra,&layout));
        void *bytes=malloc(layout.output_size);
        if (!bytes) native_fail("allocate authored mip copy",HALO_METAL_MEMORY);
        if (volume) require_status("copy original authored volume",xgpu_texture_volume_mip_copy(texture->Format,texture->Size,
            PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data),layout.source_total_size,colors,colors ? 256 : 0,
            face,mip,_xgpu_texture_copy_bgra,bytes,layout.output_size,&volume_layout));
        else require_status("copy original authored texture",xgpu_texture_mip_copy(texture->Format,texture->Size,
            PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data),layout.source_total_size,colors,colors ? 256 : 0,
            face,mip,_xgpu_texture_copy_bgra,bytes,layout.output_size,&layout));
        struct halo_metal_upload_ex upload={0};
        upload.command.opcode=HALO_METAL_UPLOAD_EX;upload.resource=entry->ref;
        upload.mip=mip;upload.slice=face;upload.width=layout.width;upload.height=layout.height;
        upload.depth=volume ? volume_layout.depth:1;upload.bytes_per_row=layout.output_row_pitch;
        upload.bytes_per_image=volume ? volume_layout.output_image_pitch:layout.output_size;
        upload.data_size=layout.output_size;upload.plane=HALO_METAL_COLOR;
        uint32_t payloads[]={layout.output_size};
        packet_begin(sizeof(upload),payloads,1);uint32_t offset=command_append(&upload,sizeof(upload));
        payload_append(offset,offsetof(struct halo_metal_upload_ex,data_offset),bytes,layout.output_size);
        packet_finish();free(bytes);
    }
    entry->generation=generation;entry->palette_hash=hash;
    return entry;
}
static struct native_program *program_get(struct vertex_shader_object *vertex,uint32_t packed,
    const struct nv2a_pixel_shader_key *key, uint32_t alpha_border_mask,uint32_t volume_border_mask,uint32_t depth_contract,
    BOOL compact_vertices) {
    struct native_program *entry;
    unsigned lookup=program_lookup_hash(vertex,packed,key,alpha_border_mask,volume_border_mask,
        depth_contract,(uint32_t)compact_vertices);
    for (entry=program_buckets[lookup&(NATIVE_PROGRAM_BUCKETS-1)];entry;entry=entry->bucket_next)
        if (entry->lookup_hash==lookup && entry->vertex==vertex && entry->packed_mask==packed &&
            entry->alpha_border_mask==alpha_border_mask && entry->depth_contract==depth_contract &&
            entry->volume_border_mask==volume_border_mask && entry->compact_vertices==(uint32_t)compact_vertices &&
            !memcmp(&entry->key,key,sizeof(*key))) return entry;
    const BOOL diagnostics=config_boolean("debug.gpu_stats");
    const uint64_t translation_started=diagnostics ? monotonic_ns():0;
    char *vs=vertex==&fixed_function_vertex ? metal_fixed_function_vertex_to_msl():
        compact_vertices ? nv2a_vertex_shader_to_msl_compact(vertex->instructions,vertex->instruction_count,packed):
        nv2a_vertex_shader_to_msl(vertex->instructions,vertex->instruction_count,packed);
    struct nv2a_pixel_shader_msl_options options={volume_border_mask ? 3:2,
        depth_contract==METAL_DRAW_DEPTH_RAW_D24 ? XGPU_MSL_DEPTH_RAW_D24:XGPU_MSL_DEPTH_NONE,alpha_border_mask,volume_border_mask};
    char *ps=(alpha_border_mask || volume_border_mask || depth_contract) ?
        nv2a_pixel_shader_to_msl_with_options(key,&options):nv2a_pixel_shader_to_msl(key);
    if (!vs || !ps) {
        unsigned original_float_z=original_float_z_value();
        DWORD active_depth_format=device.depth_stencil ? device.depth_stencil->Format:0;
        DWORD active_depth_size=device.depth_stencil ? device.depth_stencil->Size:0;
        platform_log("Native unsupported NV2A program: vertex %u pixel %u instructions %lu packed %08x alpha-border %u; texture-modes %08lx samplers %u/%u/%u/%u combiner-count %08lx",
            vs!=NULL,ps!=NULL,vertex->instruction_count,packed,alpha_border_mask,key->texture_modes,
            key->sampler_type[0],key->sampler_type[1],key->sampler_type[2],key->sampler_type[3],
            key->combiner_state[D3DRS_PSCOMBINERCOUNT]);
        platform_log("Native failed program depth: original float-Z %u presentation AutoDepthStencilFormat %lu active Format %08lx Size %08lx; viewport %.9g/%.9g scale-Z %.9g offset-Z %.9g zsprite-c33 %.9g/%.9g/%.9g/%.9g c34 %.9g/%.9g/%.9g/%.9g",
            original_float_z,(unsigned long)device.presentation.AutoDepthStencilFormat,active_depth_format,active_depth_size,
            device.viewport.MinZ,device.viewport.MaxZ,device.viewport_scale[2],device.viewport_offset[2],
            device.constants[33][0],device.constants[33][1],device.constants[33][2],device.constants[33][3],
            device.constants[34][0],device.constants[34][1],device.constants[34][2],device.constants[34][3]);
        /* Failure-only original input preservation for the existing diagnostic
           setting. This does not supply replacement shader results. */
        const char *directory=config_string("debug.texture_dump_directory");
        if (*directory) {
            char path[1024];
            const DWORD metadata[]={1,original_float_z,(DWORD)device.presentation.AutoDepthStencilFormat,
                active_depth_format,active_depth_size,device.depth_stencil ? device.depth_stencil->Data:0};
            const void *data[6]={key,vertex->instructions,metadata,&device.viewport,device.viewport_scale,device.constants};
            const size_t sizes[6]={sizeof(*key),vertex->instruction_count*4*sizeof(DWORD),sizeof(metadata),
                sizeof(device.viewport),sizeof(device.viewport_scale)+sizeof(device.viewport_offset),sizeof(device.constants)};
            const char *names[6]={"pixel-key","vertex-words","original-depth-metadata","viewport","viewport-scale-offset","vertex-constants"};
            for (unsigned i=0;i<6;i++) {
                int length=snprintf(path,sizeof(path),"%s/native-program-frame%lu-%s.bin",directory,device.frame,names[i]);
                if (length<=0 || (size_t)length>=sizeof(path)) continue;
                FILE *file=fopen(path,"wb");
                if (!file) continue;
                size_t written=fwrite(data[i],1,sizes[i],file);
                int closed=fclose(file);
                platform_log("Native failed program dump: %s bytes %lu/%lu close %d",path,
                    (unsigned long)written,(unsigned long)sizes[i],closed);
            }
        }
        free(vs);free(ps);native_fail("translate original NV2A program",HALO_METAL_UNSUPPORTED);
    }
    entry=calloc(1,sizeof(*entry));
    if (!entry) native_fail("allocate native program",HALO_METAL_MEMORY);
    entry->vertex=vertex;entry->packed_mask=packed;entry->key=*key;entry->alpha_border_mask=alpha_border_mask;
    entry->depth_contract=depth_contract;
    entry->volume_border_mask=volume_border_mask;
    entry->compact_vertices=compact_vertices;
    entry->lookup_hash=lookup;
    entry->ref.id=next_program_id++;entry->ref.generation=1;
    struct halo_metal_program command={0};
    command.command.opcode=HALO_METAL_CREATE_PROGRAM;command.resource=entry->ref;
    command.vertex_compiler_contract=vertex==&fixed_function_vertex ? 0:1;
    /* Depth replacement and other finite guards require the safe
       floating-point contract. Keep it explicit in the program cache path. */
    command.fragment_compiler_contract=(depth_contract || strstr(ps,"isfinite")) ? 0:1;
    if (depth_contract && config_boolean("debug.gpu_stats"))
        platform_log("Native original D24 depth program %u: texture-modes %08lx alpha-border %u float-Z %u requested-format %lu target-format %08lx viewport %.9g/%.9g scale-Z %.9g offset-Z %.9g fragment-compiler %u",
            entry->ref.id,key->texture_modes,alpha_border_mask,original_float_z_value(),
            (unsigned long)device.presentation.AutoDepthStencilFormat,device.depth_stencil->Format,
            device.viewport.MinZ,device.viewport.MaxZ,device.viewport_scale[2],device.viewport_offset[2],
            command.fragment_compiler_contract);
    if (volume_border_mask && config_boolean("debug.gpu_stats"))
        platform_log("Native original volume border program %u: texture-modes %08lx volume-mask %u alpha-mask %u same-texture black/white sampler footprints",
            entry->ref.id,key->texture_modes,volume_border_mask,alpha_border_mask);
    command.vertex_source_size=strlen(vs);command.fragment_source_size=strlen(ps);
    const uint64_t translation_ns=diagnostics ? monotonic_ns()-translation_started:0;
    uint32_t payloads[]={command.vertex_source_size,command.fragment_source_size};
    packet_begin(sizeof(command),payloads,2);uint32_t offset=command_append(&command,sizeof(command));
    payload_append(offset,offsetof(struct halo_metal_program,vertex_source_offset),vs,command.vertex_source_size);
    payload_append(offset,offsetof(struct halo_metal_program,fragment_source_offset),ps,command.fragment_source_size);
    if (diagnostics)
        platform_log("Native program: frame %lu, sequence %llu, command-index %u, program %u, vertex-object %lu, vertex-handle %08lx, declaration-object %lu, loaded-slot %lu, instructions %lu, packed %08x, texture-modes %08lx, samplers %u/%u/%u/%u, alpha-border %u, volume-border %u, depth-contract %u, compilers %u/%u, sources %u/%u bytes, translation %llu ns",
            device.frame,(unsigned long long)transport.sequence,transport.command_count-1,entry->ref.id,vertex->id,
            vertex==&fixed_function_vertex ? 0:(unsigned long)(uintptr_t)vertex,
            device.vertex_shader ? device.vertex_shader->id:0,device.program_address,
            vertex->instruction_count,packed,key->texture_modes,
            key->sampler_type[0],key->sampler_type[1],key->sampler_type[2],key->sampler_type[3],
            alpha_border_mask,volume_border_mask,depth_contract,
            command.vertex_compiler_contract,command.fragment_compiler_contract,
            command.vertex_source_size,command.fragment_source_size,
            (unsigned long long)translation_ns);
    packet_finish();free(vs);free(ps);
    entry->next=programs;programs=entry;
    entry->bucket_next=program_buckets[lookup&(NATIVE_PROGRAM_BUCKETS-1)];
    program_buckets[lookup&(NATIVE_PROGRAM_BUCKETS-1)]=entry;
    return entry;
}
/* display.anisotropic_filtering, clamped to the range Metal's sampler path
validates (1..16). 1 is the Xbox's own filtering. Read again when a setting
changes, so the Graphics screen applies without a restart. */
static uint32_t metal_anisotropic_level(void) {
    static long level=-1;
    static unsigned long read_at;
    if (level<0 || read_at!=config_changes()) {
        long chosen=config_integer("display.anisotropic_filtering");
        read_at=config_changes();
        if (chosen>16) chosen=16;
        if (chosen<1) chosen=1;
        if (chosen!=level)
            platform_log("anisotropic filtering: %ldx",chosen);
        level=chosen;
    }
    return (uint32_t)level;
}

static void draw_original(D3DPRIMITIVETYPE type,UINT first,UINT count,const WORD *indices,BOOL immediate) {
    if (!count || !device.ready) return;
    struct vertex_shader_object *vertex=device.fixed_function_selected ? &fixed_function_vertex:current_program();
    /* Match prepare_draw's pre-program startup path. A draw with no selected
       shader is rejected by the original frontend before any GPU operation. */
    if (!device.fixed_function_selected && (!vertex || !vertex->instructions || !device.vertex_shader)) return;
    if (device.fixed_function_selected && !immediate)
        native_fail("fixed-function stream/FVF draw is unsupported",HALO_METAL_UNSUPPORTED);
    uint64_t profile_started=draw_diagnostics ? monotonic_ns():0,profile_submitted=submit_wall_ns;
    struct metal_vertex_index_plan plan;
    require_status("original primitive plan",metal_vertex_index_plan(type,indices,indices ? count*sizeof(WORD):0,
        count,first,indices ? device.base_vertex_index:0,&plan));
    if (plan.source_count>(HALO_METAL_MAX_PACKET-8192)/256 || !plan.index_count)
        native_fail("expanded original stream size",HALO_METAL_MEMORY);
    size_t expanded_bytes=(size_t)plan.source_count*256;
    size_t vertex_bytes=expanded_bytes,index_bytes=(size_t)plan.index_count*4;
    struct metal_vertex_stream streams[16]={0};
    BOOL compact_vertices=FALSE;
    if (!immediate) {
        for (unsigned i=0;i<16;i++) if (device.streams[i].data) {
            physical_span(device.streams[i].data,1);
            streams[i].pointer=PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[i].data);
            streams[i].byte_count=PLATFORM_CONTIGUOUS_SIZE-device.streams[i].data;
            streams[i].stride=device.streams[i].stride;
        }
        if (gpu_vertex_decode) {
            size_t compact_bytes = 0;
            /* A declaration the GPU path cannot describe stays on the CPU
               fetch. Small draws stay there too, when the compact header
               costs more than the expanded registers. */
            if (!metal_vertex_compact_size(&device.vertex_shader->declaration,streams,
                plan.source_first,plan.source_count,&compact_bytes) && compact_bytes<expanded_bytes) {
                vertex_bytes=compact_bytes;compact_vertices=TRUE;
            }
        }
    }
    /* Reject a command that cannot fit before resource/program preparation
       can submit any earlier batch. Both draw layouts begin payloads at448. */
    uint32_t draw_end,draw_payloads[]={vertex_bytes,index_bytes,
        sizeof(struct metal_draw_vertex_uniforms),sizeof(struct metal_draw_pixel_uniforms)};
    require_status("complete original draw reservation",halo_metal_packet_room(
        sizeof(struct halo_metal_packet),transport.capacity,compact_vertices ? sizeof(struct halo_metal_draw_compact):
        sizeof(struct halo_metal_draw_alpha_border),
        draw_payloads,4,&draw_end));
    void *vertices=malloc(vertex_bytes);void *rebased=malloc(index_bytes);
    if (!vertices || !rebased) native_fail("original stream allocation",HALO_METAL_MEMORY);
    require_status("original primitive indices",metal_vertex_indices(type,indices,indices ? count*sizeof(WORD):0,
        count,first,indices ? device.base_vertex_index:0,rebased,index_bytes,&plan));
    uint32_t packed=immediate ? 0 : device.vertex_shader->declaration.packed_mask;
    if (immediate) memcpy(vertices,device.immediate_vertices,vertex_bytes);
    else if (compact_vertices)
        require_status("snapshot original compact streams",metal_vertex_compact_pack(
            &device.vertex_shader->declaration,streams,device.attributes,plan.source_first,plan.source_count,
            vertices,vertex_bytes));
    else {
        require_status("fetch original registers",metal_vertex_fetch(&device.vertex_shader->declaration,streams,
            device.attributes,plan.source_first,plan.source_count,vertices,vertex_bytes));
    }
    if (draw_diagnostics) native_draw_phase(&profile_started,&profile_submitted,&draw_profile.expansion_ns);
    struct metal_draw_state_input input={0};
    struct metal_draw_state_output output;
    struct metal_draw_state_error error={0};
    struct nv2a_pixel_shader_key key;
    struct native_resource *bound[4]={0};
    input.render_state=(const uint32_t *)D3D__RenderState;
    input.texture_state=(const uint32_t (*)[32])D3D__TextureState;input.constants=device.constants;
    input.viewport_scale=device.viewport_scale;input.viewport_offset=device.viewport_offset;
    input.viewport[0]=device.viewport.X;input.viewport[1]=device.viewport.Y;
    input.viewport[2]=device.viewport.Width;input.viewport[3]=device.viewport.Height;
    input.viewport[4]=device.viewport.MinZ;input.viewport[5]=device.viewport.MaxZ;
    input.sample_count=1;input.screen_offset=UI_OFFSET;
    input.native_black_border=(transport.reply.capabilities&HALO_METAL_CAP_BLACK_BORDER)!=0;
    input.native_alpha_border=(transport.reply.capabilities&HALO_METAL_CAP_ALPHA_BORDER)!=0;
    input.native_volume=(transport.reply.capabilities&HALO_METAL_CAP_VOLUME)!=0;
    input.native_volume_border=(transport.reply.capabilities&HALO_METAL_CAP_VOLUME_BORDER)!=0;
    input.anisotropic_level=metal_anisotropic_level();
    input.native_depth_contract=METAL_DRAW_DEPTH_RAW_D24;
    input.original_auto_depth_format=device.presentation.AutoDepthStencilFormat;
    input.depth_surface_format_word=device.depth_stencil ? device.depth_stencil->Format:0;
    input.floating_point_zbuffer=original_float_z_value();
    /* Match the original ordering: resolve render-to-texture aliases before
       binding the current attachments, because ripple composition may copy. */
    for (unsigned stage=0;stage<4;stage++) {
        unsigned mode=(D3D__RenderState[D3DRS_PSTEXTUREMODES]>>(stage*5))&31;
        D3DBaseTexture *texture=device.textures[stage];
        if (!texture || !texture->Data || mode==0 || mode==4 || mode==5 || mode==17) continue;
        bound[stage]=texture_get(texture,device.palettes[stage],stage);
        struct xgpu_texture_description original;
        xgpu_texture_describe(texture->Format,texture->Size,&original);
        struct xgpu_texture_description *d=&bound[stage]->description;
        /* The replacement is a different resource. The map sheet's channel
        order is read from the address the game registered, and only while
        that sheet itself is drawn. */
        uint32_t meter_channels=!d->hires && halo_custom_edition_meter_channels(
            PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data));
        input.textures[stage]=(struct metal_draw_texture){1,bound[stage]->volume ? _xgpu_sampler_3d:
            d->cube_map ? _xgpu_sampler_cube:_xgpu_sampler_2d,
            d->width,d->height,d->levels,original.linear,d->hires,d->hires_coverage,
            d->hires_point_threshold,meter_channels,
            bound[stage]->mip_composite || (bound[stage]->usage&HALO_METAL_RENDER_TARGET) ? 1u : 0u};
        input.texture_depth[stage]=bound[stage]->volume ? d->depth:0;
    }
    struct native_resource *color=target_get(device.render_target);
    struct native_resource *depth=device.depth_stencil ? target_get(device.depth_stencil):NULL;
    input.target_width=color->description.width;input.target_height=color->description.height;
    input.has_depth=depth!=NULL;
    if (draw_diagnostics) native_draw_phase(&profile_started,&profile_submitted,&draw_profile.textures_ns);
    int status=metal_draw_state_pack(&input,&key,&output,&error);
    if (status) {
        platform_log("Native original draw state: %s stage %u state %u value %u",error.message,error.stage,error.state,error.value);
        const uint32_t metadata[]={1,packed,(uint32_t)vertex->instruction_count,
            (uint32_t)type,(uint32_t)plan.source_count,(uint32_t)plan.index_count};
        native_failure_dump("draw-metadata",metadata,sizeof(metadata));
        native_failure_dump("vertex-words",vertex->instructions,vertex->instruction_count*4*sizeof(DWORD));
        native_failure_dump("vertex-constants",device.constants,sizeof(device.constants));
        native_failure_dump("viewport",&device.viewport,sizeof(device.viewport));
        native_failure_dump("viewport-scale-offset",device.viewport_scale,sizeof(device.viewport_scale)+sizeof(device.viewport_offset));
        native_failure_dump("render-state",D3D__RenderState,sizeof(D3D__RenderState));
        native_failure_dump("texture-state",D3D__TextureState,sizeof(D3D__TextureState));
        for (unsigned stage=0;stage<4;stage++) {
            const struct metal_draw_texture *t=&input.textures[stage];
            const DWORD *s=D3D__TextureState[stage];
            platform_log("Native stage %u: mode %lu present %u type %u size %ux%u levels %u linear %u; address %lu/%lu/%lu border %08lx filters %lu/%lu/%lu aniso %lu",
                stage,(D3D__RenderState[D3DRS_PSTEXTUREMODES]>>(stage*5))&31,t->present,t->sampler_type,
                t->width,t->height,t->levels,t->linear,s[D3DTSS_ADDRESSU],s[D3DTSS_ADDRESSV],s[D3DTSS_ADDRESSW],
                s[D3DTSS_BORDERCOLOR],s[D3DTSS_MINFILTER],s[D3DTSS_MAGFILTER],s[D3DTSS_MIPFILTER],s[D3DTSS_MAXANISOTROPY]);
        }
        native_fail("pack original draw state",status);
    }
    /* A linear Xbox bitmap uses pixel coordinates in its original dimensions.
     * The replacement's real dimensions validate its native mip chain, while
     * this normalization preserves the game's original atlas/layout UVs. */
    for (unsigned stage=0;stage<4;stage++) if (bound[stage] && bound[stage]->description.hires && input.textures[stage].linear) {
        struct xgpu_texture_description original;
        xgpu_texture_describe(device.textures[stage]->Format,device.textures[stage]->Size,&original);
        output.pixel.texture_scale[stage][0]=1.0f/(float)original.width;
        output.pixel.texture_scale[stage][1]=1.0f/(float)original.height;
    }
    /* Original constants and texture normalization stay logical. Only the
       raster viewport/scissor describe the larger Metal attachment. */
    native_raster_scale(color,&output.state);
    if (device.fixed_function_selected) {
        struct metal_fixed_function_input fixed={
            (const uint32_t *)D3D__RenderState,(const uint32_t (*)[32])D3D__TextureState,
            (const float *)&device.transforms[D3DTS_WORLD],(const float *)&device.transforms[D3DTS_VIEW],
            (const float *)&device.transforms[D3DTS_PROJECTION],&output.vertex,1};
        int fixed_status=metal_fixed_function_pack_unlit_immediate(&fixed,&output.vertex,&error);
        if (fixed_status) {
            platform_log("Native fixed-function draw state: %s stage %u state %u value %u",
                error.message,error.stage,error.state,error.value);
            native_fail("pack original fixed-function vertex state",fixed_status);
        }
    }
    if (draw_diagnostics) native_draw_phase(&profile_started,&profile_submitted,&draw_profile.state_ns);
    struct native_program *program=program_get(vertex,packed,&key,output.native_alpha_border_mask,output.native_volume_border_mask,
        output.depth_contract,compact_vertices);
    if (draw_diagnostics) native_draw_phase(&profile_started,&profile_submitted,&draw_profile.program_ns);
    struct halo_metal_draw draw={0};
    draw.command.opcode=HALO_METAL_DRAW;draw.program=program->ref;draw.color=color->ref;
    if (depth) draw.depth_stencil=depth->ref;
    for (unsigned stage=0;stage<4;stage++) if (bound[stage]) draw.textures[stage]=bound[stage]->ref;
    memcpy(draw.samplers,output.samplers,sizeof(draw.samplers));draw.state=output.state;
    draw.vertex_count=plan.source_count;draw.index_count=plan.index_count;
    draw.packed_mask=packed;draw.primitive=plan.wire_primitive;
    uint32_t payloads[]={vertex_bytes,index_bytes,sizeof(output.vertex),sizeof(output.pixel)};
    packet_begin(compact_vertices ? sizeof(struct halo_metal_draw_compact):
        output.native_volume_border_mask ? sizeof(struct halo_metal_draw_volume_border):
        output.native_alpha_border_mask ? sizeof(struct halo_metal_draw_alpha_border):sizeof(draw),payloads,4);
    uint32_t offset;
    if (compact_vertices) {
        struct halo_metal_draw_compact extended={draw,vertex_bytes,output.native_volume_border_mask,output.native_alpha_border_mask,0};
        extended.draw.command.opcode=HALO_METAL_DRAW_COMPACT;
        offset=command_append(&extended,sizeof(extended));
    } else if (output.native_volume_border_mask) {
        struct halo_metal_draw_volume_border extended={draw,output.native_volume_border_mask,output.native_alpha_border_mask};
        extended.draw.command.opcode=HALO_METAL_DRAW_VOLUME_BORDER;
        offset=command_append(&extended,sizeof(extended));
    } else if (output.native_alpha_border_mask) {
        struct halo_metal_draw_alpha_border extended={draw,output.native_alpha_border_mask,0};
        extended.draw.command.opcode=HALO_METAL_DRAW_ALPHA_BORDER;
        offset=command_append(&extended,sizeof(extended));
    } else offset=command_append(&draw,sizeof(draw));
    payload_append(offset,offsetof(struct halo_metal_draw,vertices_offset),vertices,vertex_bytes);
    payload_append(offset,offsetof(struct halo_metal_draw,indices_offset),rebased,index_bytes);
    payload_append(offset,offsetof(struct halo_metal_draw,vertex_uniforms_offset),&output.vertex,sizeof(output.vertex));
    payload_append(offset,offsetof(struct halo_metal_draw,pixel_uniforms_offset),&output.pixel,sizeof(output.pixel));
    packet_finish();free(vertices);free(rebased);
    device.draws++;
    color->last_rendered=rendered_serial_next();
    if (depth) depth->last_rendered=device.resource_serial;
    if (draw_diagnostics) {
        native_draw_phase(&profile_started,&profile_submitted,&draw_profile.emit_ns);
        draw_profile.draws++;
        draw_profile.expanded_bytes+=expanded_bytes;
        draw_profile.vertex_bytes+=vertex_bytes;
        if (compact_vertices) { draw_profile.compact_draws++;draw_profile.compact_bytes+=vertex_bytes; }
    }
}
void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE type,UINT first,UINT count) {
    draw_original(type,first,count,NULL,FALSE);
}
void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE type,UINT count,const WORD *indices) {
    if (!indices && count) native_fail("missing original indices",HALO_METAL_INVALID);
    draw_original(type,0,count,indices,FALSE);
}
static void immediate_emit(void) {
    if (device.immediate_count==device.immediate_capacity) {
        unsigned long capacity=device.immediate_capacity ? device.immediate_capacity*2:256;
        if (capacity>(HALO_METAL_MAX_PACKET-8192)/256) native_fail("immediate vertex capacity",HALO_METAL_MEMORY);
        float *vertices=realloc(device.immediate_vertices,capacity*256);
        if (!vertices) native_fail("immediate vertex allocation",HALO_METAL_MEMORY);
        device.immediate_vertices=vertices;device.immediate_capacity=capacity;
    }
    memcpy(device.immediate_vertices+device.immediate_count*64,device.attributes,256);
    device.immediate_count++;
}
void WINAPI D3DDevice_End(void) {
    device.immediate_active=FALSE;
    draw_original(device.immediate_type,0,device.immediate_count,NULL,TRUE);
}

BOOL WINAPI D3DDevice_IsBusy(void) { return FALSE; }
void WINAPI D3DDevice_KickPushBuffer(void) { if (device.ready) packet_flush(NATIVE_FLUSH_KICK_PUSH_BUFFER); }
void WINAPI D3DDevice_InsertCallback(D3DCALLBACKTYPE type,D3DCALLBACK callback,DWORD context) {
    (void)type; if (callback) { if (device.ready) packet_flush(NATIVE_FLUSH_INSERT_CALLBACK);callback(context); }
}
static struct halo_metal_ref query_create(void) {
    struct halo_metal_create_visibility command={0};
    command.command.opcode=HALO_METAL_CREATE_VISIBILITY;
    command.resource=(struct halo_metal_ref){next_query_id++,1};
    /* Count samples. A boolean result treats one pixel beside the hull as
       the whole engine corona. */
    command.mode=HALO_METAL_VISIBILITY_COUNTING;
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    return command.resource;
}
static void query_collect(unsigned index) {
    if (!device.query_pending[index]) return;
    /* The Mac GL path keeps the last finished count and never waits on the
       GPU. A test still sitting in the open packet has not reached the GPU. */
    if (transport.recording && device.query_packet[index]==transport.sequence) return;
    if (device.query_packet[index] > transport.reply.completed_sequence) return;
    uint64_t result=0;
    int status=halo_metal_guest_readback(&transport,device.query_slots[index],HALO_METAL_VISIBILITY,
        &result,sizeof(result));
    if (status==HALO_METAL_UNDEFINED_CONTENT) {
        transport.status=HALO_METAL_OK;transport.error=NULL;
        return;
    }
    require_status("read original visibility result",status);
    /* Storage pixels back to the game's pixels. The flare divides this count
       by the test quad's area in game pixels. */
    {
        double area=(double)screen_scale[0]*(double)screen_scale[1];

        if (area>1.0)
            result=(uint64_t)(result/area+0.5);
    }
    if (result>VISIBILITY_ALL_SAMPLES) result=VISIBILITY_ALL_SAMPLES;
    device.query_results[index]=(UINT)result;
    device.query_pending[index]=FALSE;
}
void WINAPI D3DDevice_BeginVisibilityTest(void) {
    if (!device.ready || device.visibility_test_active) return;
    if (!device.query_scratch.id) device.query_scratch=query_create();
    struct halo_metal_visibility command={{HALO_METAL_BEGIN_VISIBILITY,0},device.query_scratch};
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    device.visibility_test_active=TRUE;
}
HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index) {
    if (!device.ready || !device.visibility_test_active) return S_OK;
    index%=VISIBILITY_TEST_SLOTS;
    query_collect(index);
    struct halo_metal_visibility command={{HALO_METAL_END_VISIBILITY,0},device.query_scratch};
    packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
    struct halo_metal_ref previous=device.query_slots[index];
    device.query_slots[index]=device.query_scratch;device.query_scratch=previous;
    device.query_packet[index]=transport.sequence;
    device.query_pending[index]=TRUE;device.visibility_test_active=FALSE;
    return S_OK;
}
HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index,UINT *result,ULONGLONG *timestamp) {
    if (timestamp) *timestamp=0;
    index%=VISIBILITY_TEST_SLOTS;
    if (device.ready) query_collect(index);
    if (result) *result=device.ready ? device.query_results[index]:0;
    return S_OK;
}
void WINAPI D3DDevice_Clear(DWORD count,const D3DRECT *rectangles,DWORD flags,D3DCOLOR color,float z,DWORD stencil) {
    if (!device.ready) return;
    struct native_resource *target=target_get(device.render_target);
    struct native_resource *depth=device.depth_stencil ? target_get(device.depth_stencil):NULL;
    struct halo_metal_clear_channels command={0};
    command.clear.command.opcode=HALO_METAL_CLEAR_CHANNELS;
    command.color_write_mask=(flags&D3DCLEAR_TARGET_R ? 1:0)|(flags&D3DCLEAR_TARGET_G ? 2:0)|
        (flags&D3DCLEAR_TARGET_B ? 4:0)|(flags&D3DCLEAR_TARGET_A ? 8:0);
    if (command.color_write_mask) { command.clear.color=target->ref;command.clear.planes|=HALO_METAL_COLOR; }
    if (depth && (flags&D3DCLEAR_ZBUFFER)) command.clear.planes|=HALO_METAL_DEPTH;
    if (depth && (flags&D3DCLEAR_STENCIL)) command.clear.planes|=HALO_METAL_STENCIL;
    if (command.clear.planes&6) command.clear.depth_stencil=depth->ref;
    if (!command.clear.planes) return;
    color_to_vec4(color,command.clear.rgba);command.clear.depth=z;command.clear.stencil=stencil&255;
    BOOL with_rectangles=count && rectangles;
    unsigned actual_count=with_rectangles ? count:1;
    for (unsigned i=0;i<actual_count;i++) {
        int64_t left=device.viewport.X,top=device.viewport.Y;
        int64_t right=left+device.viewport.Width,bottom=top+device.viewport.Height;
        if (with_rectangles) {
            if (rectangles[i].x1>left) left=rectangles[i].x1;
            if (rectangles[i].y1>top) top=rectangles[i].y1;
            if (rectangles[i].x2<right) right=rectangles[i].x2;
            if (rectangles[i].y2<bottom) bottom=rectangles[i].y2;
            left+=UI_OFFSET;right+=UI_OFFSET;
        }
        if (left<0) left=0;if (top<0) top=0;
        if (right>(int64_t)target->description.width) right=target->description.width;
        if (bottom>(int64_t)target->description.height) bottom=target->description.height;
        if (left>=right || top>=bottom) continue;
        struct halo_metal_render_dimensions dimensions=resource_dimensions(target);
        struct halo_metal_render_edges edges={left,top,right,bottom};
        struct halo_metal_render_rectangle rectangle;
        require_status("scale native clear",halo_metal_render_scale_rectangle(&dimensions,&edges,&rectangle));
        if (!rectangle.width || !rectangle.height) continue;
        command.clear.x=rectangle.x;command.clear.y=rectangle.y;
        command.clear.width=rectangle.width;command.clear.height=rectangle.height;
        packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();
        if (command.clear.planes&HALO_METAL_COLOR) target->last_rendered=rendered_serial_next();
        if (depth && (command.clear.planes&6)) depth->last_rendered=rendered_serial_next();
    }
}
static void ui_point_from_window(float wx,float wy,int centered,short *x,short *y) {
    int ww,wh,pw,ph;
    *x=*y=-1;
    if (!device.created || !device.ready) return;
    struct native_resource *back=target_get(&device.back_buffer);
    struct halo_metal_render_dimensions dimensions=resource_dimensions(back);
    struct halo_metal_render_point point;
    platform_video_window_size(&ww,&wh);platform_video_drawable_size(&pw,&ph);
    if (ww<=0 || wh<=0 || pw<=0 || ph<=0) return;
    if (halo_metal_render_window_point(&dimensions,ww,wh,pw,ph,wx,wy,
        centered ? (halo_screen_width()-640)/2 : 0,&point) || point.x<SHRT_MIN || point.x>SHRT_MAX ||
        point.y<SHRT_MIN || point.y>SHRT_MAX) return;
    *x=point.x;*y=point.y;
}
int halo_ui_pointer_update(int menus_active,struct halo_ui_pointer *pointer) {
    struct platform_ui_pointer state;
    /* the scores after a game, and the menus, take Enter as A. That mapping
    follows this flag (keyboard_gamepad). The pointer alone does not. */
    platform_menus_set_active(menus_active!=0);
    platform_ui_pointer_set_active(menus_active!=0);
    if (!menus_active || !device.ready || !platform_ui_pointer_read(&state)) return 0;
    memset(pointer,0,sizeof(*pointer));
    ui_point_from_window(state.x,state.y,1,&pointer->x,&pointer->y);
    ui_point_from_window(state.click_x,state.click_y,1,&pointer->click_x,&pointer->click_y);
    pointer->moved=state.moved!=FALSE;
    pointer->left_clicks=state.left_clicks<255 ? state.left_clicks:255;
    pointer->right_clicks=state.right_clicks<255 ? state.right_clicks:255;
    pointer->wheel_steps=state.wheel_steps<-8 ? -8:state.wheel_steps>8 ? 8:state.wheel_steps;
    return 1;
}
int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer)
{
	struct platform_ui_pointer state;

	memset(pointer, 0, sizeof(*pointer));
	if (!platform_scoreboard_pointer(offered != 0, &state) || !device.ready)
		return 0;
	ui_point_from_window(state.x, state.y, 0, &pointer->x, &pointer->y);
	ui_point_from_window(state.click_x, state.click_y, 0, &pointer->click_x, &pointer->click_y);
	pointer->moved = state.moved != FALSE;
	pointer->left_clicks = (unsigned char)(state.left_clicks < 255 ? state.left_clicks : 255);
	return 1;
}
static void write_screenshot(struct native_resource *target) {
    const char *directory=config_string("debug.screenshot_directory");
    if (!*directory) return;
    uint32_t width=target->storage_width,height=target->storage_height,size=width*height*4;
    unsigned char *pixels=malloc(size),header[54]={'B','M'};
    if (!pixels) native_fail("allocate screenshot",HALO_METAL_MEMORY);
    packet_flush(NATIVE_FLUSH_READBACK);
    require_status("native game screenshot",halo_metal_guest_readback(&transport,target->ref,HALO_METAL_COLOR,pixels,size));
    for (unsigned i=0;i<size/4;i++) pixels[i*4+3]=255;
    uint32_t file_size=54+size,offset=54,dib=40;int32_t negative_height=-(int32_t)height;uint16_t planes=1,bits=32;
    memcpy(header+2,&file_size,4);memcpy(header+10,&offset,4);memcpy(header+14,&dib,4);
    memcpy(header+18,&width,4);memcpy(header+22,&negative_height,4);memcpy(header+26,&planes,2);
    memcpy(header+28,&bits,2);memcpy(header+34,&size,4);
    char path[512];snprintf(path,sizeof(path),"%s/frame%05lu.bmp",directory,device.frame);
    FILE *file=fopen(path,"wb");
    if (file) { fwrite(header,1,sizeof(header),file);fwrite(pixels,1,size,file);fclose(file); }
    free(pixels);
}
/* This wait controls completed presentation frames only when the existing
   interpolation is enabled. Original simulation and vblank remain below. */
static struct halo_frame_pacing frame_pacing;
static void native_frame_wait(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC,&now) || now.tv_sec<0 || now.tv_nsec<0 || now.tv_nsec>=1000000000) {
        halo_frame_pacing_reset(&frame_pacing);return;
    }
    uint64_t deadline=halo_frame_pacing_deadline(&frame_pacing,
        (uint64_t)now.tv_sec*UINT64_C(1000000000)+(uint64_t)now.tv_nsec,
        config_integer("display.frame_limit"),halo_interpolation_enabled());
    if (!deadline) return;
    struct timespec until={(time_t)(deadline/UINT64_C(1000000000)),(long)(deadline%UINT64_C(1000000000))};
    int status;
    do { status=clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&until,NULL); } while (status==EINTR);
    if (status) halo_frame_pacing_reset(&frame_pacing);
}
/* Diagnostic reads of the original tick counter never advance simulation. */
extern unsigned char game_time_initialized(void);
extern long game_time_get(void);
extern unsigned char cinematic_in_progress(void);
extern unsigned char player_input_enabled(void);
static void native_frame_statistics(void) {
    static uint64_t previous_ns;
    static unsigned long previous_frame;
    static long previous_tick;
    static int previous_initialized;
    if (!config_boolean("debug.gpu_stats")) return;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC,&now)) return;
    uint64_t ns=(uint64_t)now.tv_sec*UINT64_C(1000000000)+(uint64_t)now.tv_nsec;
    int initialized=game_time_initialized()!=0;
    long tick=initialized ? game_time_get():0;
    platform_log("Native timing: frame %lu, monotonic %llu ns, tick %ld, initialized %d",
        device.frame,(unsigned long long)ns,tick,initialized);
    platform_log("Native scene: frame %lu, cinematic %d, player-input %d",
        device.frame,initialized && cinematic_in_progress()!=0,initialized && player_input_enabled()!=0);
    if (draw_diagnostics) {
        platform_log("Native guest work: frame %lu, draws %llu, expansion %llu ns, textures %llu ns, state %llu ns, program %llu ns, emit %llu ns, submit %llu ns",
            device.frame,(unsigned long long)draw_profile.draws,(unsigned long long)draw_profile.expansion_ns,
            (unsigned long long)draw_profile.textures_ns,(unsigned long long)draw_profile.state_ns,
            (unsigned long long)draw_profile.program_ns,(unsigned long long)draw_profile.emit_ns,
            (unsigned long long)draw_profile.submit_ns);
        platform_log("Native vertex input: frame %lu, GPU draws %llu/%llu, compact %llu bytes, uploaded %llu bytes, expanded reference %llu bytes",
            device.frame,(unsigned long long)draw_profile.compact_draws,(unsigned long long)draw_profile.draws,
            (unsigned long long)draw_profile.compact_bytes,(unsigned long long)draw_profile.vertex_bytes,
            (unsigned long long)draw_profile.expanded_bytes);
        memset(&draw_profile,0,sizeof(draw_profile));
    }
    if (previous_ns && ns>previous_ns && ns-previous_ns<UINT64_C(1000000000)) return;
    if (previous_ns && ns>previous_ns) {
        struct native_resource *back=target_get(&device.back_buffer);
        double seconds=(double)(ns-previous_ns)/1e9;
        platform_log("Native display: %.3f FPS, %.3f simulation Hz, %ux%u storage, %lux%lu logical, cap %ld, interpolation %d, ticks %ld-%ld",
            (device.frame-previous_frame)/seconds,
            initialized && previous_initialized && tick>=previous_tick ? (tick-previous_tick)/seconds:0.0,
            back->storage_width,back->storage_height,back->description.width,back->description.height,
            config_integer("display.frame_limit"),halo_interpolation_enabled(),previous_tick,tick);
        platform_log("Native anti-aliasing: requested %s, applied %llu, stage pre-HUD",
            device.antialiasing_enabled ? "fxaa":"off",
            (unsigned long long)device.antialias_passes);
    }
    previous_ns=ns;previous_frame=device.frame;previous_tick=tick;previous_initialized=initialized;
}
void WINAPI D3DDevice_Present(const RECT *source,const RECT *destination,void *unused,void *unused2) {
    (void)source;(void)destination;(void)unused;(void)unused2;
    if (device.ready) {
        struct native_resource *back=target_get(&device.back_buffer);
        long screenshot_every=config_integer("debug.screenshot_every");
        if (screenshot_every>0 && device.frame%(unsigned long)screenshot_every==0) write_screenshot(back);
        backbuffer_history_advance(back);
        struct halo_metal_present_scaled command={0};command.command.opcode=HALO_METAL_PRESENT_SCALED;
        command.source=back->ref;command.display_flags=config_boolean("display.vsync") ? HALO_METAL_DISPLAY_VSYNC:0;
        packet_begin(sizeof(command),NULL,0);command_append(&command,sizeof(command));packet_finish();packet_flush(NATIVE_FLUSH_PRESENT);
        native_history_sample_retire();
    }
    device.frame++;
    if (config_boolean("debug.gpu_stats") && device.frame%60==0) {
        platform_log("Native frame %lu: %lu original draws, %u texture resources, %u programs",
            device.frame,device.draws,next_resource_id-1,next_program_id-1);
        platform_log("Native submissions frames %lu-%lu: %llu batches, %llu commands, %llu bytes, %llu host-submit us, largest %u bytes",
            device.frame-59,device.frame,(unsigned long long)(submitted_batches-statistics_batches),
            (unsigned long long)(submitted_commands-statistics_commands),
            (unsigned long long)(submitted_bytes-statistics_bytes),
            (unsigned long long)((submit_wall_ns-statistics_wall_ns)/1000),largest_statistics_batch);
        statistics_batches=submitted_batches;statistics_commands=submitted_commands;
        statistics_bytes=submitted_bytes;statistics_wall_ns=submit_wall_ns;largest_statistics_batch=0;
        native_flush_statistics_log(device.frame-60,device.frame-1);
    }
    native_frame_wait();
    if (device.ready) native_frame_statistics();
    platform_pump_events();vertical_blank_start();
    pthread_mutex_lock(&vertical_blank_lock);
    if (halo_interpolation_enabled()) flip_count++;
    else {
        while (pending_flips>=2) pthread_cond_wait(&vertical_blank_condition,&vertical_blank_lock);
        pending_flips++;
    }
    pthread_mutex_unlock(&vertical_blank_lock);
    native_resolution_commit();
}
HRESULT WINAPI D3DDevice_PersistDisplay(void) { return S_OK; }
long halo_screen_commit(void)
{
	long width;
	float scale[2];

	if (!screen_width)
		return halo_screen_width();
	screen_mode_choose(&width, scale);
	if (width != screen_width || scale[0] != screen_scale[0] || scale[1] != screen_scale[1])
	{
		platform_log("screen: %ldx%d drawn at %.0fx%.0f", width, SCREEN_HEIGHT,
			width * scale[0], SCREEN_HEIGHT * scale[1]);
		screen_width = width;
		screen_scale[0] = scale[0];
		screen_scale[1] = scale[1];
#ifndef HALO_ANDROID
		if (device.created)
		{
			device.presentation.BackBufferWidth = (UINT)width;
			backbuffer_surfaces_resize((unsigned long)width,SCREEN_HEIGHT);
		}
#endif
	}
	return screen_width;
}


#endif /* HALO_MACOS_NATIVE_METAL */

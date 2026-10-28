/* Source contracts: d3d8_gl.c prepare_draw, bind_textures,
 * configure_sampler, texture_border_key and apply_raster_state. */
#include "xgpu.h"
#include "metal_draw_state.h"
#include <math.h>
#include <string.h>

static int fail(struct metal_draw_state_error *error, int status, uint32_t stage,
    uint32_t state, uint32_t value, const char *message)
{
    if (error) { error->stage=stage; error->state=state; error->value=value; error->message=message; }
    return status;
}
static float word_float(uint32_t value)
{
    float result;
    memcpy(&result, &value, sizeof(result));
    return result;
}
static void color(uint32_t value, float out[4])
{
    out[0]=(float)((value>>16)&255)/255.0f;
    out[1]=(float)((value>>8)&255)/255.0f;
    out[2]=(float)(value&255)/255.0f;
    out[3]=(float)(value>>24)/255.0f;
}
static int comparison(uint32_t value)
{
    if (!value) return 1; /* original zero function is NEVER */
    return value >= D3DCMP_NEVER && value <= D3DCMP_ALWAYS ? (int)(value-D3DCMP_NEVER+1) : 0;
}
static int stencil_op(uint32_t value)
{
    switch(value) {
    case D3DSTENCILOP_KEEP:return 1; case D3DSTENCILOP_ZERO:return 2;
    case D3DSTENCILOP_REPLACE:return 3; case D3DSTENCILOP_INCRSAT:return 4;
    case D3DSTENCILOP_DECRSAT:return 5; case D3DSTENCILOP_INVERT:return 6;
    case D3DSTENCILOP_INCR:return 7; case D3DSTENCILOP_DECR:return 8;
    default:return 0; }
}
static int blend_factor(uint32_t value)
{
    switch(value) {
    case D3DBLEND_ZERO:return 1;case D3DBLEND_ONE:return 2;
    case D3DBLEND_SRCCOLOR:return 3;case D3DBLEND_INVSRCCOLOR:return 4;
    case D3DBLEND_SRCALPHA:return 5;case D3DBLEND_INVSRCALPHA:return 6;
    case D3DBLEND_DESTALPHA:return 7;case D3DBLEND_INVDESTALPHA:return 8;
    case D3DBLEND_DESTCOLOR:return 9;case D3DBLEND_INVDESTCOLOR:return 10;
    case D3DBLEND_SRCALPHASAT:return 11;case D3DBLEND_CONSTANTCOLOR:return 12;
    case D3DBLEND_INVCONSTANTCOLOR:return 13;case D3DBLEND_CONSTANTALPHA:return 14;
    case D3DBLEND_INVCONSTANTALPHA:return 15;default:return 0; }
}
static int blend_op(uint32_t value)
{
    switch(value) {
    case D3DBLENDOP_ADD:return 1;case D3DBLENDOP_SUBTRACT:return 2;
    case D3DBLENDOP_REVSUBTRACT:return 3;case D3DBLENDOP_MIN:return 4;
    case D3DBLENDOP_MAX:return 5;default:return 0; }
}
static int address(uint32_t value)
{
    switch(value) {
    case D3DTADDRESS_WRAP:return 0;case D3DTADDRESS_MIRROR:return 1;
    case D3DTADDRESS_CLAMP:case D3DTADDRESS_BORDER:case D3DTADDRESS_CLAMPTOEDGE:return 2;
    default:return -1; }
}
static int finite_values(const float *values, unsigned count)
{
    unsigned i;
    for(i=0;i<count;i++) if(!isfinite(values[i])) return 0;
    return 1;
}

/* Failure-only provenance for the actual guest. Viewport validation failures
 * also report the raw constant bank, which can contain original unused
 * padding. Constants are copied as bytes rather than rejected or sanitized. */
#ifdef HALO_MACOS
static void nonfinite_vertex_diagnostic(const struct metal_draw_state_input *in)
{
    static const char components[] = "xyzw";
    static const char *viewport_names[] = {"X", "Y", "Width", "Height", "MinZ", "MaxZ"};
    unsigned i, component, viewport_count = 0, scale_count = 0, offset_count = 0, constant_count = 0;
    uint32_t bits;
    for(i=0;i<6;i++) if(!isfinite(in->viewport[i])) {
        memcpy(&bits,&in->viewport[i],sizeof(bits));
        platform_log("Native nonfinite vertex input: viewport.%s bits %08lx",
            viewport_names[i],(unsigned long)bits);
        viewport_count++;
    }
    for(i=0;i<4;i++) {
        if(!isfinite(in->viewport_scale[i])) {
            memcpy(&bits,&in->viewport_scale[i],sizeof(bits));
            platform_log("Native nonfinite vertex input: viewport_scale.%c bits %08lx",
                components[i],(unsigned long)bits);
            scale_count++;
        }
        if(!isfinite(in->viewport_offset[i])) {
            memcpy(&bits,&in->viewport_offset[i],sizeof(bits));
            platform_log("Native nonfinite vertex input: viewport_offset.%c bits %08lx",
                components[i],(unsigned long)bits);
            offset_count++;
        }
    }
    for(i=0;i<XGPU_VERTEX_CONSTANT_COUNT;i++) for(component=0;component<4;component++)
        if(!isfinite(in->constants[i][component])) {
            memcpy(&bits,&in->constants[i][component],sizeof(bits));
            platform_log("Native nonfinite vertex input: c[%u].%c original c[%d].%c bits %08lx",
                i,components[component],(int)i-XGPU_VERTEX_CONSTANT_BIAS,components[component],(unsigned long)bits);
            constant_count++;
        }
    platform_log("Native nonfinite vertex input summary: viewport %u scale %u offset %u constant-components %u",
        viewport_count,scale_count,offset_count,constant_count);
}
#else
#define nonfinite_vertex_diagnostic(in) ((void)(in))
#endif

int metal_draw_state_pack(const struct metal_draw_state_input *in,
    struct nv2a_pixel_shader_key *key, struct metal_draw_state_output *output,
    struct metal_draw_state_error *error)
{
    struct metal_draw_state_output out;
    struct nv2a_pixel_shader_key k;
    const uint32_t *rs;
    struct halo_metal_draw_state *s=&out.state;
    unsigned stage, i;
    int mapped;
#define BAD(status, index, value, message) return fail(error,status,UINT32_MAX,index,value,message)
#define RS_MAP(field,index,mapper) do { mapped=mapper(rs[index]); if(!mapped) BAD(HALO_METAL_UNSUPPORTED,index,rs[index],"unsupported render state enum"); s->field=(uint32_t)mapped; } while(0)
#define BOOL_RS(index) do { if(rs[index]>1) BAD(HALO_METAL_UNSUPPORTED,index,rs[index],"unsupported nonboolean render state"); } while(0)
    if(!in || !key || !output || !in->render_state || !in->texture_state ||
        !in->constants || !in->viewport_scale || !in->viewport_offset)
        BAD(HALO_METAL_INVALID,0,0,"missing CPU draw input");
    if(in->has_depth>1 || !in->target_width || !in->target_height)
        BAD(HALO_METAL_INVALID,0,0,"invalid draw target");
    if(in->native_black_border>1)
        BAD(HALO_METAL_UNSUPPORTED,0,in->native_black_border,"unknown native black-border contract");
    if(in->native_alpha_border>1)
        BAD(HALO_METAL_UNSUPPORTED,0,in->native_alpha_border,"unknown native alpha-border contract");
    if(in->native_volume>1)
        BAD(HALO_METAL_UNSUPPORTED,0,in->native_volume,"unknown native volume contract");
    if(in->native_volume_border>1)
        BAD(HALO_METAL_UNSUPPORTED,0,in->native_volume_border,"unknown native volume-border contract");
    if(in->native_depth_contract>METAL_DRAW_DEPTH_RAW_D24)
        BAD(HALO_METAL_UNSUPPORTED,0,in->native_depth_contract,"unknown native depth contract");
    if(in->sample_count!=1) BAD(HALO_METAL_UNSUPPORTED,0,in->sample_count,"multisample shader/target contract not verified");
    if(!finite_values(in->viewport,6) || !finite_values(in->viewport_scale,4) ||
        !finite_values(in->viewport_offset,4)) {
        nonfinite_vertex_diagnostic(in);
        BAD(HALO_METAL_INVALID,0,0,"nonfinite vertex uniform or viewport");
    }
    for(i=0;i<4;i++) if(in->viewport[i]<0 || (double)in->viewport[i]>=4294967296.0 ||
        in->viewport[i] != (float)(uint32_t)in->viewport[i])
        BAD(HALO_METAL_UNSUPPORTED,0,i,"viewport/scissor must use rounded nonnegative target pixels");
    if(in->viewport[2]<=0 || in->viewport[3]<=0 || in->viewport[4]<0 ||
        in->viewport[5]>1 || in->viewport[4]>in->viewport[5] ||
        in->viewport[0]+in->viewport[2]>(float)in->target_width ||
        in->viewport[1]+in->viewport[3]>(float)in->target_height)
        BAD(HALO_METAL_UNSUPPORTED,0,0,"viewport/scissor outside target or depth range");
    memset(&out,0,sizeof(out)); memset(&k,0,sizeof(k));
    rs=in->render_state;
    memcpy(k.combiner_state,rs,sizeof(k.combiner_state));
    memset(k.combiner_state+D3DRS_PSCONSTANT0_0,0,16*sizeof(uint32_t));
    k.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0]=0;
    k.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1]=0;
    k.texture_modes=rs[D3DRS_PSTEXTUREMODES];
    for(stage=0;stage<4;stage++) if(((k.texture_modes>>(5*stage))&31)==0x0a) {
        uint32_t depth_format=(in->depth_surface_format_word>>8)&255;
        if(in->native_depth_contract!=METAL_DRAW_DEPTH_RAW_D24 || !in->has_depth ||
            (in->original_auto_depth_format!=D3DFMT_D24S8 &&
             in->original_auto_depth_format!=D3DFMT_LIN_D24S8) ||
            (depth_format!=D3DFMT_D24S8 && depth_format!=D3DFMT_LIN_D24S8) ||
            in->floating_point_zbuffer!=0)
            BAD(HALO_METAL_UNSUPPORTED,D3DRS_PSTEXTUREMODES,0x0a,
                "DOT_ZW requires explicit original integer-D24 target and floating-Z0");
        if(in->viewport_scale[2]!=16777215.0f*(in->viewport[5]-in->viewport[4]) ||
            in->viewport_offset[2]!=16777215.0f*in->viewport[4])
            BAD(HALO_METAL_UNSUPPORTED,D3DRS_PSTEXTUREMODES,0x0a,
                "DOT_ZW viewport metadata is not original integer-D24");
        out.depth_contract=METAL_DRAW_DEPTH_RAW_D24;
        out.pixel.depth_scale=1.0f/16777215.0f;
        out.pixel.depth_clip_min=in->viewport[4];
        out.pixel.depth_clip_max=in->viewport[5];
    }
    k.alpha_test_function=rs[D3DRS_ALPHATESTENABLE]?rs[D3DRS_ALPHAFUNC]:0;
    k.fog_enable=rs[D3DRS_FOGENABLE]!=0;
    k.fog_table_mode=(unsigned char)rs[D3DRS_FOGTABLEMODE];
    /* Native visibility counts samples in the encoder, not a shader atomic. */
    if(k.fog_enable && rs[D3DRS_FOGTABLEMODE]>D3DFOG_LINEAR)
        BAD(HALO_METAL_UNSUPPORTED,D3DRS_FOGTABLEMODE,rs[D3DRS_FOGTABLEMODE],"unsupported fog table mode");
    if(rs[D3DRS_ALPHATESTENABLE] && !comparison(rs[D3DRS_ALPHAFUNC]))
        BAD(HALO_METAL_UNSUPPORTED,D3DRS_ALPHAFUNC,rs[D3DRS_ALPHAFUNC],"unsupported alpha comparison");
    memcpy(out.vertex.c,in->constants,sizeof(out.vertex.c));
    memcpy(out.vertex.viewport_scale,in->viewport_scale,16);
    memcpy(out.vertex.viewport_offset,in->viewport_offset,16);
    out.vertex.point_size=rs[D3DRS_POINTSIZE]?word_float(rs[D3DRS_POINTSIZE]):1.0f;
    out.vertex.screen_offset=(float)in->screen_offset;
    for(i=0;i<8;i++) {
        color(rs[D3DRS_PSCONSTANT0_0+i],out.pixel.ps_c0[i]);
        color(rs[D3DRS_PSCONSTANT1_0+i],out.pixel.ps_c1[i]);
    }
    color(rs[D3DRS_PSFINALCOMBINERCONSTANT0],out.pixel.ps_final_c0);
    color(rs[D3DRS_PSFINALCOMBINERCONSTANT1],out.pixel.ps_final_c1);
    color(rs[D3DRS_FOGCOLOR],out.pixel.fog_color);
    out.pixel.fog_parameters[0]=word_float(rs[D3DRS_FOGSTART]);
    out.pixel.fog_parameters[1]=word_float(rs[D3DRS_FOGEND]);
    out.pixel.fog_parameters[2]=word_float(rs[D3DRS_FOGDENSITY]);
    out.pixel.alpha_reference=(float)(rs[D3DRS_ALPHAREF]&255);
    for(stage=0;stage<4;stage++) {
        const uint32_t *ts=in->texture_state[stage];
        const struct metal_draw_texture *t=&in->textures[stage];
        struct halo_metal_sampler *sampler=&out.samplers[stage];
        uint32_t mode=(k.texture_modes>>(5*stage))&31;
        uint32_t min_filter,mag_filter,mip_filter,max_mip,axes,mip_limit,dimension,anisotropy;
#define TS_BAD(index,value,message) return fail(error,HALO_METAL_UNSUPPORTED,stage,index,value,message)
        k.alpha_kill[stage]=ts[D3DTSS_ALPHAKILL]==D3DTALPHAKILL_ENABLE;
        k.color_sign[stage]=(unsigned char)((ts[D3DTSS_COLORSIGN]>>28)&15);
        out.pixel.bump_matrix[stage][0]=word_float(ts[D3DTSS_BUMPENVMAT00]);
        out.pixel.bump_matrix[stage][1]=word_float(ts[D3DTSS_BUMPENVMAT01]);
        out.pixel.bump_matrix[stage][2]=word_float(ts[D3DTSS_BUMPENVMAT10]);
        out.pixel.bump_matrix[stage][3]=word_float(ts[D3DTSS_BUMPENVMAT11]);
        out.pixel.bump_luminance[stage][0]=word_float(ts[D3DTSS_BUMPENVLSCALE]);
        out.pixel.bump_luminance[stage][1]=word_float(ts[D3DTSS_BUMPENVLOFFSET]);
        out.pixel.texture_lod_bias[stage]=word_float(ts[D3DTSS_MIPMAPLODBIAS]);
        color(ts[D3DTSS_BORDERCOLOR],out.pixel.texture_border_color[stage]);
        for(i=0;i<4;i++) out.pixel.texture_scale[stage][i]=1.0f;
        if(!t->present || mode==0 || mode==4 || mode==5 || mode==0x11) {
            k.sampler_type[stage]=(unsigned char)(mode==0x11?_xgpu_sampler_2d:_xgpu_sampler_none);
            continue;
        }
        if(t->present!=1 || t->linear>1 || t->hires>1 || t->hires_coverage>1 ||
            t->hires_point_threshold>1 || t->ce_meter_channels>1 || t->rendered>1 ||
            !t->width || !t->height || !t->levels)
            TS_BAD(0,0,"invalid effective texture metadata");
        if(t->sampler_type!=_xgpu_sampler_2d && t->sampler_type!=_xgpu_sampler_cube && t->sampler_type!=_xgpu_sampler_3d)
            TS_BAD(0,t->sampler_type,"unknown native texture type");
        if(t->sampler_type==_xgpu_sampler_3d) {
            if(!in->native_volume || !in->texture_depth[stage] || in->texture_depth[stage]>512 ||
                t->width>512 || t->height>512 || t->linear || t->hires || t->hires_coverage ||
                t->hires_point_threshold || t->ce_meter_channels)
                TS_BAD(0,in->texture_depth[stage],"volume requires explicit normalized native metadata within 512 texels");
        } else if(in->texture_depth[stage]>1)
            TS_BAD(0,in->texture_depth[stage],"nonvolume resource depth exceeds one");
        if(t->hires_coverage && !t->hires)
            TS_BAD(0,0,"HUD coverage requires a high-resolution replacement");
        if(t->hires_point_threshold && !t->hires)
            TS_BAD(0,0,"HUD meter thresholds require a high-resolution replacement");
        if(t->ce_meter_channels && t->sampler_type!=_xgpu_sampler_2d)
            TS_BAD(0,t->sampler_type,"Custom Edition meter channels require texture2d");
        if(t->hires && t->sampler_type!=_xgpu_sampler_2d)
            TS_BAD(0,t->sampler_type,"high-resolution HUD replacement requires texture2d");
        if(t->sampler_type==_xgpu_sampler_cube && t->width!=t->height)
            TS_BAD(0,0,"cube resource must have square faces");
        dimension=t->width>t->height?t->width:t->height;
        if(t->sampler_type==_xgpu_sampler_3d && in->texture_depth[stage]>dimension)
            dimension=in->texture_depth[stage];
        for(mip_limit=1;dimension>1;dimension>>=1) mip_limit++;
        if(t->levels>mip_limit) TS_BAD(0,t->levels,"mip chain exceeds effective dimensions");
        if(t->linear && t->sampler_type!=_xgpu_sampler_2d)
            TS_BAD(0,t->sampler_type,"linear coordinates only validated for texture2d");
        k.sampler_type[stage]=(unsigned char)t->sampler_type;
        out.active_texture_mask|=1u<<stage;
        if(t->linear) {
            out.pixel.texture_scale[stage][0]=1.0f/(float)t->width;
            out.pixel.texture_scale[stage][1]=1.0f/(float)t->height;
        }
        if(stage==0) {
            k.coverage_alpha=(unsigned char)t->hires_coverage;
            k.point_threshold=(unsigned char)t->hires_point_threshold;
            k.ce_meter_channels=(unsigned char)t->ce_meter_channels;
        }
        min_filter=t->hires?D3DTEXF_LINEAR:ts[D3DTSS_MINFILTER];
        mag_filter=t->hires?D3DTEXF_LINEAR:ts[D3DTSS_MAGFILTER];
        mip_filter=t->hires?D3DTEXF_LINEAR:ts[D3DTSS_MIPFILTER];
        max_mip=t->hires?0:ts[D3DTSS_MAXMIPLEVEL];
        anisotropy=ts[D3DTSS_MAXANISOTROPY];
        /* Map textures the game filters linearly, along the pixel's footprint.
         * Volumes and border-addressed samplers stay as validated: Metal
         * rejects anisotropic volumes, and the border paths require anisotropy 1.
         * Render targets stay as the game asked (the water's ripple map). */
        if(in->anisotropic_level>1 && !t->hires && !t->rendered && t->levels>1 &&
            t->sampler_type!=_xgpu_sampler_3d && mip_filter!=D3DTEXF_NONE &&
            (min_filter==D3DTEXF_LINEAR || min_filter==D3DTEXF_ANISOTROPIC) &&
            ts[D3DTSS_ADDRESSU]!=D3DTADDRESS_BORDER && ts[D3DTSS_ADDRESSV]!=D3DTADDRESS_BORDER &&
            ts[D3DTSS_ADDRESSW]!=D3DTADDRESS_BORDER) {
            uint32_t level=in->anisotropic_level>16?16:in->anisotropic_level;
            min_filter=D3DTEXF_ANISOTROPIC;
            if(anisotropy<level) anisotropy=level;
        }
        /* The replacement is drawn at the original HUD size. Match the GL
         * override's unbiased trilinear footprint instead of applying a bias
         * or minimum mip chosen for the much smaller Xbox bitmap. */
        if(t->hires) out.pixel.texture_lod_bias[stage]=0.0f;
        if(min_filter<D3DTEXF_POINT || min_filter>D3DTEXF_ANISOTROPIC)
            TS_BAD(D3DTSS_MINFILTER,min_filter,"unsupported sampler filter");
        if(mag_filter<D3DTEXF_POINT || mag_filter>D3DTEXF_ANISOTROPIC)
            TS_BAD(D3DTSS_MAGFILTER,mag_filter,"unsupported sampler filter");
        if(mip_filter>D3DTEXF_LINEAR)
            TS_BAD(D3DTSS_MIPFILTER,mip_filter,"unsupported sampler mip filter");
        sampler->min_filter=min_filter!=D3DTEXF_POINT;
        sampler->mag_filter=mag_filter!=D3DTEXF_POINT;
        sampler->mip_filter=t->levels>1?mip_filter:0;
        sampler->max_anisotropy=min_filter==D3DTEXF_ANISOTROPIC && anisotropy>1?anisotropy:1;
        if(sampler->max_anisotropy>16) TS_BAD(D3DTSS_MAXANISOTROPY,sampler->max_anisotropy,"native anisotropy exceeds validated limit");
        if(t->sampler_type==_xgpu_sampler_3d && sampler->max_anisotropy!=1)
            TS_BAD(D3DTSS_MAXANISOTROPY,sampler->max_anisotropy,"volume anisotropy unverified");
        if(max_mip>=t->levels) TS_BAD(D3DTSS_MAXMIPLEVEL,max_mip,"minimum mip outside authored chain");
        sampler->lod_min=(float)max_mip; sampler->lod_max=(float)(t->levels-1);
        for(i=0;i<3;i++) {
            mapped=address(ts[D3DTSS_ADDRESSU+i]);
            if(mapped<0) TS_BAD(D3DTSS_ADDRESSU+i,ts[D3DTSS_ADDRESSU+i],"unsupported sampler address");
            /* Wire address members are contiguous fixed-width integers. */
            if(i==0) sampler->address_u=(uint32_t)mapped;
            else if(i==1) sampler->address_v=(uint32_t)mapped;
            else sampler->address_w=(uint32_t)mapped;
        }
        axes=(ts[D3DTSS_ADDRESSU]==D3DTADDRESS_BORDER?1u:0u)|
             (ts[D3DTSS_ADDRESSV]==D3DTADDRESS_BORDER?2u:0u);
        if(t->sampler_type==_xgpu_sampler_3d) {
            if(ts[D3DTSS_ADDRESSW]==D3DTADDRESS_BORDER) axes|=4u;
            if(axes) {
                if(t->levels<=1 ||
                    mag_filter!=D3DTEXF_LINEAR || ts[D3DTSS_MAXANISOTROPY]!=1 ||
                    !((min_filter==D3DTEXF_POINT && mip_filter==D3DTEXF_POINT) ||
                      (min_filter==D3DTEXF_LINEAR && mip_filter==D3DTEXF_LINEAR)))
                    TS_BAD(D3DTSS_ADDRESSU,axes,"volume border requires mipped normalized3D, linear mag, point min/mip or linear min/mip, and anisotropy1");
                if(ts[D3DTSS_BORDERCOLOR]==0 && in->native_black_border) {
                    /* Preserve the existing literal-black path and key. */
                } else if(in->native_volume_border) {
                    out.native_volume_border_mask|=1u<<stage;
                } else
                    TS_BAD(D3DTSS_ADDRESSU,axes,"volume border requires literal RGBA0 capability or explicit native authored-RGBA companion");
                if(axes&1) sampler->address_u=3;
                if(axes&2) sampler->address_v=3;
                if(axes&4) sampler->address_w=3;
            }
            continue;
        }
        if(ts[D3DTSS_ADDRESSW]==D3DTADDRESS_BORDER && t->sampler_type!=_xgpu_sampler_2d)
            TS_BAD(D3DTSS_ADDRESSW,ts[D3DTSS_ADDRESSW],"cube/volume W border unverified");
        if(axes) {
            if(t->sampler_type!=_xgpu_sampler_2d || t->levels!=1 ||
                min_filter==D3DTEXF_ANISOTROPIC || mag_filter==D3DTEXF_ANISOTROPIC) {
                if(in->native_black_border && t->sampler_type==_xgpu_sampler_2d && ts[D3DTSS_BORDERCOLOR]==0) {
                    /* The literal transparent-black path needs no companion. */
                } else if(in->native_alpha_border && t->sampler_type==_xgpu_sampler_2d &&
                    t->levels>1 && !t->linear &&
                    (ts[D3DTSS_BORDERCOLOR]&0x00ffffffu)==0 &&
                    min_filter==D3DTEXF_LINEAR && mag_filter==D3DTEXF_LINEAR &&
                    (t->hires?mip_filter==D3DTEXF_LINEAR:mip_filter==D3DTEXF_POINT) &&
                    ts[D3DTSS_MAXANISOTROPY]==1) {
                    out.native_alpha_border_mask|=1u<<stage;
                } else
                    TS_BAD(D3DTSS_ADDRESSU,axes,"border emulation requires single-mip nonanisotropic texture2d or explicit native transparent-black support; alpha-border requires validated normalized mip2D black-RGB linear/nearest (high-res HUD trilinear) aniso1");
                /* The native sampler supplies the entire footprint including
                 * mip transitions and anisotropic taps. A level-zero shader
                 * coverage estimate would apply the border twice. */
                if(axes&1) sampler->address_u=3;
                if(axes&2) sampler->address_v=3;
            } else {
                k.border_axes[stage]=(unsigned char)axes;
                k.border_filter[stage]=(unsigned char)((min_filter!=D3DTEXF_POINT?1:0)|(mag_filter!=D3DTEXF_POINT?2:0));
            }
        }
#undef TS_BAD
    }
    if(!(rs[D3DRS_ALPHABLENDENABLE] && rs[D3DRS_SRCBLEND]==D3DBLEND_CONSTANTCOLOR && rs[D3DRS_DESTBLEND]==D3DBLEND_SRCALPHA)) {
        k.coverage_alpha=0;
        k.point_threshold=0;
    }
    /* nv2a_psh.c shares this meter correction across GLSL and MSL: green is
     * the replacement's coverage, while alpha remains the Xbox meter value.
     * Only the original CONSTANTCOLOR/SRCALPHA blend needs that correction. */
    /* Original model-lighting uploads include unused float padding (actual
     * c[-73].w can contain 0xffffffff); its DP3 consumes only XYZ. Preserve
     * all 3072 constant bytes, as the original GL upload does. Validate the
     * remaining vertex fields and pixel uniforms independently. */
    if(!finite_values(out.vertex.viewport_scale,4) ||
        !finite_values(out.vertex.viewport_offset,4) ||
        !isfinite(out.vertex.point_size) || !isfinite(out.vertex.screen_offset) ||
        !finite_values(out.vertex.padding,2) ||
        !finite_values((const float *)&out.pixel,sizeof(out.pixel)/sizeof(float)))
        BAD(HALO_METAL_INVALID,0,0,"nonfinite draw uniform");
    BOOL_RS(D3DRS_ZENABLE);BOOL_RS(D3DRS_ZWRITEENABLE);BOOL_RS(D3DRS_ALPHABLENDENABLE);
    BOOL_RS(D3DRS_STENCILENABLE);BOOL_RS(D3DRS_SOLIDOFFSETENABLE);
    if(rs[D3DRS_COLORWRITEENABLE]&~UINT32_C(0x01010101)) BAD(HALO_METAL_UNSUPPORTED,D3DRS_COLORWRITEENABLE,rs[D3DRS_COLORWRITEENABLE],"unsupported color write mask");
    s->color_write_mask=((rs[D3DRS_COLORWRITEENABLE]&0x10000)?1u:0u)|((rs[D3DRS_COLORWRITEENABLE]&0x100)?2u:0u)|((rs[D3DRS_COLORWRITEENABLE]&1)?4u:0u)|((rs[D3DRS_COLORWRITEENABLE]&0x1000000)?8u:0u);
    s->blend_enabled=rs[D3DRS_ALPHABLENDENABLE];
    RS_MAP(blend_source,D3DRS_SRCBLEND,blend_factor);
    RS_MAP(blend_destination,D3DRS_DESTBLEND,blend_factor);
    RS_MAP(blend_operation,D3DRS_BLENDOP,blend_op);
    color(rs[D3DRS_BLENDCOLOR],s->blend_color);
    s->depth_enabled=in->has_depth && rs[D3DRS_ZENABLE];
    s->depth_write=s->depth_enabled && rs[D3DRS_ZWRITEENABLE];
    RS_MAP(depth_compare,D3DRS_ZFUNC,comparison);
    s->stencil_enabled=in->has_depth && rs[D3DRS_STENCILENABLE];
    RS_MAP(stencil_compare,D3DRS_STENCILFUNC,comparison);
    RS_MAP(stencil_fail,D3DRS_STENCILFAIL,stencil_op);
    RS_MAP(stencil_depth_fail,D3DRS_STENCILZFAIL,stencil_op);
    RS_MAP(stencil_pass,D3DRS_STENCILPASS,stencil_op);
    s->stencil_reference=rs[D3DRS_STENCILREF];s->stencil_read_mask=rs[D3DRS_STENCILMASK];s->stencil_write_mask=rs[D3DRS_STENCILWRITEMASK];
    if(rs[D3DRS_FRONTFACE]!=D3DFRONT_CW && rs[D3DRS_FRONTFACE]!=D3DFRONT_CCW) BAD(HALO_METAL_UNSUPPORTED,D3DRS_FRONTFACE,rs[D3DRS_FRONTFACE],"unsupported front winding");
    s->front_winding=rs[D3DRS_FRONTFACE]==D3DFRONT_CCW;
    if(rs[D3DRS_CULLMODE]!=D3DCULL_NONE && rs[D3DRS_CULLMODE]!=D3DCULL_CW && rs[D3DRS_CULLMODE]!=D3DCULL_CCW) BAD(HALO_METAL_UNSUPPORTED,D3DRS_CULLMODE,rs[D3DRS_CULLMODE],"unsupported culling");
    s->cull_mode=rs[D3DRS_CULLMODE]==D3DCULL_NONE?0:rs[D3DRS_CULLMODE]==rs[D3DRS_FRONTFACE]?1:2;
    if(rs[D3DRS_FILLMODE]!=D3DFILL_SOLID && rs[D3DRS_FILLMODE]!=D3DFILL_WIREFRAME) BAD(HALO_METAL_UNSUPPORTED,D3DRS_FILLMODE,rs[D3DRS_FILLMODE],"point fill requires separate native contract");
    s->fill_mode=rs[D3DRS_FILLMODE]==D3DFILL_WIREFRAME;
    if(rs[D3DRS_SOLIDOFFSETENABLE]) {
        s->depth_bias=word_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);
        s->slope_depth_bias=word_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
        if(!finite_values(&s->depth_bias,2)) BAD(HALO_METAL_INVALID,D3DRS_POLYGONOFFSETZOFFSET,0,"nonfinite depth bias");
    }
    memcpy(s->viewport,in->viewport,sizeof(s->viewport));
    for(i=0;i<4;i++) s->scissor[i]=(uint32_t)in->viewport[i];
    memcpy(key,&k,sizeof(k));memcpy(output,&out,sizeof(out));
    if(error) memset(error,0,sizeof(*error));
    return HALO_METAL_OK;
#undef BAD
#undef RS_MAP
#undef BOOL_RS
}

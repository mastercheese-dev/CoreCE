/*
NV2A_PSH.C

Translation of Xbox pixel shaders - the NV2A texture shader stages and
register combiners, as held in the pixel shader render states - directly into
GLSL or Metal Shading Language.

A pixel shader runs in three parts:
- texture stages 0-3 (PSTextureModes) fetch t0-t3, some of them using the
  result of an earlier stage (dot product and bump environment modes);
- up to eight general combiner stages (PSRGBInputs/Outputs and
  PSAlphaInputs/Outputs) that each compute A*B, C*D and their sum or mux
  on the registers and write them back;
- the final combiner (PSFinalCombinerInputsABCD/EFG):
  rgb = A*B + (1-A)*C + D, alpha = G.
Register values are clamped to [-1, 1] between stages, as on the hardware.
*/

#ifdef XGPU_SHADER_STANDALONE
#include "xgpu_shader_standalone.h"
#else
#include "xgpu.h"
#include "port_config.h"
#include "xgpu_msl.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

enum
{
	_register_zero = 0,
	_register_c0 = 1,
	_register_c1 = 2,
	_register_fog = 3,
	_register_v0 = 4,
	_register_v1 = 5,
	_register_t0 = 8,
	_register_t1 = 9,
	_register_t2 = 10,
	_register_t3 = 11,
	_register_r0 = 12,
	_register_r1 = 13,
	_register_v1r0_sum = 14,
	_register_ef_product = 15,
};

enum
{
	_mode_none = 0x00,
	_mode_project2d = 0x01,
	_mode_project3d = 0x02,
	_mode_cubemap = 0x03,
	_mode_passthru = 0x04,
	_mode_clipplane = 0x05,
	_mode_bumpenvmap = 0x06,
	_mode_bumpenvmap_luminance = 0x07,
	_mode_brdf = 0x08,
	_mode_dot_st = 0x09,
	_mode_dot_zw = 0x0a,
	_mode_dot_reflect_diffuse = 0x0b,
	_mode_dot_reflect_specular = 0x0c,
	_mode_dot_str_3d = 0x0d,
	_mode_dot_str_cube = 0x0e,
	_mode_dependent_ar = 0x0f,
	_mode_dependent_gb = 0x10,
	_mode_dot_product = 0x11,
	_mode_dot_reflect_specular_constant = 0x12,
};

/* ---------- combiner inputs */

enum shader_language { _shader_glsl, _shader_msl };

static const char *vector_type(enum shader_language language, int components)
{
	static const char *glsl[] = { "", "float", "vec2", "vec3", "vec4" };
	static const char *msl[] = { "", "float", "float2", "float3", "float4" };
	return (language == _shader_msl ? msl : glsl)[components];
}

static const char *uniform_prefix(enum shader_language language)
{
	return language == _shader_msl ? "u." : "";
}

static const char *discard_statement(enum shader_language language)
{
	return language == _shader_msl ? "discard_fragment()" : "discard";
}

static const char *register_expression(unsigned long reg, int stage, BOOL unique_c0, BOOL unique_c1,
	enum shader_language language)
{
	static char buffer[4][32];
	static int next = 0;
	char *result = buffer[next++ & 3];

	switch (reg)
	{
	case _register_c0:
		if (stage < 0)
			return language == _shader_msl ? "u.ps_final_c0" : "ps_final_c0";
		snprintf(result, sizeof(buffer[0]), "%sps_c0[%d]", uniform_prefix(language), unique_c0 ? stage : 0);
		return result;
	case _register_c1:
		if (stage < 0)
			return language == _shader_msl ? "u.ps_final_c1" : "ps_final_c1";
		snprintf(result, sizeof(buffer[0]), "%sps_c1[%d]", uniform_prefix(language), unique_c1 ? stage : 0);
		return result;
	case _register_fog: return "fog";
	case _register_v0: return "v0";
	case _register_v1: return "v1";
	case _register_t0: return "t0";
	case _register_t1: return "t1";
	case _register_t2: return "t2";
	case _register_t3: return "t3";
	case _register_r0: return "r0";
	case _register_r1: return "r1";
	case _register_v1r0_sum: if (stage < 0) return "v1r0_sum"; break;
	case _register_ef_product: if (stage < 0) return "ef_product"; break;
	default: break;
	}
	return language == _shader_msl ? "float4(0.0)" : "vec4(0.0)";
}

/* one combiner input byte as a vec3 (rgb) or float (alpha) expression */
static void combiner_input(struct xgpu_text *text, unsigned long input, BOOL alpha_portion, int stage,
	BOOL unique_c0, BOOL unique_c1, enum shader_language language)
{
	unsigned long reg = input & 0x0f;
	BOOL alpha_channel = (input & 0x10) != 0;
	unsigned long mapping = input & 0xe0;
	const char *source = register_expression(reg, stage, unique_c0, unique_c1, language);
	char value[64];

	if (alpha_portion)
		snprintf(value, sizeof(value), "%s.%s", source, alpha_channel ? "a" : "b");
	else if (alpha_channel)
		snprintf(value, sizeof(value), "%s(%s.a)", vector_type(language, 3), source);
	else
		snprintf(value, sizeof(value), "%s.rgb", source);

	switch (mapping)
	{
	case 0x00: xgpu_text_append(text, "max(%s, 0.0)", value); break;
	case 0x20: xgpu_text_append(text, "(1.0 - clamp(%s, 0.0, 1.0))", value); break;
	case 0x40: xgpu_text_append(text, "(2.0 * max(%s, 0.0) - 1.0)", value); break;
	case 0x60: xgpu_text_append(text, "(1.0 - 2.0 * max(%s, 0.0))", value); break;
	case 0x80: xgpu_text_append(text, "(max(%s, 0.0) - 0.5)", value); break;
	case 0xa0: xgpu_text_append(text, "(0.5 - max(%s, 0.0))", value); break;
	case 0xc0: xgpu_text_append(text, "(%s)", value); break;
	default: xgpu_text_append(text, "(-%s)", value); break;
	}
}

/* final combiner inputs only have the unsigned identity and invert mappings */
static void final_input(struct xgpu_text *text, unsigned long input, BOOL alpha_portion,
	enum shader_language language)
{
	unsigned long reg = input & 0x0f;
	BOOL alpha_channel = (input & 0x10) != 0;
	const char *source = register_expression(reg, -1, FALSE, FALSE, language);
	char value[64];

	if (alpha_portion)
		snprintf(value, sizeof(value), "%s.%s", source, alpha_channel ? "a" : "b");
	else if (alpha_channel)
		snprintf(value, sizeof(value), "%s(%s.a)", vector_type(language, 3), source);
	else
		snprintf(value, sizeof(value), "%s.rgb", source);
	if (input & 0x20)
		xgpu_text_append(text, "(1.0 - clamp(%s, 0.0, 1.0))", value);
	else if (language == _shader_msl)
		xgpu_text_append(text, "max(%s, 0.0)", value);
	else
		xgpu_text_append(text, "clamp(%s, 0.0, 1.0)", value);
}

static const char *destination_name(unsigned long reg)
{
	switch (reg)
	{
	case _register_v0: return "v0";
	case _register_v1: return "v1";
	case _register_t0: return "t0";
	case _register_t1: return "t1";
	case _register_t2: return "t2";
	case _register_t3: return "t3";
	case _register_r0: return "r0";
	case _register_r1: return "r1";
	default: return NULL;
	}
}

static const char *output_mapping(unsigned long flags)
{
	switch (flags & 0x38)
	{
	case 0x08: return "(%s - 0.5)";
	case 0x10: return "(%s * 2.0)";
	case 0x18: return "((%s - 0.5) * 2.0)";
	case 0x20: return "(%s * 4.0)";
	case 0x30: return "(%s * 0.5)";
	default: return "(%s)";
	}
}

/* ---------- one general combiner stage */

static void combiner_stage(struct xgpu_text *text, const DWORD *state, int stage,
	enum shader_language language)
{
	DWORD combiner_count = state[D3DRS_PSCOMBINERCOUNT];
	BOOL unique_c0 = (combiner_count & 0x1000) != 0;
	BOOL unique_c1 = (combiner_count & 0x10000) != 0;
	BOOL mux_msb = (combiner_count & 0x100) != 0;
	int portion;

	xgpu_text_append(text, "\t/* combiner stage %d */\n\t{\n", stage);
	for (portion = 0; portion < 2; portion++)
	{
		BOOL alpha = portion == 1;
		DWORD inputs = alpha ? state[D3DRS_PSALPHAINPUTS0 + stage] : state[D3DRS_PSRGBINPUTS0 + stage];
		DWORD outputs = alpha ? state[D3DRS_PSALPHAOUTPUTS0 + stage] : state[D3DRS_PSRGBOUTPUTS0 + stage];
		unsigned long flags = outputs >> 12;
		const char *type = vector_type(language, alpha ? 1 : 3);
		const char *prefix = alpha ? "a" : "c";
		const char *mapping = output_mapping(flags);
		char mapped[64];

		xgpu_text_append(text, "\t\t%s %sA = ", type, prefix);
		combiner_input(text, (inputs >> 24) & 0xff, alpha, stage, unique_c0, unique_c1, language);
		xgpu_text_append(text, ";\n\t\t%s %sB = ", type, prefix);
		combiner_input(text, (inputs >> 16) & 0xff, alpha, stage, unique_c0, unique_c1, language);
		xgpu_text_append(text, ";\n\t\t%s %sC = ", type, prefix);
		combiner_input(text, (inputs >> 8) & 0xff, alpha, stage, unique_c0, unique_c1, language);
		xgpu_text_append(text, ";\n\t\t%s %sD = ", type, prefix);
		combiner_input(text, inputs & 0xff, alpha, stage, unique_c0, unique_c1, language);
		xgpu_text_append(text, ";\n");

		if (!alpha && (flags & 0x02))
			xgpu_text_append(text, "\t\t%s cAB = %s(dot(cA, cB));\n", type, type);
		else
			xgpu_text_append(text, "\t\t%s %sAB = %sA * %sB;\n", type, prefix, prefix, prefix);
		if (!alpha && (flags & 0x01))
			xgpu_text_append(text, "\t\t%s cCD = %s(dot(cC, cD));\n", type, type);
		else
			xgpu_text_append(text, "\t\t%s %sCD = %sC * %sD;\n", type, prefix, prefix, prefix);
		if (flags & 0x04)
		{
			if (mux_msb)
				xgpu_text_append(text, "\t\t%s %sSUM = r0.a >= 0.5 ? %sCD : %sAB;\n", type, prefix, prefix, prefix);
			else if (language == _shader_msl)
				xgpu_text_append(text, "\t\t%s %sSUM = (int(r0.a * 255.0) & 1) != 0 ? %sCD : %sAB;\n",
					type, prefix, prefix, prefix);
			else
				xgpu_text_append(text, "\t\t%s %sSUM = (int(r0.a * 255.0 + 0.5) & 1) != 0 ? %sCD : %sAB;\n",
					type, prefix, prefix, prefix);
		}
		else
		{
			xgpu_text_append(text, "\t\t%s %sSUM = %sAB + %sCD;\n", type, prefix, prefix, prefix);
		}
		snprintf(mapped, sizeof(mapped), mapping, "%sAB");
		xgpu_text_append(text, "\t\t%sAB = clamp(", prefix);
		xgpu_text_append(text, mapped, prefix);
		xgpu_text_append(text, ", -1.0, 1.0);\n");
		snprintf(mapped, sizeof(mapped), mapping, "%sCD");
		xgpu_text_append(text, "\t\t%sCD = clamp(", prefix);
		xgpu_text_append(text, mapped, prefix);
		xgpu_text_append(text, ", -1.0, 1.0);\n");
		snprintf(mapped, sizeof(mapped), mapping, "%sSUM");
		xgpu_text_append(text, "\t\t%sSUM = clamp(", prefix);
		xgpu_text_append(text, mapped, prefix);
		xgpu_text_append(text, ", -1.0, 1.0);\n");
	}

	/* write back only after both portions have read their inputs */
	for (portion = 0; portion < 2; portion++)
	{
		BOOL alpha = portion == 1;
		DWORD outputs = alpha ? state[D3DRS_PSALPHAOUTPUTS0 + stage] : state[D3DRS_PSRGBOUTPUTS0 + stage];
		unsigned long flags = outputs >> 12;
		const char *prefix = alpha ? "a" : "c";
		const char *component = alpha ? "a" : "rgb";
		const char *ab = destination_name((outputs >> 4) & 0x0f);
		const char *cd = destination_name(outputs & 0x0f);
		const char *sum = destination_name((outputs >> 8) & 0x0f);

		if (ab)
		{
			xgpu_text_append(text, "\t\t%s.%s = %sAB;\n", ab, component, prefix);
			if (!alpha && (flags & 0x80))
				xgpu_text_append(text, "\t\t%s.a = cAB.b;\n", ab);
		}
		if (cd)
		{
			xgpu_text_append(text, "\t\t%s.%s = %sCD;\n", cd, component, prefix);
			if (!alpha && (flags & 0x40))
				xgpu_text_append(text, "\t\t%s.a = cCD.b;\n", cd);
		}
		if (sum)
			xgpu_text_append(text, "\t\t%s.%s = %sSUM;\n", sum, component, prefix);
	}
	xgpu_text_append(text, "\t}\n");
}

/* ---------- texture stages */

static const char *sampler_declaration(unsigned char type)
{
	switch (type)
	{
	case _xgpu_sampler_3d: return "sampler3D";
	case _xgpu_sampler_cube: return "samplerCube";
	default: return "sampler2D";
	}
}

static unsigned long stage_mode(const struct nv2a_pixel_shader_key *key, int stage)
{
	return (key->texture_modes >> (5 * stage)) & 0x1f;
}

static int stage_input(const DWORD *state, int stage)
{
	switch (stage)
	{
	case 2: return (state[D3DRS_PSINPUTTEXTURE] >> 16) & 1;
	case 3: return (state[D3DRS_PSINPUTTEXTURE] >> 20) & 3;
	default: return 0;
	}
}

static BOOL mode_samples_texture(unsigned long mode)
{
	return mode != _mode_none && mode != _mode_passthru &&
		mode != _mode_clipplane && mode != _mode_dot_product && mode != _mode_dot_zw;
}

/* A lens-flare occlusion test draws no texture and writes neither color nor
 * depth. The visibility count has to be the samples that pass the depth test.
 * Writing a sample mask forces that test to wait until after the shader, and
 * on this GPU the count then includes samples the hull should have hidden. */
static BOOL msl_depth_test_before_shader(const struct nv2a_pixel_shader_key *key)
{
	int stage;

	if (key->alpha_test_function)
		return FALSE;
	for (stage = 0; stage < 4; stage++)
	{
		unsigned long mode = stage_mode(key, stage);

		if (mode == _mode_dot_zw || mode == _mode_clipplane || mode_samples_texture(mode))
			return FALSE;
	}
	return TRUE;
}

/* the input texel of a dot product stage, mapped per PSDotMapping */
static void dot_input(struct xgpu_text *text, const DWORD *state, int stage,
	enum shader_language language)
{
	unsigned long mapping = (state[D3DRS_PSDOTMAPPING] >> ((stage - 1) * 4)) & 7;
	int input = stage_input(state, stage);

	switch (mapping)
	{
	case 0: xgpu_text_append(text, "t%d.rgb", input); break;
	case 1: xgpu_text_append(text, "((t%d.rgb * 255.0 - 128.0) / 127.0)", input); break;
	case 2:
		if (language == _shader_msl)
			xgpu_text_append(text, "gl_signed_bytes(t%d.rgb)", input);
		else
			xgpu_text_append(text, "(t%d.rgb * 2.0 - 1.0)", input);
		break;
	case 3: xgpu_text_append(text, "signed_bytes(t%d.rgb)", input); break;
	default: xgpu_text_append(text, "(t%d.rgb * 2.0 - 1.0)", input); break;
	}
}

#ifdef HALO_ANDROID
/* ES samplers have no LOD bias: pass D3DTSS_MIPMAPLODBIAS to the lookup */
#define SAMPLE_BIAS ", texture_lod_bias[%d]"
#define SHADER_VERSION \
	"precision highp float;\n" \
	"precision highp int;\n" \
	"precision highp sampler2D;\n" \
	"precision highp sampler3D;\n" \
	"precision highp samplerCube;\n"
#else
#define SAMPLE_BIAS ""
#define SHADER_VERSION "#version 450 core\n"
#endif

/* A clamp-to-edge sample already has the correct interior/edge texel value.
For a single level, blending it with the border by the footprint's coverage
exactly restores clamp-to-border, including the half-texel linear fringe.
Only unsupported GLES border samplers receive this helper (d3d8_gl.c). */
static void border_sample_function(struct xgpu_text *text, const struct nv2a_pixel_shader_key *key, int stage)
{
	unsigned char axes = key->border_axes[stage];
	unsigned char filtering = key->border_filter[stage];

	if (!axes)
		return;
	if (filtering == 4)
	{
		/* Optional HUD redraws use a complete, unbiased mip chain. Correct
		 * the bilinear border footprint separately at each selected level;
		 * a level-zero coverage mask is wrong across mip transitions. */
		xgpu_text_append(text,
			"vec4 sample_border_level%d(vec2 uv, int mip)\n{\n"
			"\tvec4 value = textureLod(tex%d, uv, float(mip));\n"
			"\tvec2 size = vec2(textureSize(tex%d, mip));\n"
			"\tvec2 coverage = clamp(vec2(0.5) + min(uv, vec2(1.0) - uv) * size, 0.0, 1.0);\n"
			"\treturn mix(texture_border_color[%d], value, %s);\n}\n"
			"vec4 sample_border%d(vec2 uv)\n{\n"
			"\tvec2 size = vec2(textureSize(tex%d, 0));\n"
			"\tvec2 dx = dFdx(uv) * size, dy = dFdy(uv) * size;\n"
			"\tfloat lod = clamp(0.5 * log2(max(max(dot(dx, dx), dot(dy, dy)), 1.0)),\n"
			"\t\t0.0, floor(log2(max(size.x, size.y))));\n"
			"\tint low = int(floor(lod)), high = int(ceil(lod));\n"
			"\treturn mix(sample_border_level%d(uv, low), sample_border_level%d(uv, high), fract(lod));\n}\n",
			stage, stage, stage, stage,
			axes == 3 ? "coverage.x * coverage.y" : axes == 1 ? "coverage.x" : "coverage.y",
			stage, stage, stage, stage);
		return;
	}
	xgpu_text_append(text,
		"vec4 sample_border%d(vec2 uv)\n{\n"
		"\tvec4 value = texture(tex%d, uv, texture_lod_bias[%d]);\n"
		"\tvec2 size = vec2(textureSize(tex%d, 0));\n", stage, stage, stage, stage);
	if (filtering == 1 || filtering == 2)
	{
		xgpu_text_append(text,
			"\tvec2 dx = dFdx(uv) * size, dy = dFdy(uv) * size;\n"
			"\tbool minifying = max(dot(dx, dx), dot(dy, dy)) * exp2(2.0 * texture_lod_bias[%d]) > 1.0;\n"
			"\tbool linear_filter = %sminifying;\n", stage, filtering == 1 ? "" : "!");
	}
	else
		xgpu_text_append(text, "\tbool linear_filter = %s;\n", filtering == 3 ? "true" : "false");
	xgpu_text_append(text,
		"\tvec2 coverage = linear_filter ? clamp(vec2(0.5) + min(uv, vec2(1.0) - uv) * size, 0.0, 1.0)\n"
		"\t\t: step(vec2(0.0), uv) * (vec2(1.0) - step(vec2(1.0), uv));\n"
		"\treturn mix(texture_border_color[%d], value, %s);\n}\n", stage,
		axes == 3 ? "coverage.x * coverage.y" : axes == 1 ? "coverage.x" : "coverage.y");
}

static void sample(struct xgpu_text *text, const struct nv2a_pixel_shader_key *key, int stage,
	const char *coordinates, enum shader_language language, unsigned int alpha_border_mask,
	unsigned int volume_border_mask)
{
	if (language == _shader_msl)
	{
		if (volume_border_mask & (1u << stage))
		{
			xgpu_text_append(text, "sample_volume_border%d(tex%d, sampler%d, volume_border_sampler%d, u, (%s).xyz)",
				stage, stage, stage, stage, coordinates);
		}
		else if (alpha_border_mask & (1u << stage))
		{
			xgpu_text_append(text, "sample_alpha_border%d(tex%d, sampler%d, alpha_border_sampler%d, u, (%s).xy * u.texture_scale[%d].xy)",
				stage, stage, stage, stage, coordinates, stage);
		}
		else if (key->border_axes[stage])
		{
			xgpu_text_append(text, "sample_border%d(tex%d, sampler%d, u, (%s).xy * u.texture_scale[%d].xy)",
				stage, stage, stage, coordinates, stage);
		}
		else if (key->sampler_type[stage] == _xgpu_sampler_2d)
		{
			xgpu_text_append(text, "tex%d.sample(sampler%d, (%s).xy * u.texture_scale[%d].xy, bias(u.texture_lod_bias[%d]))",
				stage, stage, coordinates, stage, stage);
		}
		else
		{
			xgpu_text_append(text, "tex%d.sample(sampler%d, (%s).xyz, bias(u.texture_lod_bias[%d]))",
				stage, stage, coordinates, stage);
		}
		return;
	}
	if (key->border_axes[stage])
	{
		xgpu_text_append(text, "sample_border%d((%s).xy * texture_scale[%d].xy)", stage, coordinates, stage);
		return;
	}
	switch (key->sampler_type[stage])
	{
	case _xgpu_sampler_3d:
		xgpu_text_append(text, "texture(tex%d, (%s).xyz" SAMPLE_BIAS ")", stage, coordinates
#ifdef HALO_ANDROID
			, stage
#endif
			);
		break;
	case _xgpu_sampler_cube:
		xgpu_text_append(text, "texture(tex%d, (%s).xyz" SAMPLE_BIAS ")", stage, coordinates
#ifdef HALO_ANDROID
			, stage
#endif
			);
		break;
	default:
		xgpu_text_append(text, "texture(tex%d, (%s).xy * texture_scale[%d].xy" SAMPLE_BIAS ")", stage, coordinates, stage
#ifdef HALO_ANDROID
			, stage
#endif
			);
		break;
	}
}

static void texture_stage(struct xgpu_text *text, const struct nv2a_pixel_shader_key *key, int stage,
	enum shader_language language, unsigned int alpha_border_mask, unsigned int volume_border_mask)
{
	const DWORD *state = key->combiner_state;
	unsigned long mode = stage_mode(key, stage);
	char coordinates[96];
	const char *vec2 = vector_type(language, 2);
	const char *vec3 = vector_type(language, 3);
	const char *vec4 = vector_type(language, 4);
	const char *u = uniform_prefix(language);

	xgpu_text_append(text, "\t/* texture stage %d, mode %lu */\n", stage, mode);
	if (key->sampler_type[stage] == _xgpu_sampler_none &&
		mode != _mode_passthru && mode != _mode_clipplane && mode != _mode_dot_product && mode != _mode_dot_zw)
	{
		mode = _mode_none;
	}
	switch (mode)
	{
	case _mode_project2d:
	case _mode_project3d:
		snprintf(coordinates, sizeof(coordinates), "%s(xT%d.xyz / (xT%d.w != 0.0 ? xT%d.w : 1.0), 1.0)", vec4, stage, stage, stage);
		xgpu_text_append(text, "\tt%d = ", stage);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		/* Stage 0 only. A Custom Edition meter sheet still in the Xbox
		channel order is read as (alpha, alpha, alpha, red). A high-res
		meter's red holds exact segment ids, so that channel is fetched at
		the texel instead of averaged with its neighbors. */
		if (stage == 0 && key->sampler_type[stage] == _xgpu_sampler_2d &&
			(key->ce_meter_channels || key->point_threshold))
		{
			if (key->ce_meter_channels)
				xgpu_text_append(text, "\tt0 = t0.aaar;\n");
			if (key->point_threshold)
			{
				if (language == _shader_msl)
				{
					xgpu_text_append(text,
						"\t{\n"
						"\t\tfloat2 uv = (%s).xy * u.texture_scale[0].xy;\n"
						"\t\tfloat2 size = float2(tex0.get_width(), tex0.get_height());\n"
						"\t\tint2 pixel = clamp(int2(floor(uv * size)), int2(0), int2(size) - int2(1));\n"
						"\t\tfloat threshold = tex0.sample(meter_nearest, (float2(pixel) + 0.5) / size, level(0.0)).r;\n"
						"\t\tif (threshold > 0.0) t0.rb = float2(threshold);\n"
						"\t}\n",
						coordinates);
				}
				else
				{
					xgpu_text_append(text,
						"\t{\n"
						"\t\tvec2 uv = (%s).xy * texture_scale[0].xy;\n"
						"\t\tivec2 size = textureSize(tex0, 0);\n"
						"\t\tivec2 pixel = clamp(ivec2(floor(uv * vec2(size))), ivec2(0), size - ivec2(1));\n"
						"\t\tfloat threshold = texelFetch(tex0, pixel, 0).r;\n"
						"\t\tif (threshold > 0.0) t0.rb = vec2(threshold);\n"
						"\t}\n",
						coordinates);
				}
			}
		}
		break;
	case _mode_cubemap:
		snprintf(coordinates, sizeof(coordinates), "xT%d", stage);
		xgpu_text_append(text, "\tt%d = ", stage);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	case _mode_passthru:
		if (language == _shader_msl)
			xgpu_text_append(text, "\tt%d = xT%d;\n", stage, stage);
		else
			xgpu_text_append(text, "\tt%d = clamp(xT%d, 0.0, 1.0);\n", stage, stage);
		break;
	case _mode_clipplane:
	{
		unsigned long compare = (state[D3DRS_PSCOMPAREMODE] >> (4 * stage)) & 0xf;
		static const char components[] = "xyzw";
		int component;

		for (component = 0; component < 4; component++)
		{
			xgpu_text_append(text, "\tif (xT%d.%c %s 0.0) %s;\n", stage, components[component],
				(compare & (1 << component)) ? ">=" : "<", discard_statement(language));
		}
		xgpu_text_append(text, "\tt%d = %s(0.0);\n", stage, vec4);
		break;
	}
	case _mode_bumpenvmap:
	case _mode_bumpenvmap_luminance:
	{
		int input = language == _shader_msl ? stage_input(state, stage) : stage - 1;

		if (language == _shader_msl)
		{
			unsigned char signed_channels = mode_samples_texture(stage_mode(key, input)) ? key->color_sign[input] : 0;
			xgpu_text_append(text, "\t{\n\t\t%s d = %s(", vec2, vec2);
			xgpu_text_append(text, (signed_channels & 8) ? "t%d.b" : "signed_byte(t%d.b)", input);
			xgpu_text_append(text, ", ");
			xgpu_text_append(text, (signed_channels & 4) ? "t%d.g" : "signed_byte(t%d.g)", input);
			xgpu_text_append(text, ");\n");
		}
		else
			xgpu_text_append(text, "\t{\n\t\t%s d = signed_bytes(t%d.rgb).rg;\n", vec2, input);
		xgpu_text_append(text, "\t\t%s coordinates = xT%d.xy + %s(%sbump_matrix[%d].x * d.x + %sbump_matrix[%d].z * d.y,"
			" %sbump_matrix[%d].y * d.x + %sbump_matrix[%d].w * d.y);\n", vec2, stage, vec2, u, stage, u, stage, u, stage, u, stage);
		xgpu_text_append(text, "\t\tt%d = ", stage);
		snprintf(coordinates, sizeof(coordinates), "%s(coordinates, 0.0, 1.0)", vec4);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		if (mode == _mode_bumpenvmap_luminance)
		{
			if (language == _shader_msl)
				xgpu_text_append(text, "\t\tt%d *= %sbump_luminance[%d].x * t%d.r + %sbump_luminance[%d].y;\n",
					stage, u, stage, input, u, stage);
			else
				xgpu_text_append(text, "\t\tt%d.rgb *= clamp(%sbump_luminance[%d].x * t%d.b + %sbump_luminance[%d].y, 0.0, 1.0);\n",
					stage, u, stage, input, u, stage);
		}
		xgpu_text_append(text, "\t}\n");
		break;
	}
	case _mode_dot_product:
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\tt%d = %s(0.0);\n", stage, vec4);
		break;
	case _mode_dot_st:
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\tt%d = ", stage);
		snprintf(coordinates, sizeof(coordinates), "%s(dot%d, dot%d, 0.0, 1.0)", vec4, stage - 1, stage);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	case _mode_dot_zw:
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\tt%d = %s(0.0);\n", stage, vec4);
		if (language == _shader_msl)
			xgpu_text_append(text,
				"\treplacement_depth = (dot%d / dot%d) * u.depth_scale;\n"
				"\tif (!isfinite(replacement_depth) || replacement_depth < u.depth_clip_min ||"
				" replacement_depth > u.depth_clip_max) discard_fragment();\n", stage - 1, stage);
		else if (stage >= 1)
			/* window depth, 0 at the near plane, in the Xbox's 24-bit units.
			A fragment behind the eye (the divisor not positive) is the far
			plane. gl_FragDepth writes it at the end of main. */
			xgpu_text_append(text,
				"\treplaced_depth = dot%d > 0.0 ? clamp(dot%d / dot%d / 16777215.0, 0.0, 1.0) : 1.0;\n",
				stage, stage - 1, stage);
		break;
	case _mode_dot_reflect_diffuse:
		/* the normal takes its third component from stage 3's dot product */
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\tdot3 = dot(xT3.xyz, ");
		dot_input(text, state, 3, language);
		xgpu_text_append(text, ");\n\tt%d = ", stage);
		snprintf(coordinates, sizeof(coordinates), "%s(dot1, dot2, dot3, 1.0)", vec4);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	case _mode_dot_reflect_specular:
	case _mode_dot_reflect_specular_constant:
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\t{\n\t\t%s n = %s(dot1, dot2, dot3);\n", vec3, vec3);
		if (mode == _mode_dot_reflect_specular)
			xgpu_text_append(text, "\t\t%s e = %s(xT1.w, xT2.w, xT3.w);\n", vec3, vec3);
		else
			xgpu_text_append(text, "\t\t%s e = %sps_c0[0].xyz;\n", vec3, u);
		xgpu_text_append(text, "\t\t%s r = 2.0 * n * dot(n, e) / max(dot(n, n), 1.0e-20) - e;\n\t\tt%d = ", vec3, stage);
		snprintf(coordinates, sizeof(coordinates), "%s(r, 1.0)", vec4);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n\t}\n");
		break;
	case _mode_dot_str_3d:
	case _mode_dot_str_cube:
		xgpu_text_append(text, "\tdot%d = dot(xT%d.xyz, ", stage, stage);
		dot_input(text, state, stage, language);
		xgpu_text_append(text, ");\n\tt%d = ", stage);
		snprintf(coordinates, sizeof(coordinates), "%s(dot1, dot2, dot3, 1.0)", vec4);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	case _mode_dependent_ar:
		snprintf(coordinates, sizeof(coordinates), "%s(t%d.a, t%d.r, 0.0, 1.0)", vec4, stage_input(state, stage), stage_input(state, stage));
		xgpu_text_append(text, "\tt%d = ", stage);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	case _mode_dependent_gb:
		snprintf(coordinates, sizeof(coordinates), "%s(t%d.g, t%d.b, 0.0, 1.0)", vec4, stage_input(state, stage), stage_input(state, stage));
		xgpu_text_append(text, "\tt%d = ", stage);
		sample(text, key, stage, coordinates, language, alpha_border_mask, volume_border_mask);
		xgpu_text_append(text, ";\n");
		break;
	default:
		if (language == _shader_msl && mode == _mode_none)
			xgpu_text_append(text, "\tt%d = %s(0.0, 0.0, 0.0, 1.0);\n", stage, vec4);
		else
			xgpu_text_append(text, "\tt%d = %s(0.0);\n", stage, vec4);
		break;
	}

	/* channels the application marked signed (D3DTSS_COLORSIGN) */
	if (key->color_sign[stage] && mode != _mode_none &&
		(language == _shader_glsl || mode_samples_texture(mode)))
	{
		static const char channels[] = "argb";
		int bit;

		for (bit = 0; bit < 4; bit++)
		{
			if (key->color_sign[stage] & (1 << bit))
				xgpu_text_append(text, "\tt%d.%c = signed_byte(t%d.%c);\n", stage, channels[bit], stage, channels[bit]);
		}
	}
	if (key->alpha_kill[stage] && mode != _mode_none &&
		(language == _shader_glsl || mode_samples_texture(mode)))
		xgpu_text_append(text, "\tif (t%d.a == 0.0) %s;\n", stage, discard_statement(language));
}

/* ---------- the whole shader */

static const char *comparison_operator(unsigned long function)
{
	switch (function)
	{
	case D3DCMP_NEVER: return NULL;
	case D3DCMP_LESS: return "<";
	case D3DCMP_EQUAL: return "==";
	case D3DCMP_LESSEQUAL: return "<=";
	case D3DCMP_GREATER: return ">";
	case D3DCMP_NOTEQUAL: return "!=";
	case D3DCMP_GREATEREQUAL: return ">=";
	default: return "";
	}
}

static void pixel_shader_body(struct xgpu_text *text, const struct nv2a_pixel_shader_key *key,
	enum shader_language language, unsigned int alpha_border_mask, unsigned int volume_border_mask)
{
	const DWORD *state = key->combiner_state;
	unsigned long combiner_count = state[D3DRS_PSCOMBINERCOUNT] & 0xff;
	DWORD final_abcd = state[D3DRS_PSFINALCOMBINERINPUTSABCD];
	DWORD final_efg = state[D3DRS_PSFINALCOMBINERINPUTSEFG];
	const char *vec3 = vector_type(language, 3);
	const char *vec4 = vector_type(language, 4);
	const char *u = uniform_prefix(language);
	int stage;
	if (combiner_count > 8) combiner_count = 8;
	xgpu_text_append(text,
		"\t%s v0 = xD0;\n\t%s v1 = xD1;\n"
		"\t%s t0 = %s(0.0), t1 = %s(0.0), t2 = %s(0.0), t3 = %s(0.0);\n"
		"\tfloat dot0 = 0.0, dot1 = 0.0, dot2 = 0.0, dot3 = 0.0;\n",
		vec4, vec4, vec4, vec4, vec4, vec4, vec4);
	for (stage = 0; stage < 4; stage++)
		texture_stage(text, key, stage, language, alpha_border_mask, volume_border_mask);

	/* the fog register: rgb is the fog color, alpha the fog factor */
	if (key->fog_enable)
	{
		switch (key->fog_table_mode)
		{
		case D3DFOG_EXP:
			xgpu_text_append(text, "\tfloat fog_factor = exp(-%sfog_parameters.z * xFog);\n", u);
			break;
		case D3DFOG_EXP2:
			xgpu_text_append(text, "\tfloat fog_factor = exp(-(%sfog_parameters.z * xFog) * (%sfog_parameters.z * xFog));\n", u, u);
			break;
		case D3DFOG_LINEAR:
			xgpu_text_append(text, "\tfloat fog_factor = (%sfog_parameters.y - xFog) / max(%sfog_parameters.y - %sfog_parameters.x, 1.0e-6);\n", u, u, u);
			break;
		default:
			xgpu_text_append(text, "\tfloat fog_factor = xFog;\n");
			break;
		}
	}
	else
	{
		xgpu_text_append(text, "\tfloat fog_factor = 1.0;\n");
	}
	xgpu_text_append(text,
		"\t%s fog = %s(%sfog_color.rgb, clamp(fog_factor, 0.0, 1.0));\n"
		"\t%s r0 = %s(0.0, 0.0, 0.0, t0.a);\n"
		"\t%s r1 = %s(0.0);\n", vec4, vec4, u, vec4, vec4, vec4, vec4);

	for (stage = 0; stage < (int)combiner_count; stage++)
		combiner_stage(text, state, stage, language);

	if (final_abcd == 0 && final_efg == 0)
	{
		xgpu_text_append(text, "\t%s result = r0;\n", vec4);
	}
	else
	{
		unsigned long settings = final_efg & 0xff;

		xgpu_text_append(text, "\t%s ef_product = %s(", vec4, vec4);
		final_input(text, (final_efg >> 24) & 0xff, FALSE, language);
		xgpu_text_append(text, " * ");
		final_input(text, (final_efg >> 16) & 0xff, FALSE, language);
		xgpu_text_append(text, ", 0.0);\n");
		if (language == _shader_msl)
			xgpu_text_append(text, "\t%s v1r0_sum = %s(%s + %s, 0.0);\n", vec4, vec4,
				(settings & 0x40) ? "(1.0 - v1.rgb)" : "v1.rgb",
				(settings & 0x20) ? "(1.0 - r0.rgb)" : "r0.rgb");
		else
			xgpu_text_append(text, "\t%s v1r0_sum = %s(%s + %s, 0.0);\n", vec4, vec4,
				(settings & 0x40) ? "(1.0 - clamp(v1.rgb, 0.0, 1.0))" : "clamp(v1.rgb, 0.0, 1.0)",
				(settings & 0x20) ? "(1.0 - clamp(r0.rgb, 0.0, 1.0))" : "clamp(r0.rgb, 0.0, 1.0)");
		if (settings & 0x80)
			xgpu_text_append(text, "\tv1r0_sum = clamp(v1r0_sum, 0.0, 1.0);\n");
		xgpu_text_append(text, "\t%s fA = ", vec3);
		final_input(text, (final_abcd >> 24) & 0xff, FALSE, language);
		xgpu_text_append(text, ";\n\t%s fB = ", vec3);
		final_input(text, (final_abcd >> 16) & 0xff, FALSE, language);
		xgpu_text_append(text, ";\n\t%s fC = ", vec3);
		final_input(text, (final_abcd >> 8) & 0xff, FALSE, language);
		xgpu_text_append(text, ";\n\t%s fD = ", vec3);
		final_input(text, final_abcd & 0xff, FALSE, language);
		xgpu_text_append(text, ";\n\tfloat fG = ");
		final_input(text, (final_efg >> 8) & 0xff, TRUE, language);
		xgpu_text_append(text, ";\n\t%s result = %s(fA * fB + (1.0 - fA) * fC + fD, fG);\n", vec4, vec4);
	}

	if (key->coverage_alpha)
		xgpu_text_append(text, "\tresult.a = mix(1.0, result.a, t0.g);\n");
	if (key->alpha_test_function)
	{
		const char *comparison = comparison_operator(key->alpha_test_function);

		if (!comparison)
			xgpu_text_append(text, "\t%s;\n", discard_statement(language));
		else if (*comparison)
			xgpu_text_append(text, "\tif (!(floor(clamp(result.a, 0.0, 1.0) * 255.0 + 0.5) %s %salpha_reference)) %s;\n", comparison, u, discard_statement(language));
	}
	if (*config_string("debug.gpu_debug_expression"))
		xgpu_text_append(text, "\tresult = %s(%s(%s), 1.0);\n", vec4, vec3, config_string("debug.gpu_debug_expression"));
	if (config_boolean("debug.gpu_debug_texture0"))
		xgpu_text_append(text, "\tresult = %s(t0.rgb, 1.0);\n", vec4);
	if (config_boolean("debug.gpu_debug_flat"))
		xgpu_text_append(text, "\tresult = xD0.a > 0.0 ? %s(xD0.rgb, 1.0) : %s(1.0, 0.0, 1.0, 1.0);\n", vec4, vec4);
}

char *nv2a_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *key)
{
	struct xgpu_text text = { 0 };
	int stage;

#ifdef HALO_ANDROID
	xgpu_text_append(&text, "#version %s\n", xgpu_capabilities.shading_language);
	if (key->count_samples)
	{
		/* samples that pass the depth and stencil tests, as the NV2A's
		occlusion counter */
		xgpu_text_append(&text,
			"layout(early_fragment_tests) in;\n"
			"layout(binding = 0, offset = 0) uniform atomic_uint visible_samples;\n");
	}
#endif
	xgpu_text_append(&text,
		SHADER_VERSION
		"in vec4 xD0;\n"
		"in vec4 xD1;\n"
		"in vec4 xB0;\n"
		"in vec4 xB1;\n"
		"in vec4 xT0;\n"
		"in vec4 xT1;\n"
		"in vec4 xT2;\n"
		"in vec4 xT3;\n"
		"in float xFog;\n"
		"layout(location = 0) out vec4 fragment_color;\n"
		XGPU_PIXEL_UNIFORMS);
	for (stage = 0; stage < 4; stage++)
		xgpu_text_append(&text, "uniform %s tex%d;\n", sampler_declaration(key->sampler_type[stage]), stage);
	for (stage = 0; stage < 4; stage++)
		border_sample_function(&text, key, stage);
	xgpu_text_append(&text,
		"float signed_byte(float x)\n"
		"{\n"
		"	float b = floor(x * 255.0 + 0.5);\n"
		"	return (b >= 128.0 ? b - 256.0 : b) / 127.0;\n"
		"}\n"
		"vec3 signed_bytes(vec3 x)\n"
		"{\n"
		"	return vec3(signed_byte(x.r), signed_byte(x.g), signed_byte(x.b));\n"
		"}\n"
		"void main()\n"
		"{\n");
	{
		int stage_index;
		int replaces_depth = 0;

		for (stage_index = 1; stage_index < 4; stage_index++)
			if (stage_mode(key, stage_index) == _mode_dot_zw)
				replaces_depth = 1;
		if (replaces_depth)
			xgpu_text_append(&text, "\tfloat replaced_depth = 1.0;\n");
		pixel_shader_body(&text, key, _shader_glsl, 0, 0);
#ifdef HALO_ANDROID
		if (key->count_samples)
			xgpu_text_append(&text, "\tatomicCounterIncrement(visible_samples);\n");
#endif
		xgpu_text_append(&text, "\tfragment_color = clamp(result, 0.0, 1.0);\n");
		if (replaces_depth)
			xgpu_text_append(&text, "\tgl_FragDepth = %sreplaced_depth;\n",
				xgpu_reversed_depth ? "1.0 - " : "");
		xgpu_text_append(&text, "}\n");
	}
	return text.buffer;
}

/* Metal rejects states whose NV2A behavior is not yet represented. This is
 * deliberately separate from the legacy GL fallback behavior: an unsupported
 * draw must be reported by the native backend, never become a black texel. */
static BOOL msl_input_register_valid(unsigned long input, BOOL final)
{
	unsigned long reg = input & 15;
	if (reg == _register_zero || (reg >= _register_c0 && reg <= _register_v1) ||
		(reg >= _register_t0 && reg <= _register_r1))
		return TRUE;
	return final && (reg == _register_v1r0_sum || reg == _register_ef_product);
}

static BOOL msl_output_register_valid(unsigned long reg)
{
	return reg == _register_zero || destination_name(reg) != NULL;
}

static BOOL msl_key_supported(const struct nv2a_pixel_shader_key *key,
	const struct nv2a_pixel_shader_msl_options *options)
{
	const DWORD *state;
	unsigned long count;
	unsigned int alpha_border_mask = 0, volume_border_mask = 0;
	int stage, portion, component;
	if (!key || key->count_samples || *config_string("debug.gpu_debug_expression"))
		return FALSE;
	if (options)
	{
		if (options->version == 1)
		{
			/* Do not read a third word from original version1 callers. */
			if (options->depth_contract != XGPU_MSL_DEPTH_RAW_D24) return FALSE;
		}
		else if (options->version == 2 || options->version == 3)
		{
			/* Version2 callers own exactly three words. Read the fourth
			 * only after establishing the version3 ABI. */
			alpha_border_mask = options->native_alpha_border_mask;
			if (options->version == 3) volume_border_mask = options->native_volume_border_mask;
			if (options->depth_contract > XGPU_MSL_DEPTH_RAW_D24 ||
				((alpha_border_mask | volume_border_mask) & ~15u) ||
				(alpha_border_mask & volume_border_mask)) return FALSE;
		}
		else return FALSE;
	}
	state = key->combiner_state;
	count = state[D3DRS_PSCOMBINERCOUNT] & 255;
	if (count > 8 || (state[D3DRS_PSCOMBINERCOUNT] & ~0x111ffu) ||
		(key->texture_modes & ~0xfffffu) ||
		(key->alpha_test_function && (key->alpha_test_function < D3DCMP_NEVER ||
		key->alpha_test_function > D3DCMP_ALWAYS)) ||
		(key->fog_enable && key->fog_table_mode > D3DFOG_LINEAR))
		return FALSE;
	for (stage = 0; stage < 4; stage++)
	{
		unsigned long mode = stage_mode(key, stage);
		BOOL samples = mode_samples_texture(mode);
		if (mode >= _mode_dot_reflect_specular_constant || mode == _mode_brdf ||
			(mode == _mode_dot_zw && (!options ||
			options->depth_contract != XGPU_MSL_DEPTH_RAW_D24)) ||
			(stage == 0 && mode >= _mode_bumpenvmap) ||
			key->sampler_type[stage] > _xgpu_sampler_cube ||
			(samples && key->sampler_type[stage] == _xgpu_sampler_none) ||
			(key->color_sign[stage] & ~15u) || key->border_axes[stage] > 3 ||
			key->border_filter[stage] > 3 ||
			(key->border_axes[stage] && key->sampler_type[stage] != _xgpu_sampler_2d))
			return FALSE;
		if ((alpha_border_mask & (1u << stage)) &&
			(!samples || key->sampler_type[stage] != _xgpu_sampler_2d ||
			key->border_axes[stage] || key->border_filter[stage])) return FALSE;
		if ((volume_border_mask & (1u << stage)) &&
			(!samples || key->sampler_type[stage] != _xgpu_sampler_3d ||
			key->border_axes[stage] || key->border_filter[stage])) return FALSE;
		if (((mode == _mode_dot_st || mode == _mode_dot_zw) && stage < 2) ||
			(mode == _mode_dot_reflect_diffuse && stage != 2) ||
			((mode == _mode_dot_reflect_specular || mode == _mode_dot_reflect_specular_constant ||
			mode == _mode_dot_str_3d || mode == _mode_dot_str_cube) && stage != 3) ||
			(mode == _mode_dot_product && stage != 1 && stage != 2))
			return FALSE;
		/* A dependent DOT lookup consumes the preceding stages' scalar
		 * results, not their RGBA registers. Reject inconsistent chains
		 * rather than substitute the zero-initialized local dot variables.
		 * NV_texture_shader sections 3.8.13.1.15 and .17-.20 describe these
		 * stage dependencies; the game's bumped reflections use 17/11/12. */
		if ((mode == _mode_dot_st || mode == _mode_dot_zw) &&
			stage_mode(key, stage - 1) != _mode_dot_product)
			return FALSE;
		if (mode == _mode_dot_reflect_diffuse &&
			(stage_mode(key, 1) != _mode_dot_product ||
			stage_mode(key, 3) != _mode_dot_reflect_specular)) return FALSE;
		if (mode == _mode_dot_reflect_specular &&
			(stage_mode(key, 1) != _mode_dot_product ||
			(stage_mode(key, 2) != _mode_dot_product &&
			stage_mode(key, 2) != _mode_dot_reflect_diffuse))) return FALSE;
		if ((mode == _mode_dot_str_3d || mode == _mode_dot_str_cube) &&
			(stage_mode(key, 1) != _mode_dot_product ||
			stage_mode(key, 2) != _mode_dot_product)) return FALSE;
		if ((mode == _mode_project2d || mode == _mode_bumpenvmap || mode == _mode_bumpenvmap_luminance ||
			mode == _mode_dot_st || mode == _mode_dependent_ar || mode == _mode_dependent_gb) &&
			key->sampler_type[stage] != _xgpu_sampler_2d) return FALSE;
		if ((mode == _mode_project3d || mode == _mode_dot_str_3d) &&
			key->sampler_type[stage] != _xgpu_sampler_3d) return FALSE;
		if ((mode == _mode_cubemap || mode == _mode_dot_reflect_diffuse || mode == _mode_dot_reflect_specular ||
			mode == _mode_dot_str_cube || mode == _mode_dot_reflect_specular_constant) &&
			key->sampler_type[stage] != _xgpu_sampler_cube) return FALSE;
		if (stage && mode >= _mode_bumpenvmap && stage_input(state, stage) >= stage) return FALSE;
		if (mode == _mode_bumpenvmap_luminance &&
			(key->color_sign[stage] ||
			(mode_samples_texture(stage_mode(key, stage_input(state, stage))) &&
			(key->color_sign[stage_input(state, stage)] & 2)))) return FALSE;
		if ((mode >= _mode_dot_st && mode <= _mode_dot_str_cube) ||
			mode == _mode_dot_product || mode == _mode_dot_reflect_specular_constant)
		{
			unsigned long mapping = (state[D3DRS_PSDOTMAPPING] >> ((stage - 1) * 4)) & 15;
			if (mapping > 3) return FALSE;
			/* The unsigned-texture upload contract does not yet establish
			 * the ordering of COLOR_SIGN conversion and dot mapping. */
			if (mode_samples_texture(stage_mode(key, stage_input(state, stage))) &&
				key->color_sign[stage_input(state, stage)]) return FALSE;
			/* Diffuse reflection also reads stage 3's dot input before the
			 * stage 3 texture operation runs. */
			if (mode == _mode_dot_reflect_diffuse && stage_input(state, 3) >= stage) return FALSE;
			if (mode == _mode_dot_reflect_diffuse &&
				((state[D3DRS_PSDOTMAPPING] >> 8) & 15) > 3) return FALSE;
			if (mode == _mode_dot_reflect_diffuse &&
				mode_samples_texture(stage_mode(key, stage_input(state, 3))) &&
				key->color_sign[stage_input(state, 3)]) return FALSE;
		}
	}
	for (stage = 0; stage < (int)count; stage++)
	{
		for (portion = 0; portion < 2; portion++)
		{
			DWORD inputs = state[(portion ? D3DRS_PSALPHAINPUTS0 : D3DRS_PSRGBINPUTS0) + stage];
			DWORD outputs = state[(portion ? D3DRS_PSALPHAOUTPUTS0 : D3DRS_PSRGBOUTPUTS0) + stage];
			unsigned long flags = outputs >> 12;
			unsigned long mapping = flags & 0x38;
			if (flags > 255 || mapping == 0x28 || mapping == 0x38 ||
				(portion && (flags & 0xc3)) ||
				!msl_output_register_valid(outputs & 15) ||
				!msl_output_register_valid((outputs >> 4) & 15) ||
				!msl_output_register_valid((outputs >> 8) & 15))
				return FALSE;
			for (component = 0; component < 4; component++)
				if (!msl_input_register_valid((inputs >> (component * 8)) & 255, FALSE))
					return FALSE;
		}
	}
	for (component = 0; component < 7; component++)
	{
		DWORD word = component < 4 ? state[D3DRS_PSFINALCOMBINERINPUTSABCD] :
			state[D3DRS_PSFINALCOMBINERINPUTSEFG];
		unsigned long input = (word >> (component < 4 ? component * 8 : (component - 3) * 8)) & 255;
		if ((input & 0xc0) || !msl_input_register_valid(input, TRUE)) return FALSE;
		/* E and F create ef_product before the final-only sum/product
		 * registers exist; they cannot depend on either of those results. */
		if (component >= 5 && (input & 15) >= _register_v1r0_sum) return FALSE;
	}
	if (state[D3DRS_PSFINALCOMBINERINPUTSEFG] & 0x1f) return FALSE;
	return TRUE;
}

/* The same clamp-to-border footprint as the GLES path. The native caller
 * binds clamp-to-edge samplers when this explicit border helper is enabled. */
static void msl_border_sample_function(struct xgpu_text *text,
	const struct nv2a_pixel_shader_key *key, int stage)
{
	unsigned char axes = key->border_axes[stage];
	unsigned char filtering = key->border_filter[stage];
	if (!axes) return;
	xgpu_text_append(text,
		"float4 sample_border%d(texture2d<float> tex, sampler tex_sampler,\n"
		" constant XgpuPixelUniforms &u, float2 uv)\n{\n"
		"\tfloat4 value = tex.sample(tex_sampler, uv, bias(u.texture_lod_bias[%d]));\n"
		"\tfloat2 size = float2(tex.get_width(), tex.get_height());\n", stage, stage);
	if (filtering == 1 || filtering == 2)
		xgpu_text_append(text,
			"\tfloat2 dx = dfdx(uv) * size, dy = dfdy(uv) * size;\n"
			"\tbool minifying = max(dot(dx, dx), dot(dy, dy)) * exp2(2.0 * u.texture_lod_bias[%d]) > 1.0;\n"
			"\tbool linear_filter = %sminifying;\n", stage, filtering == 1 ? "" : "!");
	else
		xgpu_text_append(text, "\tbool linear_filter = %s;\n", filtering == 3 ? "true" : "false");
	xgpu_text_append(text,
		"\tfloat2 coverage = linear_filter ? clamp(float2(0.5) + min(uv, float2(1.0) - uv) * size, 0.0, 1.0)\n"
		"\t\t: step(float2(0.0), uv) * (float2(1.0) - step(float2(1.0), uv));\n"
		"\treturn mix(u.texture_border_color[%d], value, %s);\n}\n", stage,
		axes == 3 ? "coverage.x * coverage.y" : axes == 1 ? "coverage.x" : "coverage.y");
}

/* A native pair uses the same original texture, UV and implicit footprint.
 * Only the sampler border alpha differs. This reconstructs the authored
 * alpha at sampled taps; it does not estimate a level-zero coverage or alter
 * RGB, mip selection, COLOR_SIGN, alpha kill or the original combiner math. */
static void msl_alpha_border_sample_function(struct xgpu_text *text, int stage)
{
	xgpu_text_append(text,
		"float4 sample_alpha_border%d(texture2d<float> tex, sampler black_sampler, sampler opaque_sampler,\n"
		" constant XgpuPixelUniforms &u, float2 uv)\n{\n"
		"\tfloat4 b = tex.sample(black_sampler, uv, bias(u.texture_lod_bias[%d]));\n"
		"\tfloat4 o = tex.sample(opaque_sampler, uv, bias(u.texture_lod_bias[%d]));\n"
		"\treturn float4(b.rgb, b.a + u.texture_border_color[%d].a * (o.a - b.a));\n}\n",
		stage, stage, stage, stage);
}

/* Filtering is linear in each sampled channel. With identical native
 * footprints, white minus transparent black isolates the border's weight,
 * so the authored color can be restored without guessing derivatives, LOD
 * or a level-zero footprint. GPU sampler precision and floating arithmetic
 * can affect boundary rounding; this identity is not a byte-parity claim. */
static void msl_volume_border_sample_function(struct xgpu_text *text, int stage)
{
	xgpu_text_append(text,
		"float4 sample_volume_border%d(texture3d<float> tex, sampler black_sampler, sampler white_sampler,\n"
		" constant XgpuPixelUniforms &u, float3 uvw)\n{\n"
		"\tfloat4 b = tex.sample(black_sampler, uvw, bias(u.texture_lod_bias[%d]));\n"
		"\tfloat4 w = tex.sample(white_sampler, uvw, bias(u.texture_lod_bias[%d]));\n"
		"\treturn b + u.texture_border_color[%d] * (w - b);\n}\n",
		stage, stage, stage, stage);
}

char *nv2a_pixel_shader_to_msl_with_options(const struct nv2a_pixel_shader_key *key,
	const struct nv2a_pixel_shader_msl_options *options)
{
	struct xgpu_text text = { 0 };
	int stage;
	BOOL depth_replace = FALSE, depth_before_shader = FALSE;
	unsigned int alpha_border_mask, volume_border_mask;
	if (!msl_key_supported(key, options))
	{
		errno = key ? ENOTSUP : EINVAL;
		return NULL;
	}
	alpha_border_mask = options && (options->version == 2 || options->version == 3) ?
		options->native_alpha_border_mask : 0;
	volume_border_mask = options && options->version == 3 ? options->native_volume_border_mask : 0;
	for (stage = 0; stage < 4; stage++)
		if (stage_mode(key, stage) == _mode_dot_zw) depth_replace = TRUE;
	depth_before_shader = !depth_replace && msl_depth_test_before_shader(key);
	xgpu_text_append(&text, XGPU_MSL_TYPES XGPU_MSL_FRAGMENT_VARYINGS XGPU_MSL_PIXEL_UNIFORMS);
	/* Preserve covered samples while writing the helper sample mask used by
	 * the working Apple GPU backend. Omitting this neutral all-ones output
	 * changes texture-derivative results on Apple GPUs (including ordinary
	 * single-sample draws). Alpha kill still discards the entire fragment.
	 * Occlusion tests skip the mask and test depth first, so a light behind
	 * the ship is not counted as visible. */
	if (depth_before_shader)
		xgpu_text_append(&text, "struct XgpuFragmentResult { float4 color [[color(0)]]; };\n");
	else
		xgpu_text_append(&text,
			"struct XgpuFragmentResult { float4 color [[color(0)]]; uint sample_mask [[sample_mask]];%s };\n",
			depth_replace ? " float depth [[depth(any)]];" : "");
	for (stage = 0; stage < 4; stage++) msl_border_sample_function(&text, key, stage);
	for (stage = 0; stage < 4; stage++)
		if (alpha_border_mask & (1u << stage)) msl_alpha_border_sample_function(&text, stage);
	for (stage = 0; stage < 4; stage++)
		if (volume_border_mask & (1u << stage)) msl_volume_border_sample_function(&text, stage);
	xgpu_text_append(&text,
		"float signed_byte(float x)\n{\n"
		"\tfloat b = floor(x * 255.0 + 0.5);\n"
		"\treturn (b >= 128.0 ? b - 256.0 : b) / 127.0;\n}\n"
		"float3 signed_bytes(float3 x)\n{\n"
		"\treturn float3(signed_byte(x.r), signed_byte(x.g), signed_byte(x.b));\n}\n"
		"float gl_signed_byte(float x)\n{\n"
		"\tfloat b = x * 255.0;\n"
		"\treturn (b >= 128.0 ? b - 255.5 : b + 0.5) / 127.5;\n}\n"
		"float3 gl_signed_bytes(float3 x)\n{\n"
		"\treturn float3(gl_signed_byte(x.r), gl_signed_byte(x.g), gl_signed_byte(x.b));\n}\n");
	xgpu_text_append(&text, "%sfragment XgpuFragmentResult xgpu_fragment(XgpuVaryings in [[stage_in]],\n"
		" constant XgpuPixelUniforms &u [[buffer(0)]]",
		depth_before_shader ? "[[early_fragment_tests]] " : "");
	for (stage = 0; stage < 4; stage++)
	{
		const char *type = key->sampler_type[stage] == _xgpu_sampler_cube ? "texturecube" :
			key->sampler_type[stage] == _xgpu_sampler_3d ? "texture3d" : "texture2d";
		xgpu_text_append(&text, ",\n %s<float> tex%d [[texture(%d)]], sampler sampler%d [[sampler(%d)]]",
			type, stage, stage, stage, stage);
	}
	for (stage = 0; stage < 4; stage++)
		if (alpha_border_mask & (1u << stage))
			xgpu_text_append(&text, ",\n sampler alpha_border_sampler%d [[sampler(%d)]]", stage, 4 + stage);
	for (stage = 0; stage < 4; stage++)
		if (volume_border_mask & (1u << stage))
			xgpu_text_append(&text, ",\n sampler volume_border_sampler%d [[sampler(%d)]]", stage, 4 + stage);
	xgpu_text_append(&text,
		")\n{\n"
		"\tfloat4 xD0 = in.xD0, xD1 = in.xD1;\n"
		"\tfloat4 xT0 = in.xT0, xT1 = in.xT1, xT2 = in.xT2, xT3 = in.xT3;\n"
		"\tfloat xFog = in.xFog;\n");
	if (depth_replace)
		xgpu_text_append(&text,
			"\tif (u.depth_scale != (1.0 / 16777215.0) || !isfinite(u.depth_clip_min) ||"
			" !isfinite(u.depth_clip_max) || u.depth_clip_min < 0.0 ||"
			" u.depth_clip_max > 1.0 || u.depth_clip_min > u.depth_clip_max) discard_fragment();\n"
			"\tfloat replacement_depth = in.position.z;\n");
	if (key->point_threshold)
		xgpu_text_append(&text, "\tconstexpr sampler meter_nearest(filter::nearest, mip_filter::none);\n");
	pixel_shader_body(&text, key, _shader_msl, alpha_border_mask, volume_border_mask);
	if (depth_replace && xgpu_reversed_depth)
		xgpu_text_append(&text, "\treplacement_depth = 1.0 - replacement_depth;\n");
	if (depth_before_shader)
		xgpu_text_append(&text, "\treturn XgpuFragmentResult { clamp(result, 0.0, 1.0) };\n}\n");
	else if (depth_replace)
		xgpu_text_append(&text, "\treturn XgpuFragmentResult { clamp(result, 0.0, 1.0), 0xffffffffu, replacement_depth };\n}\n");
	else
		xgpu_text_append(&text, "\treturn XgpuFragmentResult { clamp(result, 0.0, 1.0), 0xffffffffu };\n}\n");
	return text.buffer;
}

char *nv2a_pixel_shader_to_msl(const struct nv2a_pixel_shader_key *key)
{
	return nv2a_pixel_shader_to_msl_with_options(key, NULL);
}

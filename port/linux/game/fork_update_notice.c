/*
FORK_UPDATE_NOTICE.C

The map download line is the terminal font across the top of the screen.
This is that line, in a game and in the menus. It does not take a key
or pause the game.
*/

#include "cseries.h"
#include "math/integer_math.h"
#include "math/real_math.h"
#include "interface/interface.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "fork_update_notice.h"

#if !defined(HALO_ANDROID) || defined(HALO_MACOS_NATIVE_METAL)
int halo_fork_update_notice(char *out, unsigned long capacity);
#endif

void fork_update_notice_render(void)
{
#if !defined(HALO_ANDROID) || defined(HALO_MACOS_NATIVE_METAL)
	char text[160];
	long font_tag_index;
	real_argb_color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	rectangle2d bounds;

	if (!halo_fork_update_notice(text, sizeof(text)))
		return;
	font_tag_index = interface_get_tag_index(_interface_font_terminal);
	if (font_tag_index == NONE)
		return;
	/* the map download line's band, above the menus' safe area */
	bounds = render.camera.window_bounds;
	offset_rectangle2d(&bounds, -render.camera.viewport_bounds.x0,
		-render.camera.viewport_bounds.y0);
	bounds.y0 = 4;
	bounds.y1 = 22;
	/* centered, as the map download line is */
	draw_string_set_draw_mode(font_tag_index, NONE, 2, 0, &color);
	rasterizer_draw_string(&bounds, NULL, NULL, 0, text);
#endif
}

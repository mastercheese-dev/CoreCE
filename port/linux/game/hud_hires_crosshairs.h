/*
HUD_HIRES_CROSSHAIRS.H

The supplied high-res sniper pictures on a Custom Edition map
(hud_hires_tags.c). The Xbox sheet is a different layout and is drawn
by the ordinary bitmap override.
*/

#ifndef HUD_HIRES_CROSSHAIRS_H
#define HUD_HIRES_CROSSHAIRS_H

struct bitmap_data;

struct bitmap_data const *hud_hires_crosshair_bitmap(
	struct bitmap_data const *bitmap,
	short sequence_index,
	short frame_index);

#endif

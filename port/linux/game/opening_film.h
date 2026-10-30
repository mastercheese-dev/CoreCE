#ifndef OPENING_FILM_H
#define OPENING_FILM_H

#include "cseries/cseries.h"

/* The opening film, maps/opening_frames.bin. shown_ms picks the frame.
   opacity 1 replaces the screen. Below that it fades over the match.
   FALSE when that file is not there. */
boolean opening_film_show(unsigned long shown_ms, real opacity);
/* The film's length, or 0 when the file is not a film. */
unsigned long opening_film_duration_ms(void);
/* One frame of the film, in milliseconds. A presented frame advances the
picture by at most this, so a hitch does not skip ahead. */
unsigned long opening_film_step_ms(void);

/* A campaign or co-op load. The next map copy plays the film instead of
   the stand-in ring, then fades it off the level. A multiplayer match
   keeps its own opening. */
void opening_film_load_arm(void);
boolean opening_film_load_take_arm(void);
void opening_film_load_begin(void);
void opening_film_load_cancel(void);
boolean opening_film_load_active(void);
/* FALSE when the film is not the picture, so the ring can draw. */
boolean opening_film_load_frame(void);
void opening_film_load_overlay(void);

#endif

# HRTF

The head-related impulse responses the mixer renders 3D sounds with for
headphones (`audio.hrtf`; `port/linux/src/dsound_sdl.c`). The builds embed
`sadie_d2_48k.hrtf` (`tools/embed_assets.py`).

| File | What it is |
| --- | --- |
| `sadie_d2_48k.hrtf` | 360 directions (every 10 degrees around, every 15 from -45 to 90 up), each ear's response made minimum phase and cut to 48 taps at 48 kHz, and its interaural delay. Made by `tools/hrtf_assets.py`, which describes the format. |
| `LICENSE-Apache-2.0.txt` | The license of the measurements, and so of this file. |

The responses are from the SADIE II database, subject D2 (a KEMAR
mannequin), `D2_48K_24bit_256tap_FIR_SOFA.sofa` in `D2_HRIR_SOFA.zip`:

> SADIE II Database. Copyright 2018, University of York (AudioLab,
> Department of Electronic Engineering). Licensed under the Apache License,
> Version 2.0. https://www.york.ac.uk/sadie-project/database.html

C. Armstrong, L. Thresh, D. Murphy and G. Kearney, "A Perceptual
Evaluation of Individual and Non-Individual HRTFs: A Case Study of the
SADIE II Database", Applied Sciences 8(11), 2029 (2018),
https://doi.org/10.3390/app8112029.

This file is a modified form of the database's: tools/hrtf_assets.py takes
a grid of its 8802 directions, makes each response minimum phase, shortens
it to 48 taps, measures the interaural delay apart, scales the set and
stores it as 16-bit integers.

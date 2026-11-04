#!/usr/bin/env python3
"""Makes the head-related impulse responses the desktop mixer renders 3D
sounds for headphones with (audio.hrtf; port/linux/src/dsound_sdl.c, hrtf)
from the SADIE II database's KEMAR (subject D2) measurements, 48 kHz:

    python3 tools/hrtf_assets.py D2_48K_24bit_256tap_FIR_SOFA.sofa

writes port/assets/hrtf/sadie_d2_48k.hrtf. The SOFA file is in
D2_HRIR_SOFA.zip of the SADIE II database (University of York,
https://www.york.ac.uk/sadie-project/database.html, Apache License 2.0;
port/assets/hrtf/LICENSE-Apache-2.0.txt). Needs numpy, scipy and h5py.

The measurements (8802 directions, diffuse-field equalized) are taken at
a grid of GRID_ELEVATIONS rings from -45 to 90 degrees of elevation, every
15, and GRID_AZIMUTHS directions around each, every 10 degrees: each grid
point the nearest measurement. Each ear's response is made minimum phase
and cut to TAPS taps, faded out over its last quarter; the delay before it
(the interaural time difference: the far ear's) is kept apart, in
samples, so the mixer can interpolate the filters between directions
without the comb filtering of mixing two delays. The filters are scaled so
that both ears together have a power of 1 over the sphere, as the stereo
pan's gains do.

The file, little-endian:
    "HRTF", uint32 version (1), uint32 sample rate, uint32 taps,
    uint32 elevations, float first elevation, float elevation step (degrees),
    uint32 azimuths, float azimuth step (degrees, counterclockwise from
    straight ahead, as SOFA's), float coefficient scale;
    then for each elevation from the first and each azimuth from 0:
    int16 left delay and right delay (in 1/256 samples),
    int16 left taps[taps], int16 right taps[taps] (times the scale).
"""

import struct
import sys
from pathlib import Path

import h5py
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "port/assets/hrtf/sadie_d2_48k.hrtf"
RATE = 48000
TAPS = 48
GRID_ELEVATIONS = 10
FIRST_ELEVATION = -45.0
ELEVATION_STEP = 15.0
GRID_AZIMUTHS = 36
AZIMUTH_STEP = 10.0
FFT_SIZE = 4096
ONSET_UPSAMPLING = 8


def unit_vectors(azimuth, elevation):
    azimuth = np.radians(azimuth)
    elevation = np.radians(elevation)
    return np.stack([np.cos(elevation) * np.cos(azimuth), np.cos(elevation) * np.sin(azimuth),
                     np.sin(elevation)], axis=-1)


def interaural_delay(left, right):
    """How many samples (fractional) the right ear hears the sound after the
    left (negative: before): the lag of the largest cross-correlation of the
    two responses below 1.5 kHz, where the delay is what the ears tell
    apart, upsampled ONSET_UPSAMPLING times. (Each ear's onset, the first
    sample near its peak, is not as steady: the far ear's is faint.)"""
    from scipy.signal import butter, resample_poly, sosfiltfilt

    lowpass = butter(4, 1500, fs=RATE, output="sos")
    left = resample_poly(sosfiltfilt(lowpass, left), ONSET_UPSAMPLING, 1)
    right = resample_poly(sosfiltfilt(lowpass, right), ONSET_UPSAMPLING, 1)
    correlation = np.correlate(right, left, "full")
    return (int(np.argmax(correlation)) - (len(left) - 1)) / ONSET_UPSAMPLING


def minimum_phase(response):
    """The minimum-phase response of the same magnitude (the real
    cepstrum's folding), TAPS taps, faded out over its last quarter."""
    spectrum = np.fft.fft(response, FFT_SIZE)
    log_magnitude = np.log(np.maximum(np.abs(spectrum), 1e-8))
    cepstrum = np.fft.ifft(log_magnitude).real
    folded = np.zeros(FFT_SIZE)
    folded[0] = cepstrum[0]
    folded[1:FFT_SIZE // 2] = 2.0 * cepstrum[1:FFT_SIZE // 2]
    folded[FFT_SIZE // 2] = cepstrum[FFT_SIZE // 2]
    result = np.fft.ifft(np.exp(np.fft.fft(folded))).real
    full_energy = float((result ** 2).sum())
    result = result[:TAPS].copy()
    fade = TAPS // 4
    result[TAPS - fade:] *= 0.5 * (1.0 + np.cos(np.pi * (np.arange(fade) + 1) / (fade + 1)))
    return result, float((result ** 2).sum()) / full_energy


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: hrtf_assets.py D2_48K_24bit_256tap_FIR_SOFA.sofa")
    with h5py.File(sys.argv[1], "r") as sofa:
        rate = float(np.array(sofa["Data.SamplingRate"])[0])
        if rate != RATE:
            sys.exit(f"{sys.argv[1]}: {rate:g} Hz, not {RATE}")
        positions = np.array(sofa["SourcePosition"])
        responses = np.array(sofa["Data.IR"])
    measured = unit_vectors(positions[:, 0], positions[:, 1])

    delays, filters, kept, weights = [], [], [], []
    for ring in range(GRID_ELEVATIONS):
        elevation = FIRST_ELEVATION + ring * ELEVATION_STEP
        for step in range(GRID_AZIMUTHS):
            azimuth = step * AZIMUTH_STEP
            nearest = int(np.argmax((measured * unit_vectors(azimuth, elevation)).sum(axis=1)))
            pair = []
            for ear in range(2):
                shaped, fraction = minimum_phase(responses[nearest, ear])
                pair.append(shaped)
                kept.append(fraction)
            lag = interaural_delay(responses[nearest, 0], responses[nearest, 1])
            delays.append([max(-lag, 0.0), max(lag, 0.0)])
            filters.append(pair)
            # (the grid's share of the sphere)
            weights.append(np.cos(np.radians(elevation)) if abs(elevation) < 90 else 0.05)
    filters = np.array(filters)
    weights = np.array(weights) / np.sum(weights)
    power = float(np.sum(weights * (filters ** 2).sum(axis=(1, 2))))
    filters /= np.sqrt(power)
    scale = float(np.abs(filters).max()) / 32767.0

    data = bytearray(b"HRTF")
    data += struct.pack("<IIIIffIff", 1, RATE, TAPS, GRID_ELEVATIONS, FIRST_ELEVATION, ELEVATION_STEP,
                        GRID_AZIMUTHS, AZIMUTH_STEP, scale)
    for pair, ear_delays in zip(filters, delays):
        data += struct.pack("<hh", *(int(round(value * 256)) for value in ear_delays))
        for ear in range(2):
            data += struct.pack(f"<{TAPS}h", *(int(round(value / scale)) for value in pair[ear]))
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(bytes(data))
    print(f"{OUTPUT.relative_to(ROOT)}: {len(data)} bytes, {len(filters)} directions, {TAPS} taps; "
          f"the taps keep {100 * min(kept):.1f}% of a response's energy at least, "
          f"{100 * float(np.mean(kept)):.1f}% on average; largest delay "
          f"{max(max(pair) for pair in delays):.1f} samples")


if __name__ == "__main__":
    main()

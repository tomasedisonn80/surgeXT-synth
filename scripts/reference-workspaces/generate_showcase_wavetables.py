#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the original, audible wavetable sources used by native captures.

Only Python's standard library is needed. The .wt layout is documented in
doc/WT fileformat.txt. These are periodic harmonic signals, not image-derived
plot samples. The normal Surge wavetable loader and oscillator consume them.
"""

from __future__ import annotations

import argparse
import hashlib
import math
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "resources/data/wavetables/Reference Showcases"
TAU = 2.0 * math.pi
PEAK = 0.92
# name, analytic family, frames, samples per frame
TABLES = (
    ("Spectral Sweep", "sweep", 48, 512),
    ("Analog Stack", "analog", 48, 512),
    ("Spectral Bloom", "dunes", 256, 2048),
    ("Bauhaus Waves", "bauhaus", 48, 512),
    ("Orbiting Saw", "saw", 48, 512),
    ("JT Saw RM", "industrial", 48, 512),
    ("Aurora Waves", "aurora", 48, 512),
    ("Glass Sweep", "glass", 48, 512),
    ("GlassWavetable 03", "horizon", 48, 512),
    ("VocalFormant A", "vocal", 48, 512),
)


# Original finite-harmonic pulse family. Each tuple is phase, integer power,
# amplitude, phase travel, depth rate and depth phase. An integer power of a
# raised cosine has a finite Fourier series; no image samples enter this source.
DUNES_PEAKS = (
    (.075, 40, .80, .035, .70, .2),
    (.225, 72, 1.03, .055, .94, 2.3),
    (.395, 108, 1.20, .032, 1.13, .9),
    (.565, 56, 1.00, .045, .81, 3.2),
    (.740, 96, .88, .030, 1.27, 4.4),
    (.900, 48, .65, .040, .59, 1.5),
)
DUNES_MEANS = tuple(math.comb(2 * peak[1], peak[1]) / 4 ** peak[1]
                    for peak in DUNES_PEAKS)
GLOBAL_SCALE_FAMILIES = {"dunes", "aurora"}


def signal(family: str, x: float, morph: float) -> float:
    """Evaluate a continuous, exactly periodic, bounded harmonic family."""
    phase = TAU * x
    travel = (morph - 0.5) * math.pi
    if family in {"analog", "saw"}:
        cycles = 2 if family == "analog" else 3
        return sum(
            math.sin(harmonic * (cycles * phase + 0.14 * travel))
            * (1.0 - 0.62 * morph * (harmonic - 1) / 15.0)
            / harmonic ** (1.22 + 0.45 * morph)
            for harmonic in range(1, 17)
        )
    if family == "dunes":
        result = 0.0
        for index, ((centre, power, height, drift, rate, offset), mean) in enumerate(
            zip(DUNES_PEAKS, DUNES_MEANS)
        ):
            amplitude = height * (
                .28 + .72 * (.5 + .5 * math.cos(TAU * rate * morph + offset))
            )
            moving_centre = centre + drift * math.sin(
                TAU * (.55 + .17 * index) * morph + offset
            )
            pulse = (.5 + .5 * math.cos(TAU * (x - moving_centre))) ** power
            result += amplitude * (pulse - mean)
        return (
            result + .055 * math.sin(phase + .6 * math.sin(math.pi * morph))
            + .025 * math.sin(5 * phase - 1.1 * math.pi * morph)
        )
    if family == "bauhaus":
        return (
            0.68 * math.sin(5 * phase + 0.75 * travel)
            + (0.16 + 0.12 * morph) * math.sin(4 * phase - 0.6 * travel)
            + 0.17 * math.sin(phase + travel)
        )
    if family == "industrial":
        return (
            0.65 * math.sin(3 * phase + 0.72 * travel)
            + 0.22 * math.sin(6 * phase + 0.22 * travel)
            + (0.10 + 0.12 * morph) * math.sin(2 * phase - travel)
            + 0.08 * math.sin(9 * phase)
        )
    if family == "aurora":
        # Three main crests cross neighboring harmonics whose phase travel and
        # level evolve independently. The wider periodic window and quieter
        # upper partials retain asymmetric shoulders across the audible table.
        # Multiplication by this first-harmonic window remains finite-band.
        envelope = 0.60 + 0.40 * math.sin(0.5 * phase) ** 2
        phase -= 0.08 * math.pi
        drift = (
            0.28 * math.sin(TAU * 0.72 * morph + 0.3)
            + 0.10 * math.sin(TAU * 1.75 * morph + 1.0)
        )
        counter = 0.23 * math.sin(TAU * 1.15 * morph + 1.1)
        return envelope * (
            (0.60 + 0.06 * math.cos(1.3 * math.pi * morph + 0.2))
            * math.sin(3 * phase + 1.48 * travel + drift)
            + (0.18 + 0.045 * math.sin(math.pi * morph + 0.35))
            * math.sin(4 * phase - 1.22 * travel + counter + 0.35)
            + 0.15 * math.sin(2 * phase - 0.82 * travel - 0.6 * drift + 0.65)
            + (0.10 + 0.035 * math.cos(TAU * 0.8 * morph + 1.0))
            * math.sin(5 * phase + 0.55 * travel - counter)
            + 0.075 * math.sin(7 * phase - 0.9 * travel + 0.4)
            + 0.055 * math.sin(phase - 0.7 * travel)
        )
    if family == "glass":
        return (
            0.70 * math.sin(2 * phase + 0.90 * travel)
            + 0.22 * math.sin(3 * phase - 0.55 * travel)
            + (0.10 + 0.12 * morph) * math.sin(5 * phase + 0.4 * travel)
        )
    if family == "horizon":
        # Independent, nonlinear morph travel separates the three broad folds;
        # small sixth/seventh partials add audible, periodic crest shoulders.
        drift = (
            0.31 * math.sin(TAU * 0.8 * morph + 0.2)
            + 0.10 * math.sin(TAU * 1.6 * morph + 1.1)
        )
        counter = 0.26 * math.sin(TAU * 1.1 * morph + 1.4)
        return (
            (0.47 + 0.065 * math.cos(TAU * 0.65 * morph + 0.3))
            * math.sin(3 * phase + 1.08 * travel + drift)
            + (0.23 + 0.055 * math.sin(TAU * 0.75 * morph + 0.6))
            * math.sin(2 * phase - 0.92 * travel + 0.5 * counter)
            + (0.17 + 0.045 * math.cos(TAU * 0.9 * morph + 1.2))
            * math.sin(4 * phase - 1.25 * travel + counter + 0.5)
            + 0.17 * math.sin(5 * phase + 0.58 * travel - 0.7 * drift + 0.8)
            + 0.06 * math.sin(6 * phase - 0.63 * travel + 0.4)
            + 0.065 * math.sin(7 * phase + 0.76 * travel + counter)
            + 0.10 * math.sin(phase - 0.88 * travel)
        )
    if family == "vocal":
        return (
            0.44 * math.sin(2 * phase + 0.5 * travel)
            + 0.32 * math.sin(5 * phase + 0.8 * travel)
            + (0.16 + 0.10 * morph) * math.sin(7 * phase - 0.5 * travel)
            + 0.09 * math.sin(9 * phase + travel)
        )
    if family == "sweep":
        return (
            0.68 * math.sin(2 * phase + 0.85 * travel)
            + (0.19 + 0.10 * morph) * math.sin(3 * phase - 0.65 * travel)
            + 0.18 * math.sin(phase + 0.6 * travel)
            + 0.08 * math.sin(5 * phase - travel)
        )
    raise ValueError(f"Unknown analytic family: {family}")


def generate(family: str, frames: int, samples: int) -> bytes:
    result = bytearray(struct.pack("<4sIHH", b"vawt", samples, frames, 0))
    rows = []
    for frame in range(frames):
        morph = frame / (frames - 1)
        row = [signal(family, sample / samples, morph) for sample in range(samples)]
        dc = sum(row) / samples
        rows.append([value - dc for value in row])
    # Preserve real variation between the landscape/ribbon source frames. This
    # scaling is part of the audible WT asset, never a display normalization.
    global_peak = max(abs(value) for row in rows for value in row)
    for row in rows:
        peak = global_peak if family in GLOBAL_SCALE_FAMILIES else max(map(abs, row))
        scale = PEAK / peak
        result.extend(struct.pack(f"<{samples}f", *(value * scale for value in row)))
    return bytes(result)


def validate(data: bytes, family: str, frames: int, samples: int) -> None:
    assert struct.unpack_from("<4sIHH", data) == (b"vawt", samples, frames, 0)
    assert len(data) == 12 + frames * samples * 4
    rows = []
    table_peak = 0.0
    for frame in range(frames):
        row = struct.unpack_from(f"<{samples}f", data, 12 + frame * samples * 4)
        assert all(math.isfinite(value) for value in row)
        assert abs(sum(row) / samples) < 1e-7
        frame_peak = max(map(abs, row))
        assert frame_peak <= PEAK + 1e-6
        table_peak = max(table_peak, frame_peak)
        if family not in GLOBAL_SCALE_FAMILIES:
            assert abs(frame_peak - PEAK) < 1e-6
        # A table omits the duplicate endpoint. Check the analytic endpoint,
        # not the last stored sample, for true periodic continuity.
        morph = frame / (frames - 1)
        assert abs(signal(family, 0.0, morph) - signal(family, 1.0, morph)) < 1e-12
        rows.append(data[12 + frame * samples * 4 : 12 + (frame + 1) * samples * 4])
    assert len(set(rows)) == frames
    assert abs(table_peak - PEAK) < 1e-6


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify checked-in files without writing")
    args = parser.parse_args()
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    total = 0
    for name, family, frames, samples in TABLES:
        data = generate(family, frames, samples)
        validate(data, family, frames, samples)
        destination = OUTPUT / f"{name}.wt"
        if args.check:
            if not destination.is_file() or destination.read_bytes() != data:
                raise SystemExit(f"Generated wavetable differs: {destination.relative_to(ROOT)}")
        else:
            destination.write_bytes(data)
        total += len(data)
        print(f"{hashlib.sha256(data).hexdigest()}  {name}.wt  {frames}x{samples}  {len(data)} bytes")
    print(f"{'Verified' if args.check else 'Generated'} {len(TABLES)} native wavetables: {total} bytes")


if __name__ == "__main__":
    main()

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
        return (
            (0.60 + 0.12 * math.cos(travel)) * math.sin(3 * phase + 0.76 * travel)
            + (0.22 + 0.06 * morph) * math.sin(4 * phase - 0.85 * travel)
            + 0.15 * math.sin(phase + 1.2 * travel)
            + 0.07 * math.sin(7 * phase + 0.35 * travel)
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
        return (
            0.70 * math.sin(3 * phase + 0.80 * travel)
            + (0.19 + 0.11 * morph) * math.sin(2 * phase - 0.5 * travel)
            + 0.20 * math.sin(phase + travel)
            + 0.06 * math.sin(6 * phase - 0.4 * travel)
        )
    if family == "glass":
        return (
            0.70 * math.sin(2 * phase + 0.90 * travel)
            + 0.22 * math.sin(3 * phase - 0.55 * travel)
            + (0.10 + 0.12 * morph) * math.sin(5 * phase + 0.4 * travel)
        )
    if family == "horizon":
        return (
            0.60 * math.sin(3 * phase + 0.90 * travel)
            + 0.27 * math.sin(2 * phase - 0.8 * travel)
            + 0.18 * math.sin(5 * phase + 0.6 * travel)
            + 0.12 * math.sin(phase - travel)
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
    for frame in range(frames):
        morph = frame / (frames - 1)
        row = [signal(family, sample / samples, morph) for sample in range(samples)]
        dc = sum(row) / samples
        row = [value - dc for value in row]
        scale = PEAK / max(abs(value) for value in row)
        result.extend(struct.pack(f"<{samples}f", *(value * scale for value in row)))
    return bytes(result)


def validate(data: bytes, family: str, frames: int, samples: int) -> None:
    assert struct.unpack_from("<4sIHH", data) == (b"vawt", samples, frames, 0)
    assert len(data) == 12 + frames * samples * 4
    rows = []
    for frame in range(frames):
        row = struct.unpack_from(f"<{samples}f", data, 12 + frame * samples * 4)
        assert all(math.isfinite(value) for value in row)
        assert abs(sum(row) / samples) < 1e-7
        assert abs(max(map(abs, row)) - PEAK) < 1e-6
        # A table omits the duplicate endpoint. Check the analytic endpoint,
        # not the last stored sample, for true periodic continuity.
        morph = frame / (frames - 1)
        assert abs(signal(family, 0.0, morph) - signal(family, 1.0, morph)) < 1e-12
        rows.append(data[12 + frame * samples * 4 : 12 + (frame + 1) * samples * 4])
    assert len(set(rows)) == frames


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

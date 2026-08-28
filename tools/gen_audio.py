#!/usr/bin/env python3
"""Generate 8-bit mono 22050 Hz WAV effects for Butano / Maxmod."""

from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 22050
AUDIO_DIR = Path(__file__).resolve().parent.parent / "audio"


def write_wav(path: Path, samples: list[int]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(1)
        handle.setframerate(SAMPLE_RATE)
        packed = bytes(max(0, min(255, sample)) for sample in samples)
        handle.writeframes(packed)


def envelope(length: int, attack: float, decay: float) -> list[float]:
    env: list[float] = []
    attack_samples = int(length * attack)
    decay_samples = int(length * decay)
    sustain_samples = max(0, length - attack_samples - decay_samples)

    for i in range(attack_samples):
        env.append(i / max(1, attack_samples))

    env.extend([1.0] * sustain_samples)

    for i in range(decay_samples):
        env.append(1.0 - (i / max(1, decay_samples)))

    while len(env) < length:
        env.append(0.0)

    return env[:length]


def make_chuff() -> list[int]:
    """Short steam exhaust — two-tone thump for Thomas chuff-chuff."""
    length = int(SAMPLE_RATE * 0.09)
    env = envelope(length, 0.02, 0.75)
    samples: list[int] = []

    for i, amp in enumerate(env):
        t = i / SAMPLE_RATE
        body = math.sin(2 * math.pi * 95 * t) * 0.55
        click = math.sin(2 * math.pi * 210 * t) * 0.25
        noise = math.sin(2 * math.pi * (430 + 40 * math.sin(t * 40)) * t) * 0.12
        value = 128 + int((body + click + noise) * amp * 70)
        samples.append(value)

    return samples


def make_whistle() -> list[int]:
    """Bright two-note PEEP for level complete."""
    notes = [(660, 0.12), (880, 0.18)]
    samples: list[int] = []

    for freq, duration in notes:
        note_len = int(SAMPLE_RATE * duration)
        env = envelope(note_len, 0.05, 0.35)
        for i, amp in enumerate(env):
            t = i / SAMPLE_RATE
            tone = math.sin(2 * math.pi * freq * t)
            harmonic = math.sin(2 * math.pi * freq * 2 * t) * 0.2
            value = 128 + int((tone + harmonic) * amp * 55)
            samples.append(value)

        samples.extend([128] * int(SAMPLE_RATE * 0.02))

    return samples


def main() -> None:
    write_wav(AUDIO_DIR / "chuff.wav", make_chuff())
    write_wav(AUDIO_DIR / "whistle.wav", make_whistle())
    print(f"Wrote chuff.wav and whistle.wav to {AUDIO_DIR}")


if __name__ == "__main__":
    main()

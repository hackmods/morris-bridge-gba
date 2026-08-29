#!/usr/bin/env python3
"""Generate original GBA audio for Thomas's Bridge.

Direct Sound SFX are 8-bit mono 22050 Hz WAV files (Maxmod).
Looping background music is ProTracker MOD (4 channel, M.K.).

These compositions are original. They aim for cheerful steam-railway /
1980s British children's-TV energy. They do **not** reproduce the
copyrighted *Thomas the Tank Engine* theme (O'Donnell & Campbell) or
any other licensed recording.

Usage:
    python3 tools/gen_audio.py
    python3 tools/gen_audio.py --check
"""

from __future__ import annotations

import argparse
import math
import struct
import sys
import wave
from pathlib import Path

SAMPLE_RATE = 22050
MOD_C2_HZ = 8363.0
AUDIO_DIR = Path(__file__).resolve().parent.parent / "audio"
PREVIEW_DIR = Path(__file__).resolve().parent.parent / "audio_preview"

# ProTracker period table (tuning 0). Index: octave 0..4, note 0=C .. 11=B.
PERIODS = (
    (1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907),
    (856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453),
    (428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226),
    (214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113),
    (107, 101, 95, 90, 85, 80, 75, 71, 67, 63, 60, 56),
)

NOTE_INDEX = {
    "C": 0,
    "C#": 1,
    "Db": 1,
    "D": 2,
    "D#": 3,
    "Eb": 3,
    "E": 4,
    "F": 5,
    "F#": 6,
    "Gb": 6,
    "G": 7,
    "G#": 8,
    "Ab": 8,
    "A": 9,
    "A#": 10,
    "Bb": 10,
    "B": 11,
}

# Instruments in every module.
INS_BRASS = 1
INS_BASS = 2
INS_STEAM = 3
INS_BELL = 4


def clamp_u8(value: int) -> int:
    return max(0, min(255, value))


def write_wav_u8(path: Path, samples: list[int], rate: int = SAMPLE_RATE) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    packed = bytes(clamp_u8(sample) for sample in samples)
    with wave.open(str(path), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(1)
        handle.setframerate(rate)
        handle.writeframes(packed)


def write_wav_s16(path: Path, samples: list[float], rate: int = SAMPLE_RATE) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    packed = bytearray()
    for sample in samples:
        value = int(max(-1.0, min(1.0, sample)) * 32767)
        packed.extend(struct.pack("<h", value))
    with wave.open(str(path), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(bytes(packed))


def envelope(length: int, attack: float, decay: float) -> list[float]:
    attack_samples = max(1, int(length * attack))
    decay_samples = max(1, int(length * decay))
    sustain_samples = max(0, length - attack_samples - decay_samples)
    env = [i / attack_samples for i in range(attack_samples)]
    env.extend([1.0] * sustain_samples)
    env.extend([1.0 - (i / decay_samples) for i in range(decay_samples)])
    if len(env) < length:
        env.extend([0.0] * (length - len(env)))
    return env[:length]


def mix_tone(freq: float, duration: float, amp: float, harmonics: tuple[tuple[float, float], ...],
             attack: float, decay: float) -> list[int]:
    length = max(1, int(SAMPLE_RATE * duration))
    env = envelope(length, attack, decay)
    samples: list[int] = []
    for i, gain in enumerate(env):
        t = i / SAMPLE_RATE
        wave_v = 0.0
        for ratio, weight in harmonics:
            wave_v += math.sin(2 * math.pi * freq * ratio * t) * weight
        samples.append(128 + int(wave_v * gain * amp * 127))
    return samples


def concat_u8(*chunks: list[int], gap: int = 0) -> list[int]:
    out: list[int] = []
    for index, chunk in enumerate(chunks):
        if index:
            out.extend([128] * gap)
        out.extend(chunk)
    return out


def noise_burst(duration: float, amp: float, lowpass: float) -> list[int]:
    length = max(1, int(SAMPLE_RATE * duration))
    env = envelope(length, 0.02, 0.85)
    samples: list[int] = []
    state = 0.0
    seed = 0xACE1
    for gain in env:
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        white = ((seed >> 16) / 32768.0) * 2.0 - 1.0
        state += lowpass * (white - state)
        samples.append(128 + int(state * gain * amp * 127))
    return samples


# ---------------------------------------------------------------------------
# Sound effects
# ---------------------------------------------------------------------------

def make_chuff() -> list[int]:
    length = int(SAMPLE_RATE * 0.10)
    env = envelope(length, 0.03, 0.72)
    samples: list[int] = []
    seed = 0x504F
    lp = 0.0
    for i, amp in enumerate(env):
        t = i / SAMPLE_RATE
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        white = ((seed >> 16) / 32768.0) * 2.0 - 1.0
        lp += 0.35 * (white - lp)
        body = math.sin(2 * math.pi * 92 * t) * 0.55
        click = math.sin(2 * math.pi * 188 * t) * 0.22
        value = 128 + int((body + click + lp * 0.28) * amp * 72)
        samples.append(value)
    return samples


def make_whistle() -> list[int]:
    a = mix_tone(659.25, 0.13, 0.42, ((1.0, 0.85), (2.0, 0.15)), 0.06, 0.4)
    b = mix_tone(880.00, 0.20, 0.46, ((1.0, 0.8), (2.0, 0.2)), 0.05, 0.38)
    return concat_u8(a, b, gap=int(SAMPLE_RATE * 0.02))


def make_peep() -> list[int]:
    return mix_tone(784.0, 0.16, 0.5, ((1.0, 0.78), (2.0, 0.18), (3.0, 0.04)), 0.04, 0.45)


def make_useful() -> list[int]:
    notes = [(523.25, 0.12), (659.25, 0.12), (783.99, 0.12), (1046.5, 0.28)]
    chunks = [
        mix_tone(freq, dur, 0.44, ((1.0, 0.7), (2.0, 0.2), (3.0, 0.1)), 0.05, 0.35)
        for freq, dur in notes
    ]
    return concat_u8(*chunks, gap=int(SAMPLE_RATE * 0.012))


def make_delay() -> list[int]:
    notes = [(392.0, 0.14), (311.13, 0.16), (246.94, 0.18), (196.0, 0.32)]
    chunks = [
        mix_tone(freq, dur, 0.4, ((1.0, 0.75), (2.0, 0.15), (0.5, 0.1)), 0.04, 0.55)
        for freq, dur in notes
    ]
    return concat_u8(*chunks, gap=int(SAMPLE_RATE * 0.02))


def make_snap() -> list[int]:
    thump = mix_tone(90.0, 0.11, 0.55, ((1.0, 0.7), (2.0, 0.3)), 0.01, 0.85)
    crack = noise_burst(0.09, 0.7, 0.55)
    length = max(len(thump), len(crack))
    mixed: list[int] = []
    for i in range(length):
        a = thump[i] - 128 if i < len(thump) else 0
        b = crack[i] - 128 if i < len(crack) else 0
        mixed.append(128 + max(-127, min(127, a + b)))
    return mixed


def make_splash() -> list[int]:
    length = int(SAMPLE_RATE * 0.42)
    env = envelope(length, 0.02, 0.8)
    samples: list[int] = []
    seed = 0x51A5
    lp = 0.0
    for i, amp in enumerate(env):
        t = i / SAMPLE_RATE
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        white = ((seed >> 16) / 32768.0) * 2.0 - 1.0
        cutoff = 0.55 - 0.35 * (i / length)
        lp += max(0.08, cutoff) * (white - lp)
        drip = math.sin(2 * math.pi * (220 - 90 * t) * t) * 0.12
        samples.append(128 + int((lp * 0.85 + drip) * amp * 80))
    return samples


def make_ui() -> list[int]:
    return mix_tone(523.25, 0.05, 0.28, ((1.0, 1.0),), 0.05, 0.7)


def make_unlock() -> list[int]:
    """Yard unlock leitmotif — bell phrase in G major (matches yard.mod patterns 2–3)."""
    notes = [(392.0, 0.11), (493.88, 0.11), (523.25, 0.13), (659.25, 0.22)]
    chunks = [
        mix_tone(freq, dur, 0.48, ((1.0, 0.72), (2.0, 0.22), (3.0, 0.06)), 0.04, 0.42)
        for freq, dur in notes
    ]
    return concat_u8(*chunks, gap=int(SAMPLE_RATE * 0.012))


# ---------------------------------------------------------------------------
# MOD writer
# ---------------------------------------------------------------------------

def parse_note(token: str) -> tuple[int, int] | None:
    if token in ("-", "--", "---"):
        return None
    token = token.strip()
    octave = int(token[-1])
    name = token[:-1].rstrip("-")
    return NOTE_INDEX[name], octave


def period_for(token: str) -> int:
    parsed = parse_note(token)
    if parsed is None:
        return 0
    note, octave = parsed
    return PERIODS[octave][note]


def pack_cell(period: int, sample: int, effect: int, param: int) -> bytes:
    b0 = ((sample & 0xF0) | ((period >> 8) & 0x0F))
    b1 = period & 0xFF
    b2 = ((sample & 0x0F) << 4) | (effect & 0x0F)
    b3 = param & 0xFF
    return bytes((b0, b1, b2, b3))


def empty_pattern() -> list[bytes]:
    return [pack_cell(0, 0, 0, 0) for _ in range(64 * 4)]


def put_note(pattern: list[bytes], row: int, channel: int, note: str, sample: int,
             effect: int = 0, param: int = 0) -> None:
    pattern[row * 4 + channel] = pack_cell(period_for(note), sample, effect, param)


def put_fx(pattern: list[bytes], row: int, channel: int, effect: int, param: int) -> None:
    pattern[row * 4 + channel] = pack_cell(0, 0, effect, param)


def signed_loop(length: int, fn) -> bytes:
    data = bytearray()
    for i in range(length):
        value = max(-127, min(127, int(fn(i / length) * 120)))
        data.append(value & 0xFF)
    data[0] = 0
    data[1] = 0
    if len(data) % 2:
        data.append(0)
    return bytes(data)


def brass_sample() -> bytes:
    return signed_loop(512, lambda t: (
        0.62 * math.sin(2 * math.pi * t)
        + 0.24 * math.sin(4 * math.pi * t)
        + 0.10 * math.sin(6 * math.pi * t)
        + 0.08 * (1.0 if (t % 1.0) < 0.52 else -1.0)
    ))


def bass_sample() -> bytes:
    return signed_loop(512, lambda t: (
        0.85 * (2.0 * abs(2.0 * t - 1.0) - 1.0)
        + 0.15 * math.sin(2 * math.pi * t)
    ))


def steam_sample() -> bytes:
    seed = 0xC0FFEE
    data = bytearray()
    lp = 0.0
    length = 1024
    for i in range(length):
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        white = ((seed >> 16) / 32768.0) * 2.0 - 1.0
        lp += 0.28 * (white - lp)
        env = 1.0 - (i / length)
        value = max(-127, min(127, int(lp * env * 110)))
        data.append(value & 0xFF)
    data[0] = 0
    data[1] = 0
    return bytes(data)


def bell_sample() -> bytes:
    data = bytearray()
    length = 1024
    for i in range(length):
        t = i / length
        env = math.exp(-3.2 * t)
        value = max(-127, min(127, int((
            math.sin(2 * math.pi * t) * 0.7
            + math.sin(4 * math.pi * t) * 0.3
        ) * env * 120)))
        data.append(value & 0xFF)
    data[0] = 0
    data[1] = 0
    return bytes(data)


def pack_sample_header(name: str, data: bytes, volume: int, loop: bool) -> bytes:
    words = len(data) // 2
    loop_start = 1 if loop else 0
    loop_len = (words - 1) if loop else 1
    header = bytearray(30)
    raw_name = name.encode("ascii")[:22]
    header[0:len(raw_name)] = raw_name
    header[22:24] = struct.pack(">H", words)
    header[24] = 0
    header[25] = volume
    header[26:28] = struct.pack(">H", loop_start)
    header[28:30] = struct.pack(">H", loop_len)
    return bytes(header)


def write_mod(path: Path, title: str, patterns: list[list[bytes]], order: list[int],
              speed: int, bpm: int) -> None:
    samples = [
        ("brass", brass_sample(), 48, True),
        ("bass", bass_sample(), 52, True),
        ("steam", steam_sample(), 32, False),
        ("bell", bell_sample(), 40, False),
    ]

    # Speed/BPM on first cell of pattern 0 if not already used.
    put_fx(patterns[0], 0, 3, 0xF, speed)
    if len(patterns[0]) > 7:
        # Keep BPM on row 1 ch3 so it does not collide with a melody note on 0.
        put_fx(patterns[0], 1, 3, 0xF, bpm)

    buf = bytearray()
    title_bytes = title.encode("ascii")[:20]
    buf.extend(title_bytes)
    buf.extend(b"\x00" * (20 - len(title_bytes)))

    for name, data, volume, loop in samples:
        buf.extend(pack_sample_header(name, data, volume, loop))
    for _ in range(31 - len(samples)):
        buf.extend(pack_sample_header("", b"", 0, False))

    buf.append(len(order))
    buf.append(127)
    order_bytes = bytearray(128)
    for i, pattern_index in enumerate(order):
        order_bytes[i] = pattern_index
    buf.extend(order_bytes)
    buf.extend(b"M.K.")

    for pattern in patterns:
        for cell in pattern:
            buf.extend(cell)

    for _, data, _, _ in samples:
        buf.extend(data)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(buf))


def melody_quarters(pattern: list[bytes], notes: list[str], sample: int, channel: int = 0,
                    start_row: int = 0, volume: int = 40) -> None:
    row = start_row
    for index, note in enumerate(notes):
        if note != "-":
            effect, param = (0xC, volume) if index == 0 else (0, 0)
            put_note(pattern, row, channel, note, sample, effect, param)
        row += 4


def bass_chug(pattern: list[bytes], sequence: list[str], start_row: int = 0) -> None:
    row = start_row
    for note in sequence:
        if note != "-":
            put_note(pattern, row, 1, note, INS_BASS, 0xC, 44)
        row += 2


def steam_beats(pattern: list[bytes], rows: tuple[int, ...] = (0, 16, 32, 48)) -> None:
    for row in rows:
        if row == 0:
            put_note(pattern, row, 2, "C-2", INS_STEAM, 0xC, 28)
        else:
            put_note(pattern, row, 2, "C-2", INS_STEAM)


def loop_pattern(pattern: list[bytes], loop_to: int) -> None:
    """ProTracker Bxx — jump back to @p loop_to at end of pattern."""
    put_fx(pattern, 63, 3, 0xB, loop_to & 0xFF)


# Original scores -----------------------------------------------------------
# Island Line March — bright C-major steam march (NOT the TV theme).
ISLAND_A = ["E-2", "G-2", "C-3", "G-2", "A-2", "G-2", "F-2", "E-2",
            "D-2", "E-2", "F-2", "G-2", "E-2", "D-2", "C-2", "C-2"]
ISLAND_B = ["G-2", "C-3", "E-3", "C-3", "D-3", "C-3", "B-2", "G-2",
            "A-2", "G-2", "E-2", "G-2", "C-3", "G-2", "E-2", "C-2"]
ISLAND_BASS = (["C-1", "-", "C-1", "-", "G-1", "-", "G-1", "-"] * 4)

# Knapford Yard — light station idler.
YARD_A = ["C-2", "-", "E-2", "-", "G-2", "-", "E-2", "-",
          "D-2", "-", "F-2", "-", "G-2", "-", "D-2", "-"]
YARD_B = ["E-2", "-", "G-2", "-", "C-3", "-", "G-2", "-",
          "F-2", "-", "A-2", "-", "G-2", "-", "E-2", "-"]
YARD_BASS = (["C-1", "-", "-", "-", "G-1", "-", "-", "-"] * 4)
# Unlock leitmotif — brighter bell figures woven into the Yard loop (patterns 2–3).
YARD_UNLOCK_A = ["G-2", "-", "B-2", "-", "C-3", "-", "E-3", "-",
                   "G-3", "-", "E-3", "-", "C-3", "-", "G-2", "-"]
YARD_UNLOCK_B = ["E-3", "-", "G-3", "-", "C-4", "-", "G-3", "-",
                 "E-3", "-", "C-3", "-", "G-2", "-", "C-2", "-"]

# Per-engine idle motifs — engine select (patterns 4–13, pairs loop via Bxx).
ENGINE_MOTIFS: list[tuple[list[str], list[str]]] = [
    (["C-3", "-", "E-3", "-", "G-3", "-", "C-4", "-",
      "E-3", "-", "G-3", "-", "C-3", "-", "G-2", "-"],
     ["G-3", "-", "C-4", "-", "E-4", "-", "C-4", "-",
      "G-3", "-", "E-3", "-", "C-3", "-", "G-2", "-"]),  # Thomas
    (["E-3", "-", "G-3", "-", "B-3", "-", "E-4", "-",
      "G-3", "-", "B-3", "-", "E-3", "-", "G-2", "-"],
     ["B-3", "-", "E-4", "-", "G-4", "-", "E-4", "-",
      "B-3", "-", "G-3", "-", "E-3", "-", "B-2", "-"]),  # Percy
    (["G-2", "-", "B-2", "-", "D-3", "-", "G-3", "-",
      "A-2", "-", "G-2", "-", "E-2", "-", "G-2", "-"],
     ["B-2", "-", "D-3", "-", "G-3", "-", "D-3", "-",
      "B-2", "-", "A-2", "-", "G-2", "-", "D-2", "-"]),  # Edward
    (["D-3", "-", "F-3", "-", "A-3", "-", "D-4", "-",
      "F-3", "-", "A-3", "-", "D-3", "-", "F-2", "-"],
     ["A-3", "-", "D-4", "-", "F-4", "-", "D-4", "-",
      "A-3", "-", "F-3", "-", "D-3", "-", "A-2", "-"]),  # James
    (["C-2", "-", "E-2", "-", "G-2", "-", "C-3", "-",
      "G-2", "-", "E-2", "-", "C-2", "-", "G-1", "-"],
     ["G-2", "-", "C-3", "-", "E-3", "-", "C-3", "-",
      "G-2", "-", "E-2", "-", "C-2", "-", "G-1", "-"]),  # Gordon
]

# Workshop Shed — slower thinking music for build mode.
WORK_A = ["C-2", "E-2", "G-2", "E-2", "F-2", "A-2", "G-2", "E-2",
          "D-2", "G-2", "F-2", "D-2", "E-2", "C-2", "D-2", "C-2"]
WORK_B = ["A-2", "C-3", "B-2", "G-2", "A-2", "F-2", "G-2", "E-2",
          "F-2", "D-2", "E-2", "C-2", "D-2", "B-1", "C-2", "C-2"]
WORK_BASS = (["C-1", "-", "G-1", "-", "F-1", "-", "G-1", "-"] * 4)

# Full Steam — crossing urgency, still original.
STEAM_A = ["G-2", "G-2", "C-3", "G-2", "E-2", "C-3", "G-2", "E-2",
           "A-2", "A-2", "C-3", "A-2", "F-2", "C-3", "A-2", "F-2"]
STEAM_B = ["G-2", "B-2", "C-3", "D-3", "E-3", "C-3", "G-2", "E-2",
           "F-2", "A-2", "G-2", "E-2", "D-2", "G-2", "C-3", "C-2"]
STEAM_BASS = (["C-1", "G-1", "C-1", "G-1", "A-1", "E-1", "A-1", "E-1"] * 2
              + ["F-1", "C-1", "F-1", "C-1", "G-1", "D-1", "G-1", "D-1"] * 2)


def two_pattern_track(melody_a: list[str], melody_b: list[str], bass: list[str],
                      melody_sample: int = INS_BRASS, melody_vol: int = 40,
                      eighths: bool = False) -> list[list[bytes]]:
    patterns = [empty_pattern(), empty_pattern()]
    for pattern, melody in ((patterns[0], melody_a), (patterns[1], melody_b)):
        if eighths:
            row = 0
            looped = melody * 2
            for index, note in enumerate(looped):
                if row >= 64:
                    break
                if note != "-":
                    fx, param = (0xC, melody_vol) if index == 0 else (0, 0)
                    put_note(pattern, row, 0, note, melody_sample, fx, param)
                row += 2
        else:
            melody_quarters(pattern, melody, melody_sample, volume=melody_vol)
        bass_chug(pattern, bass)
        steam_beats(pattern)
        put_note(pattern, 8, 2, "G-3", INS_BELL, 0xC, 24)
        put_note(pattern, 40, 2, "E-3", INS_BELL, 0xC, 20)
    return patterns


def yard_track() -> list[list[bytes]]:
    patterns = two_pattern_track(
        YARD_A, YARD_B, YARD_BASS, melody_sample=INS_BELL, melody_vol=36,
    )
    loop_pattern(patterns[1], 0)

    unlock = two_pattern_track(
        YARD_UNLOCK_A, YARD_UNLOCK_B, YARD_BASS, melody_sample=INS_BELL, melody_vol=40,
    )
    loop_pattern(unlock[1], 2)
    patterns.extend(unlock)

    base = len(patterns)
    for index, (melody_a, melody_b) in enumerate(ENGINE_MOTIFS):
        pair = two_pattern_track(
            melody_a, melody_b, YARD_BASS, melody_sample=INS_BELL, melody_vol=38,
        )
        loop_pattern(pair[1], base + index * 2)
        patterns.extend(pair)

    return patterns


def generate_modules() -> None:
    write_mod(
        AUDIO_DIR / "island_line.mod",
        "Island Line",
        two_pattern_track(ISLAND_A, ISLAND_B, ISLAND_BASS, melody_vol=42),
        [0, 1],
        speed=6,
        bpm=125,
    )
    yard_patterns = yard_track()
    write_mod(
        AUDIO_DIR / "yard.mod",
        "Knapford Yard",
        yard_patterns,
        list(range(len(yard_patterns))),
        speed=7,
        bpm=118,
    )
    write_mod(
        AUDIO_DIR / "workshop.mod",
        "Workshop Shed",
        two_pattern_track(WORK_A, WORK_B, WORK_BASS, melody_vol=34),
        [0, 1],
        speed=8,
        bpm=110,
    )
    write_mod(
        AUDIO_DIR / "full_steam.mod",
        "Full Steam",
        two_pattern_track(STEAM_A, STEAM_B, STEAM_BASS, melody_vol=44, eighths=True),
        [0, 1],
        speed=4,
        bpm=132,
    )


# ---------------------------------------------------------------------------
# WAV previews of the same scores (not imported by Butano)
# ---------------------------------------------------------------------------

def note_hz(token: str) -> float:
    parsed = parse_note(token)
    if parsed is None:
        return 0.0
    note, octave = parsed
    midi = (octave + 1) * 12 + note  # C-1 ≈ MIDI 24-ish; relative is enough
    return 32.703 * (2.0 ** (midi / 12.0))


def render_score(melody: list[str], bass: list[str], bpm: int, seconds: float,
                 eighth_melody: bool = False) -> list[float]:
    beat = 60.0 / bpm
    step_mel = beat / (2.0 if eighth_melody else 1.0)
    step_bass = beat / 2.0
    total = int(SAMPLE_RATE * seconds)
    out = [0.0] * total

    def add_note(start: float, dur: float, freq: float, amp: float, kind: str) -> None:
        if freq <= 0:
            return
        n = int(dur * SAMPLE_RATE)
        s0 = int(start * SAMPLE_RATE)
        for i in range(n):
            idx = s0 + i
            if idx >= total:
                break
            t = i / SAMPLE_RATE
            env = min(1.0, i / 80.0) * max(0.0, 1.0 - i / max(1, n))
            if kind == "brass":
                wave_v = (math.sin(2 * math.pi * freq * t) * 0.65
                          + math.sin(4 * math.pi * freq * t) * 0.25)
            elif kind == "bass":
                wave_v = 2.0 * abs(2.0 * ((freq * t) % 1.0) - 1.0) - 1.0
            else:
                wave_v = math.sin(2 * math.pi * freq * t)
            out[idx] += wave_v * env * amp

    t = 0.0
    mi = 0
    while t < seconds:
        note = melody[mi % len(melody)]
        add_note(t, step_mel * 0.9, 0.0 if note == "-" else note_hz(note), 0.24, "brass")
        t += step_mel
        mi += 1

    t = 0.0
    bi = 0
    while t < seconds:
        note = bass[bi % len(bass)]
        add_note(t, step_bass * 0.85, 0.0 if note == "-" else note_hz(note), 0.18, "bass")
        t += step_bass
        bi += 1

    t = 0.0
    seed = 0x1111
    while t < seconds:
        n = int(0.06 * SAMPLE_RATE)
        s0 = int(t * SAMPLE_RATE)
        for i in range(n):
            idx = s0 + i
            if idx >= total:
                break
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
            white = ((seed >> 16) / 32768.0) * 2.0 - 1.0
            env = max(0.0, 1.0 - i / n)
            out[idx] += white * env * 0.05
        t += beat

    return _normalize(out)


def _normalize(samples: list[float]) -> list[float]:
    peak = max((abs(s) for s in samples), default=1.0)
    if peak < 1e-6:
        return samples
    scale = 0.9 / peak
    return [s * scale for s in samples]


def generate_previews() -> None:
    specs = (
        ("island_line.wav", ISLAND_A + ISLAND_B, ISLAND_BASS, 125, False),
        ("yard.wav", YARD_A + YARD_B, YARD_BASS, 118, False),
        ("workshop.wav", WORK_A + WORK_B, WORK_BASS, 110, False),
        ("full_steam.wav", STEAM_A + STEAM_B, STEAM_BASS, 152, True),
    )
    for name, melody, bass, bpm, eighths in specs:
        samples = render_score(melody, bass, bpm, seconds=8.0, eighth_melody=eighths)
        write_wav_s16(PREVIEW_DIR / name, samples)


def generate_sfx() -> None:
    write_wav_u8(AUDIO_DIR / "chuff.wav", make_chuff())
    write_wav_u8(AUDIO_DIR / "whistle.wav", make_whistle())
    write_wav_u8(AUDIO_DIR / "peep.wav", make_peep())
    write_wav_u8(AUDIO_DIR / "useful.wav", make_useful())
    write_wav_u8(AUDIO_DIR / "delay.wav", make_delay())
    write_wav_u8(AUDIO_DIR / "snap.wav", make_snap())
    write_wav_u8(AUDIO_DIR / "splash.wav", make_splash())
    write_wav_u8(AUDIO_DIR / "ui.wav", make_ui())
    write_wav_u8(AUDIO_DIR / "unlock.wav", make_unlock())


def validate() -> int:
    errors: list[str] = []
    wavs = [
        "chuff.wav", "whistle.wav", "peep.wav", "useful.wav",
        "delay.wav", "snap.wav", "splash.wav", "ui.wav", "unlock.wav",
    ]
    mods = ["island_line.mod", "yard.mod", "workshop.mod", "full_steam.mod"]

    for name in wavs:
        path = AUDIO_DIR / name
        if not path.exists():
            errors.append(f"missing {path}")
            continue
        with wave.open(str(path), "r") as handle:
            if handle.getnchannels() != 1:
                errors.append(f"{name}: expected mono")
            if handle.getsampwidth() != 1:
                errors.append(f"{name}: expected 8-bit")
            if handle.getframerate() != SAMPLE_RATE:
                errors.append(f"{name}: expected {SAMPLE_RATE} Hz")
            if handle.getnframes() < 200:
                errors.append(f"{name}: too short")

    for name in mods:
        path = AUDIO_DIR / name
        if not path.exists():
            errors.append(f"missing {path}")
            continue
        data = path.read_bytes()
        if len(data) < 1084 + 1024:
            errors.append(f"{name}: truncated")
            continue
        if data[1080:1084] != b"M.K.":
            errors.append(f"{name}: bad magic {data[1080:1084]!r}")
        song_len = data[950]
        if song_len < 1:
            errors.append(f"{name}: empty order")

    if errors:
        for error in errors:
            print(f"FAIL {error}", file=sys.stderr)
        return 1

    print("ok audio assets")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate original Thomas's Bridge audio.")
    parser.add_argument("--check", action="store_true", help="validate existing assets only")
    args = parser.parse_args()

    if not args.check:
        generate_sfx()
        generate_modules()
        generate_previews()
        print(f"Wrote SFX + modules to {AUDIO_DIR}")
        print(f"Wrote listenable previews to {PREVIEW_DIR}")

    return validate()


if __name__ == "__main__":
    raise SystemExit(main())

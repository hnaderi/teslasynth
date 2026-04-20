"""
Shared fixtures and skip markers.

Imports that require compiled extensions or native libraries are deferred into
fixtures so that pytest can collect and skip tests gracefully when the
environment is not fully set up.
"""

import pytest

try:
    import teslasynth._teslasynth  # noqa: F401

    HAS_EXTENSION = True
except ImportError:
    HAS_EXTENSION = False

requires_extension = pytest.mark.skipif(
    not HAS_EXTENSION,
    reason="C++ extension not built — run: uv sync --group dev --no-build-isolation",
)


def _make_440hz_pulses(step_us: int = 10_000):
    """
    Build a synthetic pulse array matching firmware output for a ~440 Hz note
    (period = 2272 µs) with max_on_time = 100 µs and min_deadtime = 100 µs.

    The firmware alternates between active pulses and silence gaps:
        (on=100, off=100)   ← fires the coil
        (on=0,   off=2072)  ← waits for next period

    With step_us = 10 000:
        4 active pulses × 100 µs = 400 µs on-time  →  4 % window duty
        Active pulse spacing = 2272 µs              →  ~440 Hz frequency
    """
    import numpy as np

    period = 2272  # µs  (1e6 / 440 ≈ 2272.7)
    on_us = 100
    dead = 100
    gap = period - on_us - dead  # 2072 µs

    rows = []
    t = 0
    while t + period <= step_us:
        rows.append([on_us, dead])
        rows.append([0, gap])
        t += period
    if t < step_us:
        rows.append([0, step_us - t])  # final pad

    return np.array(rows, dtype=np.uint32)


@pytest.fixture
def recording_440hz():
    """Recording with one 10 ms step of a synthesised ~440 Hz note."""
    from teslasynth.render import Recording

    return Recording(pulses=_make_440hz_pulses(), step_us=10_000)


@pytest.fixture
def simple_midi(tmp_path):
    """
    Write a minimal MIDI file with one C4 note lasting one beat at 120 BPM.

    tempo = 500 000 µs/beat, ticks_per_beat = 480:
        note_on  at tick   0 → 0 µs
        note_off at tick 480 → 500 000 µs
    """
    import mido

    mid = mido.MidiFile(ticks_per_beat=480)
    track = mido.MidiTrack()
    mid.tracks.append(track)
    track.append(mido.MetaMessage("set_tempo", tempo=500_000, time=0))
    track.append(mido.Message("note_on", channel=0, note=60, velocity=100, time=0))
    track.append(mido.Message("note_off", channel=0, note=60, velocity=0, time=480))
    path = tmp_path / "test.mid"
    mid.save(str(path))
    return str(path)

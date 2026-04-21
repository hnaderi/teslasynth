# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Convert mido MIDI messages to MidiChannelMessage and drive the synth from a
.mid file, yielding pulses in fixed-size time steps.
"""

from __future__ import annotations

from typing import Generator, NamedTuple

import mido

from ._teslasynth import MidiChannelMessage, Teslasynth
from ._types import NoteEvent


class _NoteKey(NamedTuple):
    channel: int
    note: int


class _PendingNote(NamedTuple):
    start_us: int
    velocity: int


def from_mido(msg: mido.Message) -> MidiChannelMessage | None:
    """Convert a mido Message to MidiChannelMessage.

    Returns None for non-channel messages (sysex, meta, clock, etc.).
    """
    if msg.type == "note_on":
        return MidiChannelMessage.note_on(msg.channel, msg.note, msg.velocity)
    if msg.type == "note_off":
        return MidiChannelMessage.note_off(msg.channel, msg.note, msg.velocity)
    if msg.type == "program_change":
        return MidiChannelMessage.program_change(msg.channel, msg.program)
    if msg.type == "pitchwheel":
        # mido range: -8192..8191 → MIDI spec: 0..16383 (centre 8192)
        return MidiChannelMessage.pitchbend(msg.channel, msg.pitch + 8192)
    if msg.type == "control_change":
        return MidiChannelMessage.control_change(msg.channel, msg.control, msg.value)
    return None


def _build_tempo_map(mid: mido.MidiFile) -> list[tuple[int, int]]:
    """Return a global tempo map as [(abs_tick, tempo_us), ...] sorted by tick.

    Tempo changes in SMF affect all tracks simultaneously regardless of which
    track they appear on. This collects them all into one sorted list.
    """
    changes: list[tuple[int, int]] = [(0, 500_000)]  # default 120 BPM
    for track in mid.tracks:
        abs_ticks = 0
        for msg in track:
            abs_ticks += msg.time
            if msg.type == "set_tempo":
                changes.append((abs_ticks, msg.tempo))
    changes.sort()
    return changes


def _ticks_to_us(
    abs_tick: int, ticks_per_beat: int, tempo_map: list[tuple[int, int]]
) -> int:
    """Convert an absolute tick position to microseconds using a global tempo map."""
    us = 0
    prev_tick, prev_tempo = 0, 500_000
    for t_tick, t_tempo in tempo_map:
        if t_tick >= abs_tick:
            break
        us += (t_tick - prev_tick) * prev_tempo // ticks_per_beat
        prev_tick, prev_tempo = t_tick, t_tempo
    us += (abs_tick - prev_tick) * prev_tempo // ticks_per_beat
    return us


def render_file(
    synth: Teslasynth,
    path: str,
    step_us: int = 10_000,
    channel: int = 0,
) -> Generator[tuple[int, list], None, None]:
    """Drive *synth* with the events in a Standard MIDI File.

    Yields ``(time_us, pulses)`` where *time_us* is the absolute position of
    the synthesis window start and *pulses* is the list of ``[on_us, off_us]``
    pairs for the requested output *channel*.

    Parameters
    ----------
    synth:
        A :class:`Teslasynth` instance (will be silenced at start).
    path:
        Path to the ``.mid`` file.
    step_us:
        Synthesis window size in microseconds (default 10 ms).
    channel:
        Output channel index to extract from the 8-channel result (default 0).
    """
    synth.off()
    mid = mido.MidiFile(path)
    tempo_map = _build_tempo_map(mid)

    # Collect all channel events from every track with absolute tick times,
    # then convert to microseconds using the single global tempo map.
    raw: list[tuple[int, MidiChannelMessage]] = []
    for track in mid.tracks:
        abs_ticks = 0
        for msg in track:
            abs_ticks += msg.time
            cm = from_mido(msg)
            if cm is not None:
                raw.append((abs_ticks, cm))

    if not raw:
        return

    raw.sort(key=lambda x: x[0])
    events = [
        (_ticks_to_us(tick, mid.ticks_per_beat, tempo_map), cm) for tick, cm in raw
    ]
    total_us = events[-1][0]

    event_idx = 0
    time_us = 0

    while time_us <= total_us + step_us:
        window_end = time_us + step_us
        while event_idx < len(events) and events[event_idx][0] < window_end:
            t, msg = events[event_idx]
            synth.handle(msg, t)
            event_idx += 1

        pulses = synth.sample_all(step_us)[channel]
        yield time_us, pulses
        time_us += step_us


def pulse_stream(
    synth: Teslasynth,
    path: str,
    step_us: int = 10_000,
    channel: int = 0,
) -> Generator[tuple[int, int, int], None, None]:
    """Flatten :func:`render_file` into individual ``(time_us, on_us, off_us)`` tuples.

    *time_us* is the absolute start time of each pulse from the first played
    pulse, derived from the cumulative pulse lengths — not from the wall clock.

    Parameters
    ----------
    channel:
        Output channel index to extract (default 0).
    """
    abs_us = 0
    for _, pulses in render_file(synth, path, step_us=step_us, channel=channel):
        for p in pulses:
            yield abs_us, p.on_us, p.off_us
            abs_us += p.on_us + p.off_us


def notes_from_midi(path: str) -> list[NoteEvent]:
    """Extract note events from a MIDI file.

    Returns a list of :class:`~teslasynth._types.NoteEvent` sorted by start
    time.
    """
    mid = mido.MidiFile(path)
    tempo_map = _build_tempo_map(mid)

    events: list[tuple[int, object]] = []
    for track in mid.tracks:
        abs_ticks = 0
        for msg in track:
            abs_ticks += msg.time
            if msg.type in ("note_on", "note_off"):
                events.append((abs_ticks, msg))
    events.sort(key=lambda x: x[0])

    pending: dict[_NoteKey, _PendingNote] = {}
    notes: list[NoteEvent] = []

    for abs_ticks, msg in events:
        time_us = _ticks_to_us(abs_ticks, mid.ticks_per_beat, tempo_map)
        key = _NoteKey(msg.channel, msg.note)
        if msg.type == "note_on" and msg.velocity > 0:
            pending[key] = _PendingNote(time_us, msg.velocity)
        else:
            if key in pending:
                p = pending.pop(key)
                notes.append(
                    NoteEvent(
                        channel=key.channel,
                        note=key.note,
                        velocity=p.velocity,
                        start_us=p.start_us,
                        end_us=time_us,
                    )
                )

    # Close any notes still open at end of file
    if events:
        tail_us = _ticks_to_us(events[-1][0], mid.ticks_per_beat, tempo_map)
        for key, p in pending.items():
            notes.append(
                NoteEvent(
                    channel=key.channel,
                    note=key.note,
                    velocity=p.velocity,
                    start_us=p.start_us,
                    end_us=tail_us,
                )
            )

    notes.sort(key=lambda n: n.start_us)
    return notes

# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Tests for teslasynth.midi — tempo map building, tick→µs conversion, note extraction.
"""

import pytest

from .conftest import requires_extension


@requires_extension
class TestBuildTempoMap:
    def test_default_tempo(self):
        """A file with no tempo message uses 120 BPM (500 000 µs/beat)."""
        import mido

        from teslasynth.midi import _build_tempo_map

        mid = mido.MidiFile(ticks_per_beat=480)
        mid.tracks.append(mido.MidiTrack())
        tmap = _build_tempo_map(mid)
        assert tmap[0] == (0, 500_000)

    def test_single_tempo_change(self):
        """Explicit set_tempo message overrides the default."""
        import mido

        from teslasynth.midi import _build_tempo_map

        mid = mido.MidiFile(ticks_per_beat=480)
        track = mido.MidiTrack()
        mid.tracks.append(track)
        track.append(mido.MetaMessage("set_tempo", tempo=1_000_000, time=0))
        tmap = _build_tempo_map(mid)
        assert any(tempo == 1_000_000 for _, tempo in tmap)

    def test_sorted_by_tick(self):
        """Tempo map entries are in ascending tick order."""
        import mido

        from teslasynth.midi import _build_tempo_map

        mid = mido.MidiFile(ticks_per_beat=480)
        track = mido.MidiTrack()
        mid.tracks.append(track)
        track.append(mido.MetaMessage("set_tempo", tempo=600_000, time=480))
        track.append(mido.MetaMessage("set_tempo", tempo=400_000, time=480))
        tmap = _build_tempo_map(mid)
        ticks = [t for t, _ in tmap]
        assert ticks == sorted(ticks)

    def test_fast_tempo_at_tick_zero_not_overridden_by_default(self):
        """A fast (>120 BPM) set_tempo at tick 0 must win over the 500_000 default.

        Bug: default (0, 500_000) was appended before sorting, so for any real
        tempo < 500_000 it sorted after the real entry and overwrote prev_tempo
        in _ticks_to_us, producing timings ~33% too slow for e.g. 160 BPM songs.
        """
        import mido

        from teslasynth.midi import _build_tempo_map, _ticks_to_us

        mid = mido.MidiFile(ticks_per_beat=480)
        track = mido.MidiTrack()
        mid.tracks.append(track)
        # 160 BPM = 375_000 us/beat, which is faster than the 120 BPM default
        track.append(mido.MetaMessage("set_tempo", tempo=375_000, time=0))
        tmap = _build_tempo_map(mid)
        # One beat (480 ticks) must take 375_000 us, not the default 500_000 us
        assert _ticks_to_us(480, 480, tmap) == 375_000


@requires_extension
class TestTicksToUs:
    """_ticks_to_us converts tick positions to microseconds using a tempo map."""

    def test_zero_tick_is_zero(self):
        from teslasynth.midi import _ticks_to_us

        tmap = [(0, 500_000)]
        assert _ticks_to_us(0, 480, tmap) == 0

    def test_one_beat_at_120bpm(self):
        from teslasynth.midi import _ticks_to_us

        # 120 BPM = 500 000 µs/beat; 1 beat = 480 ticks → 500 000 µs
        tmap = [(0, 500_000)]
        assert _ticks_to_us(480, 480, tmap) == 500_000

    def test_half_beat(self):
        from teslasynth.midi import _ticks_to_us

        tmap = [(0, 500_000)]
        assert _ticks_to_us(240, 480, tmap) == 250_000

    def test_tempo_change_mid_file(self):
        from teslasynth.midi import _ticks_to_us

        # Beat 0–1: 500 000 µs/beat; beat 1+: 1 000 000 µs/beat
        tmap = [(0, 500_000), (480, 1_000_000)]
        # Tick 480 = end of beat 1 = 500 000 µs
        assert _ticks_to_us(480, 480, tmap) == 500_000
        # Tick 960 = one beat after tempo change = 500 000 + 1 000 000 µs
        assert _ticks_to_us(960, 480, tmap) == 1_500_000


@requires_extension
class TestNotesFromMidi:
    def test_returns_note_events(self, simple_midi):
        from teslasynth import NoteEvent
        from teslasynth.midi import notes_from_midi

        notes = notes_from_midi(simple_midi)
        assert all(isinstance(n, NoteEvent) for n in notes)

    def test_single_note_extracted(self, simple_midi):
        from teslasynth.midi import notes_from_midi

        notes = notes_from_midi(simple_midi)
        assert len(notes) == 1

    def test_note_number(self, simple_midi):
        from teslasynth.midi import notes_from_midi

        note = notes_from_midi(simple_midi)[0]
        assert note.note == 60

    def test_note_channel(self, simple_midi):
        from teslasynth.midi import notes_from_midi

        note = notes_from_midi(simple_midi)[0]
        assert note.channel == 0

    def test_note_timing(self, simple_midi):
        from teslasynth.midi import notes_from_midi

        # 480 ticks at 120 BPM (500 000 µs/beat) → 500 000 µs duration
        note = notes_from_midi(simple_midi)[0]
        assert note.start_us == 0
        assert note.end_us == pytest.approx(500_000, rel=0.01)

    def test_note_velocity(self, simple_midi):
        from teslasynth.midi import notes_from_midi

        note = notes_from_midi(simple_midi)[0]
        assert note.velocity == 100

    def test_required_fields_present(self, simple_midi):
        import dataclasses

        from teslasynth.midi import notes_from_midi

        note = notes_from_midi(simple_midi)[0]
        field_names = {f.name for f in dataclasses.fields(note)}
        assert {"channel", "note", "velocity", "start_us", "end_us"} <= field_names

    def test_sorted_by_start(self, tmp_path):
        """Multiple notes are returned sorted by start_us."""
        import mido

        from teslasynth.midi import notes_from_midi

        mid = mido.MidiFile(ticks_per_beat=480)
        track = mido.MidiTrack()
        mid.tracks.append(track)
        track.append(mido.MetaMessage("set_tempo", tempo=500_000, time=0))
        # Note 60 starts at tick 480
        track.append(mido.Message("note_on", channel=0, note=60, velocity=80, time=480))
        track.append(mido.Message("note_off", channel=0, note=60, velocity=0, time=480))
        # Note 62 starts at tick 0 (earlier)
        track.append(mido.Message("note_on", channel=1, note=62, velocity=80, time=0))
        track.append(mido.Message("note_off", channel=1, note=62, velocity=0, time=480))
        path = tmp_path / "two.mid"
        mid.save(str(path))
        notes = notes_from_midi(str(path))
        starts = [n.start_us for n in notes]
        assert starts == sorted(starts)

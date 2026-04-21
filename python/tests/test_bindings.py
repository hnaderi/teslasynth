# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Binding coverage and sanity-check tests for the C++ extension.

Covers every public type and function exposed through _teslasynth and the
typed Python wrappers in teslasynth._types.
"""

import pytest

from .conftest import requires_extension

# ---------------------------------------------------------------------------
# Enums
# ---------------------------------------------------------------------------


@requires_extension
class TestInstrumentId:
    def test_values_exist(self):
        from teslasynth import InstrumentId

        assert hasattr(InstrumentId, "SquareWave")
        assert hasattr(InstrumentId, "SynthPluck")
        assert hasattr(InstrumentId, "FallFX")

    def test_distinct_values(self):
        from teslasynth import InstrumentId

        assert InstrumentId.SquareWave != InstrumentId.SynthPluck


@requires_extension
class TestPercussionId:
    def test_values_exist(self):
        from teslasynth import PercussionId

        assert hasattr(PercussionId, "Kick")
        assert hasattr(PercussionId, "Snare")
        assert hasattr(PercussionId, "Ride")

    def test_distinct_values(self):
        from teslasynth import PercussionId

        assert PercussionId.Kick != PercussionId.Snare


@requires_extension
class TestMidiMessageType:
    def test_values_exist(self):
        from teslasynth import MidiMessageType

        assert hasattr(MidiMessageType, "NoteOn")
        assert hasattr(MidiMessageType, "NoteOff")
        assert hasattr(MidiMessageType, "ProgramChange")
        assert hasattr(MidiMessageType, "PitchBend")


# ---------------------------------------------------------------------------
# Pulse
# ---------------------------------------------------------------------------


@requires_extension
class TestPulse:
    def test_construction(self):
        from teslasynth import Pulse

        p = Pulse(100, 200)
        assert p.on_us == 100
        assert p.off_us == 200

    def test_equality(self):
        from teslasynth import Pulse

        assert Pulse(100, 200) == Pulse(100, 200)
        assert Pulse(100, 200) != Pulse(100, 201)

    def test_repr(self):
        from teslasynth import Pulse

        r = repr(Pulse(50, 150))
        assert "50" in r and "150" in r

    def test_zero_pulse(self):
        from teslasynth import Pulse

        p = Pulse(0, 10_000)
        assert p.on_us == 0
        assert p.off_us == 10_000


# ---------------------------------------------------------------------------
# Envelope (PyEnvelope)
# ---------------------------------------------------------------------------


@requires_extension
class TestEnvelope:
    def test_adsr_fields(self):
        from teslasynth import InstrumentId, get_instrument

        info = get_instrument(InstrumentId.WarmPad)
        env = info.envelope
        assert env.type in ("adsr", "ad", "const")
        assert env.attack_ms >= 0.0
        assert env.decay_ms >= 0.0
        assert 0.0 <= env.sustain <= 1.0
        assert env.release_ms >= 0.0
        assert env.curve in ("linear", "exponential")

    def test_const_envelope(self):
        from teslasynth import InstrumentId, get_instrument

        info = get_instrument(InstrumentId.SquareWave)
        env = info.envelope
        assert env.type == "const"
        assert 0.0 <= env.sustain <= 1.0

    def test_repr_contains_type(self):
        from teslasynth import InstrumentId, get_instrument

        env = get_instrument(InstrumentId.SynthPluck).envelope
        assert env.type in repr(env)


# ---------------------------------------------------------------------------
# EnvelopeEngine
# ---------------------------------------------------------------------------


@requires_extension
class TestEnvelopeEngine:
    def test_instrument_init(self):
        from teslasynth import EnvelopeEngine, InstrumentId

        eng = EnvelopeEngine(InstrumentId.SynthPluck)
        assert not eng.is_off

    def test_percussion_init(self):
        from teslasynth import EnvelopeEngine, PercussionId

        eng = EnvelopeEngine(PercussionId.Kick)
        assert not eng.is_off

    def test_update_returns_amplitude(self):
        from teslasynth import EnvelopeEngine, InstrumentId

        eng = EnvelopeEngine(InstrumentId.SynthPluck)
        amp = eng.update(1_000, True)
        assert 0.0 <= amp <= 1.0

    def test_eventually_turns_off(self):
        from teslasynth import EnvelopeEngine, PercussionId

        eng = EnvelopeEngine(PercussionId.Kick)
        # AD envelopes are self-terminating; step 1 ms at a time for up to 10 s
        for _ in range(10_000):
            eng.update(1_000, False)
            if eng.is_off:
                break
        assert eng.is_off, "Kick envelope did not terminate within 10 s"


# ---------------------------------------------------------------------------
# MidiChannelMessage
# ---------------------------------------------------------------------------


@requires_extension
class TestMidiChannelMessage:
    def test_note_on(self):
        from teslasynth import MidiChannelMessage, MidiMessageType

        msg = MidiChannelMessage.note_on(0, 60, 100)
        assert msg.type == MidiMessageType.NoteOn
        assert msg.data0 == 60
        assert msg.data1 == 100

    def test_note_off(self):
        from teslasynth import MidiChannelMessage, MidiMessageType

        msg = MidiChannelMessage.note_off(1, 60, 0)
        assert msg.type == MidiMessageType.NoteOff

    def test_program_change(self):
        from teslasynth import MidiChannelMessage, MidiMessageType

        msg = MidiChannelMessage.program_change(0, 5)
        assert msg.type == MidiMessageType.ProgramChange

    def test_pitchbend(self):
        from teslasynth import MidiChannelMessage, MidiMessageType

        msg = MidiChannelMessage.pitchbend(0, 8192)
        assert msg.type == MidiMessageType.PitchBend

    def test_repr(self):
        from teslasynth import MidiChannelMessage

        r = repr(MidiChannelMessage.note_on(0, 60, 100))
        assert isinstance(r, str) and len(r) > 0


# ---------------------------------------------------------------------------
# ChannelConfig
# ---------------------------------------------------------------------------


@requires_extension
class TestChannelConfig:
    def test_defaults(self):
        from teslasynth import ChannelConfig

        ch = ChannelConfig()
        assert ch.max_on_time_us > 0
        assert ch.min_deadtime_us >= 0
        assert 0.0 < ch.max_duty_percent <= 100.0
        assert ch.notes >= 1
        assert ch.instrument is None

    def test_set_fields(self):
        from teslasynth import ChannelConfig

        ch = ChannelConfig()
        ch.max_on_time_us = 200
        ch.notes = 2
        assert ch.max_on_time_us == 200
        assert ch.notes == 2

    def test_instrument_override(self):
        from teslasynth import ChannelConfig

        ch = ChannelConfig()
        ch.instrument = 5
        assert ch.instrument == 5
        ch.instrument = None
        assert ch.instrument is None


# ---------------------------------------------------------------------------
# SynthConfig
# ---------------------------------------------------------------------------


@requires_extension
class TestSynthConfig:
    def test_default_tuning(self):
        from teslasynth import SynthConfig

        s = SynthConfig()
        assert s.tuning_hz == pytest.approx(440.0, rel=0.001)

    def test_set_tuning(self):
        from teslasynth import SynthConfig

        s = SynthConfig()
        s.tuning_hz = 432.0
        assert s.tuning_hz == pytest.approx(432.0, rel=0.001)


# ---------------------------------------------------------------------------
# RoutingConfig
# ---------------------------------------------------------------------------


@requires_extension
class TestRoutingConfig:
    def test_default_mapping_length(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        assert len(r.mapping) == 16

    def test_default_first_entries(self):
        """Default maps MIDI ch 0→output 0, ch 1→output 1, rest unmapped."""
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        assert r.mapping[0] == 0
        assert r.mapping[1] == 1

    def test_set_mapping_with_valid_indices(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        mapping = [i % 8 for i in range(16)]
        r.mapping = mapping
        assert r.mapping == mapping

    def test_set_mapping_with_none(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        mapping = [None] * 16
        r.mapping = mapping
        assert all(v is None for v in r.mapping)

    def test_invalid_output_index(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        with pytest.raises(Exception):
            r.mapping = [8] + [None] * 15  # 8 is out of range

    def test_wrong_mapping_length(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        with pytest.raises(Exception):
            r.mapping = [0] * 15

    def test_percussion_flag(self):
        from teslasynth import RoutingConfig

        r = RoutingConfig()
        assert isinstance(r.percussion, bool)
        r.percussion = True
        assert r.percussion is True


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------


@requires_extension
class TestConfiguration:
    def test_channels_size(self):
        from teslasynth import Configuration

        assert Configuration().channels_size == 8

    def test_channel_access(self):
        from teslasynth import ChannelConfig, Configuration

        cfg = Configuration()
        for i in range(8):
            ch = cfg.channel(i)
            assert isinstance(ch, ChannelConfig)

    def test_channel_out_of_range(self):
        from teslasynth import Configuration

        with pytest.raises(Exception):
            Configuration().channel(8)

    def test_synth_config(self):
        from teslasynth import Configuration, SynthConfig

        cfg = Configuration()
        assert isinstance(cfg.synth, SynthConfig)

    def test_routing_config(self):
        from teslasynth import Configuration, RoutingConfig

        cfg = Configuration()
        assert isinstance(cfg.routing, RoutingConfig)

    def test_set_firmware_syntax_tuning(self):
        from teslasynth import Configuration

        cfg = Configuration()
        cfg.set("synth.tuning=432hz")
        assert cfg.synth.tuning_hz == pytest.approx(432.0, rel=0.001)

    def test_set_firmware_syntax_output(self):
        from teslasynth import Configuration

        cfg = Configuration()
        cfg.set("output.1.max-duty=50")
        assert cfg.channel(0).max_duty_percent == pytest.approx(50.0, rel=0.01)

    def test_set_firmware_syntax_routing(self):
        from teslasynth import Configuration

        cfg = Configuration()
        cfg.set("routing.percussion=y")
        assert cfg.routing.percussion is True

    def test_set_invalid_expression_raises(self):
        from teslasynth import Configuration

        with pytest.raises(ValueError):
            Configuration().set("synth.tuning=notavalue")


# ---------------------------------------------------------------------------
# Instrument info (typed wrappers)
# ---------------------------------------------------------------------------


@requires_extension
class TestGetInstrument:
    def test_returns_instrument_info(self):
        from teslasynth import InstrumentId, InstrumentInfo, get_instrument

        info = get_instrument(InstrumentId.SynthPluck)
        assert isinstance(info, InstrumentInfo)

    def test_fields_populated(self):
        from teslasynth import InstrumentId, get_instrument

        info = get_instrument(InstrumentId.SynthPluck)
        assert isinstance(info.name, str) and len(info.name) > 0
        assert isinstance(info.index, int)
        assert info.vibrato_rate_hz >= 0.0
        assert info.vibrato_depth_hz >= 0.0

    def test_get_all_instruments_count(self):
        from teslasynth import get_all_instruments

        instruments = get_all_instruments()
        assert len(instruments) > 0

    def test_get_all_instruments_types(self):
        from teslasynth import InstrumentInfo, get_all_instruments

        for info in get_all_instruments():
            assert isinstance(info, InstrumentInfo)

    def test_indices_are_unique(self):
        from teslasynth import get_all_instruments

        indices = [i.index for i in get_all_instruments()]
        assert len(indices) == len(set(indices))

    def test_names_are_unique(self):
        from teslasynth import get_all_instruments

        names = [i.name for i in get_all_instruments()]
        assert len(names) == len(set(names))


# ---------------------------------------------------------------------------
# Percussion info (typed wrappers)
# ---------------------------------------------------------------------------


@requires_extension
class TestGetPercussion:
    def test_returns_percussion_info(self):
        from teslasynth import PercussionId, PercussionInfo, get_percussion

        info = get_percussion(PercussionId.Kick)
        assert isinstance(info, PercussionInfo)

    def test_fields_populated(self):
        from teslasynth import PercussionId, get_percussion

        info = get_percussion(PercussionId.Snare)
        assert isinstance(info.name, str) and len(info.name) > 0
        assert info.burst_ms >= 0.0
        assert info.prf_hz >= 0.0
        assert 0.0 <= info.noise <= 1.0
        assert 0.0 <= info.skip <= 1.0

    def test_get_all_percussions_types(self):
        from teslasynth import PercussionInfo, get_all_percussions

        for info in get_all_percussions():
            assert isinstance(info, PercussionInfo)

    def test_percussion_for_note(self):
        from teslasynth import PercussionInfo, percussion_for_note

        info = percussion_for_note(36)  # standard kick note
        assert isinstance(info, PercussionInfo)


# ---------------------------------------------------------------------------
# Build info and version
# ---------------------------------------------------------------------------


@requires_extension
class TestBuildInfo:
    def test_returns_build_info(self):
        from teslasynth import BuildInfo, build_info

        info = build_info()
        assert isinstance(info, BuildInfo)

    def test_fields_are_strings(self):
        from teslasynth import build_info

        info = build_info()
        assert isinstance(info.version, str)
        assert isinstance(info.date, str)
        assert isinstance(info.time, str)

    def test_version_nonempty(self):
        from teslasynth import build_info

        assert len(build_info().version) > 0

    def test_version_function(self):
        from teslasynth import version

        v = version()
        assert isinstance(v, str) and len(v) > 0


# ---------------------------------------------------------------------------
# Teslasynth engine
# ---------------------------------------------------------------------------


@requires_extension
class TestTeslasynth:
    def test_default_construction(self):
        from teslasynth import Teslasynth

        s = Teslasynth()
        assert s is not None

    def test_configured_construction(self):
        from teslasynth import Configuration, Teslasynth

        cfg = Configuration()
        cfg.set("synth.tuning=432hz")
        s = Teslasynth(cfg)
        assert s is not None

    def test_sample_all_returns_8_channels(self):
        from teslasynth import Teslasynth

        s = Teslasynth()
        result = s.sample_all(10_000)
        assert len(result) == 8

    def test_sample_all_channels_are_lists(self):
        from teslasynth import Teslasynth

        s = Teslasynth()
        result = s.sample_all(10_000)
        for ch in result:
            assert isinstance(ch, list)

    def test_sample_all_pulses_are_pulse_objects(self, simple_midi):
        from teslasynth import Pulse, Teslasynth
        from teslasynth.midi import render_file

        s = Teslasynth()
        # Feed one step of events and sample
        for _, pulses in render_file(s, simple_midi, step_us=10_000):
            for p in pulses:
                assert isinstance(p, Pulse)
            break

    def test_sample_all_pulse_values_nonnegative(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.midi import render_file

        s = Teslasynth()
        for _, pulses in render_file(s, simple_midi, step_us=10_000):
            for p in pulses:
                assert p.on_us >= 0
                assert p.off_us >= 0
            break

    def test_handle_note_on(self):
        from teslasynth import MidiChannelMessage, Teslasynth

        s = Teslasynth()
        s.handle(MidiChannelMessage.note_on(0, 60, 100), 0)
        result = s.sample_all(10_000)
        # Channel 0 should have at least one pulse after a note-on
        assert len(result[0]) > 0

    def test_off_silences(self):
        from teslasynth import MidiChannelMessage, Teslasynth

        s = Teslasynth()
        s.handle(MidiChannelMessage.note_on(0, 60, 100), 0)
        s.off()
        result = s.sample_all(10_000)
        assert all(len(ch) == 0 for ch in result)

    def test_configuration_property(self):
        from teslasynth import Configuration, Teslasynth

        s = Teslasynth()
        assert isinstance(s.configuration, Configuration)

    def test_reload_config(self):
        from teslasynth import Teslasynth

        s = Teslasynth()
        s.configuration.set("output.1.max-duty=50")
        s.reload_config()  # should not raise

    def test_invalid_time_raises(self):
        from teslasynth import MidiChannelMessage, Teslasynth

        s = Teslasynth()
        with pytest.raises(ValueError):
            s.handle(MidiChannelMessage.note_on(0, 60, 100), -1)

    def test_budget_too_large_raises(self):
        from teslasynth import Teslasynth

        s = Teslasynth()
        with pytest.raises(ValueError):
            s.sample_all(70_000)  # > 65535

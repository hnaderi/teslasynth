"""
Tests for teslasynth.render.Recording properties.

All data is constructed directly from numpy arrays — no MIDI rendering needed,
but the C++ extension must be importable (render.py imports Teslasynth at the
module level).
"""

import pytest

from .conftest import requires_extension


@requires_extension
class TestStartTimes:
    def test_first_pulse_starts_at_zero(self, recording_440hz):
        times = recording_440hz.start_times_us
        assert times[0] == 0

    def test_second_pulse_starts_after_first(self, recording_440hz):
        # Pulse 0: on=100, off=100 → length 200 µs
        times = recording_440hz.start_times_us
        assert times[1] == 200

    def test_third_pulse_starts_at_one_period(self, recording_440hz):
        # Pulses 0+1 span exactly one 2272 µs period
        times = recording_440hz.start_times_us
        assert times[2] == 2272

    def test_monotonically_increasing(self, recording_440hz):
        import numpy as np

        times = recording_440hz.start_times_us
        assert np.all(np.diff(times) > 0)


@requires_extension
class TestDurationUs:
    def test_total_equals_step(self, recording_440hz):
        # make_440hz_pulses pads the last entry so pulses fill step_us exactly
        assert recording_440hz.duration_us == recording_440hz.step_us

    def test_sum_of_pulse_lengths(self, recording_440hz):
        import numpy as np

        pulses = recording_440hz.pulses
        on = pulses[:, 0].astype(np.int64)
        off = pulses[:, 1].astype(np.int64)
        expected = int((on + off).sum())
        assert recording_440hz.duration_us == expected


@requires_extension
class TestDutyCycle:
    def test_active_pulse_duty(self, recording_440hz):
        # Each active pulse: on=100, off=100 → per-pulse duty = 0.5
        duty = recording_440hz.duty_cycle
        active = recording_440hz.pulses[:, 0] > 0
        assert pytest.approx(duty[active], abs=1e-6) == 0.5

    def test_silence_pulse_duty_is_zero(self, recording_440hz):
        import numpy as np

        duty = recording_440hz.duty_cycle
        silent = recording_440hz.pulses[:, 0] == 0
        assert np.all(duty[silent] == 0.0)

"""
Tests for the internal plot helpers that compute frequency and duty from pulses.

These verify the *correct* calculations introduced to replace the broken
per-pulse frequency (always showed 5 kHz) and per-pulse duty (always 50%).
"""

import pytest

from .conftest import requires_extension


@requires_extension
class TestFreqFromPulses:
    def test_440hz_frequency(self, recording_440hz):
        from teslasynth.plot import _freq_from_pulses

        _, freq = _freq_from_pulses(recording_440hz)
        # Active pulse spacing = 2272 µs → 1e6/2272 = 440.14 Hz
        assert len(freq) > 0
        assert pytest.approx(freq, rel=0.01) == 1e6 / 2272

    def test_returns_microsecond_times(self, recording_440hz):
        import numpy as np

        from teslasynth.plot import _freq_from_pulses

        t_us, _ = _freq_from_pulses(recording_440hz)
        # Midpoints between active pulses should be in µs range (> 100)
        assert np.all(t_us > 100)

    def test_window_filtering(self, recording_440hz):
        import numpy as np

        from teslasynth.plot import _freq_from_pulses

        # Restrict to first 3 ms — should only capture active pulses in that window
        t_us, _ = _freq_from_pulses(recording_440hz, end_ms=3.0)
        assert np.all(t_us <= 3000)

    def test_min_freq_filter(self, recording_440hz):
        from teslasynth.plot import _freq_from_pulses

        _, freq = _freq_from_pulses(recording_440hz, min_freq=1000.0)
        # 440 Hz < 1000 Hz → nothing passes
        assert len(freq) == 0

    def test_empty_on_no_active_pulses(self):
        import numpy as np

        from teslasynth.plot import _freq_from_pulses
        from teslasynth.render import Recording

        silent = np.array([[0, 10_000]], dtype=np.uint32)
        rec = Recording(pulses=silent, step_us=10_000)
        t, f = _freq_from_pulses(rec)
        assert len(t) == 0 and len(f) == 0


@requires_extension
class TestDutyByStep:
    def test_correct_window_duty(self, recording_440hz):
        from teslasynth.plot import _duty_by_step

        _, duty = _duty_by_step(recording_440hz)
        # 4 active pulses × 100 µs on / 10 000 µs step = 4 %
        assert len(duty) > 0
        assert pytest.approx(duty[0], rel=0.01) == 0.04

    def test_not_per_pulse_duty(self, recording_440hz):
        from teslasynth.plot import _duty_by_step

        _, duty = _duty_by_step(recording_440hz)
        # Per-pulse duty would be 0.5 (100/200); window duty must be much lower
        assert duty[0] < 0.10

    def test_returns_microsecond_times(self, recording_440hz):
        from teslasynth.plot import _duty_by_step

        t_us, _ = _duty_by_step(recording_440hz)
        # Step midpoint at step_us//2 = 5000 µs
        assert pytest.approx(t_us[0], abs=1) == 5000.0

    def test_zero_duty_for_silence(self):
        import numpy as np

        from teslasynth.plot import _duty_by_step
        from teslasynth.render import Recording

        silent = np.array([[0, 10_000]], dtype=np.uint32)
        rec = Recording(pulses=silent, step_us=10_000)
        _, duty = _duty_by_step(rec)
        assert np.all(duty == 0.0)


@requires_extension
class TestBuildSignalTrace:
    def test_returns_nonempty_for_active_window(self, recording_440hz):
        from teslasynth.plot import _build_signal_trace

        xs, ys = _build_signal_trace(recording_440hz, 0.0, 10.0)
        assert len(xs) > 0 and len(ys) > 0

    def test_y_values_are_zero_or_one(self, recording_440hz):
        from teslasynth.plot import _build_signal_trace

        _, ys = _build_signal_trace(recording_440hz, 0.0, 10.0)
        assert all(y in (0.0, 1.0) for y in ys)

    def test_x_values_in_microseconds(self, recording_440hz):
        from teslasynth.plot import _build_signal_trace

        xs, _ = _build_signal_trace(recording_440hz, 0.0, 10.0)
        # xs should be in µs (0–10 000), not seconds (0–0.01)
        assert max(xs) > 100

    def test_empty_window_returns_flat_line(self, recording_440hz):
        from teslasynth.plot import _build_signal_trace

        xs, ys = _build_signal_trace(recording_440hz, 20.0, 30.0)
        assert all(y == 0.0 for y in ys)

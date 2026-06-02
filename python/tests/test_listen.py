# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Tests for teslasynth.listen._pulses_to_float32.

No C++ extension or audio hardware required — the function is pure Python + numpy.
"""

from types import SimpleNamespace

import numpy as np

from teslasynth.listen import _pulses_to_float32


def _pulse(on_us: int, off_us: int):
    return SimpleNamespace(on_us=on_us, off_us=off_us)


SR = 1_000_000  # 1 MHz: 1 sample == 1 µs, makes assertions exact


class TestEmpty:
    def test_no_pulses_returns_zeros(self):
        out = _pulses_to_float32([], n_samples=100, sample_rate=SR)
        assert out.shape == (100,)
        assert out.dtype == np.float32
        assert np.all(out == 0.0)


class TestSilentPulse:
    def test_on_zero_produces_no_output(self):
        pulses = [_pulse(0, 500)]
        out = _pulses_to_float32(pulses, n_samples=1000, sample_rate=SR)
        assert np.all(out == 0.0)


class TestSinglePulse:
    def test_pulse_fills_correct_range(self):
        # on=200µs, off=300µs; at 1 MHz → samples 0..199 should be 1.0
        pulses = [_pulse(200, 300)]
        out = _pulses_to_float32(pulses, n_samples=500, sample_rate=SR)
        assert np.all(out[:200] == 1.0)
        assert np.all(out[200:] == 0.0)

    def test_pulse_exactly_fills_buffer(self):
        pulses = [_pulse(100, 0)]
        out = _pulses_to_float32(pulses, n_samples=100, sample_rate=SR)
        assert np.all(out == 1.0)

    def test_pulse_clipped_at_buffer_end(self):
        # Pulse longer than buffer — should not overflow
        pulses = [_pulse(200, 0)]
        out = _pulses_to_float32(pulses, n_samples=100, sample_rate=SR)
        assert out.shape == (100,)
        assert np.all(out == 1.0)


class TestMultiplePulses:
    def test_two_pulses_with_gap(self):
        # pulse at 0..99, gap 100..199, pulse at 200..299
        pulses = [_pulse(100, 100), _pulse(100, 0)]
        out = _pulses_to_float32(pulses, n_samples=400, sample_rate=SR)
        assert np.all(out[:100] == 1.0)
        assert np.all(out[100:200] == 0.0)
        assert np.all(out[200:300] == 1.0)
        assert np.all(out[300:] == 0.0)

    def test_values_are_zero_or_one(self):
        pulses = [_pulse(50, 50)] * 5
        out = _pulses_to_float32(pulses, n_samples=500, sample_rate=SR)
        assert set(out.tolist()).issubset({0.0, 1.0})


class TestSampleRate:
    def test_48khz_440hz_note_has_active_samples(self):
        # 440 Hz period ~= 2272 µs; on=100 µs, off=2172 µs
        # At 48 kHz: on_samples = int(100 * 48000 / 1e6) = 4
        pulses = [_pulse(100, 2172)] * 4
        out = _pulses_to_float32(pulses, n_samples=480, sample_rate=48_000)
        assert out.sum() > 0

    def test_output_dtype_is_float32(self):
        pulses = [_pulse(100, 100)]
        out = _pulses_to_float32(pulses, n_samples=200, sample_rate=48_000)
        assert out.dtype == np.float32

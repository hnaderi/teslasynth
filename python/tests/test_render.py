# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Tests for teslasynth.render — signal_stream and signal_stream_all_channels.
"""

import numpy as np

from .conftest import requires_extension


@requires_extension
class TestSignalStreamAllChannels:
    def test_yields_correct_shape(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.render import signal_stream_all_channels

        synth = Teslasynth()
        chunks = list(
            signal_stream_all_channels(
                synth, simple_midi, sample_rate=44_100, step_us=10_000
            )
        )
        assert len(chunks) > 0
        for chunk in chunks:
            assert chunk.ndim == 2
            assert chunk.shape[1] == 8

    def test_dtype_is_uint8(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.render import signal_stream_all_channels

        synth = Teslasynth()
        chunk = next(
            iter(
                signal_stream_all_channels(
                    synth, simple_midi, sample_rate=44_100, step_us=10_000
                )
            )
        )
        assert chunk.dtype == np.uint8

    def test_values_are_binary(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.render import signal_stream_all_channels

        synth = Teslasynth()
        for chunk in signal_stream_all_channels(
            synth, simple_midi, sample_rate=44_100, step_us=10_000
        ):
            assert np.all((chunk == 0) | (chunk == 1))

    def test_channel_count_matches_signal_stream(self, simple_midi):
        """Each column of signal_stream_all_channels matches
        the corresponding signal_stream channel."""
        from teslasynth import Teslasynth
        from teslasynth.render import signal_stream, signal_stream_all_channels

        for ch in range(8):
            synth_single = Teslasynth()
            synth_all = Teslasynth()
            single = np.concatenate(
                list(
                    signal_stream(
                        synth_single,
                        simple_midi,
                        sample_rate=44_100,
                        step_us=10_000,
                        channel=ch,
                    )
                )
            )
            all_ch = np.concatenate(
                [
                    c[:, ch]
                    for c in signal_stream_all_channels(
                        synth_all, simple_midi, sample_rate=44_100, step_us=10_000
                    )
                ]
            )
            assert np.array_equal(single, all_ch)


@requires_extension
class TestRenderFileAllChannels:
    def test_yields_eight_channels(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.midi import render_file_all_channels

        synth = Teslasynth()
        _, all_channels = next(iter(render_file_all_channels(synth, simple_midi)))
        assert len(all_channels) == 8

    def test_each_channel_is_list(self, simple_midi):
        from teslasynth import Teslasynth
        from teslasynth.midi import render_file_all_channels

        synth = Teslasynth()
        for _, all_channels in render_file_all_channels(synth, simple_midi):
            for ch in all_channels:
                assert isinstance(ch, list)

    def test_consistent_with_render_file(self, simple_midi):
        """render_file_all_channels[channel] must equal render_file for same channel."""
        from teslasynth import Teslasynth
        from teslasynth.midi import render_file, render_file_all_channels

        for ch in range(8):
            synth_a = Teslasynth()
            synth_b = Teslasynth()
            single = [
                (t, pulses)
                for t, pulses in render_file(synth_a, simple_midi, channel=ch)
            ]
            all_ch = [
                (t, chans[ch])
                for t, chans in render_file_all_channels(synth_b, simple_midi)
            ]
            assert len(single) == len(all_ch)
            for (t1, p1), (t2, p2) in zip(single, all_ch):
                assert t1 == t2
                assert len(p1) == len(p2)

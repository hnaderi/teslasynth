# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Tests for teslasynth.wav — format detection, scaling, and file output.
"""

import numpy as np
import pytest

from .conftest import requires_extension


@requires_extension
class TestIsFlac:
    def test_flac_extension(self):
        from teslasynth.wav import _is_flac

        assert _is_flac("out.flac") is True

    def test_flac_uppercase(self):
        from teslasynth.wav import _is_flac

        assert _is_flac("out.FLAC") is True

    def test_wav_extension(self):
        from teslasynth.wav import _is_flac

        assert _is_flac("out.wav") is False

    def test_wav_uppercase(self):
        from teslasynth.wav import _is_flac

        assert _is_flac("out.WAV") is False

    def test_path_with_directories(self):
        from teslasynth.wav import _is_flac

        assert _is_flac("/some/path/recording.flac") is True
        assert _is_flac("/some/path/recording.wav") is False


@requires_extension
class TestToInt16:
    def test_zero_stays_zero(self):
        from teslasynth.wav import _to_int16

        arr = np.array([0], dtype=np.uint8)
        assert _to_int16(arr)[0] == 0

    def test_one_becomes_32767(self):
        from teslasynth.wav import _to_int16

        arr = np.array([1], dtype=np.uint8)
        assert _to_int16(arr)[0] == 32767

    def test_output_dtype_is_int16(self):
        from teslasynth.wav import _to_int16

        arr = np.array([0, 1, 0, 1], dtype=np.uint8)
        assert _to_int16(arr).dtype == np.int16

    def test_2d_array_preserved(self):
        from teslasynth.wav import _to_int16

        arr = np.array([[0, 1], [1, 0]], dtype=np.uint8)
        result = _to_int16(arr)
        assert result.shape == (2, 2)
        assert result[0, 1] == 32767
        assert result[1, 0] == 32767


try:
    import soundfile  # noqa: F401

    HAS_SOUNDFILE = True
except ImportError:
    HAS_SOUNDFILE = False

requires_soundfile = pytest.mark.skipif(
    not HAS_SOUNDFILE, reason="soundfile not installed"
)


@requires_extension
@requires_soundfile
class TestWrite:
    def test_creates_wav_file(self, tmp_path, simple_midi):
        from teslasynth.wav import write

        out = str(tmp_path / "out.wav")
        write(simple_midi, out, sample_rate=44_100)
        assert (tmp_path / "out.wav").exists()

    def test_creates_flac_file(self, tmp_path, simple_midi):
        from teslasynth.wav import write

        out = str(tmp_path / "out.flac")
        write(simple_midi, out, sample_rate=44_100)
        assert (tmp_path / "out.flac").exists()

    def test_wav_is_mono_by_default(self, tmp_path, simple_midi):
        import soundfile as sf

        from teslasynth.wav import write

        out = str(tmp_path / "out.wav")
        write(simple_midi, out, sample_rate=44_100)
        info = sf.info(out)
        assert info.channels == 1

    def test_multichannel_wav_has_correct_channel_count(self, tmp_path, simple_midi):
        import soundfile as sf

        from teslasynth.wav import write

        out = str(tmp_path / "out.wav")
        write(simple_midi, out, sample_rate=44_100, channels=[0, 1, 2])
        info = sf.info(out)
        assert info.channels == 3

    def test_all_channels_flac(self, tmp_path, simple_midi):
        import soundfile as sf

        from teslasynth.wav import write

        out = str(tmp_path / "out.flac")
        write(simple_midi, out, sample_rate=44_100, channels=list(range(8)))
        info = sf.info(out)
        assert info.channels == 8

    def test_sample_rate_preserved(self, tmp_path, simple_midi):
        import soundfile as sf

        from teslasynth.wav import write

        out = str(tmp_path / "out.wav")
        write(simple_midi, out, sample_rate=44_100)
        assert sf.info(out).samplerate == 44_100

    def test_write_recording(self, tmp_path):
        import numpy as np
        import soundfile as sf

        from teslasynth.render import Recording
        from teslasynth.wav import write_recording

        pulses = np.array([[1_000, 1_000]], dtype=np.uint32)
        rec = Recording(pulses=pulses, step_us=2_000)
        out = str(tmp_path / "rec.flac")
        write_recording(rec, out, sample_rate=44_100)
        assert (tmp_path / "rec.flac").exists()
        assert sf.info(out).channels == 1

# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Export coil signal to WAV or FLAC.

The output format is chosen from the file extension:

* ``.wav``  — 16-bit PCM via soundfile (correct multichannel headers for any
              channel count)
* ``.flac`` — lossless FLAC via soundfile (~10–15× smaller than WAV for coil
              signals)

Both require the ``wav`` extra: ``pip install teslasynth[wav]``.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from ._teslasynth import Teslasynth


def _is_flac(path: str) -> bool:
    return Path(path).suffix.lower() == ".flac"


def _soundfile(path: str, sample_rate: int, n_channels: int, is_flac: bool):
    try:
        import soundfile as sf
    except ImportError:
        raise ImportError(
            "soundfile is required for WAV/FLAC output. "
            "Install it with: pip install teslasynth[wav]"
        ) from None
    return sf.SoundFile(
        path,
        "w",
        samplerate=sample_rate,
        channels=n_channels,
        format="FLAC" if is_flac else "WAV",
        subtype="PCM_16",
    )


def _to_int16(chunk: np.ndarray) -> np.ndarray:
    return chunk.astype(np.int16) * 32767


def write(
    path_mid: str,
    path_out: str,
    synth: Teslasynth | None = None,
    sample_rate: int = 192_000,
    step_us: int = 10_000,
    channels: int | list[int] = 0,
) -> None:
    """Stream a MIDI file to a WAV or FLAC file.

    The output format is selected from the file extension (``.wav`` / ``.flac``).
    FLAC is strongly recommended for multichannel or long recordings.

    Parameters
    ----------
    path_mid:
        Input ``.mid`` file.
    path_out:
        Output file path.  Extension determines format (``.wav`` / ``.flac``).
    synth:
        Optional pre-configured :class:`~teslasynth._teslasynth.Teslasynth`.
    sample_rate:
        Samples per second.
    step_us:
        Synthesis window size in microseconds.
    channels:
        Which output channel(s) to render.  Pass a single ``int`` for a
        mono file, or a ``list[int]`` for a multichannel file (one audio
        track per channel index).  Valid indices are 0–7.
    """
    from .render import signal_stream, signal_stream_all_channels

    if synth is None:
        synth = Teslasynth()

    ch_list = [channels] if isinstance(channels, int) else list(channels)
    flac = _is_flac(path_out)

    with _soundfile(path_out, sample_rate, len(ch_list), flac) as sf:
        if len(ch_list) == 1:
            for chunk in signal_stream(
                synth,
                path_mid,
                sample_rate=sample_rate,
                step_us=step_us,
                channel=ch_list[0],
            ):
                sf.write(_to_int16(chunk))
        else:
            for chunk in signal_stream_all_channels(
                synth,
                path_mid,
                sample_rate=sample_rate,
                step_us=step_us,
            ):
                sf.write(_to_int16(chunk[:, ch_list]))


def write_recording(
    recording,
    path_out: str,
    sample_rate: int = 192_000,
) -> None:
    """Write a pre-built :class:`~teslasynth.render.Recording` to WAV or FLAC.

    Use :func:`write` when exporting directly from a MIDI file — it is more
    memory-efficient.
    """
    flac = _is_flac(path_out)
    signal = recording.to_signal(sample_rate)

    with _soundfile(path_out, sample_rate, 1, flac) as sf:
        sf.write(_to_int16(signal))

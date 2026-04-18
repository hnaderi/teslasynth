"""
Export coil signal to WAV — either from a :class:`~teslasynth.render.Recording`
or streamed directly from a MIDI file without materializing the full signal.
"""
from __future__ import annotations

import wave

import numpy as np

from ._teslasynth import Teslasynth


def write(
    path_mid: str,
    path_wav: str,
    synth: Teslasynth | None = None,
    sample_rate: int = 192_000,
    step_us: int = 10_000,
    channel: int = 0,
) -> None:
    """Stream a MIDI file to a WAV file without holding the full signal in RAM.

    At 192 kHz a 5-minute recording is ~1.1 GB as a flat array — this avoids
    that by writing each synthesis window as it is produced.

    Parameters
    ----------
    path_mid:
        Input ``.mid`` file.
    path_wav:
        Output ``.wav`` file.
    synth:
        Optional pre-configured :class:`~teslasynth._teslasynth.Teslasynth`.
        A default instance is created if not provided.
    sample_rate:
        Samples per second.  192 kHz faithfully captures typical coil pulse
        widths; 44100 Hz is fine for casual listening.
    step_us:
        Synthesis window size in microseconds.
    channel:
        Output channel index to render (default 0).
    """
    from .render import signal_stream

    if synth is None:
        synth = Teslasynth()

    with wave.open(path_wav, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)      # 16-bit PCM
        wf.setframerate(sample_rate)
        for chunk in signal_stream(synth, path_mid, sample_rate=sample_rate,
                                   step_us=step_us, channel=channel):
            wf.writeframes((chunk.astype(np.int16) * 32767).tobytes())


def write_recording(
    recording,
    path_wav: str,
    sample_rate: int = 192_000,
) -> None:
    """Write a pre-built :class:`~teslasynth.render.Recording` to a WAV file.

    Use :func:`write` instead when exporting directly from a MIDI file —
    it is more memory-efficient.  This function is useful when you already
    have a ``Recording`` for analysis and also want an audio export.
    """
    with wave.open(path_wav, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        signal = recording.to_signal(sample_rate)
        wf.writeframes((signal.astype(np.int16) * 32767).tobytes())

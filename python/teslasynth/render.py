"""
Collect synthesis output into a :class:`Recording` and derive analysis signals.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Generator, Iterable

import numpy as np

from ._teslasynth import Teslasynth
from .midi import pulse_stream, render_file


@dataclass
class Recording:
    """All pulse data from a completed synthesis run.

    Build one via :func:`from_file` or :meth:`from_stream`, then use the
    properties to derive analysis signals.

    Attributes
    ----------
    pulses:
        ``(N, 2)`` uint32 array — each row is ``[on_us, off_us]``.
    step_us:
        The synthesis step size used to produce this recording.
    """

    pulses: np.ndarray  # shape (N, 2), dtype uint32
    step_us: int = 10_000

    # ------------------------------------------------------------------
    # Derived signals
    # ------------------------------------------------------------------

    @property
    def start_times_us(self) -> np.ndarray:
        """Absolute start time (µs) of each pulse."""
        lengths = self.pulses[:, 0].astype(np.int64) + self.pulses[:, 1].astype(np.int64)
        return np.concatenate([[0], np.cumsum(lengths)[:-1]])

    @property
    def duration_us(self) -> int:
        """Total recording duration in microseconds."""
        return int(
            (self.pulses[:, 0].astype(np.int64) + self.pulses[:, 1].astype(np.int64)).sum()
        )

    @property
    def frequency_hz(self) -> np.ndarray:
        """Instantaneous frequency (Hz) per pulse. Silent pulses yield 0."""
        on  = self.pulses[:, 0].astype(float)
        off = self.pulses[:, 1].astype(float)
        period = on + off
        return np.where((period > 0) & (on > 0), 1e6 / period, 0.0)

    @property
    def duty_cycle(self) -> np.ndarray:
        """Duty cycle [0, 1] per pulse."""
        on     = self.pulses[:, 0].astype(float)
        period = on + self.pulses[:, 1].astype(float)
        return np.where(period > 0, on / period, 0.0)

    def to_signal(self, sample_rate: int = 192_000) -> np.ndarray:
        """Render the pulse stream as a digital (0/1) signal array.

        Parameters
        ----------
        sample_rate:
            Samples per second.  192 kHz gives ~5 µs resolution.
        """
        if len(self.pulses) == 0:
            return np.array([], dtype=np.uint8)

        starts_us = self.start_times_us
        on_us     = self.pulses[:, 0].astype(np.int64)
        total_us  = int(starts_us[-1] + on_us[-1] + self.pulses[-1, 1])

        n      = total_us * sample_rate // 1_000_000 + 1
        signal = np.zeros(n, dtype=np.uint8)

        mask    = on_us > 0
        s_start = (starts_us[mask] * sample_rate // 1_000_000).astype(np.int64)
        s_end   = ((starts_us[mask] + on_us[mask]) * sample_rate // 1_000_000).astype(np.int64)
        s_end   = np.minimum(s_end, n)

        for s, e in zip(s_start, s_end):
            if e > s:
                signal[s:e] = 1

        return signal

    # ------------------------------------------------------------------
    # Factories
    # ------------------------------------------------------------------

    @classmethod
    def from_stream(
        cls,
        stream: Iterable[tuple[int, int, int]],
        step_us: int = 10_000,
    ) -> Recording:
        """Build a Recording by consuming a :func:`~teslasynth.midi.pulse_stream`.

        Parameters
        ----------
        stream:
            Any iterable of ``(time_us, on_us, off_us)`` tuples.
        step_us:
            Step size recorded for reference; does not affect the data.
        """
        pulses = [(on, off) for _, on, off in stream]
        arr = np.array(pulses, dtype=np.uint32) if pulses else np.empty((0, 2), dtype=np.uint32)
        return cls(pulses=arr, step_us=step_us)


# ------------------------------------------------------------------
# Streaming signal generator
# ------------------------------------------------------------------

def signal_stream(
    synth: Teslasynth,
    path: str,
    sample_rate: int = 192_000,
    step_us: int = 10_000,
) -> Generator[np.ndarray, None, None]:
    """Yield signal chunks (uint8 0/1), one per synthesis step.

    Never materializes the full signal — suitable for streaming WAV export
    or real-time processing of long recordings.

    Parameters
    ----------
    synth:
        A :class:`~teslasynth._teslasynth.Teslasynth` instance.
    path:
        Path to the ``.mid`` file.
    sample_rate:
        Samples per second.
    step_us:
        Synthesis window size in microseconds.
    """
    spus = sample_rate / 1_000_000  # samples per microsecond

    for _, pulses in render_file(synth, path, step_us=step_us):
        if not pulses:
            yield np.zeros(int(step_us * spus), dtype=np.uint8)
            continue

        total_us = sum(p[0] + p[1] for p in pulses)
        n     = int(total_us * spus) + 1
        chunk = np.zeros(n, dtype=np.uint8)
        pos   = 0
        for on_us, off_us in pulses:
            if on_us > 0:
                s = int(pos * spus)
                e = min(int((pos + on_us) * spus), n)
                chunk[s:e] = 1
            pos += on_us + off_us
        yield chunk


# ------------------------------------------------------------------
# Convenience factory
# ------------------------------------------------------------------

def from_file(
    path: str,
    synth: Teslasynth | None = None,
    step_us: int = 10_000,
) -> Recording:
    """Render a MIDI file and return a :class:`Recording`.

    Convenience wrapper around :meth:`Recording.from_stream`.
    """
    if synth is None:
        synth = Teslasynth()
    return Recording.from_stream(
        pulse_stream(synth, path, step_us=step_us),
        step_us=step_us,
    )

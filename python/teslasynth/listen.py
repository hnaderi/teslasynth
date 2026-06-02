# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Real-time MIDI → teslasynth → audio pipeline.

Requires the ``listen`` extras::

    pip install teslasynth[listen]
"""

from __future__ import annotations

import queue
import time
from typing import TYPE_CHECKING

import numpy as np

if TYPE_CHECKING:
    from ._teslasynth import Teslasynth


def list_ports() -> None:
    """Print available MIDI input ports and audio output devices."""
    try:
        import mido
        import sounddevice as sd
    except ImportError as exc:
        raise ImportError(
            f"{exc}\n\nInstall the listen extras: pip install teslasynth[listen]"
        ) from exc

    names = mido.get_input_names()
    print("MIDI input ports:")
    if names:
        for name in names:
            print(f"  {name}")
    else:
        print("  (none found)")

    print("\nAudio output devices:")
    default_out = sd.default.device[1]
    for i, dev in enumerate(sd.query_devices()):
        if dev["max_output_channels"] > 0:
            marker = " *" if i == default_out else ""
            print(f"  {i}: {dev['name']}{marker}")


def _pulses_to_float32(pulses, n_samples: int, sample_rate: int) -> np.ndarray:
    chunk = np.zeros(n_samples, dtype=np.float32)
    spus = sample_rate / 1_000_000
    pos_us = 0
    for p in pulses:
        if p.on_us > 0:
            s = int(pos_us * spus)
            e = min(int((pos_us + p.on_us) * spus), n_samples)
            if e > s:
                chunk[s:e] = 1.0
        pos_us += p.on_us + p.off_us
    return chunk


_VIRTUAL_PORT_NAME = "teslasynth"


def listen(
    synth: Teslasynth,
    *,
    midi_port: str | None = None,
    sample_rate: int = 48_000,
    blocksize: int = 512,
    channels: int | list[int] = 0,
    audio_device: int | str | None = None,
) -> None:
    """Listen on a MIDI input port and stream synthesised audio to speakers.

    Parameters
    ----------
    synth:
        Configured :class:`~teslasynth.Teslasynth` instance.
    midi_port:
        MIDI input port name (from :func:`list_ports`).  ``None`` opens a
        virtual port named ``"teslasynth"`` that other software can connect to.
    sample_rate:
        Audio output sample rate in Hz (default: 48 000).
    blocksize:
        Audio output buffer size in frames (default: 512 approx 10.7 ms at 48 kHz).
        Smaller values reduce latency but increase CPU load.
    channels:
        Teslasynth output channel(s) to listen on.  A single int selects one
        channel; a list selects multiple, each mapped to its own audio output
        channel.  Accepts the same expressions as the render command.
    audio_device:
        PortAudio device index or name substring.  ``None`` uses the system
        default output device.
    """
    try:
        import mido
        import sounddevice as sd
    except ImportError as exc:
        raise ImportError(
            f"{exc}\n\nInstall the listen extras: pip install teslasynth[listen]"
        ) from exc

    ch_indices: list[int] = [channels] if isinstance(channels, int) else list(channels)
    n_out = len(ch_indices)
    step_us = blocksize * 1_000_000 // sample_rate

    # MidiChannelMessage objects produced by the MIDI thread, consumed by the
    # audio callback.  SimpleQueue is lock-free and safe for cross-thread use.
    midi_queue: queue.SimpleQueue = queue.SimpleQueue()

    synth.off()
    audio_time_us = 0

    def audio_callback(outdata, frames, time_info, status):
        nonlocal audio_time_us
        # Drain pending MIDI events, scheduling them at the window start.
        while True:
            try:
                cm = midi_queue.get_nowait()
                synth.handle(cm, audio_time_us)
            except queue.Empty:
                break
        all_channels = synth.sample_all(step_us)
        for col, ch in enumerate(ch_indices):
            outdata[:, col] = _pulses_to_float32(all_channels[ch], frames, sample_rate)
        audio_time_us += step_us

    from .midi import from_mido

    def midi_callback(msg):
        cm = from_mido(msg)
        if cm is not None:
            midi_queue.put(cm)

    if midi_port is None:
        port_name = _VIRTUAL_PORT_NAME
        port_kwargs = {"virtual": True}
    else:
        available = mido.get_input_names()
        if midi_port not in available:
            names = "\n  ".join(available) or "(none)"
            raise RuntimeError(
                f"MIDI port {midi_port!r} not found. Available ports:\n  {names}"
            )
        port_name = midi_port
        port_kwargs = {}

    with mido.open_input(port_name, callback=midi_callback, **port_kwargs) as port:
        print(f"MIDI input : {port.name}")
        out_device = audio_device if audio_device is not None else sd.default.device[1]
        dev_info = sd.query_devices(out_device)
        with sd.OutputStream(
            samplerate=sample_rate,
            blocksize=blocksize,
            channels=n_out,
            dtype="float32",
            device=audio_device,
            callback=audio_callback,
        ):
            ch_str = str(ch_indices[0]) if n_out == 1 else f"{n_out}ch {ch_indices}"
            print(
                f"Audio out  : {dev_info['name']}"
                f"  {sample_rate} Hz  blocksize={blocksize}"
                f"  latency~{blocksize * 1000 / sample_rate:.1f} ms"
                f"  channels={ch_str}"
            )
            print("Press Ctrl+C to stop.")
            try:
                while True:
                    time.sleep(0.1)
            except KeyboardInterrupt:
                pass

"""
Plotly visualizations for pulse recordings and instrument envelopes.

All functions return a ``plotly.graph_objects.Figure`` that can be shown with
``.show()`` in Jupyter or saved with ``.write_html()``.

Time parameters are always in **milliseconds** (user-facing API).
All x-axis data is stored in **microseconds**. Ticks always show exact integer
µs values (e.g. "1500 µs") at any zoom level — no unit conversion ambiguity.
"""
from __future__ import annotations

from typing import Iterable

import numpy as np

from ._teslasynth import Envelope, EnvelopeEngine, InstrumentId, PercussionId
from ._types import (InstrumentInfo, NoteEvent, PercussionInfo,
                     get_all_instruments, get_instrument, get_percussion)
from .render import Recording


def _plotly():
    try:
        import plotly.graph_objects as go
        from plotly.subplots import make_subplots
        return go, make_subplots
    except ImportError as e:
        raise ImportError(
            "plotly is required for visualization. "
            "Install it with: pip install plotly"
        ) from e


_CHANNEL_COLORS = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf",
    "#aec7e8", "#ffbb78", "#98df8a", "#ff9896", "#c5b0d5", "#c49c94",
]


def _channel_color(ch: int) -> str:
    return _CHANNEL_COLORS[ch % len(_CHANNEL_COLORS)]


def _time_axis(fig, row=None, col=None, **extra) -> None:
    """Apply microsecond time format to an x-axis.

    Data must be in microseconds. Tick labels show exact integer µs values
    (e.g. "0 µs", "500 µs", "1500 µs") at any zoom level — no unit ambiguity.
    """
    opts = dict(tickformat=",.0f", ticksuffix=" µs", title_text="Time", **extra)
    if row is not None:
        fig.update_xaxes(**opts, row=row, col=col)
    else:
        fig.update_xaxes(**opts)


# ─────────────────────────────────────────────────────────────────────────────
# Internal helpers
# ─────────────────────────────────────────────────────────────────────────────

def _build_signal_trace(
    recording: Recording,
    start_ms: float,
    end_ms: float,
) -> tuple[list, list]:
    """Step-function trace built directly from the pulse array (no resampling).

    Parameters in milliseconds; returns xs/ys in **microseconds**.
    O(pulses in window) — efficient for any recording length.
    """
    start_us = int(start_ms * 1e3)
    end_us   = int(end_ms   * 1e3)

    times  = recording.start_times_us
    on_us  = recording.pulses[:, 0].astype(np.int64)
    off_us = recording.pulses[:, 1].astype(np.int64)
    end_t  = times + on_us + off_us

    in_win = (end_t > start_us) & (times < end_us)
    idx    = np.where(in_win)[0]

    xs: list = []
    ys: list = []
    prev = start_us

    for i in idx:
        t0  = int(times[i])
        ton = int(times[i] + on_us[i])
        t1  = int(end_t[i])

        if t0 > prev:
            xs += [prev, t0]
            ys += [0.0, 0.0]

        if on_us[i] > 0:
            xs += [t0,  t0,  ton, ton]
            ys += [0.0, 1.0, 1.0, 0.0]

        prev = t1

    if prev < end_us:
        xs += [prev, end_us]
        ys += [0.0, 0.0]

    return xs, ys


def _compute_envelope(
    instrument_id: InstrumentId | PercussionId,
    note_duration_ms: float,
    dt_ms: float = 0.2,
) -> tuple[np.ndarray, np.ndarray]:
    """Step the C++ EnvelopeEngine; return (time_us, amplitude) arrays."""
    eng     = EnvelopeEngine(instrument_id)
    dt_us   = max(1, int(dt_ms * 1000))
    note_us = int(note_duration_ms * 1000)

    t_us: list[float] = []
    amp: list[float] = []
    time_us = 0

    while not eng.is_off:
        level = eng.update(dt_us, time_us < note_us)
        t_us.append(float(time_us))
        amp.append(level)
        time_us += dt_us

    return np.array(t_us), np.array(amp)


# ─────────────────────────────────────────────────────────────────────────────
# Envelope plots
# ─────────────────────────────────────────────────────────────────────────────

def plot_envelope(
    instrument_id: InstrumentId | PercussionId,
    note_duration_ms: float = 400.0,
):
    """Plot the amplitude envelope for an instrument or percussion preset.

    Parameters
    ----------
    instrument_id:
        An :class:`~teslasynth._teslasynth.InstrumentId` or
        :class:`~teslasynth._teslasynth.PercussionId` value.
    note_duration_ms:
        How long to hold the note before release.  Ignored for percussion
        (AD envelopes are self-terminating).
    """
    go, _ = _plotly()
    is_perc = isinstance(instrument_id, PercussionId)
    info = get_percussion(instrument_id) if is_perc else get_instrument(instrument_id)
    hold_ms = 0.0 if is_perc else note_duration_ms
    t, amp = _compute_envelope(instrument_id, hold_ms)

    fig = go.Figure()
    fig.add_trace(go.Scatter(
        x=t, y=amp,
        mode="lines",
        name=info.name,
        line=dict(width=2),
    ))
    _add_envelope_markers(fig, info.envelope, hold_ms)
    _time_axis(fig)
    fig.update_layout(
        title=f"Envelope — {info.name}",
        yaxis_title="Amplitude",
        yaxis=dict(range=[0, 1.1]),
    )
    return fig


def plot_envelope_comparison(
    instrument_ids: Iterable[InstrumentId] | None = None,
    note_duration_ms: float = 400.0,
):
    """Overlay envelopes for multiple instruments.

    Parameters
    ----------
    instrument_ids:
        Instruments to compare.  Defaults to all built-in instruments.
    note_duration_ms:
        Note hold duration before release.
    """
    go, _ = _plotly()
    if instrument_ids is None:
        all_info = get_all_instruments()
    else:
        all_info = [get_instrument(i) for i in instrument_ids]

    fig = go.Figure()
    for info in all_info:
        t, amp = _compute_envelope(info.id, note_duration_ms)
        fig.add_trace(go.Scatter(x=t, y=amp, mode="lines", name=info.name))

    _time_axis(fig)
    fig.update_layout(
        title="Envelope Comparison",
        yaxis_title="Amplitude",
        yaxis=dict(range=[0, 1.1]),
    )
    return fig


def _add_envelope_markers(fig, env: Envelope, note_duration_ms: float) -> None:
    """Add stage-boundary lines and sustain reference to an envelope plot."""
    if env.type == "const":
        return
    boundaries = {
        "Attack end": env.attack_ms * 1000,
        "Decay end":  (env.attack_ms + env.decay_ms) * 1000,
        "Release":    (env.attack_ms + env.decay_ms + note_duration_ms) * 1000,
    }
    for label, x in boundaries.items():
        if x > 0:
            fig.add_vline(x=x, line_dash="dot", line_color="grey",
                          annotation_text=label, annotation_position="top right")
    if env.type == "adsr" and env.sustain > 0:
        fig.add_hline(y=env.sustain, line_dash="dot", line_color="green",
                      annotation_text=f"Sustain ({env.sustain:.2f})",
                      annotation_position="bottom right")


# ─────────────────────────────────────────────────────────────────────────────
# Piano roll
# ─────────────────────────────────────────────────────────────────────────────

def plot_piano_roll(
    notes: list[NoteEvent],
    start_ms: float | None = None,
    end_ms: float | None = None,
):
    """Piano roll: MIDI notes as horizontal bars, colored by channel.

    Parameters
    ----------
    notes:
        List of :class:`~teslasynth._types.NoteEvent` from
        :func:`~teslasynth.midi.notes_from_midi`.
    start_ms / end_ms:
        Optional time window (milliseconds).
    """
    go, _ = _plotly()

    visible = notes
    if start_ms is not None or end_ms is not None:
        s_us = (start_ms or 0.0) * 1000
        e_us = (end_ms or float("inf")) * 1000
        visible = [n for n in notes if n.end_us >= s_us and n.start_us <= e_us]

    fig = go.Figure()
    for ch in sorted(set(n.channel for n in visible)):
        ch_notes = [n for n in visible if n.channel == ch]
        xs: list = []
        ys: list = []
        for n in ch_notes:
            xs += [n.start_us, n.end_us, None]
            ys += [n.note, n.note, None]
        fig.add_trace(go.Scatter(
            x=xs, y=ys, mode="lines",
            line=dict(width=4, color=_channel_color(ch)),
            name=f"Ch {ch}",
        ))

    note_nums = [n.note for n in visible] or [0, 127]
    _time_axis(fig)
    fig.update_layout(
        title="Piano Roll",
        yaxis_title="MIDI Note",
        yaxis=dict(range=[min(note_nums) - 2, max(note_nums) + 2]),
    )
    if start_ms is not None or end_ms is not None:
        fig.update_xaxes(range=[
            (start_ms or 0.0) * 1000,
            (end_ms   or 0.0) * 1000,
        ])
    return fig


# ─────────────────────────────────────────────────────────────────────────────
# Recording plots
# ─────────────────────────────────────────────────────────────────────────────

def plot_signal(
    recording: Recording,
    start_ms: float = 0.0,
    end_ms: float = 10.0,
):
    """Plot the coil output as a digital step function.

    Built directly from the pulse array — no high-resolution signal
    materialization needed.  Zoom in to see individual pulses; tick labels show
    e.g. "1500.100 ms" giving 1 µs precision.

    Parameters
    ----------
    start_ms / end_ms:
        Time window in milliseconds.
    """
    go, _ = _plotly()
    xs, ys  = _build_signal_trace(recording, start_ms, end_ms)

    fig = go.Figure()
    fig.add_trace(go.Scatter(
        x=xs, y=ys,
        mode="lines",
        line=dict(width=1, color="royalblue"),
        name="Coil signal",
    ))
    _time_axis(fig)
    fig.update_layout(
        title=f"Coil output — {start_ms:.3f} ms to {end_ms:.3f} ms",
        yaxis_title="State",
        yaxis=dict(range=[-0.1, 1.2], tickvals=[0, 1], ticktext=["Off", "On"]),
    )
    return fig


plot_timing = plot_signal  # backwards-compat alias


def _freq_from_pulses(
    recording: Recording,
    start_ms: float = 0.0,
    end_ms: float = float("inf"),
    min_freq: float = 50.0,
) -> tuple[np.ndarray, np.ndarray]:
    """Compute frequency from spacing between consecutive active pulses.

    Active pulses are those with on_us > 0. The frequency at the midpoint
    between two consecutive active pulses is 1 / (their start-time difference).
    This correctly reflects the note pitch regardless of on/off durations.

    Returns (times_us, freq_hz) arrays filtered to the given window and min_freq.
    """
    times_us  = recording.start_times_us
    active    = recording.pulses[:, 0] > 0
    at_us     = times_us[active].astype(float)
    if len(at_us) < 2:
        return np.array([]), np.array([])
    periods   = np.diff(at_us)
    freq      = np.where(periods > 0, 1e6 / periods, 0.0)
    t_us      = (at_us[:-1] + at_us[1:]) / 2.0
    mask      = (t_us >= start_ms * 1000) & (t_us <= end_ms * 1000) & (freq >= min_freq)
    return t_us[mask], freq[mask]


def _duty_by_step(
    recording: Recording,
    start_ms: float = 0.0,
    end_ms: float = float("inf"),
) -> tuple[np.ndarray, np.ndarray]:
    """Compute duty cycle per synthesis step.

    Returns (times_us, duty_fraction) where each point represents one
    step_us-wide window. This matches what max_duty_percent enforces in
    the firmware: total on-time / step duration.
    """
    step    = recording.step_us
    times   = recording.start_times_us
    on_us   = recording.pulses[:, 0].astype(float)
    idx     = (times // step).astype(np.intp)
    step_on = np.bincount(idx, weights=on_us)
    duty    = step_on / step
    t_us    = (np.arange(len(step_on)) * step + step // 2).astype(float)
    mask    = (t_us >= start_ms * 1000) & (t_us <= end_ms * 1000)
    return t_us[mask], duty[mask]


def plot_frequency(
    recording: Recording,
    min_freq: float = 50.0,
):
    """Plot instantaneous note frequency over time.

    Frequency is derived from the spacing between consecutive active pulses,
    which correctly reflects the synthesized pitch rather than the per-pulse
    on/off ratio.

    Parameters
    ----------
    min_freq:
        Frequencies below this Hz are treated as silence and excluded.
    """
    go, _ = _plotly()
    t_us, freq = _freq_from_pulses(recording, min_freq=min_freq)

    fig = go.Figure()
    fig.add_trace(go.Scatter(
        x=t_us, y=freq,
        mode="markers",
        marker=dict(size=2, color=freq, colorscale="Viridis"),
        name="Frequency",
    ))
    _time_axis(fig)
    fig.update_layout(title="Instantaneous Frequency", yaxis_title="Frequency (Hz)")
    return fig


def plot_duty_cycle(recording: Recording):
    """Plot duty cycle over time.

    Duty is computed per synthesis step (total on-time / step duration),
    matching what max_duty_percent enforces in the firmware.
    """
    go, _ = _plotly()
    t_us, duty = _duty_by_step(recording)
    active = duty > 0

    fig = go.Figure()
    fig.add_trace(go.Scatter(
        x=t_us[active], y=duty[active] * 100,
        mode="markers",
        marker=dict(size=2),
        name="Duty cycle",
    ))
    _time_axis(fig)
    fig.update_layout(
        title="Duty Cycle",
        yaxis_title="Duty cycle (%)",
        yaxis=dict(range=[0, 105]),
    )
    return fig


def plot_overview(
    recording: Recording,
    notes: list[NoteEvent] | None = None,
    start_ms: float = 0.0,
    end_ms: float | None = None,
):
    """Four-panel overview: piano roll + coil signal + frequency + duty cycle.

    All panels share the x-axis; the time unit (µs / ms / s) adapts as the
    user zooms.

    Parameters
    ----------
    recording:
        The recording to visualize.
    notes:
        Optional list from :func:`~teslasynth.midi.notes_from_midi`.
        When provided, a piano-roll panel is added at the top.
    start_ms / end_ms:
        Time window in milliseconds.  *end_ms* defaults to full length.
    """
    go, make_subplots = _plotly()

    end_ms = end_ms if end_ms is not None else (recording.duration_us / 1e3)

    has_piano = notes is not None

    if has_piano:
        rows, row_heights = 4, [3, 2, 1, 1]
        titles = ("Piano Roll", "Coil Signal", "Frequency (Hz)", "Duty Cycle (%)")
        sig_row, freq_row, duty_row = 2, 3, 4
    else:
        rows, row_heights = 3, [2, 1, 1]
        titles = ("Coil Signal", "Frequency (Hz)", "Duty Cycle (%)")
        sig_row, freq_row, duty_row = 1, 2, 3

    fig = make_subplots(
        rows=rows, cols=1,
        shared_xaxes=True,
        subplot_titles=titles,
        row_heights=row_heights,
        vertical_spacing=0.05,
    )

    # ── piano roll ────────────────────────────────────────────────────────────
    if has_piano:
        start_us_win = start_ms * 1000
        end_us_win   = end_ms   * 1000
        visible = [
            n for n in notes
            if n.end_us >= start_us_win and n.start_us <= end_us_win
        ]
        for ch in sorted(set(n.channel for n in visible)):
            ch_notes = [n for n in visible if n.channel == ch]
            xs: list = []
            ys: list = []
            for n in ch_notes:
                xs += [n.start_us, n.end_us, None]
                ys += [n.note, n.note, None]
            fig.add_trace(go.Scatter(
                x=xs, y=ys, mode="lines",
                line=dict(width=4, color=_channel_color(ch)),
                name=f"Ch {ch}",
            ), row=1, col=1)
        if visible:
            note_nums = [n.note for n in visible]
            fig.update_yaxes(
                range=[min(note_nums) - 2, max(note_nums) + 2],
                title_text="MIDI Note", row=1, col=1,
            )

    # ── coil signal ───────────────────────────────────────────────────────────
    xs, ys = _build_signal_trace(recording, start_ms, end_ms)
    fig.add_trace(go.Scatter(
        x=xs, y=ys, mode="lines",
        line=dict(width=1, color="royalblue"),
        name="Coil signal", showlegend=False,
    ), row=sig_row, col=1)
    fig.update_yaxes(
        range=[-0.1, 1.2], tickvals=[0, 1], ticktext=["Off", "On"],
        title_text="State", row=sig_row, col=1,
    )

    # ── frequency ─────────────────────────────────────────────────────────────
    t_freq_us, freq = _freq_from_pulses(recording, start_ms=start_ms, end_ms=end_ms)
    fig.add_trace(go.Scatter(
        x=t_freq_us, y=freq,
        mode="markers", marker=dict(size=2, color="royalblue"),
        name="Frequency", showlegend=False,
    ), row=freq_row, col=1)
    fig.update_yaxes(title_text="Hz", row=freq_row, col=1)

    # ── duty cycle ────────────────────────────────────────────────────────────
    t_duty_us, duty = _duty_by_step(recording, start_ms=start_ms, end_ms=end_ms)
    active_duty = duty > 0
    fig.add_trace(go.Scatter(
        x=t_duty_us[active_duty], y=duty[active_duty] * 100,
        mode="markers", marker=dict(size=2, color="coral"),
        name="Duty cycle", showlegend=False,
    ), row=duty_row, col=1)
    fig.update_yaxes(title_text="%", row=duty_row, col=1)

    # ── shared time axis ──────────────────────────────────────────────────────
    _time_axis(fig, row=rows, col=1)

    fig.update_layout(
        title="Recording Overview",
        height=900 if has_piano else 600,
    )
    return fig

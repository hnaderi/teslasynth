"""
Named Python types returned by the public teslasynth API.

Importing from here (rather than from __init__) avoids circular imports
for internal modules such as plot.py and midi.py.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from ._teslasynth import Envelope, InstrumentId, PercussionId


@dataclass(frozen=True)
class InstrumentInfo:
    """Static metadata for a built-in instrument."""

    index: int
    id: InstrumentId
    name: str
    envelope: Envelope
    vibrato_rate_hz: float
    vibrato_depth_hz: float


@dataclass(frozen=True)
class PercussionInfo:
    """Static metadata for a built-in percussion preset."""

    index: int
    id: PercussionId
    name: str
    burst_ms: float
    prf_hz: float
    noise: float
    skip: float
    envelope: Envelope


@dataclass(frozen=True)
class BuildInfo:
    """Engine version and compile-time information."""

    version: str
    date: str
    time: str


@dataclass(frozen=True)
class NoteEvent:
    """A single MIDI note extracted from a .mid file."""

    channel: int
    note: int
    velocity: int
    start_us: int
    end_us: int


# ---------------------------------------------------------------------------
# Typed wrappers around the raw C++ functions
# ---------------------------------------------------------------------------


def get_instrument(id: InstrumentId) -> InstrumentInfo:
    """Return metadata for one instrument."""
    from ._teslasynth import get_instrument as _raw

    return InstrumentInfo(**_raw(id))


def get_all_instruments() -> list[InstrumentInfo]:
    """Return metadata for all built-in instruments."""
    from ._teslasynth import get_all_instruments as _raw

    return [InstrumentInfo(**d) for d in _raw()]


def get_percussion(id: PercussionId) -> PercussionInfo:
    """Return metadata for one percussion preset."""
    from ._teslasynth import get_percussion as _raw

    return PercussionInfo(**_raw(id))


def get_all_percussions() -> list[PercussionInfo]:
    """Return metadata for all built-in percussion presets."""
    from ._teslasynth import get_all_percussions as _raw

    return [PercussionInfo(**d) for d in _raw()]


def percussion_for_note(note: int) -> PercussionInfo:
    """Return the percussion preset for a MIDI drum note (0–127)."""
    from ._teslasynth import percussion_for_note as _raw

    return PercussionInfo(**_raw(note))


def build_info() -> BuildInfo:
    """Return engine version and build timestamp."""
    from ._teslasynth import build_info as _raw

    return BuildInfo(**_raw())

# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
teslasynth — Python analysis toolkit for the teslasynth firmware.

Quick start
-----------
>>> import teslasynth as ts
>>>
>>> # Render a MIDI file
>>> rec = ts.render.from_file("song.mid")
>>>
>>> # Visualize
>>> ts.plot.plot_overview(rec).show()
>>> ts.plot.plot_envelope(ts.InstrumentId.SynthPluck).show()
>>>
>>> # Export audio
>>> ts.wav.write("song.mid", "out.flac")
"""

from . import (
    config,  # noqa: F401
    midi,  # noqa: F401
    plot,  # noqa: F401
    render,  # noqa: F401
    wav,  # noqa: F401
)
from ._teslasynth import (  # noqa: F401 — re-export C++ types
    ChannelConfig,
    Configuration,
    Envelope,
    EnvelopeEngine,
    InstrumentId,
    MidiChannelMessage,
    MidiMessageType,
    PercussionId,
    Pulse,
    RoutingConfig,
    SynthConfig,
    Teslasynth,
    version,
)
from ._types import (  # noqa: F401 — typed Python wrappers
    BuildInfo,
    InstrumentInfo,
    NoteEvent,
    PercussionInfo,
    build_info,
    get_all_instruments,
    get_all_percussions,
    get_instrument,
    get_percussion,
    percussion_for_note,
)

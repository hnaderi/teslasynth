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
>>> ts.wav.write(rec, "out.wav")
"""

from ._teslasynth import (  # noqa: F401 — re-export C++ types
    Configuration,
    ChannelConfig,
    Envelope,
    EnvelopeEngine,
    InstrumentId,
    MidiChannelMessage,
    MidiMessageType,
    PercussionId,
    RoutingConfig,
    SynthConfig,
    Teslasynth,
    build_info,
    get_all_instruments,
    get_all_percussions,
    get_instrument,
    get_percussion,
    percussion_for_note,
    version,
)

from . import config  # noqa: F401
from . import midi    # noqa: F401
from . import plot    # noqa: F401
from . import render  # noqa: F401
from . import wav     # noqa: F401

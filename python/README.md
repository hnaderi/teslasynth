# teslasynth Python library

Python bindings and offline analysis tools for the
[Teslasynth](http://projects.hnaderi.dev/teslasynth/) MIDI synthesizer firmware.

The library exposes the same synthesis engine used by the ESP32 firmware so you
can render MIDI files, visualise pulse signals, and tune safety parameters
**before connecting any high-voltage hardware**.

Licensed under **LGPL v3**.

## Installation

```sh
pip install teslasynth
```

Optional extras:

```sh
pip install "teslasynth[plot]"   # Plotly visualisations
pip install "teslasynth[wav]"    # WAV/FLAC export (requires soundfile)
```

## Quick start

```python
from teslasynth import Teslasynth
from teslasynth import wav

synth = Teslasynth()

# Render a MIDI file to a single-channel FLAC (recommended — much smaller than WAV)
wav.write("song.mid", "song.flac", synth=synth)

# Render specific channels into a multichannel file
wav.write("song.mid", "song.flac", synth=synth, channels=[0, 1, 2])

# Render all 8 output channels
wav.write("song.mid", "all.flac", synth=synth, channels=list(range(8)))
```

## CLI

```
teslasynth render   <midi> <out>    [--config FILE] [--sample-rate HZ] [--channel CH [CH ...]]
teslasynth plot     <midi>          [--config FILE] [--out FILE.html] [--channel N]
teslasynth signal   <midi>          [--config FILE] [--out FILE.html] [--channel N]
teslasynth config   [--config FILE] [key=value ...]
teslasynth instruments
teslasynth percussions
teslasynth envelope <instrument>    [--out FILE.html]
teslasynth version
```

The output format for `render` is selected from the file extension (`.wav` or `.flac`).
FLAC is recommended for multichannel exports or long recordings.

```sh
# Single channel FLAC (default channel 0)
teslasynth render song.mid out.flac

# Specific channels into a multichannel file
teslasynth render song.mid out.flac --channel 0,1,2

# Range of channels
teslasynth render song.mid out.flac --channel 0-3

# All 8 channels
teslasynth render song.mid out.flac --channel all
```

## Documentation

Full documentation at **http://projects.hnaderi.dev/teslasynth/python**

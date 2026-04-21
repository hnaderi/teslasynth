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
pip install "teslasynth[wav]"    # WAV export (requires SciPy)
```

## Quick start

```python
from teslasynth import Teslasynth
from teslasynth import render, wav

synth = Teslasynth()

# Render a MIDI file to WAV
wav.write("song.mid", "song.wav", synth=synth)

# Or get the raw pulse stream
for time_us, on_us, off_us in render.pulse_stream("song.mid", synth=synth):
    print(f"{time_us:10d} µs  on={on_us} µs  off={off_us} µs")
```

## CLI

```
teslasynth render   <midi> <wav>   [--config FILE] [--sample-rate HZ]
teslasynth plot     <midi>         [--config FILE] [--out FILE.html]
teslasynth signal   <midi>         [--config FILE] [--out FILE.html]
teslasynth config   [--config FILE] [key=value ...]
teslasynth instruments
teslasynth percussions
teslasynth envelope <instrument>   [--out FILE.html]
teslasynth version
```

## Documentation

Full documentation at **http://projects.hnaderi.dev/teslasynth/python**

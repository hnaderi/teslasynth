<p align="center">
  <img src="web/assets/logo.jpg" height="100px" alt="Teslasynth logo" />
</p>

<p align="center">
  <strong>Teslasynth</strong> — MIDI synthesizer firmware for ESP32
  <br/>
  <img alt="CI" src="https://img.shields.io/github/actions/workflow/status/hnaderi/teslasynth/ci.yml?style=flat-square">
</p>

Teslasynth is open-source MIDI synthesizer firmware for ESP32 family that turns
interruptible high-voltage devices — Tesla coils, flyback transformers,
high-power lasers — into musical instruments.

**Full documentation:** http://projects.hnaderi.dev/teslasynth/

## ⚠ Safety

**This firmware controls high-voltage and high-power devices. These are
dangerous.**

- Tesla coils and flyback transformers produce lethal voltages.
- High-power lasers can cause permanent eye damage and fire.
- Never work on live circuits. Always discharge capacitors first.
- The firmware has no awareness of what it is connected to. Safe operation is
  the responsibility of the builder and operator.

## Quick start

1. Open the [web installer](http://projects.hnaderi.dev/teslasynth/) in Chrome or
   Edge and flash your ESP32 board.
2. Hold **BOOT** for 3 seconds to enter maintenance mode.
3. Connect to the **Teslasynth** Wi-Fi network and open
   **http://teslasynth.local** to configure the device.
4. Connect a MIDI source and play.

## Python library

```sh
pip install teslasynth
```

Render MIDI files offline, visualise pulse signals, and tune safety parameters
before connecting any hardware. See the
[Python docs](http://projects.hnaderi.dev/teslasynth/python) for details.

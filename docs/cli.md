# Firmware CLI Reference

The Teslasynth firmware exposes a serial command-line interface (CLI) for configuration, diagnostics, and control. It runs over UART or USB serial and provides a persistent REPL session.

---

## System Commands

### `version`
Print firmware version, build timestamp, IDF version, and chip info (model, cores, features, flash size, revision).

### `restart`
Software reset the device.

### `maintenance`
Reboot into maintenance mode to allow web-based configuration.

### `free`
Print current available heap memory in bytes.

### `heap`
Print the minimum free heap size observed since boot.

---

## Playback Control

### `off`
Immediately stop all note playback on all channels.

---

## Instrument Listing

### `instruments`
List all predefined instruments with their ID numbers.

---

## Hardware Info

### `limits`
Show compile-time hard limits: maximum concurrent notes per channel.

### `hwconfig`
Show current hardware configuration: number of outputs, GPIO pins per output, maintenance button GPIO, status LED GPIO and polarity.

---

## Configuration

### `config [<key[:<ch>]=value> ...] [flags]`

Read or write configuration. Called without arguments it prints the full current config.

**Flags:**

| Flag | Description |
|------|-------------|
| `-s`, `--save` | Persist changes to NVS flash |
| `-r`, `--reload` | Reload synthesizer config after update |
| `--reset` | Reset all config to firmware defaults |

Multiple `key=value` pairs can be passed in a single call.

**Examples:**
```
config
config synth.tuning=432hz
config output.1.max-duty=15.5 output.2.notes=2 --save
config --reset
config -r -s output.*.max-on-time=200us
```

---

### Configuration Keys

#### Global Synth

| Key | Format | Default | Description |
|-----|--------|---------|-------------|
| `synth.tuning` | `<n>[hz\|khz]` | `440hz` | Reference tuning frequency |
| `synth.instrument` | `<1–28>` or `-` | — | Default instrument (all channels) |

**Frequency format:** `440`, `440hz`, `1khz`

---

#### Per-Output / Per-Channel

Replace `<ch>` with an output number (e.g. `1`–`4`) or `*` to apply to all outputs.

| Key | Format | Default | Description |
|-----|--------|---------|-------------|
| `output.<ch>.max-on-time` | `<n>[us\|ms]` | `100us` | Maximum continuous on-time per note pulse |
| `output.<ch>.min-deadtime` | `<n>[us\|ms]` | `100us` | Minimum dead time between pulses |
| `output.<ch>.max-duty` | `<float>` (0–100) | `10` | Maximum duty cycle (%) — resolution 0.5% |
| `output.<ch>.duty-window` | `<n>[us\|ms]` | `10ms` | Time window for duty cycle enforcement |
| `output.<ch>.notes` | `<1–8>` | `4` | Max concurrent notes on this output |
| `output.<ch>.instrument` | `<1–28>` or `-` | — | Instrument override for this output |

**Duration format:** `100`, `100us`, `10ms` — max value 65535 µs

---

#### MIDI Routing

| Key | Format | Description |
|-----|--------|-------------|
| `routing.percussion` | `y` / `n` | Enable/disable percussion channel (MIDI ch 10) routing |
| `routing.channel.<midi_ch>` | `<1–4>` or `-` | Map MIDI channel (1–16) to an output, or unmap with `-` |

Use `*` as the MIDI channel number to apply to all channels at once.

**Examples:**
```
config routing.percussion=y
config routing.channel.1=1
config routing.channel.*=2
config routing.channel.10=-
```

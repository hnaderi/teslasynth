# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
JSON serialization and deserialization for :class:`~teslasynth.Configuration`.

Schema
------
::

    {
      "synth": {
        "tuning_hz": 440.0,
        "instrument": null          // or 0–27
      },
      "channels": [
        {
          "max_on_time_us":  100,
          "min_deadtime_us": 100,
          "duty_window_us":  10000,
          "pulse_resolution_us": 0,
          "notes":           4,
          "max_duty_percent": 100.0,
          "instrument":      null   // or 0-27
        },
        ...                         // up to 8 entries (one per output channel)
      ],
      "routing": {
        "mapping":    [0, 1, null, null, null, null, null, null,
                       null, null, null, null, null, null, null, null],
        "percussion": false
      }
    }

``routing.mapping`` is a 16-element list, one entry per MIDI channel (0–15).
Each entry is an output index (0–7) or ``null`` (ignore that channel).
``routing.percussion`` enables percussion handling on channel 9.

Omitted ``channels`` entries keep firmware defaults.  A single ``"channel"``
key (legacy) is accepted and applied to output channel 0.
"""

from __future__ import annotations

import json
from pathlib import Path

from ._teslasynth import Configuration


def to_dict(cfg: Configuration) -> dict:
    """Serialize a :class:`Configuration` to a plain Python dict."""
    r = cfg.routing
    return {
        "synth": {
            "tuning_hz": cfg.synth.tuning_hz,
            "instrument": cfg.synth.instrument,
        },
        "channels": [
            {
                "max_on_time_us": cfg.channel(i).max_on_time_us,
                "min_deadtime_us": cfg.channel(i).min_deadtime_us,
                "duty_window_us": cfg.channel(i).duty_window_us,
                "pulse_resolution_us": cfg.channel(i).pulse_resolution_us,
                "notes": cfg.channel(i).notes,
                "max_duty_percent": cfg.channel(i).max_duty_percent,
                "instrument": cfg.channel(i).instrument,
            }
            for i in range(cfg.channels_size)
        ],
        "routing": {
            "mapping": r.mapping,
            "percussion": r.percussion,
        },
    }


def from_dict(d: dict) -> Configuration:
    """Deserialize a :class:`Configuration` from a plain Python dict.

    Unknown keys are ignored; missing keys keep the firmware default.
    """
    cfg = Configuration()
    _load_synth(cfg, d.get("synth", {}))
    # Support both "channels" list and legacy "channel" single-entry key.
    if "channels" in d:
        for i, ch_dict in enumerate(d["channels"]):
            if i >= cfg.channels_size:
                break
            _load_channel(cfg.channel(i), ch_dict)
    elif "channel" in d:
        _load_channel(cfg.channel(0), d["channel"])
    _load_routing(cfg.routing, d.get("routing", {}))
    return cfg


def _load_synth(cfg: Configuration, s: dict) -> None:
    if "tuning_hz" in s:
        v = float(s["tuning_hz"])
        if v <= 0:
            raise ValueError(f"synth.tuning_hz must be positive, got {v}")
        cfg.synth.tuning_hz = v
    if "instrument" in s:
        cfg.synth.instrument = _opt_instrument(s["instrument"], "synth.instrument")


def _load_channel(ch, c: dict) -> None:
    for key, attr, lo, hi in [
        ("max_on_time_us", "max_on_time_us", 1, 65535),
        ("min_deadtime_us", "min_deadtime_us", 0, 65535),
        ("duty_window_us", "duty_window_us", 1, 65535),
        ("pulse_resolution_us", "pulse_resolution_us", 0, 65535),
        ("notes", "notes", 1, 255),
    ]:
        if key in c:
            v = int(c[key])
            if not (lo <= v <= hi):
                raise ValueError(f"channel.{key} must be {lo}-{hi}, got {v}")
            setattr(ch, attr, v)
    if "max_duty_percent" in c:
        v = float(c["max_duty_percent"])
        if not (0.0 <= v <= 100.0):
            raise ValueError(f"channel.max_duty_percent must be 0–100, got {v}")
        ch.max_duty_percent = v
    if "instrument" in c:
        ch.instrument = _opt_instrument(c["instrument"], "channel.instrument")


def _load_routing(r, d: dict) -> None:
    if "mapping" in d:
        lst = d["mapping"]
        if len(lst) != 16:
            raise ValueError(f"routing.mapping must have 16 entries, got {len(lst)}")
        validated = []
        for i, v in enumerate(lst):
            if v is None:
                validated.append(None)
            else:
                v = int(v)
                if not (0 <= v <= 7):
                    raise ValueError(
                        f"routing.mapping[{i}]: output index must be 0–7, got {v}"
                    )
                validated.append(v)
        r.mapping = validated
    if "percussion" in d:
        r.percussion = bool(d["percussion"])


def _opt_instrument(v, field: str):
    if v is None:
        return None
    v = int(v)
    if not (0 <= v <= 27):
        raise ValueError(f"{field} must be 0–27 or null, got {v}")
    return v


# ─────────────────────────────────────────────────────────────────────────────
# File I/O
# ─────────────────────────────────────────────────────────────────────────────


def load(path: str | Path) -> Configuration:
    """Load a :class:`Configuration` from a JSON file."""
    with open(path) as f:
        return from_dict(json.load(f))


def save(cfg: Configuration, path: str | Path) -> None:
    """Write a :class:`Configuration` to a JSON file."""
    with open(path, "w") as f:
        json.dump(to_dict(cfg), f, indent=2)
        f.write("\n")


def dumps(cfg: Configuration) -> str:
    """Return a :class:`Configuration` as a formatted JSON string."""
    return json.dumps(to_dict(cfg), indent=2)

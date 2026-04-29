# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
JSON serialization and deserialization for :class:`~teslasynth.Configuration`.

The schema matches the firmware HTTP API (``GET/PUT /api/config/synth``) byte
for byte, so a JSON file produced here loads on a real device, and a config
exported from the dashboard loads here.

Schema
------
::

    {
      "tuning":     440.0,
      "instrument": null,            // global instrument override, or 0-27
      "channels": [
        {
          "notes":            4,
          "max-on-time":      100,   // microseconds
          "min-deadtime":     100,   // microseconds
          "max-duty":         100.0, // percent
          "duty-window":      10000, // microseconds
          "pulse-resolution": 0,     // microseconds (0 disables)
          "instrument":       null   // or 0-27
        },
        ...                          // up to 8 entries (one per output)
      ],
      "routing": {
        "percussion": false,
        "mapping":    [0, 1, -1, -1, -1, -1, -1, -1,
                       -1, -1, -1, -1, -1, -1, -1, -1]
      }
    }

``routing.mapping`` is a 16-element list, one entry per MIDI channel (0-15).
Each entry is an output index (0-7) or ``-1`` to ignore that channel.
``routing.percussion`` enables percussion handling on channel 9.

Omitted ``channels`` entries keep firmware defaults.
"""

from __future__ import annotations

import json
from pathlib import Path

from ._teslasynth import Configuration


def to_dict(cfg: Configuration) -> dict:
    """Serialize a :class:`Configuration` to a plain Python dict."""
    r = cfg.routing
    return {
        "tuning": cfg.synth.tuning_hz,
        "instrument": cfg.synth.instrument,
        "channels": [
            {
                "notes": cfg.channel(i).notes,
                "max-on-time": cfg.channel(i).max_on_time_us,
                "min-deadtime": cfg.channel(i).min_deadtime_us,
                "max-duty": cfg.channel(i).max_duty_percent,
                "duty-window": cfg.channel(i).duty_window_us,
                "pulse-resolution": cfg.channel(i).pulse_resolution_us,
                "instrument": cfg.channel(i).instrument,
            }
            for i in range(cfg.channels_size)
        ],
        "routing": {
            "percussion": r.percussion,
            "mapping": [-1 if v is None else v for v in r.mapping],
        },
    }


def from_dict(d: dict) -> Configuration:
    """Deserialize a :class:`Configuration` from a plain Python dict.

    Unknown keys are ignored; missing keys keep firmware defaults.
    """
    cfg = Configuration()
    _load_synth(cfg, d)
    if "channels" in d:
        for i, ch_dict in enumerate(d["channels"]):
            if i >= cfg.channels_size:
                break
            _load_channel(cfg.channel(i), ch_dict)
    _load_routing(cfg.routing, d.get("routing", {}))
    return cfg


def _load_synth(cfg: Configuration, d: dict) -> None:
    if "tuning" in d:
        v = float(d["tuning"])
        if v <= 0:
            raise ValueError(f"tuning must be positive, got {v}")
        cfg.synth.tuning_hz = v
    if "instrument" in d:
        cfg.synth.instrument = _opt_instrument(d["instrument"], "instrument")


def _load_channel(ch, c: dict) -> None:
    for key, attr, lo, hi in [
        ("max-on-time", "max_on_time_us", 1, 65535),
        ("min-deadtime", "min_deadtime_us", 0, 65535),
        ("duty-window", "duty_window_us", 1, 65535),
        ("pulse-resolution", "pulse_resolution_us", 0, 65535),
        ("notes", "notes", 1, 255),
    ]:
        if key in c:
            v = int(c[key])
            if not (lo <= v <= hi):
                raise ValueError(f"channels.{key} must be {lo}-{hi}, got {v}")
            setattr(ch, attr, v)
    if "max-duty" in c:
        v = float(c["max-duty"])
        if not (0.0 <= v <= 100.0):
            raise ValueError(f"channels.max-duty must be 0-100, got {v}")
        ch.max_duty_percent = v
    if "instrument" in c:
        ch.instrument = _opt_instrument(c["instrument"], "channels.instrument")


def _load_routing(r, d: dict) -> None:
    if "mapping" in d:
        lst = d["mapping"]
        if len(lst) != 16:
            raise ValueError(f"routing.mapping must have 16 entries, got {len(lst)}")
        validated = []
        for i, v in enumerate(lst):
            if v is None or v == -1:
                validated.append(None)
            else:
                v = int(v)
                if not (0 <= v <= 7):
                    raise ValueError(
                        f"routing.mapping[{i}]: output index must be 0-7 or -1, got {v}"
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
        raise ValueError(f"{field} must be 0-27 or null, got {v}")
    return v


# -----------------------------------------------------------------------------
# File I/O
# -----------------------------------------------------------------------------


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

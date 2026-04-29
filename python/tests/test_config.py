# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""
Tests for teslasynth.config — JSON serialization and validation.

Schema matches the firmware HTTP API (kebab-case keys, flat top-level synth
fields, ``-1`` for unmapped routing entries).
"""

import json

import pytest

from .conftest import requires_extension


@requires_extension
class TestRoundTrip:
    def test_default_config_round_trips(self):
        from teslasynth import Configuration
        from teslasynth.config import from_dict, to_dict

        cfg = Configuration()
        d = to_dict(cfg)
        cfg2 = from_dict(d)
        assert to_dict(cfg2) == d

    def test_dict_has_expected_keys(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict

        d = to_dict(Configuration())
        assert set(d.keys()) == {"tuning", "instrument", "channels", "routing"}
        assert isinstance(d["tuning"], float)
        assert isinstance(d["channels"], list)
        assert len(d["channels"]) == 8
        assert "max-on-time" in d["channels"][0]
        assert "pulse-resolution" in d["channels"][0]
        assert "mapping" in d["routing"]
        assert "percussion" in d["routing"]

    def test_routing_mapping_length(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict

        d = to_dict(Configuration())
        assert len(d["routing"]["mapping"]) == 16

    def test_routing_mapping_unmapped_is_minus_one(self):
        from teslasynth.config import from_dict, to_dict

        # Explicitly set channels 0 and 1, leave the rest unmapped.
        mapping_in = [-1] * 16
        mapping_in[0] = 0
        mapping_in[1] = 1
        cfg = from_dict({"routing": {"mapping": mapping_in}})
        out = to_dict(cfg)["routing"]["mapping"]
        assert out[0] == 0
        assert out[1] == 1
        # Channels 2..15 must serialize as -1 (firmware schema for unmapped).
        assert all(v == -1 for v in out[2:])

    def test_dumps_is_valid_json(self):
        from teslasynth import Configuration
        from teslasynth.config import dumps

        s = dumps(Configuration())
        parsed = json.loads(s)
        assert "tuning" in parsed
        assert "channels" in parsed

    def test_save_and_load(self, tmp_path):
        from teslasynth import Configuration
        from teslasynth.config import load, save, to_dict

        cfg = Configuration()
        path = tmp_path / "config.json"
        save(cfg, path)
        cfg2 = load(path)
        assert to_dict(cfg2) == to_dict(cfg)

    def test_unknown_keys_are_ignored(self):
        from teslasynth.config import from_dict, to_dict

        d = {
            "tuning": 440.0,
            "unknown_field": 99,
            "channels": [],
            "routing": {},
        }
        cfg = from_dict(d)
        assert to_dict(cfg)["tuning"] == pytest.approx(440.0)

    def test_missing_keys_use_defaults(self):
        from teslasynth import Configuration
        from teslasynth.config import from_dict, to_dict

        defaults = to_dict(Configuration())
        cfg = from_dict({})
        assert to_dict(cfg) == defaults

    def test_channels_list_applied_per_output(self):
        from teslasynth.config import from_dict

        cfg = from_dict(
            {
                "channels": [
                    {"max-on-time": 111},
                    {"max-on-time": 222},
                ]
            }
        )
        assert cfg.channel(0).max_on_time_us == 111
        assert cfg.channel(1).max_on_time_us == 222
        # Remaining outputs keep defaults
        assert cfg.channel(2).max_on_time_us == 100  # firmware default

    def test_pulse_resolution_round_trip(self):
        from teslasynth.config import from_dict, to_dict

        cfg = from_dict({"channels": [{"pulse-resolution": 5}]})
        assert cfg.channel(0).pulse_resolution_us == 5
        d = to_dict(cfg)
        assert d["channels"][0]["pulse-resolution"] == 5

    def test_pulse_resolution_default_is_zero(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict

        d = to_dict(Configuration())
        assert d["channels"][0]["pulse-resolution"] == 0

    def test_pulse_resolution_missing_keeps_default(self):
        from teslasynth.config import from_dict

        cfg = from_dict({"channels": [{}]})
        assert cfg.channel(0).pulse_resolution_us == 0

    def test_routing_accepts_minus_one_for_unmapped(self):
        from teslasynth.config import from_dict, to_dict

        mapping = [-1] * 16
        mapping[0] = 0
        mapping[1] = 1
        cfg = from_dict({"routing": {"mapping": mapping}})
        out = to_dict(cfg)["routing"]["mapping"]
        assert out[0] == 0
        assert out[1] == 1
        assert out[2] == -1


@requires_extension
class TestValidation:
    def test_invalid_tuning(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="tuning"):
            from_dict({"tuning": -1.0})

    def test_zero_tuning(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="tuning"):
            from_dict({"tuning": 0.0})

    def test_invalid_max_on_time(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="max-on-time"):
            from_dict({"channels": [{"max-on-time": 0}]})

    def test_invalid_pulse_resolution(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="pulse-resolution"):
            from_dict({"channels": [{"pulse-resolution": 65536}]})

    def test_invalid_max_duty(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="max-duty"):
            from_dict({"channels": [{"max-duty": 101.0}]})

    def test_invalid_routing_mapping_length(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="16"):
            from_dict({"routing": {"mapping": [0] * 15}})

    def test_invalid_routing_output_index(self):
        from teslasynth.config import from_dict

        mapping = [-1] * 16
        mapping[3] = 8  # 8 is out of range for 8 outputs (valid: 0-7 or -1)
        with pytest.raises(ValueError):
            from_dict({"routing": {"mapping": mapping}})

    def test_valid_routing_output_indices(self):
        from teslasynth.config import from_dict, to_dict

        mapping = list(range(8)) + [-1] * 8
        cfg = from_dict({"routing": {"mapping": mapping}})
        assert to_dict(cfg)["routing"]["mapping"][:8] == list(range(8))

    def test_instrument_out_of_range(self):
        from teslasynth.config import from_dict

        with pytest.raises(ValueError, match="instrument"):
            from_dict({"instrument": 28})

    def test_instrument_none_is_valid(self):
        from teslasynth.config import from_dict

        cfg = from_dict({"instrument": None})
        assert cfg.synth.instrument is None

"""
Tests for teslasynth.config — JSON serialization and validation.
"""
import json
import pytest
from .conftest import requires_extension


@requires_extension
class TestRoundTrip:
    def test_default_config_round_trips(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict, from_dict
        cfg = Configuration()
        d = to_dict(cfg)
        cfg2 = from_dict(d)
        assert to_dict(cfg2) == d

    def test_dict_has_expected_keys(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict
        d = to_dict(Configuration())
        assert set(d.keys()) == {"synth", "channels", "routing"}
        assert "tuning_hz" in d["synth"]
        assert isinstance(d["channels"], list)
        assert len(d["channels"]) == 8
        assert "max_on_time_us" in d["channels"][0]
        assert "mapping" in d["routing"]

    def test_routing_mapping_length(self):
        from teslasynth import Configuration
        from teslasynth.config import to_dict
        d = to_dict(Configuration())
        assert len(d["routing"]["mapping"]) == 16

    def test_dumps_is_valid_json(self):
        from teslasynth import Configuration
        from teslasynth.config import dumps
        s = dumps(Configuration())
        parsed = json.loads(s)
        assert "synth" in parsed

    def test_save_and_load(self, tmp_path):
        from teslasynth import Configuration
        from teslasynth.config import save, load, to_dict
        cfg = Configuration()
        path = tmp_path / "config.json"
        save(cfg, path)
        cfg2 = load(path)
        assert to_dict(cfg2) == to_dict(cfg)

    def test_unknown_keys_are_ignored(self):
        from teslasynth.config import from_dict, to_dict
        d = {"synth": {"tuning_hz": 440.0, "unknown_field": 99}, "channels": [], "routing": {}}
        cfg = from_dict(d)
        assert to_dict(cfg)["synth"]["tuning_hz"] == pytest.approx(440.0)

    def test_missing_keys_use_defaults(self):
        from teslasynth import Configuration
        from teslasynth.config import from_dict, to_dict
        defaults = to_dict(Configuration())
        cfg = from_dict({})
        assert to_dict(cfg) == defaults

    def test_legacy_channel_key_is_accepted(self):
        """Single 'channel' key (legacy) is applied to output 0."""
        from teslasynth.config import from_dict
        cfg = from_dict({"channel": {"max_on_time_us": 200}})
        assert cfg.channel(0).max_on_time_us == 200

    def test_channels_list_applied_per_output(self):
        from teslasynth.config import from_dict
        cfg = from_dict({"channels": [
            {"max_on_time_us": 111},
            {"max_on_time_us": 222},
        ]})
        assert cfg.channel(0).max_on_time_us == 111
        assert cfg.channel(1).max_on_time_us == 222
        # Remaining outputs keep defaults
        assert cfg.channel(2).max_on_time_us == 100  # firmware default


@requires_extension
class TestValidation:
    def test_invalid_tuning_hz(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="tuning_hz"):
            from_dict({"synth": {"tuning_hz": -1.0}})

    def test_zero_tuning_hz(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="tuning_hz"):
            from_dict({"synth": {"tuning_hz": 0.0}})

    def test_invalid_max_on_time(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="max_on_time_us"):
            from_dict({"channels": [{"max_on_time_us": 0}]})

    def test_invalid_duty_percent(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="max_duty_percent"):
            from_dict({"channels": [{"max_duty_percent": 101.0}]})

    def test_invalid_routing_mapping_length(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="16"):
            from_dict({"routing": {"mapping": [0] * 15}})

    def test_invalid_routing_output_index(self):
        from teslasynth.config import from_dict
        mapping = [0] * 16
        mapping[3] = 8   # 8 is out of range for 8 outputs (valid: 0–7)
        with pytest.raises(ValueError):
            from_dict({"routing": {"mapping": mapping}})

    def test_valid_routing_output_indices(self):
        from teslasynth.config import from_dict, to_dict
        mapping = list(range(8)) + [None] * 8  # outputs 0–7 then unrouted
        cfg = from_dict({"routing": {"mapping": mapping}})
        assert to_dict(cfg)["routing"]["mapping"][:8] == list(range(8))

    def test_instrument_out_of_range(self):
        from teslasynth.config import from_dict
        with pytest.raises(ValueError, match="instrument"):
            from_dict({"synth": {"instrument": 28}})

    def test_instrument_none_is_valid(self):
        from teslasynth.config import from_dict
        cfg = from_dict({"synth": {"instrument": None}})
        assert cfg.synth.instrument is None

# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

"""Tests for CLI helpers in teslasynth.__main__."""

import pytest

from teslasynth.__main__ import _parse_channels


class TestParseChannels:
    def test_none_defaults_to_zero(self):
        assert _parse_channels(None) == 0

    def test_single_channel(self):
        assert _parse_channels("0") == 0
        assert _parse_channels("3") == 3
        assert _parse_channels("7") == 7

    def test_comma_list(self):
        assert _parse_channels("0,1,3,5") == [0, 1, 3, 5]

    def test_range(self):
        assert _parse_channels("0-4") == [0, 1, 2, 3, 4]

    def test_range_single_element(self):
        assert _parse_channels("2-2") == 2

    def test_mixed_range_and_list(self):
        assert _parse_channels("0,2,4-7") == [0, 2, 4, 5, 6, 7]

    def test_all_keyword(self):
        assert _parse_channels("all") == list(range(8))

    def test_star_keyword(self):
        assert _parse_channels("*") == list(range(8))

    def test_spaces_around_commas(self):
        assert _parse_channels("0, 1, 2") == [0, 1, 2]

    def test_invalid_raises(self):
        with pytest.raises((ValueError, AttributeError)):
            _parse_channels("foo")

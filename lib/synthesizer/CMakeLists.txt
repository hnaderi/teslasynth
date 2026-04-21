# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: GPL-3.0-only

idf_component_register(
  SRCS
    "curve.cpp"
    "lfo.cpp"
    "voice_event.cpp"
    "voices/hit.cpp"
    "voices/note.cpp"
  INCLUDE_DIRS "."
)

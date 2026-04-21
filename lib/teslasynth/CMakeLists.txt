# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: GPL-3.0-only

idf_component_register(
  SRCS
    "config_parser.cpp"
    "config_patch_update.cpp"
  INCLUDE_DIRS "."
  REQUIRES midi synthesizer
)

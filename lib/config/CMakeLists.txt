# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: GPL-3.0-only

idf_component_register(
  SRCS
    "codec.cpp"
    "factory_settings.cpp"
    "json.cpp"
  INCLUDE_DIRS "."
  REQUIRES
    teslasynth
    synthesizer
    esp_driver_gpio
)

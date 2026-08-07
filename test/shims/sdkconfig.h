// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

// Host stand-in for the Kconfig-generated header, so the portable parts of
// lib/app can be built and tested natively. Values mirror main/Kconfig.projbuild.

#pragma once

#define CONFIG_TESLASYNTH_OUTPUT_COUNT 4
#define CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN1 12
#define CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN2 13
#define CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN3 14
#define CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN4 27
#define CONFIG_TESLASYNTH_OUTPUT_GPIO_LED -1

#define CONFIG_TESLASYNTH_DEVICE_NAME "Teslasynth"
#define CONFIG_TESLASYNTH_WIFI_PASSWORD "Wardenclyffe1891!"
#define CONFIG_TESLASYNTH_WIFI_CHANNEL 1

#define CONFIG_TESLASYNTH_MAX_NOTES 4
#define CONFIG_TESLASYNTH_DEFAULT_MAX_DUTY 5

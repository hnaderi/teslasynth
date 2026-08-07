// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

// Host stand-in for the ESP-IDF header. Only the enumerators the configuration
// layer uses are provided; GPIO_NUM_MAX matches the ESP32.

#pragma once

typedef enum {
  GPIO_NUM_NC = -1,
  GPIO_NUM_0 = 0,
  GPIO_NUM_MAX = 40,
} gpio_num_t;

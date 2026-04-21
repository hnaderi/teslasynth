/*
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include "esp_err.h"
#include <cstdint>
#include <stdint.h>

struct ChipInfo {
  const char *model;
  uint8_t cores;
  bool wifi, ble, bt, emb_flash, otg;
  uint32_t flash_size;
  uint16_t revision;
};

esp_err_t get_chip_info(ChipInfo &result);

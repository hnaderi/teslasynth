#include "sysinfo.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include <cstddef>
#include <stdint.h>

esp_err_t get_chip_info(ChipInfo &chip) {
  esp_chip_info_t info;
  uint32_t flash_size;
  esp_chip_info(&info);
  esp_err_t err = esp_flash_get_size(NULL, &flash_size);
  if (err != ESP_OK) {
    return err;
  }

  switch (info.model) {
  case CHIP_ESP32:
    chip.model = "ESP32";
    break;
  case CHIP_ESP32S2:
    chip.model = "ESP32-S2";
    break;
  case CHIP_ESP32S3:
    chip.model = "ESP32-S3";
    break;
  case CHIP_ESP32C3:
    chip.model = "ESP32-C3";
    break;
  case CHIP_ESP32H2:
    chip.model = "ESP32-H2";
    break;
  case CHIP_ESP32C2:
    chip.model = "ESP32-C2";
    break;
  case CHIP_ESP32P4:
    chip.model = "ESP32-P4";
    break;
  case CHIP_ESP32C5:
    chip.model = "ESP32-C5";
    break;
  default:
    chip.model = "Unknown";
    break;
  }

  chip.cores = info.cores;
  chip.wifi = static_cast<bool>(info.features & CHIP_FEATURE_WIFI_BGN);
  chip.ble = static_cast<bool>(info.features & CHIP_FEATURE_BLE);
  chip.bt = static_cast<bool>(info.features & CHIP_FEATURE_BT);
  chip.emb_flash = static_cast<bool>(info.features & CHIP_FEATURE_EMB_FLASH);
  chip.flash_size = flash_size / (1024 * 1024);
  chip.revision = info.revision;

  return ESP_OK;
}

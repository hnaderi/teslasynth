#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

const char *model_info(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32:
    return "ESP32";
  case CHIP_ESP32S2:
    return "ESP32S2";
  case CHIP_ESP32S3:
    return "ESP32S3";
  case CHIP_ESP32C3:
    return "ESP32C3";
  case CHIP_ESP32H2:
    return "ESP32H2";
  case CHIP_ESP32C2:
    return "ESP32C2";
  default:
    return "Unknown";
  }
}

void print_chip_info() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("Chip model %s with %d CPU core(s), WiFi%s%s, ",
         model_info(chip_info.model), chip_info.cores,
         (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d\n", major_rev, minor_rev);
}

void app_main() {
  for (;;) {
    print_chip_info();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  };
}

#pragma once

#include "esp_event.h"
#include "freertos/FreeRTOS.h"

StreamBufferHandle_t ble_begin();

ESP_EVENT_DECLARE_BASE(EVENT_BLE_BASE);
enum {
  BLE_DEVICE_CONNECTED,
  BLE_DEVICE_DISCONNECTED,
};

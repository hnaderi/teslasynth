#include "ble-midi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"

static const char *TAG = "BLE_MIDI";

// BLE MIDI UUIDs (128-bit, little endian)
static const uint8_t MIDI_SERVICE_UUID[16] = {
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7,
    0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03};
static const uint8_t MIDI_CHAR_UUID[16] = {0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2,
                                           0xA9, 0xA1, 0x12, 0x41, 0x68, 0x38,
                                           0xDB, 0xE5, 0x72, 0x77};

// Handles
static uint16_t midi_service_handle;
static uint16_t midi_char_handle;
static esp_gatt_if_t midi_gatts_if;
static uint16_t midi_conn_id = 0;

// ==== Callback for received MIDI data ====
__attribute__((weak)) void midi_rx_callback(const uint8_t *data, uint16_t len) {
  ESP_LOGI(TAG, "MIDI RX (%d bytes):", len);
  for (int i = 0; i < len; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
}

// ==== GAP callback ====
static void gap_cb(esp_gap_ble_cb_event_t event,
                   esp_ble_gap_cb_param_t *param) {
  if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT) {
    ESP_LOGI(TAG, "Advertising started");
  }
}

// ==== GATTS callback ====
static void gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                     esp_ble_gatts_cb_param_t *param) {
  switch (event) {
  case ESP_GATTS_REG_EVT: {
    ESP_LOGI(TAG, "Registered app, creating service");
    esp_gatt_srvc_id_t service_id = {
        .is_primary = true,
        .id.inst_id = 0,
        .id.uuid.len = ESP_UUID_LEN_128,
    };
    memcpy(service_id.id.uuid.uuid.uuid128, MIDI_SERVICE_UUID, 16);
    esp_ble_gatts_create_service(gatts_if, &service_id, 4);
    midi_gatts_if = gatts_if;

    // Advertising
    esp_ble_gap_set_device_name("ESP32-BLE-MIDI");
    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    esp_ble_gap_start_advertising(&adv_params);
    break;
  }
  case ESP_GATTS_CREATE_EVT: {
    ESP_LOGI(TAG, "Service created");
    midi_service_handle = param->create.service_handle;
    esp_ble_gatts_start_service(midi_service_handle);

    esp_bt_uuid_t char_uuid = {.len = ESP_UUID_LEN_128};
    memcpy(char_uuid.uuid.uuid128, MIDI_CHAR_UUID, 16);

    esp_gatt_char_prop_t props =
        ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

    esp_ble_gatts_add_char(midi_service_handle, &char_uuid, ESP_GATT_PERM_WRITE,
                           props, NULL, NULL);
    break;
  }
  case ESP_GATTS_ADD_CHAR_EVT: {
    midi_char_handle = param->add_char.attr_handle;
    ESP_LOGI(TAG, "Characteristic added, handle=%d", midi_char_handle);
    break;
  }
  case ESP_GATTS_CONNECT_EVT:
    midi_conn_id = param->connect.conn_id;
    ESP_LOGI(TAG, "Client connected (conn_id=%d)", midi_conn_id);
    break;
  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(TAG, "Client disconnected, restarting adv");
    esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    });
    break;
  case ESP_GATTS_WRITE_EVT:
    if (param->write.len > 0) {
      midi_rx_callback(param->write.value, param->write.len);
    }
    break;
  default:
    break;
  }
}

// ==== Init function ====
void ble_midi_receiver_init(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
  ESP_ERROR_CHECK(esp_bluedroid_init());
  ESP_ERROR_CHECK(esp_bluedroid_enable());

  ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));
  ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_cb));
  ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
}

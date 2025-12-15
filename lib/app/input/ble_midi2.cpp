#include "ble_midi.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "sdkconfig.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <string.h>

ESP_EVENT_DEFINE_BASE(EVENT_BLE_BASE);

namespace teslasynth::app::devices::ble_midi {
namespace {
const char *TAG = "BLE_MIDI";

const ble_uuid128_t midi_service_uuid =
    BLE_UUID128_INIT(0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7, 0x33, 0x4B,
                     0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03);

const ble_uuid128_t midi_char_uuid =
    BLE_UUID128_INIT(0xF3, 0x10, 0x19, 0xD2, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41,
                     0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77);

uint16_t midi_char_handle;
uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
StreamBufferHandle_t midi_buffer;

void ble_app_on_sync(void);
void ble_app_advertise(void);

inline void receive_midi(ble_gatt_access_ctxt *ctxt) {
  ESP_LOGD(TAG, "MIDI write, om_len=%d", ctxt->om->om_len);
  uint8_t buf[64];
  uint16_t copied = 0;

  int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &copied);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_hs_mbuf_to_flat failed, rc=%d", rc);
    return;
  }

  if (copied > 0) {
    if (xStreamBufferSend(midi_buffer, buf, copied, 0) != copied) {
      ESP_LOGE(TAG, "Couldn't write received BLE data!");
    }
  }
}

int midi_gatt_access_cb(uint16_t conn_hdl, uint16_t attr_hdl,
                        struct ble_gatt_access_ctxt *ctxt, void *arg) {
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_WRITE_CHR:
    receive_midi(ctxt);
    return 0;

  case BLE_GATT_ACCESS_OP_READ_CHR:
    return BLE_ATT_ERR_READ_NOT_PERMITTED;

  default:
    return BLE_ATT_ERR_UNLIKELY;
  }
}

const struct ble_gatt_chr_def midi_gatt_chr_defs[]{
    {
        .uuid = &midi_char_uuid.u,
        .access_cb = midi_gatt_access_cb,
        .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE_NO_RSP |
                 BLE_GATT_CHR_F_WRITE,
        .min_key_size = 0,
        .val_handle = &midi_char_handle,
    },
    {0} // No more characteristics
};

const struct ble_gatt_svc_def midi_gatt_svr_defs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &midi_service_uuid.u,
        .characteristics = midi_gatt_chr_defs,
    },
    {0} // No more services
};

int gap_event_handler(struct ble_gap_event *event, void *) {
  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    if (event->connect.status == 0) {
      ESP_LOGI(TAG, "Connected, handle=%d", event->connect.conn_handle);
      conn_handle = event->connect.conn_handle;
      ESP_ERROR_CHECK(esp_event_post(EVENT_BLE_BASE, BLE_DEVICE_DISCONNECTED,
                                     NULL, 0, portMAX_DELAY));
    } else {
      ESP_LOGI(TAG, "Connect failed; status=%d", event->connect.status);
      ble_app_advertise();
    }
    return 0;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
    conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ble_app_advertise();
    ESP_ERROR_CHECK(esp_event_post(EVENT_BLE_BASE, BLE_DEVICE_DISCONNECTED,
                                   NULL, 0, portMAX_DELAY));
    return 0;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(TAG, "Advertising complete; restarting");
    ble_app_advertise();
    return 0;

  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(TAG, "Subscribe event; cur_notify=%d",
             event->subscribe.cur_notify);
    return 0;

  default:
    return 0;
  }
}

void ble_app_advertise(void) {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  int rc;

  memset(&fields, 0, sizeof(fields));
  const char *name = ble_svc_gap_device_name();
  fields.name = (uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  // Advertise MIDI service UUID
  fields.uuids128 = (ble_uuid128_t *)&midi_service_uuid;
  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_gap_adv_set_fields failed; rc=%d", rc);
    return;
  }

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params,
                         gap_event_handler, NULL);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_gap_adv_start failed; rc=%d", rc);
  } else {
    ESP_LOGI(TAG, "Advertising as BLE MIDI device");
  }
}

void gatt_svr_init(void) {
  int rc = ble_gatts_count_cfg(midi_gatt_svr_defs);
  assert(rc == 0);

  rc = ble_gatts_add_svcs(midi_gatt_svr_defs);
  assert(rc == 0);
}

void ble_app_on_sync(void) {
  int rc;
  uint8_t addr_val[6];

  rc = ble_hs_id_infer_auto(0, &addr_val[0]);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
    return;
  }

  rc = ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, addr_val, NULL);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_hs_id_copy_addr failed: %d", rc);
    return;
  }

  ESP_LOGI(TAG, "Device address: %02X:%02X:%02X:%02X:%02X:%02X", addr_val[5],
           addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);

  ble_app_advertise();
}

void host_task(void *) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}
} // namespace

void init(StreamBufferHandle_t sbuf) {
  assert(sbuf != nullptr);
  midi_buffer = sbuf;
  ESP_ERROR_CHECK(nimble_port_init());

  ble_hs_cfg.sync_cb = ble_app_on_sync;
  ble_hs_cfg.gatts_register_cb = NULL;

  ble_svc_gap_device_name_set(CONFIG_TESLASYNTH_DEVICE_NAME);
  ble_svc_gap_init();
  ble_svc_gatt_init();
  gatt_svr_init();
  nimble_port_freertos_init(host_task);
}
} // namespace teslasynth::app::devices::ble_midi

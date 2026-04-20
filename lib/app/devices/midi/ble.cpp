#include "sdkconfig.h"

#if CONFIG_SOC_BT_SUPPORTED

#include "../midi.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_store.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <cassert>
#include <cstdint>
#include <string.h>

namespace teslasynth::app::devices::midi::ble {
namespace {
constexpr char TAG[] = "BLE_MIDI";

// BLE MIDI service and I/O characteristic UUIDs (little-endian).
// Spec: "MIDI over Bluetooth Low Energy" v1.0a, section 3.
const ble_uuid128_t midi_service_uuid =
    BLE_UUID128_INIT(0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7, 0x33, 0x4B,
                     0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03);

const ble_uuid128_t midi_char_uuid =
    BLE_UUID128_INIT(0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41,
                     0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77);

uint16_t midi_char_handle;
StreamBufferHandle_t midi_buffer;
static bool adv_in_progress = false;

void ble_app_on_sync(void);
void ble_app_advertise(void);

inline void receive_midi(ble_gatt_access_ctxt *ctxt) {
  ESP_LOGD(TAG, "MIDI write, om_len=%d", ctxt->om->om_len);
  uint8_t buf[128];
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
    return 0; // spec requires Read; return empty payload

  default:
    return BLE_ATT_ERR_UNLIKELY;
  }
}

const struct ble_gatt_chr_def midi_gatt_chr_defs[]{
    {
        .uuid = &midi_char_uuid.u,
        .access_cb = midi_gatt_access_cb,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY |
                 BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE,
        .min_key_size = 0,
        .val_handle = &midi_char_handle,
    },
    {0}};

const struct ble_gatt_svc_def midi_gatt_svr_defs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &midi_service_uuid.u,
        .characteristics = midi_gatt_chr_defs,
    },
    {0}};

int gap_event_handler(struct ble_gap_event *event, void *) {
  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    if (event->connect.status == 0) {
      ESP_LOGI(TAG, "Connected, handle=%d", event->connect.conn_handle);
      adv_in_progress = false;
      ESP_ERROR_CHECK(esp_event_post(EVENT_MIDI_DEVICE_BASE,
                                     MIDI_DEVICE_CONNECTED, NULL, 0,
                                     portMAX_DELAY));
    } else {
      ESP_LOGE(TAG, "Connect failed; status=%d", event->connect.status);
      if (!adv_in_progress) {
        ble_app_advertise();
      }
    }
    return 0;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
    adv_in_progress = false;
    ble_app_advertise();
    ESP_ERROR_CHECK(esp_event_post(EVENT_MIDI_DEVICE_BASE,
                                   MIDI_DEVICE_DISCONNECTED, NULL, 0,
                                   portMAX_DELAY));
    return 0;

  case BLE_GAP_EVENT_ENC_CHANGE:
    ESP_LOGI(TAG, "Encryption change; conn=%d status=%d",
             event->enc_change.conn_handle, event->enc_change.status);
    return 0;

  case BLE_GAP_EVENT_REPEAT_PAIRING: {
    struct ble_gap_conn_desc desc;
    ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
    ble_store_util_delete_peer(&desc.peer_id_addr);
    return BLE_GAP_REPEAT_PAIRING_RETRY;
  }

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(TAG, "Advertising complete; reason=%d",
             event->adv_complete.reason);
    adv_in_progress = false;
    ble_app_advertise();
    return 0;

  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(TAG,
             "Subscribe event; attr=%d prev_notify=%d cur_notify=%d "
             "prev_indicate=%d cur_indicate=%d",
             event->subscribe.attr_handle, event->subscribe.prev_notify,
             event->subscribe.cur_notify, event->subscribe.prev_indicate,
             event->subscribe.cur_indicate);
    return 0;

  case BLE_GAP_EVENT_PASSKEY_ACTION: {
    ESP_LOGI(TAG, "Passkey action; action=%d", event->passkey.params.action);
    struct ble_sm_io io = {};
    io.action = event->passkey.params.action;
    ble_sm_inject_io(event->passkey.conn_handle, &io);
    return 0;
  }

  default:
    return 0;
  }
}

void ble_app_advertise(void) {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  int rc;

  ble_gap_adv_stop();

  // Advertising packet: flags (3B) + MIDI service UUID128 (18B) = 21B.
  memset(&fields, 0, sizeof(fields));
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.uuids128 = (ble_uuid128_t *)&midi_service_uuid;
  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_gap_adv_set_fields failed; rc=%d", rc);
    return;
  }

  // Scan response: device name.
  struct ble_hs_adv_fields rsp_fields;
  memset(&rsp_fields, 0, sizeof(rsp_fields));
  const char *name = ble_svc_gap_device_name();
  rsp_fields.name = (uint8_t *)name;
  rsp_fields.name_len = strlen(name);
  rsp_fields.name_is_complete = 1;

  rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields failed; rc=%d", rc);
    return;
  }

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params,
                         gap_event_handler, NULL);
  if (rc != 0) {
    ESP_LOGE(TAG, "ble_gap_adv_start failed; rc=%d", rc);
    adv_in_progress = false;
  } else {
    ESP_LOGI(TAG, "Advertising as BLE MIDI device");
    adv_in_progress = true;
  }
}

void gatt_svr_init(void) {
  int rc = ble_gatts_count_cfg(midi_gatt_svr_defs);
  assert(rc == 0);

  rc = ble_gatts_add_svcs(midi_gatt_svr_defs);
  assert(rc == 0);
}

void ble_app_on_sync(void) {
  uint8_t addr_val[6];

  int rc = ble_hs_id_infer_auto(0, &addr_val[0]);
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
  ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;

  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_gap_device_name_set(CONFIG_TESLASYNTH_DEVICE_NAME);
  gatt_svr_init();
  nimble_port_freertos_init(host_task);
}
} // namespace teslasynth::app::devices::midi::ble

#endif

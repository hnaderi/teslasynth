#include "freertos/idf_additions.h"
#include "sdkconfig.h"
#include <cstddef>

#if CONFIG_SOC_USB_OTG_SUPPORTED

#include "../midi.hpp"
#include "esp_log.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include <stdlib.h>

namespace teslasynth::app::devices::midi::usb {
namespace {
const char *TAG = "USB_MIDI";
StreamBufferHandle_t midi_buffer;

enum interface_count {
#if CFG_TUD_MIDI
  ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING,
#endif
  ITF_COUNT
};

// USB Endpoint numbers
enum usb_endpoints {
  // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
  EP_EMPTY = 0,
#if CFG_TUD_MIDI
  EPNUM_MIDI,
#endif
};

/** TinyUSB descriptors **/

#define TUSB_DESCRIPTOR_TOTAL_LEN                                              \
  (TUD_CONFIG_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN)

/**
 * @brief String descriptor
 */
const char *s_str_desc[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "hnaderi",            // 1: Manufacturer
    "Teslasynth dongle",  // 2: Product
    "123456",             // 3: Serials, should use chip ID
    CONFIG_TESLASYNTH_DEVICE_NAME, // 4: MIDI
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and a
 * MIDI interface
 */
const uint8_t s_midi_cfg_desc[] = {
    // Configuration number, interface count, string index, total length,
    // attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
};

#if (TUD_OPT_HIGH_SPEED)
/**
 * @brief High Speed configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and a
 * MIDI interface
 */
const uint8_t s_midi_hs_cfg_desc[] = {
    // Configuration number, interface count, string index, total length,
    // attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 512),
};
#endif // TUD_OPT_HIGH_SPEED

inline uint8_t usb_midi_cin_length(uint8_t cin) {
  static const uint8_t len_table[16] = {
      /*0*/ 0, /*1*/ 0,
      /*2*/ 2, /*3*/ 3,
      /*4*/ 3, /*5*/ 1,
      /*6*/ 2, /*7*/ 3,
      /*8*/ 3, /*9*/ 3,
      /*A*/ 3, /*B*/ 3,
      /*C*/ 2, /*D*/ 2,
      /*E*/ 3, /*F*/ 1,
  };
  return len_table[cin & 0x0F];
}

void midi_read(void *) {
  uint8_t packet[4];
  bool read = false;
  for (;;) {
    vTaskDelay(1);
    while (tud_midi_available()) {
      read = tud_midi_packet_read(packet);
      if (read) {
        size_t count = usb_midi_cin_length(packet[0] & 0x0F);
        if (xStreamBufferSend(midi_buffer, &packet[1], count, 0) != count) {
          ESP_LOGE(TAG, "Couldn't write received USB data!");
        }
      }
    }
  }
}
} // namespace
void init(StreamBufferHandle_t sbuf) {
  assert(sbuf != nullptr);
  midi_buffer = sbuf;

  ESP_LOGI(TAG, "USB initialization");

  tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

  tusb_cfg.descriptor.string = s_str_desc;
  tusb_cfg.descriptor.string_count = sizeof(s_str_desc) / sizeof(s_str_desc[0]);
  tusb_cfg.descriptor.full_speed_config = s_midi_cfg_desc;
#if (TUD_OPT_HIGH_SPEED)
  tusb_cfg.descriptor.high_speed_config = s_midi_hs_cfg_desc;
  tusb_cfg.descriptor.qualifier = NULL;
#endif // TUD_OPT_HIGH_SPEED

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  ESP_LOGI(TAG, "USB initialization DONE");

  // Read received MIDI packets
  ESP_LOGI(TAG, "MIDI read task init");
  xTaskCreate(midi_read, "usb_midi_read", 2 * 1024, NULL, 5, NULL);
}
} // namespace teslasynth::app::devices::midi::usb

#endif

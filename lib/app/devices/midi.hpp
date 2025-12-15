#pragma once

#include "esp_event_base.h"
#include "freertos/idf_additions.h"

ESP_EVENT_DECLARE_BASE(EVENT_MIDI_DEVICE_BASE);
enum {
  MIDI_DEVICE_CONNECTED,
  MIDI_DEVICE_DISCONNECTED,
};

namespace teslasynth::app::devices::midi {
constexpr bool ble_support =
#if CONFIG_SOC_BT_SUPPORTED
    true;
#else
    false;
#endif

namespace ble {
void init(StreamBufferHandle_t sbuf);
}

constexpr bool usb_support =
#if CONFIG_SOC_OTG_SUPPORTED
    true;
#else
    false;
#endif
namespace usb {
void init(StreamBufferHandle_t sbuf);
}

void init(StreamBufferHandle_t buf);
} // namespace teslasynth::app::devices::midi

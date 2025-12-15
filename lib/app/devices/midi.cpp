#include "midi.hpp"
#include "esp_event_base.h"

ESP_EVENT_DEFINE_BASE(EVENT_MIDI_DEVICE_BASE);

namespace teslasynth::app::devices::midi {
void init(StreamBufferHandle_t buf) {
  if (usb_support)
    usb::init(buf);
  else if (ble_support)
    ble::init(buf);
  else
    static_assert(ble_support || usb_support,
                  "Must support at least one midi driver");
}
} // namespace teslasynth::app::devices::midi

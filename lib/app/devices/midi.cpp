#include "midi.hpp"
#include "esp_event_base.h"

ESP_EVENT_DEFINE_BASE(EVENT_MIDI_DEVICE_BASE);

namespace teslasynth::app::devices::midi {
void init(StreamBufferHandle_t buf) {
  if (ble_support)
    ble::init(buf);
  else if (usb_support)
    usb::init(buf);
}
} // namespace teslasynth::app::devices::midi

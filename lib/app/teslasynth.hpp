#pragma once

#include "application.hpp"
#include "freertos/idf_additions.h"

namespace teslasynth::app {

namespace devices {

namespace storage {
void init();
}

namespace rmt {
void init(void);
}

namespace ble_midi {
void init(StreamBufferHandle_t buf);
}
} // namespace devices

namespace synth {
StreamBufferHandle_t init(PlaybackHandle handle);
}

namespace gui {
void init();
}

namespace cli {
void init(UIHandle handle);
}

} // namespace teslasynth::app

#pragma once

namespace teslasynth::app::helpers::maintenance {
bool check();
void reboot() __attribute__((__noreturn__));
} // namespace teslasynth::app::helpers::maintenance

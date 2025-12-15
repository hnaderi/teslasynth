#pragma once

#include "configuration/hardware.hpp"

namespace teslasynth::app::gui {

void init(const configuration::hardware::MinimalDisplayPanelConfig &config);
void init(const configuration::hardware::FullDisplayPanelConfig &config);
void init(const configuration::hardware::DisplayConfig &display);

} // namespace teslasynth::app::gui

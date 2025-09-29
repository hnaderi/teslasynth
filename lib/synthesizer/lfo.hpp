#pragma once

#include <stdint.h>

struct Vibrato {
  uint8_t freq, depth;
  bool enabled = false;
};

class LFO {};

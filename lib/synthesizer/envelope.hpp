#pragma once

#include <stdint.h>

enum CurveType { Lin, Exp };

struct ADSR {
  uint16_t attack, decay;
  uint8_t sustain;
  uint16_t release;
  enum CurveType type;
};

class Envelope {};
class Curve {};

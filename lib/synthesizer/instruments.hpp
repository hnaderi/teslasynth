#pragma once

#include "envelope.hpp"
#include "lfo.hpp"

struct Instrument {
  ADSR envelope;
  Vibrato vibrato;
};

static Instrument instruments[] = {
    {.envelope = {30, 20, 50, 40, CurveType::Exp}, .vibrato = {}},
    {.envelope = {30, 20, 50, 40, CurveType::Exp}, .vibrato = {1, 2, true}},
    {.envelope = {30, 20, 50, 40, CurveType::Exp}, .vibrato = {1, 3, true}},
};

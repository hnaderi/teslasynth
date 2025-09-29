#pragma once

#include "core.hpp"
#include "synth.hpp"
#include <stdint.h>

#define OUTPUT_BUFFER_SIZE 100
class AudioOutput {
  Pulse buffer[OUTPUT_BUFFER_SIZE];
  volatile uint16_t head, tail;
  SynthChannel *channel;

public:
  AudioOutput(SynthChannel *channel);
};

#pragma once

#include <stdint.h>
#define MIDI_EVENT_QUEUE_SIZE 64

enum midi_event_type_t {
  MIDI_NOTE_ON,
  MIDI_NOTE_OFF,
  MIDI_CC,
  // ... extend as needed
};

struct midi_event_t {
  midi_event_type_t type;
  uint8_t channel;
  uint8_t note;
  uint8_t velocity;
  uint32_t sampleTime; // absolute sample index when it should occur
};

struct midi_ring_t {
  midi_event_t events[MIDI_EVENT_QUEUE_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
};

static midi_ring_t midiQueue;

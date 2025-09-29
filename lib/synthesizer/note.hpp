#pragma once

// #include <chrono>
#include <stdint.h>

// using namespace std::chrono;

constexpr auto ticks_per_sec = 2'000'000;
// constexpr auto tick_period = 1s / ticks_per_sec;
// constexpr auto max_on_time_us = 100us;
// const auto max_on_time = max_on_time_us / tick_period;

struct NotePulse {
  uint32_t start, off, end;
};

class Note {
  uint8_t _number, _velocity;
  uint32_t _on, _release, _now;
  uint8_t instrument;
  bool _started = false;
  bool _active = false;
  bool _released = false;
  bool _high = false;

public:
  Note();
  void start(uint8_t number, uint8_t velocity, uint8_t instrument,
             uint32_t time);
  void release(uint32_t time);
  bool tick(NotePulse *out);

  inline bool is_active() const { return _active; }
  inline uint32_t time() const { return _now; }
  inline uint8_t number() const { return _number; }
};

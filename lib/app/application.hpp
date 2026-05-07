// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "configuration/storage.hpp"
#include "esp_event.h"
#include "freertos/idf_additions.h"
#include "midi_synth.hpp"
#include "synthesizer_events.hpp"
#include <cassert>
#include <configuration/synth.hpp>

namespace teslasynth::app {
using namespace midisynth;

namespace {
void on_track_play(bool playing) {
  if (playing) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        esp_event_post(EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, NULL, 0, 0));
  } else {
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        esp_event_post(EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, NULL, 0, 0));
  }
}
}; // namespace

class PlaybackHandle {
  AppSynth *impl = nullptr;
  SemaphoreHandle_t lock = nullptr;

public:
  PlaybackHandle() = default;
  PlaybackHandle(AppSynth *impl, SemaphoreHandle_t lock) : impl(impl), lock(lock) {}

  inline void acquire() {
    assert(lock != nullptr);
    xSemaphoreTake(lock, portMAX_DELAY);
  }
  inline void release() {
    assert(lock != nullptr);
    xSemaphoreGive(lock);
  }

  inline void handle(MidiChannelMessage msg, Duration time) {
    assert(impl != nullptr);
    impl->handle(msg, time);
  }
  template <size_t BUFSIZE>
  inline void
  sample_all(Duration16 max,
             PulseBuffer<configuration::hardware::OutputConfig::size, BUFSIZE> &output) {
    assert(impl != nullptr);
    impl->sample_all(max, output);
  };
};

class UIHandle {
  AppSynth *impl;
  SemaphoreHandle_t synth_lock, config_lock;

public:
  UIHandle() {}
  UIHandle(AppSynth *impl, SemaphoreHandle_t synth, SemaphoreHandle_t config)
      : impl(impl), synth_lock(synth), config_lock(config) {}

  inline AppConfig config_read() const {
    xSemaphoreTake(config_lock, portMAX_DELAY);
    auto res = impl->configuration();
    xSemaphoreGive(config_lock);
    return res;
  }

  inline void config_set(const AppConfig &config, bool reload = false, bool persist = false) {
    xSemaphoreTake(config_lock, portMAX_DELAY);

    xSemaphoreTake(synth_lock, portMAX_DELAY);
    impl->configuration() = config;
    if (reload)
      impl->reload_config();
    xSemaphoreGive(synth_lock);

    xSemaphoreGive(config_lock);
    ESP_ERROR_CHECK(
        esp_event_post(EVENT_SYNTHESIZER_BASE, SYNTHESIZER_CONFIG_UPDATED, NULL, 0, portMAX_DELAY));
    if (persist)
      configuration::synth::persist(config);
  }

  inline AppConfig config_reset(bool reload = false, bool persist = false) {
    AppConfig config;
    config_set(config, reload, persist);
    return config;
  }

  inline void playback_off() {
    xSemaphoreTake(synth_lock, portMAX_DELAY);
    impl->off();
    xSemaphoreGive(synth_lock);
  }
};

class Application {
  AppSynth impl;
  SemaphoreHandle_t synth_lock, config_lock;

public:
  Application() : synth_lock(xSemaphoreCreateMutex()), config_lock(xSemaphoreCreateMutex()) {}
  Application(const AppConfig &config)
      : impl(config, on_track_play), synth_lock(xSemaphoreCreateMutex()),
        config_lock(xSemaphoreCreateMutex()) {}
  void load(const AppConfig &config) {
    impl.configuration() = config;
    impl.reload_config();
  }
  bool reload_config() {
    AppConfig config;
    bool res = configuration::synth::read(config);
    if (res)
      load(config);
    return res;
  }
  PlaybackHandle playback() { return PlaybackHandle(&impl, synth_lock); }
  UIHandle ui() { return UIHandle(&impl, synth_lock, config_lock); }
};
}; // namespace teslasynth::app

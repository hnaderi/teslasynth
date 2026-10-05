// Copyright Hossein Naderi 2025, 2026
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "midi_synth.hpp"
#include "config_data.hpp"
#include "hardware.hpp"

using AppConfig = teslasynth::midisynth::Configuration<
    teslasynth::app::configuration::hardware::OutputConfig::size>;
using AppMidiRoutingConfig = teslasynth::midisynth::MidiRoutingConfig<
    teslasynth::app::configuration::hardware::OutputConfig::size>;
using AppSynth =
    teslasynth::midisynth::Teslasynth<teslasynth::app::configuration::hardware::OutputConfig::size>;

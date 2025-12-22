#pragma once

#include "array"
#include "envelope.hpp"
#include "lfo.hpp"
#include <stddef.h>

namespace teslasynth::synth {
using namespace teslasynth::core;

struct Instrument {
  ADSR envelope;
  Vibrato vibrato;

  constexpr bool operator==(const Instrument &b) const {
    return envelope == b.envelope && vibrato == b.vibrato;
  }
  constexpr bool operator!=(const Instrument &b) const {
    return envelope != b.envelope || vibrato != b.vibrato;
  }

  inline operator std::string() const {
    std::string stream = "envelope " + std::string(envelope) + " vibrato " +
                         std::string(vibrato);
    return stream;
  }
};

enum class InstrumentId : uint8_t {
  SquareWave = 0,

  // Leads
  MonoLead,
  SoftLead,
  BrightLead,
  SyncLead,
  SawLead,

  // Plucks & Keys
  SynthPluck,
  HarpPluck,
  EPluck,
  FMKey,
  BellKey,

  // Bass
  SubBass,
  AnalogBass,
  RubberBass,
  AcidBass,

  // Pads
  WarmPad,
  SlowPad,
  ChoirPad,
  GlassPad,
  MotionPad,

  // Organs & Brass
  Organ,
  Brass,
  SoftBrass,

  // Strings
  Strings,
  StaccatoStrings,

  // Percussive / FX
  SynthHit,
  NoiseHit,
  RiseFX,
  FallFX,

  Count
};
constexpr size_t instruments_size = static_cast<size_t>(InstrumentId::Count);
constexpr std::array<const char *, instruments_size> instrument_names = {{
    "Square Wave",

    // Leads
    "Mono Lead",
    "Soft Lead",
    "Bright Lead",
    "Sync Lead",
    "Saw Lead",

    // Plucks & Keys
    "Synth Pluck",
    "Harp Pluck",
    "Electric Pluck",
    "FM Key",
    "Bell Key",

    // Bass
    "Sub Bass",
    "Analog Bass",
    "Rubber Bass",
    "Acid Bass",

    // Pads
    "Warm Pad",
    "Slow Pad",
    "Choir Pad",
    "Glass Pad",
    "Motion Pad",

    // Organs & Brass
    "Organ",
    "Brass",
    "Soft Brass",

    // Strings
    "Strings",
    "Staccato Strings",

    // Percussive / FX
    "Synth Hit",
    "Noise Hit",
    "Rise FX",
    "Fall FX",
}};
constexpr std::array<Instrument, instruments_size> instruments{{
    {.envelope = ADSR::constant(EnvelopeLevel(1)), .vibrato = Vibrato::none()},

    // ====================
    // Leads
    // ====================

    // 1 — Mono Lead
    {.envelope = {5_ms, 10_ms, EnvelopeLevel(0.85), 20_ms, CurveType::Exp},
     .vibrato = {5_hz, 1.5_hz}},

    // 2 — Soft Lead
    {.envelope = {15_ms, 20_ms, EnvelopeLevel(0.70), 40_ms, CurveType::Lin},
     .vibrato = {3_hz, 1_hz}},

    // 3 — Bright Lead
    {.envelope = {3_ms, 8_ms, EnvelopeLevel(0.90), 15_ms, CurveType::Const},
     .vibrato = {6_hz, 2_hz}},

    // 4 — Sync Lead
    {.envelope = {8_ms, 5_ms, EnvelopeLevel(1.0), 10_ms, CurveType::Exp},
     .vibrato = {7_hz, 3_hz}},

    // 5 — Saw Lead
    {.envelope = {10_ms, 10_ms, EnvelopeLevel(0.80), 25_ms, CurveType::Lin},
     .vibrato = {4_hz, 1.5_hz}},

    // ====================
    // Plucks & Keys
    // ====================

    // 6 — Synth Pluck
    {.envelope = {2_ms, 20_ms, EnvelopeLevel(0.20), 15_ms, CurveType::Exp},
     .vibrato = Vibrato::none()},

    // 7 — Harp Pluck
    {.envelope = {3_ms, 30_ms, EnvelopeLevel(0.15), 40_ms, CurveType::Lin},
     .vibrato = {1_hz, 0.3_hz}},

    // 8 — Electric Pluck
    {.envelope = {5_ms, 15_ms, EnvelopeLevel(0.35), 25_ms, CurveType::Lin},
     .vibrato = {2_hz, 0.5_hz}},

    // 9 — FM Key
    {.envelope = {10_ms, 20_ms, EnvelopeLevel(0.75), 40_ms, CurveType::Const},
     .vibrato = {4_hz, 0.8_hz}},

    // 10 — Bell Key
    {.envelope = {2_ms, 40_ms, EnvelopeLevel(0.10), 80_ms, CurveType::Exp},
     .vibrato = {6_hz, 2_hz}},

    // ====================
    // Bass
    // ====================

    // 11 — Sub Bass
    {.envelope = {40_ms, 20_ms, EnvelopeLevel(0.95), 60_ms, CurveType::Lin},
     .vibrato = Vibrato::none()},

    // 12 — Analog Bass
    {.envelope = {25_ms, 15_ms, EnvelopeLevel(0.80), 45_ms, CurveType::Exp},
     .vibrato = {1_hz, 1.5_hz}},

    // 13 — Rubber Bass
    {.envelope = {15_ms, 25_ms, EnvelopeLevel(0.65), 30_ms, CurveType::Lin},
     .vibrato = {3_hz, 2_hz}},

    // 14 — Acid Bass
    {.envelope = {5_ms, 10_ms, EnvelopeLevel(0.90), 20_ms, CurveType::Const},
     .vibrato = {6_hz, 3_hz}},

    // ====================
    // Pads
    // ====================

    // 15 — Warm Pad
    {.envelope = {40_ms, 60_ms, EnvelopeLevel(0.70), 80_ms, CurveType::Exp},
     .vibrato = {0.8_hz, 2_hz}},

    // 16 — Slow Pad
    {.envelope = {80_ms, 100_ms, EnvelopeLevel(0.60), 120_ms, CurveType::Exp},
     .vibrato = {0.5_hz, 3_hz}},

    // 17 — Choir Pad
    {.envelope = {60_ms, 50_ms, EnvelopeLevel(0.65), 90_ms, CurveType::Lin},
     .vibrato = {1.2_hz, 4_hz}},

    // 18 — Glass Pad
    {.envelope = {30_ms, 40_ms, EnvelopeLevel(0.50), 70_ms, CurveType::Exp},
     .vibrato = {2_hz, 5_hz}},

    // 19 — Motion Pad
    {.envelope = {25_ms, 35_ms, EnvelopeLevel(0.55), 60_ms, CurveType::Exp},
     .vibrato = {3_hz, 6_hz}},

    // ====================
    // Organs & Brass
    // ====================

    // 20 — Organ
    {.envelope = {10_ms, 5_ms, EnvelopeLevel(1.0), 15_ms, CurveType::Const},
     .vibrato = {5_hz, 1_hz}},

    // 21 — Brass
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(0.85), 40_ms, CurveType::Lin},
     .vibrato = {4_hz, 2_hz}},

    // 22 — Soft Brass
    {.envelope = {45_ms, 30_ms, EnvelopeLevel(0.75), 60_ms, CurveType::Lin},
     .vibrato = {2_hz, 1.5_hz}},

    // ====================
    // Strings
    // ====================

    // 23 — Strings
    {.envelope = {50_ms, 40_ms, EnvelopeLevel(0.80), 70_ms, CurveType::Exp},
     .vibrato = {1.5_hz, 3_hz}},

    // 24 — Staccato Strings
    {.envelope = {15_ms, 10_ms, EnvelopeLevel(0.70), 20_ms, CurveType::Lin},
     .vibrato = {3_hz, 1_hz}},

    // ====================
    // Percussive / FX
    // ====================

    // 25 — Synth Hit
    {.envelope = {2_ms, 10_ms, EnvelopeLevel(0.30), 15_ms, CurveType::Exp},
     .vibrato = Vibrato::none()},

    // 26 — Noise Hit
    {.envelope = {1_ms, 20_ms, EnvelopeLevel(0.20), 30_ms, CurveType::Lin},
     .vibrato = Vibrato::none()},

    // 27 — Rise FX
    {.envelope = {60_ms, 80_ms, EnvelopeLevel(1.0), 100_ms, CurveType::Lin},
     .vibrato = {4_hz, 6_hz}},

    // 28 — Fall FX
    {.envelope = {5_ms, 40_ms, EnvelopeLevel(0.40), 80_ms, CurveType::Exp},
     .vibrato = {2_hz, 5_hz}},
}};

const inline Instrument &default_instrument() { return instruments[0]; }

constexpr const char *get_instrument_name(InstrumentId id) {
  if (id < InstrumentId::Count)
    return instrument_names[static_cast<size_t>(id)];
  else
    return "-";
}

}; // namespace teslasynth::synth

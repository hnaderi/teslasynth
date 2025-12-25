#pragma once

#include "percussion.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace teslasynth::synth::bank {
enum class PercussionId : uint8_t {
  Kick,
  Snare,
  Clap,
  ClosedHat,
  OpenHat,
  LowTom,
  MidTom,
  HighTom,
  Rimshot,
  Cowbell,
  Shaker,
  Crash,
  Ride,

  Count
};

constexpr size_t percussion_size = static_cast<size_t>(PercussionId::Count);

constexpr std::array<const char *, percussion_size> percussion_names = {{
    "Kick",
    "Snare",
    "Clap",
    "Closed Hat",
    "Open Hat",
    "Low Tom",
    "Mid Tom",
    "High Tom",
    "Rimshot",
    "Cowbell",
    "Shaker",
    "Crash",
    "Ride",
}};

constexpr std::array<Percussion, percussion_size> percussion_kit{{
    // Kick — tonal body + small noise, long decay
    {.burst = 200_ms,
     .prf = 55_hz,
     .noise = Probability(0.10),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 180_ms)},

    // Snare — strong noise + mid body
    {.burst = 40_ms,
     .prf = 180_hz,
     .noise = Probability(0.95),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(2_ms, 220_ms)},

    // Clap — pure noise, short-mid decay
    {.burst = 100_ms,
     .prf = 0_hz, // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 80_ms)},

    // Closed Hat — bright, short, crisp
    {.burst = 20_ms,
     .prf = 0_hz, // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 30_ms)},

    // Open Hat — long noise shimmer
    {.burst = 300_ms, // long enough for full decay
     .prf = 0_hz,     // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(2_ms, 280_ms)},

    // Low Tom — tonal, deep, resonant
    {.burst = 100_ms,
     .prf = 110_hz,
     .noise = Probability(0.15),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 200_ms)},

    // Mid Tom — tonal, punchy
    {.burst = 70_ms,
     .prf = 180_hz,
     .noise = Probability(0.10),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 170_ms)},

    // High Tom — tonal, bright
    {.burst = 60_ms,
     .prf = 260_hz,
     .noise = Probability(0.08),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 140_ms)},

    // Rimshot — sharp, bright, noisy transient
    {.burst = 20_ms,
     .prf = 900_hz,
     .noise = Probability(0.40),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 80_ms)},

    // Cowbell — tonal metallic ring
    {.burst = 80_ms,
     .prf = 540_hz,
     .noise = Probability(0.05),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(2_ms, 300_ms)},

    // Shaker — pure noise, short burst
    {.burst = 18_ms,
     .prf = 0_hz, // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(1_ms, 100_ms)},

    // Crash — long broadband noise
    {.burst = 300_ms,
     .prf = 0_hz, // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(5_ms, 1200_ms)},

    // Ride — long metallic noise wash
    {.burst = 200_ms,
     .prf = 0_hz, // noise-only mode
     .noise = Probability(1.0),
     .skip = Probability(0.0),
     .envelope = envelopes::AD::exponential(3_ms, 900_ms)},
}};

constexpr const PercussionId midi_to_percussion(uint8_t note) {
  switch (note) {
  // 35–36: Kicks
  case 35: // Acoustic Bass Drum
  case 36: // Bass Drum 1
    return PercussionId::Kick;

  // 37: Rimshot
  case 37: // Side Stick
    return PercussionId::Rimshot;

  // 38,40: Snares
  case 38: // Acoustic Snare
  case 40: // Electric Snare
    return PercussionId::Snare;

  // 39: Clap
  case 39: // Hand Clap
    return PercussionId::Clap;

  // 41,43,45: Low / Mid Toms
  case 41: // Low Floor Tom
  case 43: // High Floor Tom
  case 45: // Low Tom
    return PercussionId::LowTom;

  // 47,48,50: Mid / High Toms
  case 47: // Low-Mid Tom
    return PercussionId::MidTom;
  case 48: // Hi-Mid Tom
  case 50: // High Tom
    return PercussionId::HighTom;

  // 42,44: Closed hats
  case 42: // Closed Hi-Hat
  case 44: // Pedal Hi-Hat
    return PercussionId::ClosedHat;

  // 46: Open hat
  case 46: // Open Hi-Hat
    return PercussionId::OpenHat;

  // 49,52,55,57: Crashes
  case 49: // Crash Cymbal 1
  case 52: // Chinese Cymbal
  case 55: // Splash Cymbal
  case 57: // Crash Cymbal 2
    return PercussionId::Crash;

  // 51,53,59: Rides
  case 51: // Ride Cymbal 1
  case 53: // Ride Bell
  case 59: // Ride Cymbal 2
    return PercussionId::Ride;

  // 54,68: Shakers / Tambourines
  case 54: // Tambourine
  case 68: // Maracas
    return PercussionId::Shaker;

  // 56: Cowbell
  case 56: // Cowbell
    return PercussionId::Cowbell;

  // 58: Vibraslap → map to Shaker
  case 58: // Vibraslap
    return PercussionId::Shaker;

  default:
    // Reasonable fallback: use Kick for any unmapped note
    return PercussionId::Kick;
  }
}

constexpr const Percussion &percussion_from_midi_note(uint8_t note) {
  auto id = midi_to_percussion(note);
  assert(id < PercussionId::Count);
  return percussion_kit[static_cast<uint8_t>(id)];
}

} // namespace teslasynth::synth::bank

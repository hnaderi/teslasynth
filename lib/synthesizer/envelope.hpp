#pragma once

#include "core.hpp"
#include "core/envelope_level.hpp"
#include "curve.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace teslasynth::synth {
using namespace teslasynth::core;

template <typename Config> using StageFn = Curve (*)(const Config &);

namespace envelopes {

struct ADSR {
  Duration32 attack;
  Duration32 decay;
  EnvelopeLevel sustain = EnvelopeLevel(1);
  Duration32 release;
  enum CurveType type = CurveType::Exp;

  constexpr static ADSR linear(Duration32 attack, Duration32 decay,
                               EnvelopeLevel sustain, Duration32 release) {
    return {attack, decay, sustain, release, CurveType::Lin};
  }
  constexpr static ADSR exponential(Duration32 attack, Duration32 decay,
                                    EnvelopeLevel sustain, Duration32 release) {
    return {attack, decay, sustain, release, CurveType::Exp};
  }

  constexpr bool operator==(const ADSR &b) const {
    return attack == b.attack && decay == b.decay && sustain == b.sustain &&
           release == b.release && type == b.type;
  }

  constexpr bool operator!=(const ADSR &b) const {
    return attack != b.attack || decay != b.decay || sustain != b.sustain ||
           release != b.release || type != b.type;
  }

  inline operator std::string() const {
    std::string stream;
    switch (type) {
    case CurveType::Lin:
      stream = "lin";
      break;
    case CurveType::Exp:
      stream = "exp";
      break;
    }

    stream += " A: " + std::string(attack) + " D: " + std::string(decay) +
              " S: " + std::string(sustain) + " R: " + std::string(release);

    return stream;
  }
};

struct ADSRConfig {
  using Config = ADSR;
  static Curve attack_fn(const ADSR &cfg) {
    return Curve(EnvelopeLevel(0), EnvelopeLevel(1), cfg.attack, cfg.type);
  }

  static Curve decay_fn(const ADSR &cfg) {
    return Curve(EnvelopeLevel(1), cfg.sustain, cfg.decay, cfg.type);
  }

  static Curve sustain_fn(const ADSR &cfg) { return Curve(cfg.sustain); }

  static Curve release_fn(const ADSR &cfg) {
    return Curve(cfg.sustain, EnvelopeLevel(0), cfg.release, cfg.type);
  }
  static constexpr std::array<StageFn<Config>, 4> stages = {
      attack_fn, decay_fn, sustain_fn, release_fn};
};

using Const = EnvelopeLevel;
struct ConstConfig {
  using Config = Const;
  static Curve sustain(const Config &cfg) { return Curve(cfg); }
  static constexpr std::array<StageFn<Config>, 1> stages = {sustain};
};

struct AD {
  Duration32 attack;
  Duration32 decay;
  enum CurveType type = CurveType::Exp;

  constexpr static AD linear(Duration32 attack, Duration32 decay) {
    return {attack, decay, CurveType::Lin};
  }
  constexpr static AD exponential(Duration32 attack, Duration32 decay) {
    return {attack, decay, CurveType::Exp};
  }

  constexpr bool operator==(const AD &b) const {
    return attack == b.attack && decay == b.decay && type == b.type;
  }

  constexpr bool operator!=(const AD &b) const {
    return attack != b.attack || decay != b.decay || type != b.type;
  }

  inline operator std::string() const {
    std::string stream;
    switch (type) {
    case CurveType::Lin:
      stream = "lin";
      break;
    case CurveType::Exp:
      stream = "exp";
      break;
    }

    stream += " A: " + std::string(attack) + " D: " + std::string(decay);

    return stream;
  }
};

struct ADConfig {
  using Config = AD;
  static Curve attack_fn(const AD &cfg) {
    return Curve(EnvelopeLevel(0), EnvelopeLevel(1), cfg.attack, cfg.type);
  }

  static Curve decay_fn(const AD &cfg) {
    return Curve(EnvelopeLevel(1), EnvelopeLevel(0), cfg.decay, cfg.type);
  }
  static constexpr std::array<StageFn<Config>, 2> stages = {attack_fn,
                                                            decay_fn};
};

using EnvelopeConfig = std::variant<ADSR, AD, Const>;
} // namespace envelopes

template <typename Traits> class EnvelopeEngine {
  static constexpr uint8_t size = Traits::stages.size();
  static_assert(size > 0, "Envelope with 0 stage does not make any sense!");

  uint8_t _index = 0;
  typename Traits::Config _config;
  Curve _current;

  constexpr Duration32 progress(Duration32 delta, bool on) {
    Duration32 remained = delta;
    auto dt = _current.will_reach_target(remained);

    while (!is_off() && (dt.has_value() || (_current.hold() && !on))) {
      if (_index < size - 1) {
        if (_current.hold()) {
          dt = remained;
        }
        _current = Traits::stages[++_index](_config);
      } else {
        _index = size;
        _current = Curve(EnvelopeLevel(0));
      }
      remained = *dt;
      dt = _current.will_reach_target(remained);
    }
    return remained;
  }

public:
  using Config = typename Traits::Config;

  EnvelopeEngine(const Config &config)
      : _config(config), _current(Traits::stages[0](config)) {}
  EnvelopeLevel update(Duration32 delta, bool on) {
    if (is_off())
      return EnvelopeLevel(0);
    auto remained = progress(delta, on);
    return _current.update(remained);
  }
  constexpr uint8_t stage() const { return _index; }
  constexpr bool is_off() const { return _index >= size; }
};

class Envelope {
  using EnvelopeVariant = std::variant<EnvelopeEngine<envelopes::ADSRConfig>,
                                       EnvelopeEngine<envelopes::ADConfig>,
                                       EnvelopeEngine<envelopes::ConstConfig>>;

  EnvelopeVariant _state =
      EnvelopeEngine<envelopes::ConstConfig>(EnvelopeLevel(0));

  static constexpr EnvelopeVariant
  make_state(const envelopes::EnvelopeConfig &cfg) {
    return std::visit(
        [](auto const &c) -> EnvelopeVariant {
          using C = std::decay_t<decltype(c)>;
          if constexpr (std::is_same_v<C, envelopes::ADSR>) {
            return EnvelopeEngine<envelopes::ADSRConfig>(c);
          } else if constexpr (std::is_same_v<C, envelopes::AD>) {
            return EnvelopeEngine<envelopes::ADConfig>(c);
          } else if constexpr (std::is_same_v<C, envelopes::Const>) {
            return EnvelopeEngine<envelopes::ConstConfig>(c);
          } else {
            static_assert(false, "Unknown envelope config");
          }
        },
        cfg);
  }

public:
  Envelope() {}
  Envelope(const envelopes::EnvelopeConfig &cfg) : _state(make_state(cfg)) {}

  EnvelopeLevel update(Duration32 delta, bool on) {
    return std::visit([&](auto &e) { return e.update(delta, on); }, _state);
  }
  constexpr bool is_off() const {
    return std::visit([](auto &e) { return e.is_off(); }, _state);
  }
  constexpr uint8_t stage() const {
    return std::visit([](auto &e) { return e.stage(); }, _state);
  }
};

} // namespace teslasynth::synth

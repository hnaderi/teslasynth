#include "codec.hpp"
#include <cstdint>

namespace teslasynth::app::configuration::codec {
using helpers::JSONParser;
using namespace core;

namespace {
inline bool parse_duration16(const JSONParser::JSON &j, const char *key,
                             Duration16 &d) {
  auto nn = j.get(key);
  if (nn.is_number() && nn.number() <= UINT16_MAX) {
    d = Duration16::micros(nn.number());
    return true;
  } else
    return false;
}
} // namespace

bool parse(JSONParser &parser, AppConfig &config) {
  auto root = parser.json();

  auto tuning = root.get("tuning");
  if (tuning.is_number() && tuning.number() >= 1 && tuning.number() <= 1000)
    config.synth_config.a440 = Hertz(tuning.number());
  else
    return false;

  auto instrument = root.get("instrument");
  if (instrument.is_null())
    config.synth_config.instrument = {};
  else if (instrument.is_number())
    config.synth_config.instrument = instrument.number();
  else
    return false;

  auto channels = root.get("channels");
  if (channels.is_arr()) {
    int idx = 0;
    for (const auto &chobj : channels.arr()) {
      if (idx >= config.channels_size() || !chobj.is_obj())
        return false;

      auto &ch = config.channel(idx);

      auto notes = chobj.get("notes");
      if (notes.is_number())
        ch.notes = notes.number();
      else
        return false;

      if (!parse_duration16(chobj, "max-on-time", ch.max_on_time))
        return false;
      if (!parse_duration16(chobj, "min-dead-time", ch.min_deadtime))
        return false;
      if (!parse_duration16(chobj, "duty-window", ch.duty_window))
        return false;

      auto max_duty = chobj.get("max-duty");
      if (max_duty.is_number() && max_duty.number() < 100)
        ch.max_duty = midisynth::DutyCycle(max_duty.number_d());
      else
        return false;
    }
  }

  return true;
}
} // namespace teslasynth::app::configuration::codec

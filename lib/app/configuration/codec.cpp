#include "codec.hpp"
#include <cstdint>

namespace teslasynth::app::configuration::codec {
using namespace helpers;
using namespace core;

namespace {
inline bool parse_duration16(const JSONParser::JSONObjectView &j,
                             const char *key, Duration16 &d) {
  auto nn = j.get(key).number();
  if (nn.has_value() && *nn <= UINT16_MAX) {
    d = Duration16::micros(*nn);
    return true;
  } else
    return false;
}
} // namespace

bool parse(JSONParser &parser, AppConfig &config) {
  auto root = parser.root();

  auto tuning = root.get(keys::tuning).number();
  if (tuning.has_value() && *tuning >= 1 && *tuning <= 1000)
    config.synth().tuning = Hertz(*tuning);
  else
    return false;

  auto instrument = root.get(keys::instrument);
  if (instrument.is_null())
    config.synth().instrument = {};
  else if (instrument.is_number())
    config.synth().instrument = *instrument.number();
  else
    return false;

  auto channels = root.get(keys::channels);
  if (channels.is_arr()) {
    int idx = 0;
    for (const auto &chobj : channels.arr()) {
      if (idx >= config.channels_size() || !chobj.is_obj())
        return false;

      auto &ch = config.channel(idx);

      auto notes = chobj.get(keys::notes).number();
      if (notes.has_value())
        ch.notes = *notes;
      else
        return false;

      if (!parse_duration16(chobj, keys::max_on_time, ch.max_on_time))
        return false;
      if (!parse_duration16(chobj, keys::min_deadtime, ch.min_deadtime))
        return false;
      if (!parse_duration16(chobj, keys::duty_window, ch.duty_window))
        return false;

      auto max_duty = chobj.get(keys::max_duty).number_d();
      if (max_duty.has_value() && *max_duty <= 100)
        ch.max_duty = midisynth::DutyCycle(*max_duty);
      else
        return false;

      idx++;
    }
  }

  return true;
}

JSONEncoder encode(const AppConfig &config) {
  JSONEncoder encoder;
  auto root = encoder.object();
  root.add(keys::tuning, config.synth().tuning);
  if (config.synth().instrument.has_value())
    root.add(keys::instrument, *config.synth().instrument + 1);
  else
    root.add_null(keys::instrument);

  auto channels = root.add_array(keys::channels);

  for (const auto &ch : config.channels()) {
    auto obj = channels.add_object();
    obj.add(keys::notes, ch.notes);
    obj.add(keys::max_on_time, ch.max_on_time.micros());
    obj.add(keys::min_deadtime, ch.min_deadtime.micros());
    obj.add(keys::max_duty, ch.max_duty.percent());
    obj.add(keys::duty_window, ch.duty_window.micros());
    if (ch.instrument.has_value())
      obj.add(keys::instrument, *ch.instrument + 1);
    else
      obj.add_null(keys::instrument);
  }
  return encoder;
}
} // namespace teslasynth::app::configuration::codec

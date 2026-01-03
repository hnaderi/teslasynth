#include "codec.hpp"
#include "config_data.hpp"
#include "configuration/hardware.hpp"
#include "configuration/synth.hpp"
#include "core/duration.hpp"
#include "core/hertz.hpp"
#include "helpers/json.hpp"
#include "result.hpp"
#include "soc/gpio_num.h"
#include <cstdint>
#include <limits>
#include <optional>
#include <sys/types.h>

namespace teslasynth::app::configuration::codec {
using namespace helpers;
using namespace teslasynth::helpers;
using namespace core;
using namespace hardware;

namespace {
template <typename T = uint16_t>
Decoder<SimpleDuration<T>> parse_duration(const JSONParser::JSONObjectView &j) {
  auto nn = j.number();
  if (nn.has_value() && *nn <= std::numeric_limits<T>::max()) {
    return SimpleDuration<T>::micros(*nn);
  } else
    return "Invalid duration";
}

Decoder<Hertz> parse_frequency(const JSONParser::JSONObjectView &j) {
  auto freq = j.number_d();
  if (freq.has_value() && *freq >= 1 && *freq <= 1000) {
    return Hertz(*freq);
  } else
    return "Invalid frequency";
}

Decoder<uint8_t> parse_notes(const JSONParser::JSONObjectView &j) {
  auto notes = j.number();
  if (notes.has_value() && *notes >= 1 && *notes <= OutputConfig::size) {
    return *notes;
  } else
    return "Invalid notes";
}

Decoder<DutyCycle> parse_duty(const JSONParser::JSONObjectView &j) {
  auto duty = j.number_d();
  if (duty.has_value() && *duty > 0 && *duty <= 100) {
    return DutyCycle(*duty);
  } else
    return "Invalid dutycycle";
}

Decoder<JSONParser::JSONArrayView>
parse_array(const JSONParser::JSONObjectView &j) {
  if (j.is_arr())
    return j.arr();
  else
    return "Not an array";
}

Decoder<std::optional<uint8_t>>
parse_instrument(const JSONParser::JSONObjectView &j) {
  if (j.is_null())
    return std::optional<uint8_t>();
  else if (j.is_number())
    return std::optional{static_cast<uint8_t>(*j.number())};
  else
    return "Invalid instrument number";
}
} // namespace

Decoder<AppConfig> parse_appconfig(helpers::JSONParser &parser) {
  auto root = parser.root();
  AppConfig config;

  config.synth().tuning = TRY(parse_frequency(root.get(keys::tuning)));
  config.synth().instrument = TRY(parse_instrument(root.get(keys::instrument)));

  auto channels = TRY(parse_array(root.get(keys::channels)));
  int idx = 0;
  for (const auto &chobj : channels) {
    if (idx >= config.channels_size() || !chobj.is_obj())
      return "Invalid channel object";

    auto &ch = config.channel(idx);

    ch.notes = TRY(parse_notes(chobj.get(keys::notes)));
    ch.instrument = TRY(parse_instrument(chobj.get(keys::instrument)));
    ch.max_on_time = TRY(parse_duration(chobj.get(keys::max_on_time)));
    ch.min_deadtime = TRY(parse_duration(chobj.get(keys::min_deadtime)));
    ch.duty_window = TRY(parse_duration(chobj.get(keys::duty_window)));
    ch.max_duty = TRY(parse_duty(chobj.get(keys::max_duty)));

    idx++;
  }
  if (idx != config.channels_size())
    return "Channel objects must match the channel size exactly";

  auto routing = root.get(keys::routing);
  if (routing.is_obj()) {
    auto mapping = routing.get(keys::mapping);
    auto percussion = routing.get(keys::percussion);

    if (percussion.is_bool() && mapping.is_arr()) {
      config.routing().percussion = *percussion.boolean();
      auto &array = config.routing().mapping;
      uint8_t i = 0;
      for (const auto &m : mapping.arr()) {
        if (i >= array.size() || !m.is_number()) {
          return "Unknown MIDI channel mapping";
        }
        array[i] = *m.number();
        i++;
      }
    } else
      return "Invalid routing config";
  } else {
    return "No routing object found";
  }

  return config;
}

JSONEncoder encode(const AppConfig &config) {
  JSONEncoder encoder;
  auto root = encoder.object();
  root.add(keys::tuning, config.synth().tuning);
  if (config.synth().instrument.has_value())
    root.add(keys::instrument, *config.synth().instrument);
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
      obj.add(keys::instrument, *ch.instrument);
    else
      obj.add_null(keys::instrument);
  }

  auto routing = root.add_object(keys::routing);
  routing.add_bool(keys::percussion, config.routing().percussion);
  auto routing_array = routing.add_array(keys::mapping);
  for (const auto &m : config.routing().mapping) {
    auto o = m.value();
    routing_array.add(o.has_value() ? static_cast<int>(*o) : -1);
  }

  return encoder;
}

namespace {
void encode_display_minimal(JSONObjBuilder root,
                            const MinimalDisplayPanelConfig &display) {
  root.add("sda", display.sda);
  root.add("scl", display.scl);
  root.add("rs", display.rs);
}
void encode_display_full(JSONObjBuilder root,
                         const FullDisplayPanelConfig &display) {}
void encode_display(JSONObjBuilder root, const DisplayConfig &display) {
  switch (display.type) {
  case hardware::DisplayType::none:
    root.add_object("none");
    break;
  case hardware::DisplayType::minimal:
    encode_display_minimal(root.add_object("minimal"), display.config.minimal);
    break;
  case hardware::DisplayType::full:
    encode_display_full(root.add_object("full"), display.config.full);
    break;
  }
}
void encode_hardware_output(JSONObjBuilder root, const OutputConfig &output) {
  auto channels = root.add_array("channels");
  for (const auto &ch : output.channels) {
    channels.add(ch.pin);
  }
}
void encode_hardware_input(JSONObjBuilder root, const InputConfig &input) {
  root.add("pin", input.maintenance);
}
void encode_hardware_led(JSONObjBuilder root, const LEDConfig &led) {
  root.add("pin", led.pin);
  root.add_bool("active-high", static_cast<bool>(led.logic));
}

#define decode(exp)                                                            \
  if (!decoders::exp)                                                          \
    return false;

namespace decoders {

Decoder<gpio_num_t> gpio(const JSONParser::JSONObjectView &j) {
  auto nn = j.number();
  if (nn.has_value() && *nn < gpio_num_t::GPIO_NUM_MAX &&
      *nn >= gpio_num_t::GPIO_NUM_NC) {
    return static_cast<gpio_num_t>(*nn);
  } else
    return "";
}

Decoder<LogicType> logic_type(const JSONParser::JSONObjectView &j) {
  auto nn = j.boolean();
  if (nn.has_value()) {
    if (*nn)
      return LogicType::active_high;
    else
      return LogicType::active_low;
  } else
    return "Logic type is required!";
}

Decoder<MinimalDisplayPanelConfig>
display_minimal(JSONParser::JSONObjectView obj) {
  MinimalDisplayPanelConfig display;

  display.type = hardware::MinimalDisplayPanelConfig::DisplayType::SSD1306;
  display.sda = TRY(gpio(obj.get("sda")));
  display.scl = TRY(gpio(obj.get("scl")));
  display.rs = TRY(gpio(obj.get("rs")));

  return display;
}

Decoder<FullDisplayPanelConfig> display_full(JSONParser::JSONObjectView obj) {
  return "Not implemented yet!";
}

Decoder<DisplayConfig> display(JSONParser::JSONObjectView obj) {
  auto none = obj.get("none");
  auto minimal = obj.get("minimal");
  auto full = obj.get("full");

  uint8_t count = none.is_obj() + minimal.is_obj() + full.is_obj();
  if (count != 1)
    return "Must exactly be one of 'none', 'minimal', 'full'";

  DisplayConfig display;

  if (minimal.is_obj()) {
    display.type = hardware::DisplayType::minimal;
    display.config.minimal = TRY(display_minimal(minimal));
  } else if (full.is_obj()) {
    display.type = hardware::DisplayType::full;
    display.config.full = TRY(display_full(full));
  } else {
    display.type = hardware::DisplayType::none;
    display.config.none = hardware::NoDisplay();
  }

  return display;
}

Decoder<OutputConfig> hardware_output(JSONParser::JSONObjectView obj) {
  OutputConfig output;

  auto channels = obj.get("channels");
  if (!channels.is_arr())
    return "Invalid channels!";

  uint8_t count = 0;
  for (const auto &i : channels.arr()) {
    if (count > output.size)
      return "Cannot be more than max outputs!";
    output.channels[count].pin = TRY(gpio(i));
    count++;
  }
  if (count != output.size)
    return "Must specify all the channels!";

  return output;
}

Decoder<hardware::InputConfig> hardware_input(JSONParser::JSONObjectView obj) {
  return gpio(obj.get("pin")).map([](gpio_num_t p) {
    return hardware::InputConfig{p};
  });
}
Decoder<hardware::LEDConfig> hardware_led(JSONParser::JSONObjectView obj) {
  LEDConfig led;
  led.pin = TRY(gpio(obj.get("pin")));
  led.logic = TRY(logic_type(obj.get("active-high")));
  return led;
}

} // namespace decoders
} // namespace

Decoder<HardwareConfig> parse_hwconfig(JSONParser &parser) {
  auto root = parser.root();

  HardwareConfig hwconf;
  hwconf.input = TRY(decoders::hardware_input(root.get("input")));
  hwconf.display = TRY(decoders::display(root.get("display")));
  hwconf.output = TRY(decoders::hardware_output(root.get("output")));
  hwconf.led = TRY(decoders::hardware_led(root.get("led")));

  return hwconf;
}
JSONEncoder encode(const HardwareConfig &config) {
  JSONEncoder encoder;
  auto root = encoder.object();
  encode_display(root.add_object("display"), config.display);
  encode_hardware_output(root.add_object("output"), config.output);
  encode_hardware_input(root.add_object("input"), config.input);
  encode_hardware_led(root.add_object("led"), config.led);

  return encoder;
}
} // namespace teslasynth::app::configuration::codec

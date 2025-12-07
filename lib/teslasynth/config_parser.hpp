#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <midi_synth.hpp>
#include <optional>
#include <string_view>

namespace teslasynth::midisynth {
namespace config::parser {

struct Parsed {
  std::string_view key;
  uint8_t channel;
  std::string_view value;
};

template <typename T>
std::optional<T> parse_number(std::string_view s) {
  T value;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

  if (ec == std::errc())
    return value;
  else
    return std::nullopt;
}
std::optional<Duration16> parse_duration16(std::string_view s);
std::optional<Hertz> parse_hertz(std::string_view s);
std::optional<Parsed> parse_config_value(std::string_view s);

} // namespace config::parser
} // namespace teslasynth::midisynth

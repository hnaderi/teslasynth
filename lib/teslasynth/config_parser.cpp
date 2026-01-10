#include "config_parser.hpp"
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace teslasynth::midisynth::config::parser {

std::optional<Parsed> parse_config_value(std::string_view s) {
  const size_t pos_equal = s.find('=');
  if (pos_equal == std::string_view::npos)
    return std::nullopt;
  const size_t pos_colon = s.find(':');

  std::string_view key, value;
  uint8_t ch = 0;

  if (pos_colon == std::string_view::npos || pos_colon > pos_equal) {
    key = s.substr(0, pos_equal);
    value = s.substr(pos_equal + 1);
  } else {
    key = s.substr(0, pos_colon);
    value = s.substr(pos_equal + 1);

    auto chStr = s.substr(pos_colon + 1, pos_equal - pos_colon - 1);
    if (auto read = parse_number<uint8_t>(chStr)) {
      ch = *read;
    } else {
      return std::nullopt;
    }
  }
  if (key.empty() || value.empty())
    return std::nullopt;
  return Parsed{key, ch, value};
}

std::optional<Duration16> parse_duration16(std::string_view s) {
  size_t idx = 0;
  while (idx < s.size() && (s[idx] >= '0' && s[idx] <= '9'))
    ++idx;

  auto num = parse_number<uint16_t>(s.substr(0, idx));
  if (!num)
    return std::nullopt;

  std::string_view unit = s.substr(idx);

  if (unit.empty() || unit == "us") {
    return Duration16::micros(num.value());
  } else if (unit == "ms") {
    uint32_t v = uint32_t(num.value()) * 1000;
    if (v > UINT16_MAX)
      return std::nullopt;
    return Duration16::millis(num.value());
  }
  return std::nullopt;
}
std::optional<Hertz> parse_hertz(std::string_view s) {
  size_t idx = 0;
  while (idx < s.size() && (s[idx] >= '0' && s[idx] <= '9'))
    ++idx;

  auto num = parse_number<float>(s.substr(0, idx));
  if (!num)
    return std::nullopt;

  std::string_view unit = s.substr(idx);

  if (unit.empty() || unit == "hz") {
    return Hertz(num.value());
  } else if (unit == "khz") {
    uint32_t v = uint32_t(num.value()) * 1000;
    if (v > UINT32_MAX)
      return std::nullopt;
    return Hertz::kilohertz(num.value());
  }

  return std::nullopt;
}

std::vector<std::string_view> split(std::string_view path, char separator) {
  std::vector<std::string_view> out;
  size_t start = 0;

  while (true) {
    size_t dot = path.find(separator, start);
    if (dot == std::string_view::npos) {
      out.push_back(path.substr(start));
      return out;
    }
    out.push_back(path.substr(start, dot - start));
    start = dot + 1;
  }
}
} // namespace teslasynth::midisynth::config::parser

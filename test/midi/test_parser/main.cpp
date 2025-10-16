#include "midi_core.hpp"
#include "midi_parser.hpp"
#include <cstdint>
#include <unity.h>
#include <vector>

using Messages = std::vector<MidiChannelMessage>;

template <typename T> inline std::string __msg_for(T a, T b) {
  return std::string("Obtained: " + std::string(a) +
                     " Expected: " + std::string(b));
}

inline void __assert_midi_message_equal(MidiChannelMessage a,
                                        MidiChannelMessage b, int line) {
  UNITY_TEST_ASSERT(a == b, line, __msg_for(a, b).c_str());
}
inline void __assert_midi_message_not_equal(MidiChannelMessage a,
                                            MidiChannelMessage b, int line) {
  UNITY_TEST_ASSERT(a != b, line, __msg_for(a, b).c_str());
}

#define assert_midi_message_equal(a, b)                                        \
  __assert_midi_message_equal(a, b, __LINE__);

void parser_sanity(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  TEST_ASSERT_FALSE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());
}

void parser_feed_note_on(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::NoteOn, 0);
  const uint8_t data[] = {status, 69, 127};
  parser.feed(data, 3);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::noteOn(0, 69, 127));
}

void parser_feed_note_off(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::NoteOff, 5);
  const uint8_t data[] = {status, 80, 100};
  parser.feed(data, 3);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::noteOff(5, 80, 100));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(parser_sanity);
  RUN_TEST(parser_feed_note_on);
  RUN_TEST(parser_feed_note_off);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }

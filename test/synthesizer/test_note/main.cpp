#include "note.hpp"
#include <unity.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_should_not_be_active(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_should_not_be_active);
  UNITY_END();
}

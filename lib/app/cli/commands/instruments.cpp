#include "instruments.hpp"
#include "esp_console.h"
#include "freertos/task.h"
#include "helpers/maintenance.hpp"
#include "helpers/sysinfo.h"
#include <cstddef>
#include <stdio.h>
#include <string.h>

namespace teslasynth::app::cli {

static int print_instruments(int argc, char **argv) {
  printf("id\tname\n==============================\n");
  for (size_t i = 0; i < synth::instruments_size; i++) {
    printf("%d\t%s\n", i + 1, synth::instrument_names[i]);
  }
  return 0;
}

void register_instruments(void) {
  const esp_console_cmd_t cmd = {
      .command = "instruments",
      .help = "List instruments",
      .hint = NULL,
      .func = &print_instruments,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
} // namespace teslasynth::app::cli

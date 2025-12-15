#include "application.hpp"
#include "esp_console.h"
#include <stdio.h>
#include <string.h>

namespace teslasynth::app::cli {
extern void register_configuration_commands(UIHandle handle);
extern void register_system_common(void);

void init(UIHandle handle) {
  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = "teslasynth>";
  repl_config.max_cmdline_length = 1024;
  repl_config.history_save_path = "/history.txt";

  /* Register commands */
  esp_console_register_help_command();
  register_system_common();
  register_configuration_commands(handle);

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) ||                                \
    defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  esp_console_dev_uart_config_t hw_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
  esp_console_dev_usb_cdc_config_t hw_config =
      ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(
      esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  esp_console_dev_usb_serial_jtag_config_t hw_config =
      ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(
      esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
#else
#error Unsupported console type
#endif

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

} // namespace teslasynth::app::cli

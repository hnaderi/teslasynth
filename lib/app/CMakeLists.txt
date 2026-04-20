idf_component_register(
  SRCS
    "assets/dashboard.c"
    "cli/repl.cpp"
    "cli/commands/configurations.cpp"
    "cli/commands/instruments.cpp"
    "cli/commands/system.cpp"
    "configuration/codec.cpp"
    "configuration/factory_settings.cpp"
    "configuration/hardware.cpp"
    "configuration/synth.cpp"
    "devices/midi.cpp"
    "devices/signal_led.cpp"
    "devices/wifi.cpp"
    "devices/midi/ble.cpp"
    "devices/midi/usb.cpp"
    "events.cpp"
    "helpers/json.cpp"
    "helpers/maintenance.cpp"
    "helpers/sysinfo.cpp"
    "output/rmt_driver.cpp"
    "storage.cpp"
    "synthesizer.cpp"
    "web/server.cpp"
  INCLUDE_DIRS "."
  REQUIRES
    teslasynth
    midi
    synthesizer
    bt
    console
    esp_driver_gpio
    esp_driver_rmt
    esp_event
    esp_http_server
    esp_netif
    esp_timer
    esp_wifi
    freertos
    log
    lwip
    nvs_flash
    app_update
    spi_flash
)

# Designated initializers that omit them are correct C++20 (zero-init), but GCC
# warns anyway. Suppress rather than sprinkling nullptrs across every command.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>

__attribute__((weak)) void midi_rx_callback(const uint8_t *data, uint16_t len);
void ble_midi_receiver_init(void);

#ifdef __cplusplus
    }
#endif

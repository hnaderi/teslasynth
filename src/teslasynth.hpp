#pragma once

#include "freertos/FreeRTOS.h"

void init_storage();
StreamBufferHandle_t init_ble_midi();
void init_synth(StreamBufferHandle_t sbuf);
void init_gui(void);
void init_cli(void);

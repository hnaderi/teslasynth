#pragma once

#include "notes.hpp"

const Config &load_config();
const Config &get_config();
void update_config(const Config &config);
void reset_config();
void save_config();

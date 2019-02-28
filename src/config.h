#pragma once
#include <stddef.h>
#include <stdint.h>

#include "events.h"

//resolves ~/.config/joymon/config
//result if not null needs freeing
const char *get_config_path();

struct action_map;
//error result negative
int generate_map(const char *config, struct action_map **map, size_t *total_actions);

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "events.h"

//resolves ~/.config/joymon/config
//result if not null needs freeing
const char *get_config_path();

struct action_map;

#define C_JOYSTICK "joystick"
#define C_BUTTON "button"
#define C_AXIS "axis"
#define C_EXEC "exec"
#define C_DOWN "down"
#define C_UP "up"

//error result negative
int generate_map(const char *config, struct action_map **map, size_t *total_actions);

//2MiB
#define READALL_CHUNK 2097152
//kindly adapted from Nominal Animal
//https://stackoverflow.com/a/44894946
int readall(FILE *in, char **dataptr, size_t *sizeptr);

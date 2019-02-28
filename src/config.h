#pragma once
#include <stdint.h>


struct action_map{
    uint8_t button_count;
    char **button_down;
    char **button_up;
};

void init_action_map(struct action_map * map, int button_count);
void close_action_map(struct action_map * map);

//resolves ~/.config/joymon/config
//result if not null needs freeing
char * get_config_path();

#pragma once
#include <linux/joystick.h>
#include <stdint.h>

struct action_map {
    uint8_t button_count;
    uint8_t axis_count;
    char *name;
    int fd;
    char **button_down;
    char **button_up;
};

void init_action_map(struct action_map *map);
void close_action_map(struct action_map *map);

//mode - [d]aemon [r]aw
void listen_to_joystick(struct action_map *map, char mode);

//opens js fd or -1 on error
int open_joystick(const char *name);

void handle_event(struct js_event *event);

struct axis_state {
    int16_t x, y;
};
//returns axis, sets axes current values
uint8_t get_axis_state(struct js_event *event, struct axis_state axes[3]);
uint8_t get_button_count(int fd);
uint8_t get_axis_count(int fd);

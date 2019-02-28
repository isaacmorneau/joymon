#pragma once
#include <linux/joystick.h>
#include <stdint.h>

//mode - [d]aemon [r]aw
void listen_to_joystick(const char *path, char mode);

void handle_event(struct js_event *event);

struct axis_state {
    int16_t x, y;
};
//returns axis, sets axes current values
uint8_t get_axis_state(struct js_event *event, struct axis_state axes[3]);
uint8_t get_button_count(int fd);
uint8_t get_axis_count(int fd);

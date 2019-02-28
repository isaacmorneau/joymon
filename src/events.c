#include <fcntl.h>
#include <limits.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "events.h"

uint8_t axis_count;
uint8_t button_count;

struct axis_state axes[3] = {{0}};

void init_action_map(struct action_map *map) {
    if (!(map->button_down = calloc(sizeof(char *), map->button_count))
        || !(map->button_up = calloc(sizeof(char *), map->button_count))) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
}

void close_action_map(struct action_map *map) {
    //name isnt allocated in here
    //dont free it
    free(map->button_up);
    map->button_up = NULL;
    free(map->button_down);
    map->button_down = NULL;
}

uint8_t get_axis_count(int fd) {
    uint8_t axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1) {
        perror("ioctl()");
        return 0;
    }

    return axes;
}

uint8_t get_button_count(int fd) {
    uint8_t buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1) {
        perror("ioctl()");
        return 0;
    }

    return buttons;
}

//https://www.kernel.org/doc/Documentation/input/joystick-api.txt
//as per js_event.number its safe to assume x is even and y is odd
uint8_t get_axis_state(struct js_event *event, struct axis_state axes[3]) {
    //get axis, number 0 and 1 both are axis 0
    uint8_t axis = event->number / 2;

    if (axis < 3) {
        if (event->number % 2) {
            axes[axis].y = event->value;
        } else {
            axes[axis].x = event->value;
        }
    }

    return axis;
}

int open_joystick(const char *name) {
    int fd;

    if ((fd = open(name, O_RDONLY)) == -1) {
        perror("open()");
        return -1;
    }
    return fd;
}

void listen_to_joystick(struct action_map *restrict map, char mode) {
    int ret;
    if ((ret = fork()) < 0) { //>0 its good <0 its bad but we cant do anything about it
        perror("fork()");
        return;
    } else if (ret > 0) {
        //parent success
        return;
    }
    //child

    if (mode == 'd') {
        umask(0);

        fclose(stdin);
        fclose(stdout);
        fclose(stderr);

        if (setsid() == -1) {
            perror("setsid()");
            return;
        }

        if (chdir("/") == -1) {
            perror("chdir()");
            return;
        }
    }

    axis_count   = get_axis_count(map->fd);
    button_count = get_button_count(map->fd);

    if (mode == 'r') {
        printf("axis count: %u\nbutton count: %u\n", axis_count, button_count);
    }

    struct js_event events[64];

    while (1) {
        ssize_t nread;
        nread = read(map->fd, events, sizeof(struct js_event) * 64);
        if (nread == -1) {
            perror("read()");
            return;
        }

        for (size_t i = 0; i < nread / sizeof(struct js_event); ++i) {
            handle_event(events + i, map);
        }
    }
}

void handle_event(struct js_event *event, struct action_map *restrict map) {
    uint8_t axis;
    switch (event->type) {
        case JS_EVENT_BUTTON:
            if (event->value) {
                if (map->button_down[event->number]) {
                    printf("mapped b%ud %s\n", event->number, map->button_down[event->number]);
                } else {
                    printf("unmapped b%ud\n", event->number);
                }
            } else {
                if (map->button_up[event->number]) {
                    printf("mapped b%uu %s\n", event->number, map->button_up[event->number]);
                } else {
                    printf("unmapped b%uu\n", event->number);
                }
            }
            break;
        case JS_EVENT_AXIS:
            axis = get_axis_state(event, axes);
            if (axis < 3) {
                printf("axis %u ", axis);

                if (!axes[axis].x && !axes[axis].y) {
                    printf("centered\n");
                } else {
                    if (axes[axis].x == SHRT_MAX) {
                        printf("(max, ");
                    } else if (axes[axis].x == -SHRT_MAX) {
                        printf("(min, ");
                    } else {
                        printf("(%d, ", axes[axis].x);
                    }
                    if (axes[axis].y == SHRT_MAX) {
                        printf("max)\n");
                    } else if (axes[axis].y == -SHRT_MAX) {
                        printf("min)\n");
                    } else {
                        printf("%d)\n", axes[axis].y);
                    }
                }
            }
            break;
    }
}

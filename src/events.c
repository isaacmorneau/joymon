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
        || !(map->button_up = calloc(sizeof(char *), map->button_count))
        || !(map->axis_x_neg = calloc(sizeof(char *), map->axis_count))
        || !(map->axis_x_pos = calloc(sizeof(char *), map->axis_count))
        || !(map->axis_y_neg = calloc(sizeof(char *), map->axis_count))
        || !(map->axis_y_pos = calloc(sizeof(char *), map->axis_count))
        || !(map->axis_x_neg_tol = calloc(sizeof(int16_t *), map->axis_count))
        || !(map->axis_x_pos_tol = calloc(sizeof(int16_t *), map->axis_count))
        || !(map->axis_y_neg_tol = calloc(sizeof(int16_t *), map->axis_count))
        || !(map->axis_y_pos_tol = calloc(sizeof(int16_t *), map->axis_count))) {
        perror("calloc()");
        exit(EXIT_FAILURE);
    }
}

void close_action_map(struct action_map *map) {
    if (map->name) {
        free(map->name);
    }
    for (size_t i = 0; i < map->button_count; ++i) {
        if (map->button_up && map->button_up[i])
            free(map->button_up[i]);
        if (map->button_down && map->button_down[i])
            free(map->button_down[i]);
    }

    if (map->button_up) {
        free(map->button_up);
        map->button_up = NULL;
    }

    if (map->button_down) {
        free(map->button_down);
        map->button_down = NULL;
    }

    for (size_t i = 0; i < map->axis_count; ++i) {
        if (map->axis_x_neg && map->axis_x_neg[i])
            free(map->axis_x_neg[i]);
        if (map->axis_x_pos && map->axis_x_pos[i])
            free(map->axis_x_pos[i]);
        if (map->axis_y_neg && map->axis_y_neg[i])
            free(map->axis_y_neg[i]);
        if (map->axis_y_pos && map->axis_y_pos[i])
            free(map->axis_y_pos[i]);

        if (map->axis_x_neg_tol && map->axis_x_neg_tol[i])
            free(map->axis_x_neg_tol[i]);
        if (map->axis_x_pos_tol && map->axis_x_pos_tol[i])
            free(map->axis_x_pos_tol[i]);
        if (map->axis_y_neg_tol && map->axis_y_neg_tol[i])
            free(map->axis_y_neg_tol[i]);
        if (map->axis_y_pos_tol && map->axis_y_pos_tol[i])
            free(map->axis_y_pos_tol[i]);
    }

    if (map->axis_x_neg)
        free(map->axis_x_neg);
    if (map->axis_x_pos)
        free(map->axis_x_pos);
    if (map->axis_y_neg)
        free(map->axis_y_neg);
    if (map->axis_y_pos)
        free(map->axis_y_pos);

    if (map->axis_x_neg_tol)
        free(map->axis_x_neg_tol);
    if (map->axis_x_pos_tol)
        free(map->axis_x_pos_tol);
    if (map->axis_y_neg_tol)
        free(map->axis_y_neg_tol);
    if (map->axis_y_pos_tol)
        free(map->axis_y_pos_tol);
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
        printf("joystock: %s\naxis count: %u\nbutton count: %u\n", map->name, axis_count,
            button_count);
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
            handle_event(events + i, map, mode);
        }
    }
}

void handle_event(struct js_event *event, struct action_map *restrict map, char mode) {
    uint8_t axis;
    switch (event->type) {
        case JS_EVENT_BUTTON:
            if (event->value) {
                if (mode == 'r') {
                    printf("b%ud\n", event->number);
                } else if (map->button_down[event->number]) {
                    system(map->button_down[event->number]);
                }
            } else {
                if (mode == 'r') {
                    printf("b%uu\n", event->number);
                } else if (map->button_up[event->number]) {
                    system(map->button_up[event->number]);
                }
            }
            break;
        case JS_EVENT_AXIS:
            axis = get_axis_state(event, axes);
            if (axis < 3) {
                if (mode == 'r') {
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
                } else {
                    if (axes[axis].x < 0 && map->axis_x_neg_tol[axis] && axes[axis].x < *map->axis_x_neg_tol[axis]) {
                        system(map->axis_x_neg[axis]);
                    } else if (axes[axis].x > 0 && map->axis_x_pos_tol[axis] && axes[axis].x > *map->axis_x_pos_tol[axis]) {
                        system(map->axis_x_pos[axis]);
                    }

                    if (axes[axis].y < 0 && map->axis_y_neg_tol[axis] && axes[axis].y < *map->axis_y_neg_tol[axis]) {
                        system(map->axis_y_neg[axis]);
                    } else if (axes[axis].y > 0 && map->axis_y_pos_tol[axis] && axes[axis].y > *map->axis_y_pos_tol[axis]) {
                        system(map->axis_y_pos[axis]);
                    }
                }
            }
            break;
    }
}

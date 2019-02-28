#include <fcntl.h>
#include <linux/joystick.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "events.h"

uint8_t axis;
uint8_t axis_count;
uint8_t button_count;

struct axis_state axes[3] = {0};


uint8_t get_axis_count(int fd) {
    uint8_t axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

uint8_t get_button_count(int fd) {
    uint8_t buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}


/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
uint8_t get_axis_state(struct js_event *event, struct axis_state axes[3]) {
    uint8_t axis = event->number / 2;

    if (axis < 3) {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}

void listen_to_joystick(const char *path, char mode) {
    if (mode == 'd') {
        int ret;
        if ((ret = fork()) < 0) { //>0 its good <0 its bad but we cant do anything about it
            perror("fork()");
            return;
        } else if (ret < 0) {
            //parent success
            return;
        }
        //child

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

    int jsfd;

    if ((jsfd = open(path, O_RDONLY)) == -1) {
        perror("open()");
        return;
    }


    axis_count = get_axis_count(jsfd);
    button_count = get_button_count(jsfd);

    if (mode == 'r') {
        printf("axis count: %u\nbutton count: %u\n", axis_count, button_count);
    }


    struct js_event events[64];

    while (1) {
        ssize_t nread;
        nread = read(jsfd, events, sizeof(struct js_event) * 64);
        if (nread == -1) {
            perror("read()");
            return;
        }

        for (size_t i = 0; i < nread / sizeof(struct js_event); ++i) {
            handle_event(events + i);
        }
    }
}

void handle_event(struct js_event *event) {
    switch (event->type) {
        case JS_EVENT_BUTTON:
            printf("Button %u %s\n", event->number, event->value ? "pressed" : "released");
            break;
        case JS_EVENT_AXIS:
            axis = get_axis_state(&event, axes);
            if (axis < 3)
                printf("Axis %u at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
            break;
    }
}


#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "config.h"
#include "events.h"

const char *config = NULL;
bool config_needs_free  = false;

static void print_help(void) {
    puts(
        "joymon - a simple daemon to respond to joystick events\n"
        "-[-i]nterface - the /dev/input/js# device to read from. can be specificed multiple times\n"
        "-[-d]aemonize - run in the background\n"
        "-[-r]aw - dump all event information to the console. useful for finding keycodes\n"
        "-[-c]onfig - defaults to XDG_CONFIG_HOME/joymon/config\n"
        "-[-h]elp - this message");
}

int main(int argc, char **argv) {
    char mode = 'r';
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    int c;

    while (1) {
        int option_index = 0;

        static struct option long_options[]
            = {{"config", required_argument, 0, 'c'}, {"daemon", no_argument, 0, 'd'},
                {"raw", no_argument, 0, 'r'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "c:drh", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'd':
                mode = 'd';
                break;
            case 'r':
                mode = 'r';
                break;
            case 'c':
                config = optarg;
                break;
            case 'h':
            case '?':
                print_help();
                break;
        }
    }

    //while inform people its not being used
    if (optind < argc) {
        printf("ignoring args: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
    }

    if (!config) {
        if (!(config = get_config_path())) {
            printf("failed to find config path ensure either XDG_CONFIG_HOME or HOME is set.\n");
            exit(EXIT_FAILURE);
        }
        config_needs_free = true;
    }

    struct action_map *map = NULL;
    size_t total_actions   = 0;
    if (generate_map(config, &map, &total_actions)) {
        printf("failed to generate action map, check config\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < total_actions; ++i) {
        listen_to_joystick(map + i, mode);
    }

    if (mode == 'r') {
        wait(0);
    }

    if (config_needs_free) {
        free((void *)config);
    }

    exit(EXIT_SUCCESS);
}

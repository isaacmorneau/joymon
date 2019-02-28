#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t total_interfaces = 0;
char **interfaces       = NULL;

static void print_help(void) {
    puts(
        "joymon - a simple daemon to respond to joystick events\n"
        "-[-i]nterface - the /dev/input/js# device to read from. can be specificed multiple times\n"
        "-[-c]onfig - defaults to XDG_CONFIG_HOME/joymon/config\n"
        "-[-h]elp - this message");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    int c;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {{"config", required_argument, 0, 'c'},
            {"interface", required_argument, 0, 'i'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "i:c:h", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'c':
                printf("config not yet implemented\n");
                exit(EXIT_FAILURE);
                break;
            case 'i':
                ++total_interfaces;

                if (interfaces == NULL && !(interfaces = malloc(sizeof(char *)))) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                } else {
                    if (!(interfaces = realloc(interfaces, sizeof(char *) * total_interfaces))) {
                        perror("malloc");
                        exit(EXIT_FAILURE);
                    }
                }

                interfaces[total_interfaces - 1] = optarg;

                printf("using interface '%s'\n", optarg);
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

    for (size_t i = 0; i < total_interfaces; ++i) {
        printf("listening to: %s\n", interfaces[i]);
    }

    exit(EXIT_SUCCESS);
}

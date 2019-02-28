#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define CONFIG_EXT "/joymon/config"
#define CONFIG_EXT_UNSET "/.config/joymon/config"
void init_action_map(struct action_map *map, int button_count) {
    map->button_count = button_count;
    if (!(map->button_down = malloc(sizeof(char *) * button_count))
        && !(map->button_up = malloc(sizeof(char *) * button_count))) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
}

void close_action_map(struct action_map *map) {
    free(map->button_up);
    map->button_up = NULL;
    free(map->button_down);
    map->button_down = NULL;
}

const char *get_config_path() {
    char *home;
    char *fullpath;
    size_t len;

    if ((home = getenv("XDG_CONFIG_HOME"))) {
        len = strlen(home) + strlen(CONFIG_EXT) + 1;
        if (!(fullpath = malloc(len))) {
            perror("malloc()");
            return NULL;
        }
        strcpy(fullpath, home);
        strcat(fullpath, CONFIG_EXT);
    } else if ((home = getenv("HOME"))) {
        len = strlen(home) + strlen(CONFIG_EXT_UNSET) + 1;
        if (!(fullpath = malloc(len))) {
            perror("malloc()");
            return NULL;
        }
        strcpy(fullpath, home);
        strcat(fullpath, CONFIG_EXT_UNSET);
    } else {
        return NULL;
    }

    return fullpath;
}

void generate_map(const char *config, struct action_map * map) {

}

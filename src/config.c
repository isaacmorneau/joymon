#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define CONFIG_EXT "/joymon/config"
#define CONFIG_EXT_UNSET "/.config/joymon/config"

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

int generate_map(const char *config, struct action_map **map, size_t *total_actions) {
    *total_actions = 0;
    *map           = NULL;

    FILE *conf = fopen(config, "r");
    if (!conf) {
        perror("fopen()");
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    struct action_map *lm = NULL;

    //scanf vars
    char tmp[4096];
    char type;
    int8_t value;
    char event;
    while ((nread = getline(&line, &len, conf)) != -1) {
        memset(tmp, 0, 4096);
        if (len == 0) {
            continue;
        }

        //dont terminate with newlines
        if (len > 1) {
            line[len - 2] = '\0';
        }
        printf("read line '%s'\n", line);

        if (*line == '>') { //its a label
            //allocate the map
            if (!*total_actions) {
                ++*total_actions;
                if (!(*map = calloc(sizeof(struct action_map), 1))) {
                    perror("calloc()");
                    goto error_cleanup;
                }
            } else if (!(*map = realloc(*map, sizeof(struct action_map) * ++*total_actions))) {
                perror("realloc()");
                goto error_cleanup;
            }
            lm = *map + *total_actions - 1;
            //copy the name
            if (!(lm->name = strdup(line + 1))) {
                perror("strdup()");
                goto error_cleanup;
            }
            //open the joystick
            if ((lm->fd = open_joystick(lm->name)) == -1) {
                printf("unable to open joystick '%s'\n", lm->name);
                goto error_cleanup;
            }
            //get the buttons
            if (!(lm->button_count = get_button_count(lm->fd))) {
                printf("unable to read button count%s\n", lm->name);
                goto error_cleanup;
            }
            //get the axis
            if (!(lm->axis_count = get_axis_count(lm->fd))) {
                printf("unable to read axis count%s\n", lm->name);
                goto error_cleanup;
            }
            //alloc the buffers
            init_action_map(lm);

        } else if (*total_actions
            && sscanf(line, "%c%hhd%c=%s", &type, &value, &event, tmp) == 4) { //its a label
            if (type == 'b') {
                if (value < 0 && value > lm->button_count) {
                    printf("%s value out of range\n", line);
                }
                if (event == 'd') {
                    if (!(lm->button_down[value] = strdup(tmp))) {
                        perror("strdup()");
                        goto error_cleanup;
                    }
                } else if (event == 'u') {
                    if (!(lm->button_up[value] = strdup(tmp))) {
                        perror("strdup()");
                        goto error_cleanup;
                    }
                } else {
                    printf("%s event not recognized\n", line);
                }
            } else if (type == 'a') {
                //TODO implement axis events
                continue;
            } else {
                printf("%s type not recognized\n", line);
            }
        } else {
            printf("%s line unparseable\n", line);
        }
    }

    fclose(conf);
    free(line);
    return 0;

error_cleanup:
    fclose(conf);
    free(line);
    if (*total_actions) {
        for (size_t i = 0; i < *total_actions; ++i) {
            if ((*map)[i].name) {
                free((*map)[i].name);
            }
        }
        free(*map);
    }
    return -1;
}

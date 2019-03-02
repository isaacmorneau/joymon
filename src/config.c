#include <ctype.h>
#include <stdbool.h>
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

    const char *config_buf = NULL;
    size_t config_len      = 0;

    {
        FILE *conf;
        if (!(conf = fopen(config, "r"))) {
            perror("fopen()");
            return -1;
        }

        char *tmp;
        size_t tmplen;
        if (readall(conf, &tmp, &tmplen)) {
            return -1;
        }

        if (fclose(conf)) {
            //how do you fail to close a file?
            perror("fclose");
            free(tmp);
            return -1;
        }

        //dont change anything
        config_buf = tmp;
        config_len = tmplen;
    }

    struct action_map *lm = NULL;
    size_t lm_len         = 0;

    for (size_t i = 0; i < config_len; ++i) {
        if (!strncmp(config_buf + i, C_JOYSTICK, strlen(C_JOYSTICK))) {
            //skip whitespace and token
            for (i += strlen(C_JOYSTICK); i < config_len; ++i)
                if (!isspace(config_buf[i]))
                    break;
            size_t pstart = i;
            //collect path
            for (; i < config_len; ++i)
                if (config_buf[i] == ';' || isspace(config_buf[i]))
                    break;
            size_t pend = i;

            //make sure they didnt screw up the config
            if (i == config_len || config_buf[i] != ';') {
                fputs("unterminated joystick directive\n", stderr);
                goto error_cleanup;
            }

            //new joystick is a new map
            void *tmp;
            if (!lm_len++ && !(lm = malloc(sizeof(struct action_map)))) {
                perror("malloc()");
                goto error_cleanup;
            } else if (!(tmp = realloc(lm, sizeof(struct action_map) * lm_len))) {
                perror("malloc()");
                goto error_cleanup;
            } else {
                lm = tmp;
            }
            //to gracefully free later
            lm[lm_len-1].name = NULL;

            if (!(lm[lm_len - 1].name = strndup(config_buf + pstart, pend - pstart))) {
                perror("strndup()");
                goto error_cleanup;
            } else if (!(lm[lm_len - 1].fd = open_joystick(lm[lm_len - 1].name))) {
                goto error_cleanup;
            } else if (!(lm[lm_len - 1].button_count = get_button_count(lm[lm_len - 1].fd))) {
                goto error_cleanup;
            } else if (!(lm[lm_len - 1].axis_count = get_axis_count(lm[lm_len - 1].fd))) {
                goto error_cleanup;
            }

            printf("joystick: %s\nbuttons: %u axis: %u\n", lm[lm_len - 1].name,
                lm[lm_len - 1].button_count, lm[lm_len - 1].axis_count);
        } else if (!strncmp(config_buf+i, C_BUTTON, strlen(C_BUTTON))) {
            //skip whitespace and token
            for (i += strlen(C_BUTTON); i < config_len; ++i)
                if (!isspace(config_buf[i]))
                    break;

            uint8_t number = 0;
            if (sscanf(config_buf+i, "%hhu", &number) != 1) {
                fputs("unspecified button number\n", stderr);
                goto error_cleanup;
            }

            //go to the next token
            for (i += strlen(C_BUTTON); i < config_len; ++i)
                if (isalpha(config_buf[i]))
                    break;
            if (!strncmp(config_buf+i, C_DOWN, strlen(C_DOWN))) {
                puts("button down");
            } else if (!strncmp(config_buf+i, C_UP, strlen(C_UP))) {
                puts("button up");
            } else {
                fputs("unspecified button event\n", stderr);
                goto error_cleanup;
            }
        }
    }

    *map           = lm;
    *total_actions = lm_len;
    free((void *)config_buf);
    return 0;

error_cleanup:
    if (lm_len) {
        for (size_t i = 0; i < lm_len; ++i)
            if (lm[i].name)
                free(lm[i].name);
        free(lm);
    }
    lm = NULL;
    free((void *)config_buf);
    return -1;
}

int readall(FILE *in, char **dataptr, size_t *sizeptr) {
    char *data  = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;

    if (in == NULL || dataptr == NULL || sizeptr == NULL) {
        return -1;
    }

    //a read error already occurred
    if (ferror(in)) {
        return -1;
    }

    while (1) {
        if (used + READALL_CHUNK + 1 > size) {
            size = used + READALL_CHUNK + 1;
            //overflow check
            if (size <= used) {
                free(data);
                return -1;
            }

            if (!(temp = realloc(data, size))) {
                perror("realloc()");
                free(data);
                return -1;
            }
            data = temp;
        }

        if (!(n = fread(data + used, 1, READALL_CHUNK, in))) {
            break;
        }

        used += n;
    }

    if (ferror(in)) {
        perror("fread()");
        free(data);
        return -1;
    }

    //resize down
    if (!(temp = realloc(data, used + 1))) {
        perror("realloc()");
        free(data);
        return -1;
    }

    data       = temp;
    data[used] = '\0';

    *dataptr = data;
    *sizeptr = used;

    return 0;
}

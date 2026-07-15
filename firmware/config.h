#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool led_on;
    int32_t enc_states_per_click;
} config_t;

config_t * config();

#endif
#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool led_on;
    int32_t enc_states_per_click;
    int32_t tests_pseudo_values;
    bool show_socket_prompt;
    bool show_power_prompt;
} config_t;

config_t * config(bool load);

#endif
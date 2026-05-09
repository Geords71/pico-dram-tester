#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool led_on;
    int32_t enc_states_per_click;
} app_config_t;

extern app_config_t app_config;

extern void load_app_config(bool refresh);

void do_system_config();
uint32_t get_system_overclock();


#endif
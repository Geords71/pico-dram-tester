#ifndef _MENU_H
#define _MENU_H

#include <stdbool.h>
#include "gui.h"

typedef struct menu_t {
    void (*enter)(struct menu_t *parent);
    struct menu_t *(*do_encoder_pushed)();
    struct menu_t *(*do_back_pushed)();
    struct menu_t *(*do_encoder_clockwise)();
    struct menu_t *(*do_encoder_anticlockwise)();
    struct menu_t *(*do_tasks)();
    struct menu_t *parent;
} menu_t;

extern void menu_init();
extern void screen_task();

#endif
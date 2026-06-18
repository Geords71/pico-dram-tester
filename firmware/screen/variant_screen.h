#ifndef _VARIANT_SCREEN_H
#define _VARIANT_SCREEN_H

#include "menu.h"

typedef struct {
    menu_t base;
    gui_listbox_t *listbox;
    void (*_init_listbox)(uint16_t);
} variant_screen_t;

extern variant_screen_t variant_screen;

#endif
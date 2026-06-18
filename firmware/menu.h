#ifndef _MENU_H
#define _MENU_H

#include <stdbool.h>
#include "gui.h"

typedef struct menu_t
{
    gui_listbox_t listbox;
    void (*enter)(struct menu_t *, uint16_t); 
    struct menu_t * (*do_encoder_pushed)(); 
    struct menu_t * (*do_back_pushed)();
    struct menu_t * (*do_encoder_clockwise)();
    struct menu_t * (*do_encoder_anticlockwise)();
    bool _ready;
    struct menu_t *_parent;
    void (*_show)();
} menu_t;

extern void menu_init();
extern void menu_init_new();
extern void menu_main_show();
extern void do_inputs();
extern void do_menu_status();

#endif
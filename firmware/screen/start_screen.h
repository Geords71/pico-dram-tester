#ifndef _SDTART_SCREEN_H

#define _START_SCREEN_H
#include "menu.h"
#include "mem_chip.h"


typedef struct {
    menu_t base;
    gui_listbox_t *listbox;
    void (*_init_listbox)(uint16_t); 

} start_screen_t;

extern start_screen_t start_screen;

#endif

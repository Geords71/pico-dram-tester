#ifndef _TEST_SCREEN_H
#define _TEST_SCREEN_H

#include "menu.h"

typedef struct {
    menu_t base;
    uint16_t chip_index;
} test_screen_t;

extern menu_t *test_screen;

#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gui.h"
#include "logging/logging.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "chip_screen.h"
#include "variant_screen.h"
#include "speed_screen.h"
#include "test_screen.h"

#include "mem_chip/ram4027.h"
#include "mem_chip/ram4108.h"
#include "mem_chip/ram4116.h"
#include "mem_chip/ram4816.h"
#include "mem_chip/ram4132.h"
#include "mem_chip/ram4132stk.h"
#include "mem_chip/ram4164.h"
#include "mem_chip/ram41256.h"
#include "mem_chip/ram4408.h"
#include "mem_chip/ram4416.h"
#include "mem_chip/ram4464.h"
#include "mem_chip/ram44256.h"
#include "mem_chip/ram41128.h"

static mem_chip_t *(*chip_list[NUM_CHIPS])() = {
    ram4027_chip,
    ram4108_chip,
    ram4116_chip,
    ram4132_chip,
    ram4132stk_chip,
    ram4164_chip,
    ram4408_chip,
    ram4416_chip,
    ram4464_chip,
    ram4816_chip,
    ram41128_chip,
    ram41256_chip,
    ram44256_chip,
};

static menu_t self;

static char *listbox_items[NUM_CHIPS];
static gui_listbox_t listbox = {
    .sx = 7,
    .sy = 40,
    .width = 220,
    .tot_lines = NUM_CHIPS,
    .vis_lines = 4,
    .sel_line = 0,
    .start_line = 0,
    .items = listbox_items,
};

static void show() {
    paint_gui_dialog("Select Device");
    paint_gui_listbox(&listbox, LIST_ACTION_NONE);
}

static void init_listbox() {
    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        listbox_items[i] = chip_list[i]()->name;
    }
}

static void enter(menu_t *parent)
{
    if (parent != NULL) {
        self.parent = parent;
        init_listbox();
    }
    show();
}

static menu_t * do_encoder_pushed()
{
    mem_tester->chip = chip_list[listbox.sel_line]();

    menu_t *next_screen;

    next_screen = speed_screen;

    if (mem_tester->shared.please_seek) next_screen = test_screen;

    if (mem_tester->chip->variants.len > 1) {
        next_screen = variant_screen;
    } else {
        // Every type has at least one variant to keep things consistent
        // from a data and logic perspective. So we need to set variant_idx
        // here to cope with skipping the variant selction screen.
        mem_tester->variant_idx = 0;
    };

    next_screen->enter(&self);
    return next_screen;
}

static menu_t * do_back_pushed() {
    self.parent->enter(NULL);
    return self.parent;
}

static menu_t * do_encoder_clockwise() {
    paint_gui_listbox(&listbox, LIST_ACTION_DOWN);
    return &self;
}

static menu_t * do_encoder_anticlockwise() {
    paint_gui_listbox(&listbox, LIST_ACTION_UP);
    return &self;
}

static menu_t * do_return_self() {
    return &self;
}

static menu_t self = {
    .enter = &enter,
    .do_encoder_pushed = &do_encoder_pushed,
    .do_back_pushed = &do_back_pushed,
    .do_encoder_clockwise = &do_encoder_clockwise,
    .do_encoder_anticlockwise = &do_encoder_anticlockwise,
    .do_tasks = &do_return_self,
    .parent = NULL,
};

menu_t *chip_screen = &self;
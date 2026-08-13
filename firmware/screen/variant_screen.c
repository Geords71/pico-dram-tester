#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gui.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"
#include "variant_screen.h"
#include "speed_screen.h"
#include "test_screen.h"

// Singleton self pointer
static menu_t self;

// Persistent backing storage for this screen
static char *variant_items[MEMCHIP_MAX_VARIANTS];
static gui_listbox_t variant_listbox = {
    .sx = 7,
    .sy = 40,
    .width = 220,
    .tot_lines = 0,
    .vis_lines = 4,
    .sel_line = 0,
    .start_line = 0,
    .items = variant_items,
};

static const mem_chip_t *cur_chip = NULL;

static void show() {
    paint_gui_dialog("Select Variant");
    paint_gui_listbox(&variant_listbox, LIST_ACTION_NONE);
}

static void init_listbox()
{
    // If we have already initilised and nothing has changed then we can safely
    // return with no ation.
    if (cur_chip == mem_tester->chip) {
        return;
    }

    cur_chip = mem_tester->chip;

    uint8_t count = cur_chip->variants.len;

    for (uint8_t i = 0; i < count; i++) {
        variant_items[i] = cur_chip->variants.list[i].name;
    }

    variant_listbox.tot_lines = count;
    variant_listbox.sel_line = 0;
}

static void enter(menu_t *parent)
{
    if (parent != NULL) {
        init_listbox();
        self.parent = parent;
    }
    show();
}

static menu_t * do_back_pushed()
{
    self.parent->enter(NULL);
    return self.parent;
}

static menu_t * do_encoder_pushed()
{
    mem_tester->variant_idx = variant_listbox.sel_line;

    menu_t *next_screen;
    if (mem_tester->shared.please_seek) {
        next_screen = test_screen;
    } else {
        next_screen = speed_screen;
    };
    next_screen->enter(&self);
    return next_screen;
}

static menu_t * do_encoder_clockwise() {
    paint_gui_listbox(&variant_listbox, LIST_ACTION_DOWN);
    return &self;
}

static menu_t * do_encoder_anticlockwise() {
    paint_gui_listbox(&variant_listbox, LIST_ACTION_UP);
    return &self;
}

static menu_t * do_return_self() {
    return &self;
}

static menu_t self = {
    .enter = &enter,
    .do_back_pushed = &do_back_pushed,
    .do_encoder_pushed = &do_encoder_pushed,
    .do_encoder_clockwise = &do_encoder_clockwise,
    .do_encoder_anticlockwise = &do_encoder_anticlockwise,
    .do_tasks = &do_return_self,
    .parent = NULL,
};

menu_t * variant_screen = &self;

#include <stdbool.h>
#include <stdint.h>
#include "gui.h"
#include "logging/logging.h"
#include "mem_chip.h"
#include "start_screen.h"
#include "variant_screen.h"


static start_screen_t *self;
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

static void _show() {
    paint_dialog("Select Device");
    gui_listbox(self->listbox, LIST_ACTION_NONE);
}

static void _init_listbox(uint16_t arg1) {
    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        listbox_items[i] = (char *)chip_list[i]->name;
    }
    self->listbox = &listbox;
}

static void enter(menu_t *parent, uint16_t arg1)
{
    self = &start_screen;
    if (!self->base._ready) {
        self->base._parent = parent;
        self->_init_listbox(arg1);
        self->base._ready = true;
    }
    self->base._show();
}

static menu_t * do_encoder_pushed()
{
    variant_screen.base.enter(&self->base, self->listbox->sel_line);
    return &variant_screen.base;
}

static menu_t * do_back_pushed() {
    self->base._parent->enter(NULL, 0);
    return self->base._parent;
}

static menu_t * do_encoder_clockwise() {
    gui_listbox(self->listbox, LIST_ACTION_DOWN);

    return &self->base;
}

static menu_t * do_encoder_anticlockwise() {
    gui_listbox(self->listbox, LIST_ACTION_UP);
    return &self->base;
}

start_screen_t start_screen = {
    .base = {
        .enter = &enter,
        .do_encoder_pushed = &do_encoder_pushed,
        .do_back_pushed = &do_back_pushed,
        .do_encoder_clockwise = &do_encoder_clockwise,
        .do_encoder_anticlockwise = &do_encoder_anticlockwise,
        ._ready = false,
        ._parent = NULL,
        ._show = &_show,
    },
    ._init_listbox = &_init_listbox,
};
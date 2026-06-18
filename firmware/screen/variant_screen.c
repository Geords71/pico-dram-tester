#include <stdbool.h>
#include <stdint.h>
#include "gui.h"
#include "mem_chip.h"
#include "logging/logging.h"
#include "start_screen.h"
#include "variant_screen.h"

// Singleton self pointer
static variant_screen_t *self;

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

static void _show() {
    paint_dialog("Select Variant");
    gui_listbox(self->listbox, LIST_ACTION_NONE);
}

static void _init_listbox(uint16_t chip_index)
{
    // Populate items array
    uint8_t count = chip_list[chip_index]->variants->len;

    for (uint8_t i = 0; i < count; i++) {
        variant_items[i] = chip_list[chip_index]->variants->list[i].name;
    }

    variant_listbox.tot_lines = count;
    self->listbox = &variant_listbox;
}

static void enter(menu_t *parent, uint16_t chip_index)
{
    self = &variant_screen;

    if (!self->base._ready) {
        self->base._parent = parent;
        self->base._ready = true;
    }

    self->_init_listbox(chip_index);
    self->base._show();
}

static menu_t * do_back_pushed()
{
    self->base._parent->enter(NULL, 0);
    return self->base._parent;
}

static menu_t * do_encoder_pushed()
{
    return &self->base;
}

static menu_t * do_encoder_clockwise() {
    gui_listbox(self->listbox, LIST_ACTION_DOWN);
    return &self->base;
}

static menu_t * do_encoder_anticlockwise() {
    gui_listbox(self->listbox, LIST_ACTION_UP);
    return &self->base;
}

variant_screen_t variant_screen = {
    .base = {
        .enter = &enter,
        .do_back_pushed = &do_back_pushed,
        .do_encoder_pushed = &do_encoder_pushed,
        .do_encoder_clockwise = &do_encoder_clockwise,
        .do_encoder_anticlockwise = &do_encoder_anticlockwise,
        ._parent = NULL,
        ._ready = false,
        ._show = &_show,
    },
    ._init_listbox = &_init_listbox,
    .listbox = NULL,
};

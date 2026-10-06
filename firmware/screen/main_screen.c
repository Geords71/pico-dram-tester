#include "stdbool.h"
#include "stdint.h"
#include "mem_tester.h"
#include "main_screen.h"
#include "chip_screen.h"
#include "mem_tester.h"

enum menu_items_t {
    STANDARD_TEST,
    SOAK_TEST,
    BIN_SOAK_TEST,
    NUM_MENU_ITEMS,
};

static char *listbox_items[NUM_MENU_ITEMS] = {
    [STANDARD_TEST] = "Standard Test",
    [SOAK_TEST] = "Soak Test",
    [BIN_SOAK_TEST] = "Rate & Soak Test",
};

static gui_listbox_t listbox = {
    .sx = 7,
    .sy = 40,
    .width = 220,
    .tot_lines = NUM_MENU_ITEMS,
    .vis_lines = 4,
    .sel_line = 0,
    .start_line = 0,
    .items = listbox_items,
};

static menu_t self;

static void show() {
    paint_gui_dialog("Main Menu");
    paint_gui_listbox(&listbox, LIST_ACTION_NONE);
}

static menu_t * enter(menu_t *parent)
{
    if (parent != NULL) {
        self.parent = parent;
    }
    show();
    return &self;
}

static menu_t * do_encoder_pushed()
{
    uint8_t code = listbox.sel_line;
    menu_t *next_screen = chip_screen;

    if (code == SOAK_TEST || code == BIN_SOAK_TEST) {
        mem_tester->shared.please_soak = true;
    } else {
        mem_tester->shared.please_soak = false;
    };

    if (code == BIN_SOAK_TEST) {
        mem_tester->shared.please_bin = true;
    } else {
        mem_tester->shared.please_bin = false;
    };

    return next_screen->enter(&self);
}

static menu_t * do_back_pushed() {
    return self.parent->enter(NULL);
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

menu_t *main_screen = &self;

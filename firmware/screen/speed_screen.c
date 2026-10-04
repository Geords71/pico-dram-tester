#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gui.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"
#include "speed_screen.h"
#include "power_prompt_screen.h"

// Singleton self pointer
static menu_t self;

// Persistent backing storage for this screen
static gui_listbox_t speed_listbox = {
    .sx = 7,
    .sy = 40,
    .width = 220,
    .tot_lines = 0,
    .vis_lines = 4,
    .sel_line = 0,
    .start_line = 0,
    .items = NULL,
};

static const mem_chip_t *cur_chip = NULL;

static void show() {
    paint_gui_dialog("Select speed");
    paint_gui_listbox(&speed_listbox, LIST_ACTION_NONE);
}

static void init_listbox()
{
    // If we have already initilised and nothing has changed then we can safely
    // return with no ation.
    if (cur_chip == mem_tester->chip) {
        return;
    }
    cur_chip = mem_tester->chip;

    speed_listbox.items = (char **)cur_chip->delay_sets.names;
    speed_listbox.tot_lines = cur_chip->delay_sets.len;
    speed_listbox.sel_line = 0;
    speed_listbox.start_line = 0;
}

static menu_t * enter(menu_t *parent)
{
    if (parent != NULL) {
        self.parent = parent;
        init_listbox();
    }
    show();
    return &self;
}

static menu_t * do_back_pushed()
{
    return self.parent->enter(NULL);
}

static menu_t * do_encoder_pushed()
{
    mem_tester->speed_idx = speed_listbox.sel_line;

    menu_t *next_screen = power_prompt_screen;
    return next_screen->enter(&self);
}

static menu_t * do_encoder_clockwise() {
    paint_gui_listbox(&speed_listbox, LIST_ACTION_DOWN);
    return &self;
}

static menu_t * do_encoder_anticlockwise() {
    paint_gui_listbox(&speed_listbox, LIST_ACTION_UP);
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

menu_t * speed_screen = &self;

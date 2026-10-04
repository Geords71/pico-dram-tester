#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gui.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"
#include "power_prompt_screen.h"
#include "test_screen.h"

// Singleton self pointer
static menu_t self;

static bool prompt_active = false;
static const mem_chip_t *cur_chip = NULL;

static void show() {
    paint_gui_messagebox(
        "Place Chip in Socket",
        "Turn on external supply afterwards, if used.",
        &chip_icon
    );
}

static menu_t * do_back_pushed()
{
    return self.parent->enter(NULL);
}

static menu_t * enter(menu_t *parent)
{
    if (parent != NULL) {
        self.parent = parent;
        show();
        return &self;
    } else {
        return do_back_pushed();
    }
}

static menu_t * do_encoder_pushed()
{
    menu_t *next_screen = test_screen;
    return next_screen->enter(&self);
}

static menu_t * do_return_self() {
    return &self;
}

static menu_t self = {
    .enter = &enter,
    .do_back_pushed = &do_back_pushed,
    .do_encoder_pushed = &do_encoder_pushed,
    .do_encoder_clockwise = &do_return_self,
    .do_encoder_anticlockwise = &do_return_self,
    .do_tasks = &do_return_self,
    .parent = NULL,
};

menu_t * power_prompt_screen = &self;

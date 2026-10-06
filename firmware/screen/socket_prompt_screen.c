#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "gui.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"
#include "socket_prompt_screen.h"
#include "power_prompt_screen.h"

// Singleton self pointer
static menu_t self;

static bool prompt_active = false;

static void show() {
    char msg[64];
    sprintf(msg, "Place chip in %s socket", mem_tester->chip->get_socket()->name);
    paint_gui_messagebox(
        "Insert Chip",
        msg,
        &chip_icon
    );
}

static menu_t * do_back_pushed()
{
    return self.parent->enter(NULL);
}

static menu_t * enter(menu_t *parent)
{
    if (!config(false)->show_socket_prompt) return power_prompt_screen->enter(parent);

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
    menu_t *next_screen = power_prompt_screen;
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

menu_t * socket_prompt_screen = &self;
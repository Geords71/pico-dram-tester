#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "config.h"
#include "queue.h"
#include "mem_tester.h"
#include "mem_chip.h"
#include "board.h"
#include "test_screen.h"
#include "gui.h"
#include "logging/logging.h"

typedef enum {
    TESTS_RUNNING,
    TESTS_FINISHED,
} test_screen_state_t;

static test_screen_state_t state = TESTS_FINISHED;

static menu_t self;


static menu_t * do_back_pushed() {
    if (state == TESTS_RUNNING) return &self;

    self.parent->enter(NULL);
    return self.parent;
}

static menu_t * do_return_self() {
    return &self;
}

static void show() {
    const char *chip_name = mem_tester->chip->short_name;
    const char *chip_speed = mem_tester->chip->speed_names[mem_tester->speed_idx];
    char title[32];
    snprintf(title, 32, "Testing %s@%s...", chip_name, chip_speed);
    paint_gui_test_screen(title);
}

static void start_tests() {
    const mem_chip_t *chip = mem_tester->chip;

    mem_tester->shared.reset();
    board_ram_power_on();

    ULOG_INFO("Testing %s chip at %s...", chip->name, chip->speed_names[mem_tester->speed_idx]);
    ULOG_INFO("Setting up PIO...");
    chip->setup_pio(mem_tester->speed_idx, mem_tester->variant_idx);

    // Dispatch to the second core
    ULOG_INFO("Sending all_ram_tests() to the core1 call_queue...");
    state = TESTS_RUNNING;
    mem_tester->run_all();
}

static void stop_tests() {
    ULOG_INFO("Tearing down PIO...");
    mem_tester->chip->teardown_pio();
    board_ram_power_off();
    paint_gui_test_screen_completion_status(mem_tester->shared.run_result);
    state = TESTS_FINISHED;
}


static menu_t * do_encoder_pushed() {
    if (state == TESTS_RUNNING) return &self;
    show();
    start_tests();
    return &self;
}

static void enter (menu_t *parent) {
    self.parent = parent;
    show();
    start_tests();
}

static uint32_t last_test = 0;
static inline void refresh_status (uint8_t cur_test) {
    if (cur_test != last_test) {
        paint_gui_status(120, 35, 110, "      ");
        paint_gui_status(120, 35, 110, (char *) mem_test_names[cur_test]);
        last_test = cur_test;
    }
}

// During a RAM test, updates the status window and checks for the end of the test
// This is called every cycle of the main pmemtest.c program loop while this screen is
// the current screen.
static menu_t * do_tasks()
{
    if (state != TESTS_RUNNING) {
        return &self;
    }

    paint_gui_test_screen_visualization();
    paint_gui_drum_animation();
    refresh_status (mem_tester->shared.cur_test);

    if (mem_tester->shared.run_state != 2) {
        return &self;
    }

    // The RAM test completed, so let's handle that
    stop_tests();

    return &self;
}

static menu_t self = {
    .enter = &enter,
    .do_back_pushed = &do_back_pushed,
    .do_encoder_pushed = &do_encoder_pushed,
    .do_encoder_clockwise = &do_return_self,
    .do_encoder_anticlockwise = &do_return_self,
    .do_tasks = &do_tasks,
    .parent = NULL,
};

menu_t * test_screen = &self;
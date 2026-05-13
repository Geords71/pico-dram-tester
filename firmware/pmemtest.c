// Main entry point

// TODO:
// Make the refresh test fancier
// Bug fix the 41128

#include <stdio.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "board.h"
#include "config.h"
#include "logging/logging.h"
#include "menu.h"
#include "pio_patcher.h"
#include "queue.h"
#include "shared_storage/fat_little_flash.h"


void __no_inline_not_in_flash_func(busy_wait_ram)(uint32_t ms) {
    // Each loop is roughly 10-20 cycles. 
    // At 150MHz (Pico 2), this is a "human perceivable" delay.
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        __asm("nop");
    }
}

// Entry point for second core. This is just a generic
// function dispatcher lifted from the Raspberry Pi example code.
void __no_inline_not_in_flash_func(core1_entry)() {

    queue_entry_t entry;
    int32_t result = 0;
    while (true) {
        // Function pointer is passed to us via the queue_entry_t which also
        // contains the function parameter.
        if(queue_try_remove(&call_queue, &entry)) {
            // We provide an int32_t return value by simply pushing it back on the
            // return queue which also indicates the result is ready.
            result = entry.func(entry.mem_chip);
            queue_try_add(&results_queue, &result);
        } else {
            busy_wait_ram(100);
        };

    }
}

int main() {
    init_logging();
    ULOG_INFO("Configured Logging...");

    // Apply things like core voltage and overclock
    ULOG_INFO("Configuring Core System Settings...");
    do_system_config();

    ULOG_INFO("Initializing Flash Storage...");
    fat_little_flash_initialize();

    ULOG_INFO("Loading Application Config from Flash Storage...");
    load_app_config(true);

    // We must do this to prevent tusb from messing with lcd display later.
    ULOG_INFO("Configuring UART...");
    static uart_inst_t *uart_inst;
    uart_inst = uart_get_instance(0);
    stdio_uart_init_full(uart_inst, 57600, 0, 1);

    ULOG_INFO("Configuring TinyUSB...");
    tud_init(BOARD_TUD_RHPORT);

    // This allows us to get stdout in a terminal or serial monitor.
    // Works well with VSCode's Serial Monitor plugin.
    ULOG_INFO("Configuring USB Serial STDOUT...");
    stdio_usb_init();

    ULOG_INFO("Setting up board...");
    board_init();

    // Set up second core
    ULOG_INFO("Setting up second ARM core (to run chip tests)...");
    queue_init(&call_queue, sizeof(queue_entry_t), 2);
    queue_init(&results_queue, sizeof(int32_t), 2);
    queue_init(&stat_cur_test, sizeof(int), 2);

    // Second core will wait for the call queue.
    multicore_launch_core1(core1_entry);


    ULOG_INFO("Initializing Main Menu...");
    menu_init();
    menu_main_show();
    
    ULOG_INFO("Pico DRAM Tester reporting for duty!");

    uint32_t current_vtable = scb_hw->vtor;
    if (current_vtable >= 0x10000000 && current_vtable < 0x11000000) {
        ULOG_WARNING("VECTOR TABLE IN FLASH! USB flash writes will be unsafe and require a reboot!");
    }

    ULOG_INFO("Entering Main Program Loop...");
    while(1) {
        do_menu_wheel();
        do_menu_buttons();
        do_menu_status();
        flush_logging();
        tud_task();
    }

    return 0;
}
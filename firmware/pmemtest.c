// TODO:
// Make the refresh test fancier
// Bug fix the 41128

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "board.h"
#include "config.h"
#include "logging/logging.h"
#include "menu.h"
#include "shared_storage/fat_little_flash.h"


int main() {
    init_logging();
    ULOG_INFO("Configured Logging...");

    // Apply things like core voltage and overclock
    ULOG_INFO("Configuring Core System Settings...");
    board_pre_init();

    ULOG_INFO("Initializing Flash Storage...");
    fat_little_flash_initialize();

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

    ULOG_INFO("Initializing Main Menu...");
    menu_init();
    
    ULOG_INFO("Pico DRAM Tester reporting for duty!");

    uint32_t current_vtable = scb_hw->vtor;
    if (current_vtable >= 0x10000000 && current_vtable < 0x11000000) {
        ULOG_WARNING("VECTOR TABLE IN FLASH! USB flash writes will be unsafe and require a reboot!");
    }

    ULOG_INFO("Entering Main Program Loop...");
    while(1) {
        screen_task();
        flush_logging();
        tud_task();
    }

    return 0;
}
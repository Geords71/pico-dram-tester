
#include <bsp/board.h>
#include <tusb.h>
#include <ff.h>
#include <flash.h>

void init_usb_drive () {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
}

void handle_usb_drive() {
    tud_task();
}

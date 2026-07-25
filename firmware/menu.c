#include "board.h"
#include "logging/logging.h"
#include "mem_tester.h"
#include "menu.h"

#include "screen/main_screen.h"

menu_t *cur_screen;

void menu_init()
{

    //ULOG_INFO("Initializing Second Core for Mem Tester Module...");
    //mem_tester->shared.init();

    ULOG_INFO("Initializing GUI...");
    gui_init();

    cur_screen = main_screen;
    main_screen->enter(main_screen);
}



void do_inputs()
{
    if (board_encoder_pushed()) {
        cur_screen = cur_screen->do_encoder_pushed(cur_screen);
    }

    if (board_back_pushed()) {
        cur_screen = cur_screen->do_back_pushed(cur_screen);
    }

    switch (do_board_encoder()) {
        case BOARD_ENCODER_ROTATION_CLOCKWISE:
            cur_screen = cur_screen->do_encoder_clockwise(cur_screen);
            break;
        case BOARD_ENCODER_ROTATION_ANTICLOCKWISE:
            cur_screen = cur_screen->do_encoder_anticlockwise(cur_screen);
            break;
        default:
            break;
    }

}

void do_tasks() {
    cur_screen = cur_screen->do_tasks();
}


void screen_task() {
    do_inputs();
    do_tasks();
    gui_update();
}
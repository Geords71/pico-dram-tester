//#include <stdio.h>
//#include "hardware/timer.h"
#include "pico/util/queue.h"
#include "menu.h"
#include "board.h"
//#include "config.h"
#include "logging/logging.h"
//#include "mem_chip.h"
#include "mem_tester.h"
//#include "queue.h"
#include "menu.h"

#include "screen/main_screen.h"

/*#define MAIN_MENU_ITEMS 16
char *main_menu_items[MAIN_MENU_ITEMS];
gui_listbox_t *cur_menu;
gui_listbox_t main_menu = {7, 40, 220, MAIN_MENU_ITEMS, 4, 0, 0, main_menu_items};
gui_listbox_t variants_menu = {7, 40, 220, 0, 4, 0, 0, 0};
gui_listbox_t speed_menu = {7, 40, 220, 0, 4, 0, 0, 0};
*/

menu_t *cur_screen;

/*void menu_init()
{
    ULOG_INFO("Initializing GUI...");
    gui_init();

    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        main_menu_items[i] = (char *)chip_list[i]->name;
    }
    main_menu.tot_lines = NUM_CHIPS;
}

// Setup and display the main menu
void menu_main_show()
{
    cur_menu = &main_menu;
    paint_gui_dialog("Select Device");
    paint_gui_listbox(cur_menu, LIST_ACTION_NONE);
}

void menu_variant_show()
{
    uint chip = main_menu.sel_line;

    static char *items[MEMCHIP_MAX_VARIANTS];
    for (uint8_t i = 0; i<MEMCHIP_MAX_VARIANTS; i++) {
        items[i] = chip_list[chip]->variants->list[i].name;
    }

    paint_gui_dialog("Select Variant");
    cur_menu = &variants_menu;
    cur_menu->tot_lines = chip_list[chip]->variants->len;
    cur_menu->items = items;
    paint_gui_listbox(cur_menu, LIST_ACTION_NONE);
}

// With the selected chip, populate the speed grade menu and show it
void menu_speed_show()
{
    uint chip = main_menu.sel_line;
    cur_menu = &speed_menu;
    paint_gui_dialog("Select Speed Grade");
    speed_menu.items = (char **)chip_list[chip]->speed_names;
    speed_menu.tot_lines = chip_list[chip]->delay_sets.len;
    paint_gui_listbox(cur_menu, LIST_ACTION_NONE);
}



#define CELL_STAT_X 9
#define CELL_STAT_Y 33

// Used to update the RAM test GUI left pane
static inline void update_vis_dot(uint16_t cx, uint16_t cy, uint16_t col)
{
    st7789_fill(CELL_STAT_X + cx * 3, CELL_STAT_Y + cy * 3, 2, 2, col);
}

#define STATUS_ICON_X 155
#define STATUS_ICON_Y 65
*/
// Play the drums
/*bool drum_animation_cb_old(__unused struct repeating_timer *t)
{
    static uint8_t drum_st = 0;
    drum_st++;
    if (drum_st > 3) drum_st = 0;
    st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY);
    switch (drum_st) {
        case 0:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon0);
            break;
        case 1:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon1);
            break;
        case 2:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon2);
            break;
        case 3:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon3);
            break;
    }
    return true;
}
    */

/*struct repeating_timer drum_timer;
// Show the RAM test console GUI
void show_test_gui()
{
    uint16_t cx, cy;

    uint32_t sys_clk = get_system_overclock() / 1000000;
    char title[30];
    sprintf(title, "PIO@%dMHz Testing...", sys_clk);

    paint_gui_dialog(title);

    // Cell status area. 32x32 elements.
    paint_gui_fancy_rect(7, 31, 100, 100, B_SUNKEN_OUTER); // Usable size is 220x80.
    paint_gui_fancy_rect(8, 32, 98, 98, B_SUNKEN_INNER);
    st7789_fill(9, 33, 96, 96, COLOR_BLACK);
    for (cy = 0; cy < 32; cy++) {
        for (cx = 0; cx < 32; cx++) {
            update_vis_dot(cx, cy, COLOR_DKGRAY);
        }
    }
    mem_tester->shared.old_addr = 0;
    mem_tester->shared.cur_bit = 0;
    mem_tester->shared.cur_subtest = 0;

    // Current test indicator
    paint_gui_status(120, 35, 110, "      ");
    draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon0);
    add_repeating_timer_ms(-100, drum_animation_cb_old, NULL, &drum_timer);
}
    */

// Begins the RAM test with the selected RAM chip
/*void __no_inline_not_in_flash_func(start_the_ram_test)(const mem_chip_t *mem_chip, uint8_t speed_grade, uint8_t variant)
{
    // Get the power turned on
    board_ram_power_on();

    ULOG_INFO("Testing %s chip at %s...", mem_chip->name, speed_menu.items[speed_grade]);

    ULOG_INFO("Setting up PIO...");
    // Get the PIO going
    mem_chip->setup_pio(speed_grade, variant);

    // Dispatch to the second core
    ULOG_INFO("Sending all_ram_tests() to the core1 call_queue...");
    queue_entry_t entry = {mem_tester->run_all, mem_chip};
    if(queue_try_add(&call_queue, &entry)) {
        ULOG_INFO("Sent.");
    } else {
        ULOG_WARNING("Couldn't send!");
    };
}
*/

// Called when user presses the action button
/*void menu_select()
{
    // Do something based on the current menu
    switch (gui.state) {
        case MAIN_MENU:
            // Check for variant
            if (chip_list[main_menu.sel_line]->variants == NULL) {
                gui.state = SPEED_MENU;
                menu_speed_show();
            } else {
                gui.state = VARIANT_MENU;
                menu_variant_show();
            }
            break;
        case VARIANT_MENU:
            // Set up variant
            gui.state = SPEED_MENU;
            menu_speed_show();
            break;
        case SPEED_MENU:
            paint_gui_messagebox(
                "Place Chip in Socket",
                "Turn on external supply afterwards, if used.",
                &chip_icon
            );
            gui.state = DO_SOCKET;
            break;
        case DO_SOCKET:
            gui.state = DO_TEST;
            show_test_gui();
            start_the_ram_test(chip_list[main_menu.sel_line], speed_menu.sel_line, variants_menu.sel_line);
            break;
        case DO_TEST:
            break;
        case TEST_RESULTS:
            // Quick retest to save time
            gui.state = DO_TEST;
            show_test_gui();
            start_the_ram_test(chip_list[main_menu.sel_line], speed_menu.sel_line, variants_menu.sel_line);
            break;
        default:
            gui.state = MAIN_MENU;
            break;
    }
}*/





// Figure out where visualization dot goes and map it
/*static inline void map_vis_dot(int addr, int ox, int oy, int bitsize, uint16_t col)
{
    int cx, cy;
    if (bitsize == 4) {
        cx = addr & 0xf;
        cy = (addr >> 4) & 0xf;
    } else {
        cx = addr & 0x1f;
        cy = (addr >> 5) & 0x1f;
    }
    update_vis_dot(cx + ox, cy + oy, col);
}
    */

// Draw up visualization from current test state
/*void do_visualization()
{
    const uint16_t cmap[] = {COLOR_DKBLUE, COLOR_DKGREEN, COLOR_DKMAGENTA, COLOR_DKYELLOW, COLOR_GREEN};
    int bitsize = chip_list[main_menu.sel_line]->bits;
    int new_addr = mem_tester->shared.cur_addr * 1024 / chip_list[main_menu.sel_line]->mem_size / bitsize;
    int bit = mem_tester->shared.cur_bit;
    uint16_t col = cmap[mem_tester->shared.cur_subtest];
    int delta, i;
    int ox, oy = 0;

    if (bitsize == 4) {
        switch (bit) {
            case 1:
                oy = 0;
                ox = 16;
                break;
            case 2:
                oy = 16;
                ox = 0;
                break;
            case 3:
                ox = oy = 16;
                break;
            default:
                ox = oy = 0;
        }
    } else {
        ox = oy = 0;
    }

    if (new_addr > mem_tester->shared.old_addr) {
        delta = new_addr - mem_tester->shared.old_addr;
        for (i = 0; i < delta; i++) {
            map_vis_dot(mem_tester->shared.old_addr + i, ox, oy, bitsize, col);
        }
    } else {
        delta = mem_tester->shared.old_addr - new_addr;
        for (i = delta - 1; i >= 0; i--) {
            map_vis_dot(mem_tester->shared.old_addr + i, ox, oy, bitsize, col);
        }
    }
    mem_tester->shared.old_addr = new_addr;
}

// Stops the RAM test
void stop_the_ram_test(const mem_chip_t *mem_chip)
{
    mem_chip->teardown_pio();
    board_ram_power_off();
}
*/

//static const char *ram_test_names[] = {"March-B", "Pseudo", "Refresh"};

// During a RAM test, updates the status window and checks for the end of the test
/*void do_menu_status()
{
    uint32_t retval;
    char retstring[30];
    uint16_t v;
    static uint16_t v_prev = 0;
    int test;

    if (gui.state == DO_TEST) {
        do_visualization();

        // Update the status text
        if (queue_try_remove(&stat_cur_test, &test)) {
            paint_gui_status(120, 35, 110, "      ");
            paint_gui_status(120, 35, 110, (char *)ram_test_names[test]);
        }

        // Check official status
        if (!queue_is_empty(&results_queue)) {
            stop_the_ram_test(chip_list[main_menu.sel_line]);

            // The RAM test completed, so let's handle that
            sleep_ms(10);

            // No more drums
            cancel_repeating_timer(&drum_timer);
            queue_remove_blocking(&results_queue, &retval);

            // Show the completion status
            gui.state = TEST_RESULTS;
            st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY); // Erase icon
            if (retval == 0) {
                paint_gui_status(120, 35, 110, "Passed!");
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &check_icon);
            } else {
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &error_icon);
                if (chip_list[main_menu.sel_line]->bits == 4) {
                    sprintf(retstring, "Failed %d%d%d%d",
                        (retval >> 3) & 1,
                        (retval >> 2) & 1,
                        (retval >> 1) & 1,
                        (retval & 1));
                    paint_gui_status(120, 105, 110, retstring);
                } else {
                    paint_gui_status(120, 105, 110, "Failed");
                }
            }
        }
    }
}

// Called when the user presses the back button
void menu_back()
{
    switch (gui.state) {
        case MAIN_MENU:
            break;
        case VARIANT_MENU:
            gui.state = MAIN_MENU;
            menu_main_show();
            break;
        case SPEED_MENU:
            // Check if our selection has a variant
            if (chip_list[main_menu.sel_line]->variants == NULL) {
                gui.state = MAIN_MENU;
                menu_main_show();
            } else {
                gui.state = VARIANT_MENU;
                menu_variant_show();
            }
            break;
        case DO_SOCKET:
            gui.state = SPEED_MENU;
            menu_speed_show();
            break;
        case DO_TEST:
            break;
        case TEST_RESULTS:
            gui.state = SPEED_MENU;
            menu_speed_show();
            break;
        default:
            gui.state = MAIN_MENU;
            break;
    }
}

void menu_scroll_down()
{
    if (gui.state == MAIN_MENU || gui.state == SPEED_MENU || gui.state == VARIANT_MENU) {
        paint_gui_listbox(cur_menu, LIST_ACTION_DOWN);
    }
}

void menu_scroll_up()
{
    if (gui.state == MAIN_MENU || gui.state == SPEED_MENU || gui.state == VARIANT_MENU) {
        paint_gui_listbox(cur_menu, LIST_ACTION_UP);
    }
}


void do_menu_wheel() {
    switch (do_board_encoder()) {
        case BOARD_ENCODER_ROTATION_CLOCKWISE:
            menu_scroll_down();
            break;
        case BOARD_ENCODER_ROTATION_ANTICLOCKWISE:
            menu_scroll_up();
            break;
        default:
            break;
    }
}
    */

void menu_init()
{

    ULOG_INFO("Initializing Second Core for Mem Tester Module...");
    mem_tester->shared.init();

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
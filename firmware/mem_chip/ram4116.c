#include <stdint.h>
#include "pico/types.h"
#include "ram4116.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4116_DELAY_SET_ROWS 6

static const delay_sets_t ram4116_delay_sets = {
    .len = RAM4116_DELAY_SET_ROWS,
    .wid = RAM1B1R_DELAY_SET_COLS,
    .list = {
        {0, 0, 18,  3,  8,  6, 0, 0}, // 100ns
        {0, 0, 19,  3, 12,  8, 0, 0}, // 120ns
        {0, 0, 19,  4, 14, 11, 0, 0}, // 150ns
        {0, 0, 23,  6, 17, 17, 6, 0}, // 200ns
        {0, 0, 30,  8, 25, 19, 0, 0}, // 250ns
        {0, 8, 30, 14, 29, 22, 0, 0}  // 300ns
    },
};

void ram4116_setup_pio(uint speed_grade, uint variant);

const mem_chip_t ram4116_chip = {
    .setup_pio = ram4116_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_7p,
    .ram_write = write_ram1b1r_7p,
    .mem_size = 16384,
    .bits = 1,
    .variants = NULL,
    .name = "4116 (16Kx1)",
    .timing_family = "ram4116",
    .delay_sets = ram4116_delay_sets,
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};

void ram4116_setup_pio(uint speed_grade, uint variant) {
    get_ram_config(ram4116_chip);
    ram1b1r_setup_pio(ram4116_delay_sets.list[speed_grade], variant);
}
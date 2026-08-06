#include <stdint.h>
#include "ram4816.h"
#include "pico/types.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4816_DELAY_SET_ROWS 3

static const delay_sets_t ram4816_delay_sets = {
    .len = RAM4816_DELAY_SET_ROWS,
    .wid = RAM1B1R_DELAY_SET_COLS,
    .list = {
        {0,  0, 23, 3, 11,  4,  0,  0}, // 100ns
        {0,  0, 25, 4, 12,  6,  1,  0}, // 120ns
        {0,  0, 27, 3, 19,  9,  0,  0}, // 150ns
    },
};

static const mem_chip_variants_t ram4816_variants = {
    .len = 1,
    .list = {
        {"4816", read_ram1b1r_7p, write_ram1b1r_7p},
    },
};

void ram4816_setup_pio(uint speed_grade, uint variant);

// This RAM chip configuration
const mem_chip_t ram4816_chip = {
    .setup_pio = ram4816_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_7p,
    .ram_write = write_ram1b1r_7p,
    .mem_size = 16384,
    .bits = 1,
    .variants = &ram4816_variants,
    .name = "4816 (16Kx1 use 4164 skt)",
    .short_name = "4816",
    .timing_family = "ram4816",
    .delay_sets = ram4816_delay_sets,
    .speed_names = {"100ns", "120ns", "150ns"}
};

void ram4816_setup_pio(uint speed_grade, uint variant) {
    get_ram_config(ram4816_chip);
    ram1b1r_setup_pio(ram4816_delay_sets.list[speed_grade], variant);
}
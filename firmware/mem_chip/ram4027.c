
#include <stdint.h>
#include "pico/types.h"
#include "ram4027.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4027_DELAY_SET_ROWS 5

static const delay_sets_t ram4027_delay_sets = {
    .len = RAM4027_DELAY_SET_ROWS,
    .wid = RAM1B1R_DELAY_SET_COLS,
    .list = {
        {0, 31, 21, 1,  8,  9,  3,  8}, // 120ns
        {0, 31, 12, 3, 10, 14,  3,  4}, // 150ns
        {0, 31, 14, 5, 13, 21,  6,  7}, // 200ns
        {0, 20, 21, 8, 19, 23, 10, 11}, // 250ns
        {0, 20, 21, 7, 22, 27, 19,  1}  // 300ns
    },
};

static const mem_chip_variants_t ram4027_variants = {
    .len = 1,
    .list = {
        {"4027", read_ram1b1r_6p, write_ram1b1r_6p},
    },
};

void ram4027_setup_pio(uint speed_grade, uint variant);

// This RAM chip configuration
const mem_chip_t ram4027_chip = {
    .setup_pio = ram4027_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_6p,
    .ram_write = write_ram1b1r_6p,
    .mem_size = 4096,
    .bits = 1,
    .variants = &ram4027_variants,
    .name = "4027 (4Kx1 use 4116skt)",
    .short_name = "4027",
    .timing_family = "ram4027",
    .delay_sets = ram4027_delay_sets,
    .speed_names = {"120ns", "150ns", "200ns", "250ns", "300ns"}
};

void ram4027_setup_pio(uint speed_grade, uint variant) {
    get_ram_config(ram4027_chip);
    ram1b1r_setup_pio(ram4027_delay_sets.list[speed_grade], variant);
}
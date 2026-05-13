
#include <stdint.h>
#include "pico/types.h"
#include "ram4108.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4108_DELAY_SET_ROWS 6

static delay_sets_t ram4108_delays = {
    {0, 0, 18,  3,  8,  6, 0, 0}, // 100ns
    {0, 0, 19,  3, 12,  8, 0, 0}, // 120ns
    {0, 0, 19,  4, 14, 11, 0, 0}, // 150ns
    {0, 0, 23,  6, 17, 17, 6, 0}, // 200ns
    {0, 0, 30,  8, 25, 19, 0, 0}, // 250ns
    {0, 8, 30, 14, 29, 22, 0, 0}  // 300ns
};

void ram4108_setup_pio(uint speed_grade, uint variant);

static const mem_chip_variants_t ram4108_variants = {
    .len = 2,
    .list = {
        {"MK4108-40 (low)",  read_ram1b1r_7p_half_lc, write_ram1b1r_7p_half_lc},
        {"MK4108-41 (high)", read_ram1b1r_7p_half_hc, write_ram1b1r_7p_half_hc},
    }
};

mem_chip_t ram4108_chip= {
    .setup_pio = ram4108_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_7p,
    .ram_write = write_ram1b1r_7p,
    .mem_size = 8192,
    .bits = 1,
    .variants = &ram4108_variants,
    .name = "4108 (8Kx1 use 4116skt)",
    .timing_family = "ram4108",
    .delay_set_rows = RAM4108_DELAY_SET_ROWS,
    .delay_set_cols = RAM1B1R_DELAY_SET_COLS,
    .delay_sets = ram4108_delays,
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};

void ram4108_setup_pio(uint speed_grade, uint variant) {
    ram4108_chip.ram_read = ram4108_variants.list[variant].ram_read;
    ram4108_chip.ram_write = ram4108_variants.list[variant].ram_write;
    get_ram_config(ram4108_chip);
    ram1b1r_setup_pio(ram4108_delays[speed_grade], variant);
}

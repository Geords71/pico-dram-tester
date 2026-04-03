#include <stdint.h>
#include "pico/types.h"
#include "ram4116.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4116_DELAY_SET_ROWS 6

static uint8_t ram4116_delays[RAM4116_DELAY_SET_ROWS][RAM1B1R_DELAY_SET_COLS] = {
    {0, 0, 18,  3,  8,  6, 0, 0}, // 100ns
    {0, 0, 19,  3, 12,  8, 0, 0}, // 120ns
    {0, 0, 19,  4, 14, 11, 0, 0}, // 150ns
    {0, 0, 23,  6, 17, 17, 6, 0}, // 200ns
    {0, 0, 30,  8, 25, 19, 0, 0}, // 250ns
    {0, 8, 30, 14, 29, 22, 0, 0}  // 300ns
};

void ram4116_setup_pio(uint speed_grade, uint variant) {
    get_ram1b1r_config("ram4116", ram4116_delays, RAM4116_DELAY_SET_ROWS);
    ram1b1r_setup_pio(ram4116_delays[speed_grade], variant);
}

// This RAM chip configuration
const mem_chip_t ram4116_chip = {
    .setup_pio = ram4116_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_7p,
    .ram_write = write_ram1b1r_7p,
    .mem_size = 16384,
    .bits = 1,
    .variants = NULL,
    .speed_grades = RAM4116_DELAY_SET_ROWS,
    .chip_name = "4116 (16Kx1)",
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};

static const mem_chip_variants_t ram4116_half_chip_variants = {
    .num_variants = 2,
    .variant_names = {"MK4108-40 (low)", "MK4108-41 (high)"},
    .ram_reads = {
        read_ram1b1r_7p_half_lc,
        read_ram1b1r_7p_half_hc
    },
    .ram_writes = {
        write_ram1b1r_7p_half_lc,
        write_ram1b1r_7p_half_hc
    }
};

void ram4116_half_setup_pio(uint speed_grade, uint variant)
{
    ram4116_setup_pio(speed_grade, 0);
    ram4116_half_chip.ram_read = ram4116_half_chip_variants.ram_reads[variant];
    ram4116_half_chip.ram_write = ram4116_half_chip_variants.ram_writes[variant];
    return;
}

mem_chip_t ram4116_half_chip = {
    .setup_pio = ram4116_half_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_7p,
    .ram_write = write_ram1b1r_7p,
    .mem_size = 8192,
    .bits = 1,
    .variants = &ram4116_half_chip_variants,
    .speed_grades = RAM4116_DELAY_SET_ROWS,
    .chip_name = "4108 (8Kx1 use 4116skt)",
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};
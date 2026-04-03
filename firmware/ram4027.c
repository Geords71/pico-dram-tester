
#include <stdint.h>
#include "pico/types.h"
#include "ram4027.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4027_DELAY_SET_ROWS 5

static uint8_t ram4027_delays[RAM4027_DELAY_SET_ROWS][RAM1B1R_DELAY_SET_COLS] = {
    {0, 31, 21, 1,  8,  9,  3,  8}, // 120ns
    {0, 31, 12, 3, 10, 14,  3,  4}, // 150ns
    {0, 31, 14, 5, 13, 21,  6,  7}, // 200ns
    {0, 20, 21, 8, 19, 23, 10, 11}, // 250ns
    {0, 20, 21, 7, 22, 27, 19,  1}  // 300ns
};

void ram4027_setup_pio(uint speed_grade, uint variant) {
    get_ram1b1r_config("ram4027", ram4027_delays, RAM4027_DELAY_SET_ROWS);
    ram1b1r_setup_pio(ram4027_delays[speed_grade], variant);
}

// This RAM chip configuration
const mem_chip_t ram4027_chip = {
    .setup_pio = ram4027_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_6p,
    .ram_write = write_ram1b1r_6p,
    .mem_size = 4096,
    .bits = 1,
    .variants = NULL,
    .speed_grades = RAM4027_DELAY_SET_ROWS, // FIXME: check timings
    .chip_name = "4027 (4Kx1 use 4116skt)",
    .speed_names = {"120ns", "150ns", "200ns", "250ns", "300ns"}
};
#include <stdint.h>
#include "pico/types.h"
#include "ram4164.h"
#include "ram1b1r.pio.h"
#include "mem_chip.h"

#define RAM4164_DELAY_SET_ROWS 6


static uint8_t ram4164_delays[RAM4164_DELAY_SET_ROWS][RAM1B1R_DELAY_SET_COLS] = {
    {0,  0, 15, 3, 11,  4,  0, 0}, // 100ns - tested on km4164b-10 No margin applied yet...
    {0,  0, 14, 3, 15,  4,  0, 0}, // 120ns - tighter of all Mnfctr values and -10% margin applied
    {0,  0, 18, 4, 20,  4,  0, 0}, // 150ns - tighter of all Mnfctr values and -15% margin applied
    {0,  0, 24, 4, 20, 17,  0, 0}, // 200ns
    {0,  2, 30, 8, 26, 20,  0, 0}, // 250ns
    {0, 10, 30, 8, 26, 23,  0, 0}  // 300ns - Complete guess! But very rare chip.
};


void ram4164_setup_pio(uint speed_grade, uint variant) {
    get_ram1b1r_config("ram4164", ram4164_delays, RAM4164_DELAY_SET_ROWS);
    ram1b1r_setup_pio(ram4164_delays[speed_grade], variant);
}


// This RAM chip configuration
const mem_chip_t ram4164_chip = {
    .setup_pio = ram4164_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_8p,
    .ram_write = write_ram1b1r_8p,
    .mem_size = 65536,
    .bits = 1,
    .variants = NULL,
    .speed_grades = RAM4164_DELAY_SET_ROWS,
    .chip_name = "4164 (64Kx1)",
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};

// Only used for half-qualified 4132 devices
static const mem_chip_variants_t ram4164_half_chip_variants = {
    .num_variants = 4,
    .variant_names = {
        "TMS4532xxNL3 (low)",
        "TMS4532xxNL4 (high)",
        "M3732L (low)",
        "M3732H (high)"
    },
    .ram_reads = {
        read_ram1b1r_8p_half_lr,
        read_ram1b1r_8p_half_hr,
        read_ram1b1r_8p_half_lc,
        read_ram1b1r_8p_half_hc
    },
    .ram_writes = {
        write_ram1b1r_8p_half_lr,
        write_ram1b1r_8p_half_hr,
        write_ram1b1r_8p_half_lc,
        write_ram1b1r_8p_half_hc
    }
};

void ram4164_half_setup_pio(uint speed_grade, uint variant)
{
    ram4164_setup_pio(speed_grade, 0);
    ram4164_half_chip.ram_read = ram4164_half_chip_variants.ram_reads[variant];
    ram4164_half_chip.ram_write = ram4164_half_chip_variants.ram_writes[variant];
    return;
}

mem_chip_t ram4164_half_chip = {
    .setup_pio = ram4164_half_setup_pio,
    .teardown_pio = ram1b1r_teardown_pio,
    .ram_read = read_ram1b1r_8p,
    .ram_write = write_ram1b1r_8p,
    .mem_size = 32768,
    .bits = 1,
    .variants = &ram4164_half_chip_variants,
    .speed_grades = RAM4164_DELAY_SET_ROWS,
    .chip_name = "4132 (32Kx1 use 4164skt)",
    .speed_names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"}
};

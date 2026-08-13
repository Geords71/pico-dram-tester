#include <stdint.h>
#include "pico/types.h"
#include "ram4132.h"
#include "mem_family/fam_1bit_1ras_64k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(&self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}

static int addr_lo_rows(int addr) {
    // Funky: The column address starts at the MSB of the low (row) byte. So
    // we need to: shift column bits up by one; blat row byte's MSB; and then
    // AND/OR these values together using appropriate masking.
    return (addr & 0x007f) | ((addr << 1) & 0xff00);
}

static int addr_hi_rows(int addr) {
    // Funky: The column address starts at the MSB of the low (row) byte. So
    // we need to: shift column bits up by one; set row byte's MSB; and then
    // AND/OR these values together using appropriate masking.
    return ((addr & 0x007f) | 0x0080) | ((addr << 1) & 0xff00);
}

static int addr_lo_cols(int addr) {
    // Easy: Force the msb to 0 on the high byte - which is the column. 
    return addr & 0x7fff;
}

static int addr_hi_cols(int addr) {
    // Easy: Force the msb to 1 on the high byte - which is the column. 
    return (addr & 0xff) | ((addr & 0x7f00) | 0x8000);
}


static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 32768,
    .bits = 1,
    .name = "4132 (32Kx1 use 4164skt)",
    .short_name = "4132",
    .timing_family = "ram4132",
    .variants = {
        .len = 4,
        .list = {
            {"TMS4532xxNL3 (low)",  addr_lo_rows, NULL, NULL},
            {"TMS4532xxNL4 (high)", addr_hi_rows, NULL, NULL},
            {"M3732L (low)",  addr_lo_cols, NULL, NULL},
            {"M3732H (high)", addr_hi_cols, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_1BIT_1RAS_64K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0,  0, 15, 3, 11,  4,  0, 0}, // 100ns - tested on km4164b-10 No margin applied yet...
            {0,  0, 14, 3, 15,  4,  0, 0}, // 120ns - tighter of all Mnfctr values and -10% margin applied
            {0,  0, 18, 4, 20,  4,  0, 0}, // 150ns - tighter of all Mnfctr values and -15% margin applied
            {0,  0, 24, 4, 20, 17,  0, 0}, // 200ns
            {0,  2, 30, 8, 26, 20,  0, 0}, // 250ns
            {0, 10, 30, 8, 26, 23,  0, 0}  // 300ns - Complete guess! But very rare chip.
        },
    },
};

mem_chip_t *ram4132_chip() {
    return &self;
}

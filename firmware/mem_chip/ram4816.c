#include <stdint.h>
#include "pico/types.h"
#include "mem_family/fam_1bit_1ras_64k.h"
#include "ram4816.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(&self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}

static int addr_func(int addr) {
    // Because we only have seven pins, need to do some bit shifting to be able
    // to re-use the 8pin read function. This works because what is the 8th pin
    // on 64k chips is not connected on 16k examples.
    return (addr & 0x007f) | ((addr << 1) & 0x7f00);
}

// This RAM chip configuration
static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 16384,
    .bits = 1,
    .name = "4816 (16Kx1 use 4164 skt)",
    .short_name = "4816",
    .timing_family = "ram4816",
    .variants = {
        .len = 1,
        .list = {
            {"4816", addr_func, read_ram1b1r_7p, write_ram1b1r_7p},
        },
    },
    .delay_sets ={
        .len = 3,
        .wid = FAM_1BIT_1RAS_64K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns"},
        .list = {
            {0,  0, 23, 3, 11,  4,  0,  0}, // 100ns
            {0,  0, 25, 4, 12,  6,  1,  0}, // 120ns
            {0,  0, 27, 3, 19,  9,  0,  0}, // 150ns
        },
    },
};

mem_chip_t *ram4816_chip() {
    return &self;
}

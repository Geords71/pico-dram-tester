#include <stdint.h>
#include "pico/types.h"
#include "ram4116.h"
#include "mem_family/fam_1bit_1ras_64k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(self);
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

static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 16384,
    .bits = 1,
    .name = "4116 (16Kx1)",
    .short_name = "4116",
    .timing_family = "ram4116",
    .variants = {
        .len = 1,
        .list = {
            {"4116", addr_func, read_ram1b1r_7p, write_ram1b1r_7p},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_1BIT_1RAS_64K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0, 0, 18,  3,  8,  6, 0, 0}, // 100ns
            {0, 0, 19,  3, 12,  8, 0, 0}, // 120ns
            {0, 0, 19,  4, 14, 11, 0, 0}, // 150ns
            {0, 0, 23,  6, 17, 17, 6, 0}, // 200ns
            {0, 0, 30,  8, 25, 19, 0, 0}, // 250ns
            {0, 8, 30, 14, 29, 22, 0, 0}  // 300ns
        },
    },
};

mem_chip_t *ram4116_chip() {
    return &self;
}
#include <stdint.h>
#include "pico/types.h"
#include "ram4027.h"
#include "mem_family/fam_1bit_1ras_64k.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio () {
    self.family()->teardown_pio();
}

static int addr_func(int addr) {
    // Because we only have six pins, need to do some bit shifting to be able
    // to re-use the 8pin read function. This works because what are 7th 
    // and 8th pins on 64k chips are not used for addresses on 4k examples.
    // 4k chips like 4027 use the 4116 socket on this board.
    // The 4027's chip select pin is in the same location as address pin 6
    // on a 4116. So we need to always have this pin pulled low when testing
    // a 4027 in the 4116b socket. (Chip Select is active low). e.g.
    return (addr & 0x003f) | ((addr << 2) & 0x3f00);
};

static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 4096,
    .bits = 1,
    .name = "4027 (4Kx1 use 4116skt)",
    .short_name = "4027",
    .timing_family = "ram4027",
    .variants = {
        .len = 1,
        .list = {
            {"4027", addr_func, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 5,
        .wid = FAM_1BIT_1RAS_64K_DELAY_SET_COLS,
        .names = {"120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0, 31, 21, 1,  8,  9,  3,  8}, // 120ns
            {0, 31, 12, 3, 10, 14,  3,  4}, // 150ns
            {0, 31, 14, 5, 13, 21,  6,  7}, // 200ns
            {0, 20, 21, 8, 19, 23, 10, 11}, // 250ns
            {0, 20, 21, 7, 22, 27, 19,  1}  // 300ns
        },
    },
};

mem_chip_t *ram4027_chip() {
    return &self;
}
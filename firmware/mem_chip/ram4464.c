#include <stdint.h>
#include "pico/types.h"
#include "ram4464.h"
#include "mem_family/fam_4bit_1ras_256k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio () {
    self.family()->teardown_pio();
}

static const delay_sets_t ram4464_delay_sets = {
};

static const mem_chip_variants_t ram4464_variants = {
};


static  mem_chip_t self = {
    .family = fam_4bit_1ras_256k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 65536,
    .bits = 4,
    .name = "4464 (64Kx4)",
    .short_name = "4464",
    .timing_family = "ram4464",
    .variants = {
        .len = 1,
        .list = {
            {"4464", NULL, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_4BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"60ns", "70ns", "80ns", "100ns", "120ns", "150ns"},
        .list = {
            {0, 0,  6, 2,  1,  2,  1,  0},  // 60ns
            {0, 0, 11, 2,  1,  2,  2,  0},  // 70ns
            {0, 0, 17, 3,  1,  2,  4,  0},  // 80ns
            {0, 0, 14, 3,  6,  5,  9,  0},  // 100ns
            {0, 0, 23, 3,  6,  5,  6,  0},  // 120ns
            {0, 0, 26, 3, 10,  6,  10, 0},  // 150ns
        },
    },
};

mem_chip_t *ram4464_chip() {
    return &self;
};
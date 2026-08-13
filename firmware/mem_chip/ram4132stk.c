#include <stdint.h>
#include "pico/types.h"
#include "ram4132stk.h"
#include "mem_family/fam_1bit_stacked_32k.h"
#include "mem_chip.h"

static mem_chip_t self;

static const delay_sets_t ram4132stk_delay_sets = {
};

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(&self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}


static const mem_chip_variants_t ram4132stk_variants = {
};

// This RAM chip configuration
static mem_chip_t self = {
    .family = fam_1bit_stacked_32k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 32768,
    .bits = 1,
    .name = "4132 (32Kx1, stacked)",
    .short_name = "4132stk",
    .timing_family = "ram4132",
    .variants = {
        .len = 1,
        .list = {
            {"4132stk", NULL, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 4,
        .wid = FAM_1BIT_STACKED_32K_DELAY_SET_COLS,
        .names = {"150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0, 31, 31, 4, 11, 11,  9}, // 150ns
            {0, 23, 24, 5, 14, 18, 13}, // 200ns
            {0, 21, 22, 8, 20, 21, 16}, // 250ns
            {0, 21, 22, 8, 23, 25, 24}, // 300ns}
        },
    },
};

mem_chip_t *ram4132stk_chip() {
    return &self;
}

#include <stdint.h>
#include "pico/types.h"
#include "ram4164.h"
#include "mem_family/fam_1bit_1ras_64k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(&self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio () {
    self.family()->teardown_pio();
}

// This RAM chip configuration
static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 65536,
    .bits = 1,
    .name = "4164 (64Kx1)",
    .short_name = "4164",
    .timing_family = "ram4164",
    .variants = {
        .len = 1,
        .list = {
            {"4164", NULL, NULL, NULL},
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

mem_chip_t *ram4164_chip() {
    return &self;
};

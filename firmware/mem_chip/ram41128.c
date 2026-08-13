#include <stdint.h>
#include "pico/types.h"
#include "mem_chip.h"
#include "ram41128.h"
#include "mem_family/fam_1bit_2ras_128k.h"
#include "logging.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant) {
    get_ram_config(&self);
    ULOG_INFO("Setting up PIO");
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio () {
    self.family()->teardown_pio();
}

// This RAM chip configuration
static mem_chip_t self = {
    .family = fam_1bit_2ras_128k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 131072,
    .bits = 1,
    .name = "41128 (128Kx1)",
    .short_name = "41128",
    .timing_family = "ram41128",
    .variants = {
        .len = 1,
        .list = {
            {"41128", NULL, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 4,
        .wid = FAM_1BIT_2RAS_128K_DELAY_SET_COLS,
        .names = {"120ns", "150ns", "200ns", "250ns"},
        .list = {
            {0,  0, 27,  4,  8,  6,  8},  // 120ns
            {0,  0, 27,  5, 11,  7, 12},  // 150ns
            {0, 11, 23,  7, 14, 12, 17},  // 200ns
            {0, 20, 23, 10, 21, 25,  9},  // 250ns
        },
    },
};

mem_chip_t *ram41128_chip() {
    return &self;
}

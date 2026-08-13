#include <stdint.h>
#include "pico/types.h"
#include "ram44256.h"
#include "mem_family/fam_4bit_1ras_256k.h"

#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint delay_set_idx, uint variant_idx) {
    get_ram_config(&self);
    self.family()->setup_pio(self.delay_sets.list[delay_set_idx]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}


static mem_chip_t self = {
    .family = fam_4bit_1ras_256k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 262144,
    .bits = 4,
    .name = "44256 (256Kx4)",
    .short_name = "44256",
    .timing_family = "ram44256",
    .variants = {
        .len = 1,
        .list = {
            {"44256", NULL, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 5,
        .wid = FAM_4BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"60ns", "70ns", "80ns", "100ns", "120ns"},
        .list = {
            {0, 0,  6, 2,  1,  7,  1,  0},  // 60ns
            {0, 0, 11, 2,  1,  7,  2,  0},  // 70ns
            {0, 0, 17, 3,  1,  7,  4,  0},  // 80ns
            {0, 0, 21, 3,  3,  7,  3,  0},  // 100ns
            {0, 0, 23, 3,  4,  7,  6,  0},  // 120ns
        },
    },
};

mem_chip_t *ram44256_chip()
{
    return &self;
};

void ram44256_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(&self);
    ram4b1r_setup_pio(self.delay_sets.list[speed_grade], variant);
}

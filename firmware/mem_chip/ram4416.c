#include <stdint.h>
#include "pico/types.h"
#include "ram4416.h"
#include "mem_family/fam_4bit_1ras_256k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
};

static int addr_func(int addr) {
    return ((addr & 0x007f) | ((addr << 1) & 0x7f00));
};

static mem_chip_t self = {
    .family = fam_4bit_1ras_256k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 16384,
    .bits = 4,
    .name = "4416 (16Kx4)",
    .short_name = "4416",
    .timing_family = "ram4416",
    .variants = {
        .len = 1,
        .list = {
            {"4416", addr_func, NULL, NULL},
        },
    },
    .delay_sets = {
        .len = 3,
        .wid = FAM_4BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"120ns", "150ns", "200ns"},
        .list = {
            {0, 0, 26, 3, 10,  7,  0,  0},  // 120ns
            {0, 0, 26, 3, 15,  3,  8,  0},  // 150ns
            {0,12, 20, 3, 21,  8,  12, 3},  // 200ns
        },
    },
};

mem_chip_t *ram4416_chip() {
    return  &self;
}
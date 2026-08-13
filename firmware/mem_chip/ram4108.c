#include <stdint.h>
#include "pico/types.h"
#include "ram4108.h"
#include "mem_chip.h"
#include "mem_family/fam_1bit_1ras_64k.h"

static mem_chip_t self;

static int addr_lo_cols(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return (addr & 0x007f) | ((addr << 2) & 0x7e00);
}

static int addr_hi_cols(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return (addr & 0x007f) | ((addr << 2) & 0x7e00) | 0x100;
}

static void setup_pio(uint delay_set_idx, uint variant_idx) {
    get_ram_config(self);
    self.family()->setup_pio(self.delay_sets.list[delay_set_idx]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}

static mem_chip_t self = {
    .family = fam_1bit_1ras_64k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 8192,
    .bits = 1,
    .name = "4108 (8Kx1 use 4116skt)",
    .short_name = "4108",
    .timing_family = "ram4108",
    .variants = {
        .len = 2,
        .list = {
            {"MK4108-40 (low)",  addr_lo_cols, NULL, NULL},
            {"MK4108-41 (high)", addr_hi_cols, NULL, NULL},
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
            {0, 8, 30, 14, 29, 22, 0, 0}, // 300ns
        },
    },
};

mem_chip_t *ram4108_chip() {
    return &self;
}
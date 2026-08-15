#include "ram44256.h"
#include "mem_family/fam_4bit_1ras_256k.h"

#define SHORT_NAME "44256"

static mem_chip_t self = {
    .get_family = fam_4bit_1ras_256k,
    .mem_size = 262144,
    .bits = 4,
    .name = SHORT_NAME " (256Kx4)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
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
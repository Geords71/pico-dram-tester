#include <stdint.h>
#include "ram41128.h"
#include "mem_family/fam_1bit_2ras_128k.h"

#define SHORT_NAME "41128"

// This RAM chip configuration
static mem_chip_t self = {
    .get_family = fam_1bit_2ras_128k,
    .mem_size = 131072,
    .bits = 1,
    .name = SHORT_NAME " (128Kx1)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
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

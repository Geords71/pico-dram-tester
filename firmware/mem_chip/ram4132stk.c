#include <stdint.h>
#include "ram4132stk.h"
#include "mem_family/fam_1bit_stacked_32k.h"

#define SHORT_NAME "4132stk"

// This RAM chip configuration
static mem_chip_t self = {
    .get_family = fam_1bit_stacked_32k,
    .mem_size = 32768,
    .bits = 1,
    .name = SHORT_NAME " (32Kx1, stacked)",
    .short_name = SHORT_NAME,
    .timing_family = "ram4116",
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
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

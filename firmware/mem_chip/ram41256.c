#include <stdint.h>
#include "ram41256.h"
#include "mem_family/fam_1bit_1ras_256k.h"

#define SHORT_NAME "41256"

// This RAM chip configuration
static mem_chip_t self = {
    .get_family = fam_1bit_1ras_256k,
    .mem_size = 262144,
    .bits = 1,
    .name = SHORT_NAME " (256Kx1)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_1BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"70ns", "80ns", "85ns", "100ns", "120ns", "150ns"},
        .list = {
            {0, 0, 10, 3,  5,  2,  0, 0},  // 70ns  - interpolated guess
            {0, 0, 13, 3,  8,  1,  0, 0},  // 80ns  - from TMS data sheet
            {0, 0, 14, 3, 10,  1,  0, 0},  // 85ns  - interpolated guess
            {0, 0, 18, 3, 13,  2,  0, 0},  // 100ns - from KM and TMS data sheets
            {0, 0, 18, 3, 13,  6,  0, 0},  // 120ns - from KM amd TMS data sheets
            {0, 0, 19, 9, 15,  4,  0, 0},  // 150ns - from TMS data sheet
        },
    },
};

mem_chip_t *ram41256_chip() {
    return &self;
}

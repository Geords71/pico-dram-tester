#include "ram2114.h"
#include "mem_family/fam_2114.h"

#define SHORT_NAME "2114"

#define ADDR_PINS  8
#define ADDR_MASK ((1u << ADDR_PINS) -1)

static mem_chip_t self = {
    .get_family = fam_2114,
    .mem_size = 4096,
    .bits = 1,
    .name = SHORT_NAME " (4Kx1 use 4116skt)",
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
        .wid = FAM_2114_DELAY_SET_COLS,
        .names = {"120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0, 31, 21, 1,  8,  9,  3,  8}, // 120ns
            {0, 31, 12, 3, 10, 14,  3,  4}, // 150ns
            {0, 31, 14, 5, 13, 21,  6,  7}, // 200ns
            {0, 20, 21, 8, 19, 23, 10, 11}, // 250ns
            {0, 20, 21, 7, 22, 27, 19,  1}  // 300ns
        },
    },
};


mem_chip_t *ram2114() {
    return &self;
}


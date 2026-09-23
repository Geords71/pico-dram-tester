#include "ram2114.h"
#include "mem_family/fam_2114.h"

#define SHORT_NAME "2114"

static mem_chip_t self = {
    .get_family = fam_2114,
    .mem_size = 1024,
    .bits = 4,
    .name = SHORT_NAME " (4416skt inverted)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
        },
    },
    .delay_sets = {
        .len = 1,
        .wid = FAM_2114_DELAY_SET_COLS,
        .names = {"250ns",},
        .list = {
            {0, 19, 0, 5,  24,  0,  0}, // 250ns
        },
    },
};


mem_chip_t *ram2114_chip() {
    return &self;
}


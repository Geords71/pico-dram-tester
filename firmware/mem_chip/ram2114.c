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
    .get_socket = socket_4416,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, NULL},
        },
    },
    .delay_sets = {
        .len = 8,
        .wid = FAM_2114_DELAY_SET_COLS,
        .names = {
            "100ns",
            "120ns",
            "150ns",
            "200ns",
            "250ns",
            "300ns",
            "450ns",
            "CMOS 250ns"
        },
        .list = {
            {0, 16,  0,  0,  0, 17,  0,  4,  0,  0,  0}, // 100ns
            {0, 16,  0,  0,  4, 16,  0,  5,  0,  0,  0}, // 120ns
            {0, 18,  0,  0,  8, 15,  0,  8,  0,  0,  0}, // 150ns
            {0, 25,  0,  0, 12, 15,  0, 19,  0,  0,  0}, // 200ns
            {0, 29,  0,  0, 20, 18,  0, 28,  0,  0,  0}, // 250ns
            {0, 30,  3,  0, 30, 22,  0, 30,  7,  0,  0}, // 300ns
            {0, 30, 15, 24, 30, 22,  0, 30, 30, 14,  0}, // 450ns
            {0, 30,  3, 16,  0, 30, 29, 25,  0,  0,  0}, // CMOS 250ns
        },
    },
};

mem_chip_t *ram2114_chip() {
    return &self;
}


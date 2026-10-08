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
    .special_instructions = "INSERT UPSIDE DOWN.",
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
            {0, 15,  0,  0,  0,  3,  0,  0, 15,  0,  0}, // 100ns
            {0, 16,  0,  4,  0,  9,  0,  0, 15,  0,  0}, // 120ns
            {0, 18,  0,  8,  0, 16,  0,  0, 14,  0,  0}, // 150ns
            {0, 25,  0, 12,  0, 27,  0,  0, 13,  0,  0}, // 200ns
            {0, 28,  0, 21,  0, 30,  6,  0, 17,  0,  0}, // 250ns
            {0, 30,  2, 30,  0, 30, 15,  0, 21,  0,  0}, // 300ns
            {0, 30, 15, 30, 24, 30, 30, 22, 21,  0,  0}, // 450ns
            {0, 30,  2, 17,  0,  0,  0,  0, 30, 28,  0}, // CMOS 250ns
        },
    },
};

mem_chip_t *ram2114_chip() {
    return &self;
}


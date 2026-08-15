#include <stdint.h>
#include "ram4027.h"
#include "mem_family/fam_1bit_1ras_256k.h"

#define SHORT_NAME "4027"

static inline const mem_family_t *get_family(){
    return fam_1bit_1ras_256k();
}

#define ADDR_PINS  6
#define ROW_MASK ((1u << ADDR_PINS) -1)

static inline int addr_func (int addr) {
    return (
        (addr & ROW_MASK) |
        (((addr >> ADDR_PINS) & ROW_MASK) << get_family()->addr_pins)
    );  
}

static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 4096,
    .bits = 1,
    .name = SHORT_NAME " (4Kx1 use 4116skt)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, addr_func},
        },
    },
    .delay_sets = {
        .len = 5,
        .wid = FAM_1BIT_1RAS_256K_DELAY_SET_COLS,
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

mem_chip_t *ram4027_chip() {
    return &self;
}

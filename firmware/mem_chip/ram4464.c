#include <stdint.h>
#include "ram4464.h"
#include "mem_family/fam_4bit_1ras_256k.h"

#define SHORT_NAME "4464"

#define ADDR_PINS  8
#define ROW_MASK ((1u << ADDR_PINS) -1)

static inline const mem_family_t *get_family(){
    return fam_4bit_1ras_256k();
}

static inline int addr_func (int addr) {
    return (
        (addr & ROW_MASK) |
        (((addr >> ADDR_PINS) & ROW_MASK) << get_family()->addr_pins)
    );  
}

static  mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 65536,
    .bits = 4,
    .name = SHORT_NAME " (64Kx4)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, addr_func},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_4BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"60ns", "70ns", "80ns", "100ns", "120ns", "150ns"},
        .list = {
            {0, 0,  6, 2,  1,  2,  1,  0},  // 60ns
            {0, 0, 11, 2,  1,  2,  2,  0},  // 70ns
            {0, 0, 17, 3,  1,  2,  4,  0},  // 80ns
            {0, 0, 14, 3,  6,  5,  9,  0},  // 100ns
            {0, 0, 23, 3,  6,  5,  6,  0},  // 120ns
            {0, 0, 26, 3, 10,  6,  10, 0},  // 150ns
        },
    },
};

mem_chip_t *ram4464_chip() {
    return &self;
};

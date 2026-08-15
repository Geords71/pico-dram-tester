#include <stdint.h>
#include "mem_family/fam_1bit_1ras_256k.h"
#include "ram4816.h"

#define SHORT_NAME "4816"

static inline const mem_family_t *get_family(){
    return fam_1bit_1ras_256k();
}

#define ADDR_PINS  7
#define ROW_MASK ((1u << ADDR_PINS) -1)

static inline int addr_func (int addr) {
    return (
        (addr & ROW_MASK) |
        (((addr >> ADDR_PINS) & ROW_MASK) << get_family()->addr_pins)
    );  
}

// This RAM chip configuration
static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 16384,
    .bits = 1,
    .name = SHORT_NAME " (16Kx1 use 4164 skt)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, addr_func},
        },
    },
    .delay_sets ={
        .len = 3,
        .wid = FAM_1BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns"},
        .list = {
            {0,  0, 23, 3, 11,  4,  0,  0}, // 100ns
            {0,  0, 25, 4, 12,  6,  1,  0}, // 120ns
            {0,  0, 27, 3, 19,  9,  0,  0}, // 150ns
        },
    },
};

mem_chip_t *ram4816_chip() {
    return &self;
}
#include <stdint.h>
#include "ram4416.h"
#include "mem_family/fam_4bit_1ras_256k.h"

#define SHORT_NAME "4416"

#define ROW_PINS  8
#define COL_PINS  6
#define COL_OFFSET  1
#define ROW_MASK ((1u << ROW_PINS) -1)
#define COL_MASK ((1u << COL_PINS) -1)

static inline const mem_family_t *get_family(){
    return fam_4bit_1ras_256k();
}

static inline int addr_func (int addr) {
    // Column address starts at A1, not A0. See 4416 data sheet.
    return (
        (addr & ROW_MASK) |
        (((addr >> ROW_PINS) & COL_MASK) << (get_family()->addr_pins + COL_OFFSET))
    );  
}

static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 16384,
    .bits = 4,
    .name = SHORT_NAME " (16Kx4)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 1,
        .list = {
            {SHORT_NAME, addr_func},
        },
    },
    .delay_sets = {
        .len = 3,
        .wid = FAM_4BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"120ns", "150ns", "200ns"},
        .list = {
            {0, 0, 26, 3, 10,  7,  0,  0},  // 120ns
            {0, 0, 26, 3, 15,  3,  8,  0},  // 150ns
            {0,12, 20, 3, 21,  8,  12, 3},  // 200ns
        },
    },
};

mem_chip_t *ram4416_chip() {
    return  &self;
}

#include <stdint.h>
#include "ram4108.h"
#include "mem_family/fam_1bit_1ras_256k.h"

#define SHORT_NAME "4108"

static inline const mem_family_t *get_family(){
    return fam_1bit_1ras_256k();
}

#define ADDR_PINS  7
#define ROW_MASK ((1u << ADDR_PINS) -1)

static inline int addr_func (int addr) {
    return (
        (addr & ROW_MASK) |
        // Pin A0 for column select dictates if we are accessing lo or hi half.
        // so the column address is shifted left by an extra bit to make room.
        (((addr >> ADDR_PINS) & ROW_MASK) << (get_family()->addr_pins + 1))
    );  
}

static inline int addr_lo_cols(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return addr_func(addr) | 0x00;
}

static inline int addr_hi_cols(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return addr_func(addr) | 0x100;
}

static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 8192,
    .bits = 1,
    .name = SHORT_NAME " (8Kx1 use 4116skt)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 2,
        .list = {
            {"MK" SHORT_NAME "-40 (low)",  addr_lo_cols},
            {"MK" SHORT_NAME "-41 (high)", addr_hi_cols},
        },
    },
    .delay_sets = {
        .len = 6,
        .wid = FAM_1BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0, 0, 18,  3,  8,  6, 0, 0}, // 100ns
            {0, 0, 19,  3, 12,  8, 0, 0}, // 120ns
            {0, 0, 19,  4, 14, 11, 0, 0}, // 150ns
            {0, 0, 23,  6, 17, 17, 6, 0}, // 200ns
            {0, 0, 30,  8, 25, 19, 0, 0}, // 250ns
            {0, 8, 30, 14, 29, 22, 0, 0}, // 300ns
        },
    },
};

mem_chip_t *ram4108_chip() {
    return &self;
}

#include <stdint.h>
#include "ram4408.h"
#include "mem_family/fam_4bit_1ras_256k.h"

#define SHORT_NAME "4408"

static inline const mem_family_t *get_family(){
    return fam_4bit_1ras_256k();
}

// A TMS4408 has:
//   7‑bit row address (A0–A6)
//   7‑bit column address (A0–A6)
// A7 is not part of either — it’s the top/bottom select bit
// A7 is 1 for Bottom and 0 for Top. Because...

// The mem family this chip is part of has nine shared row/col pins (A0-A8).
// So we need to send it an 18bit number with top nine bits for the column, and
// the bottom nine bits for the row. We'll shift our 14 bit number around
// accordingly.

#define ROW_PINS  8
#define COL_PINS  6
#define ROW_MASK ((1u << ROW_PINS) -1)
#define COL_MASK ((1u << COL_PINS) -1)

static inline int addr_func (int addr) {
    return (
        (addr & ROW_MASK) |
        (((addr >> ROW_PINS) & COL_MASK) << get_family()->addr_pins)
    );  
}

static inline int addr_t(int addr)
{
    return addr_func(addr) | 0x00;  
}

static inline int addr_b(int addr)
{
    return addr_func(addr) | 0x80;  
}

static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 8192,
    .bits = 4,
    .name = SHORT_NAME " (8Kx4 use 4416skt)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 2,
        .list = {
            {"TMS" SHORT_NAME "T (Top)",    addr_t},
            {"TMS" SHORT_NAME "B (Bottom)", addr_b},
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
        }
    },
};

mem_chip_t *ram4408_chip() {
    return &self;
}

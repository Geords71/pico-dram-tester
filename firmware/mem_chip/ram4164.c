#include <stdint.h>
#include "ram4164.h"
#include "mem_family/fam_1bit_1ras_256k.h"

#define SHORT_NAME "4164"

static inline const mem_family_t *get_family(){
    return fam_1bit_1ras_256k();
}

#define ADDR_PINS 8
#define ADDR_MASK ((1u << ADDR_PINS) -1)

static inline int addr_func (int addr) {
    return (
        (addr & ADDR_MASK) |
        (((addr >> ADDR_PINS) & ADDR_MASK) << get_family()->addr_pins)
    );  
}

// This RAM chip configuration
static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 65536,
    .bits = 1,
    .name = SHORT_NAME " (64Kx1)",
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
        .wid = FAM_1BIT_1RAS_256K_DELAY_SET_COLS,
        .names = {"100ns", "120ns", "150ns", "200ns", "250ns", "300ns"},
        .list = {
            {0,  0, 15, 3, 11,  4,  0, 0}, // 100ns - tested on km4164b-10 No margin applied yet...
            {0,  0, 14, 3, 15,  4,  0, 0}, // 120ns - tighter of all Mnfctr values and -10% margin applied
            {0,  0, 18, 4, 20,  4,  0, 0}, // 150ns - tighter of all Mnfctr values and -15% margin applied
            {0,  0, 24, 4, 20, 17,  0, 0}, // 200ns
            {0,  2, 30, 8, 26, 20,  0, 0}, // 250ns
            {0, 10, 30, 8, 26, 23,  0, 0}  // 300ns - Complete guess! But very rare chip.
        },
    },
};

mem_chip_t *ram4164_chip() {
    return &self;
};

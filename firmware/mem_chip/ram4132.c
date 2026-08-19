#include <stdint.h>
#include "ram4132.h"
#include "mem_family/fam_1bit_1ras_256k.h"

#define SHORT_NAME "4132"

static inline const mem_family_t *get_family(){
    return fam_1bit_1ras_256k();
}

#define ROW_PINS_4532  7
#define COL_PINS_4532  8
#define ROW_MASK_4532 ((1u << ROW_PINS_4532) -1)
#define COL_MASK_4532 ((1u << COL_PINS_4532) -1)

static inline int addr_4532_lo(int addr) {
    // A7 on row selects the good half: 0->lo and 1->hi.
    // A7 is implicitly 'lo' for the row address here.
    return (
        (addr & ROW_MASK_4532) |
        (((addr >> ROW_PINS_4532) & COL_MASK_4532) << get_family()->addr_pins)
    );  
}

static inline int addr_4532_hi(int addr) {
    // Re-use the 'lo' logic and just OR the bit to select the high rows!
    return (addr_4532_lo(addr) | (1u << ROW_PINS_4532));
}

#define ROW_PINS_3732  8
#define COL_PINS_3732  7
#define ROW_MASK_3732 ((1u << ROW_PINS_3732) -1)
#define COL_MASK_3732 ((1u << COL_PINS_3732) -1)

// One Column Address (A7) has to be fixed at logic 0
// (low level) for MSM3732L, and at logic 1 (high level)
// for MSM3732H.
static inline int addr_3732_lo(int addr) {
    // A7 on column selects the good half: 0->lo and 1->hi.
    // A7 is implicitly 'lo' for the column address here.
    return (
        (addr & ROW_MASK_3732) |
        (((addr >> ROW_PINS_3732) & COL_MASK_3732) << get_family()->addr_pins)
    );  
}

static inline int addr_3732_hi(int addr) {
    // Re-use the 'lo' logic and just OR the bit to select the high rows!
    return (addr_3732_lo(addr) | (1u << (get_family()->addr_pins + COL_PINS_3732)));
}

static mem_chip_t self = {
    .get_family = get_family,
    .mem_size = 32768,
    .bits = 1,
    .name = SHORT_NAME " (32Kx1 use 4164skt)",
    .short_name = SHORT_NAME,
    .timing_family = "ram" SHORT_NAME,
    .variants = {
        .len = 4,
        .list = {
            // There are a lot of vague web sites on what is valid
            // But looking at schemtic for issue 3, we can infer that TMS
            // uses A7 RAS as it is swapped at the muxer pins
            // when compoared to oki. And we have a data sheet for that. :-)
            {"TMS4532xxNL3 (low)",  addr_4532_lo},
            {"TMS4532xxNL4 (high)", addr_4532_hi},
            {"M3732L (low)",  addr_3732_lo},
            {"M3732H (high)", addr_3732_hi},
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

mem_chip_t *ram4132_chip() {
    return &self;
}

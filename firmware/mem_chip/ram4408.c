#include <stdint.h>
#include "pico/types.h"
#include "ram4408.h"
#include "mem_family/fam_4bit_1ras_256k.h"
#include "mem_chip.h"

static mem_chip_t self;

static void setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(self);
    self.family()->setup_pio(self.delay_sets.list[speed_grade]);
}

static void teardown_pio() {
    self.family()->teardown_pio();
}

// A TMS4408 has:
//   7‑bit row address (A0–A6)
//   7‑bit column address (A0–A6)
// A7 is not part of either — it’s the top/bottom select bit
// A7 is 1 for Bottom and 0 for Top. Because...

static int addr_4408t(int addr)
{
    //     Row               Column                   Bank select: A7=0
    return (addr & 0x007f) | ((addr << 1) & 0x7f00) | 0x00;  
}

static int addr_4408b(int addr)
{
    //     Row               Column                   Bank select: A7=1
    return (addr & 0x007f) | ((addr << 1) & 0x7f00) | 0x80;  
}

static mem_chip_t self = {
    .family = fam_4bit_1ras_256k,
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .ram_read = NULL,
    .ram_write = NULL,
    .mem_size = 8192,
    .bits = 4,
    .name = "4408 (8Kx4 use 4416skt)",
    .short_name = "4408",
    .timing_family = "ram4408",
    .variants = {
        .len = 2,
        .list = {
            {"TMS4408T (Top)",    addr_4408t, NULL, NULL},
            {"TMS4408B (Bottom)", addr_4408b, NULL, NULL},
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

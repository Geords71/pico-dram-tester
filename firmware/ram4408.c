#include <stdint.h>
#include "pico/types.h"
#include "ram4408.h"
#include "ram4b1r.pio.h"
#include "mem_chip.h"

#define RAM4408_DELAY_SET_ROWS 3
static const delay_sets_t ram4408_delay_sets = {
    .len = RAM4408_DELAY_SET_ROWS,
    .wid = RAM4B1R_DELAY_SET_COLS,
    .list = {
        {0, 0, 26, 3, 10,  7,  0,  0},  // 120ns
        {0, 0, 26, 3, 15,  3,  8,  0},  // 150ns
        {0,12, 20, 3, 21,  8,  12, 3},  // 200ns
    }
};

// A7 low (only for row address)
int ram44080_read(int addr)
{
    // CCCCCCRRRRRRR
    pio_sm_put(pio, sm, 0 |                      // Fast page mode flag
                        (0 << 1) |               // Write flag
                        (1 << 6) |               // Initial OE is high
                        ((addr & 0x07f) << 7) |  // Row address
                        (0 << 20) |              // Final OE is low (for read)
                        ((addr >> 7) << 22));    // Column address. Note that it starts at A1, not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    return pio_sm_get(pio, sm);                 // Return the data
}

int ram44081_read(int addr)
{
    // CCCCCCRRRRRRR
    pio_sm_put(pio, sm, 0 |                      // Fast page mode flag
                        (0 << 1) |               // Write flag
                        (1 << 6) |               // Initial OE is high
                        ((addr & 0x07f | 0x80) << 7) | // Row address
                        (0 << 20) |              // Final OE is low (for read)
                        ((addr >> 7) << 22));    // Column address. Note that it starts at A1, not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    return pio_sm_get(pio, sm);                 // Return the data

}

void ram44080_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        (1 << 1) |              // Write flag
                        (1 << 6) |              // OE is high
                        ((addr & 0x7f) << 7) | // Row address
                        ((data & 0xf) << 16) |  // Data nibble
                        (1 << 20) |             // Final OE is still high (write mode)
                        ((addr >> 7) << 22));   // Column address. Note it starts at A1 not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

void ram44081_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        (1 << 1) |              // Write flag
                        (1 << 6) |              // OE is high
                        ((addr & 0x7f | 0x80) << 7) | // Row address
                        ((data & 0xf) << 16) |  // Data nibble
                        (1 << 20) |             // Final OE is still high (write mode)
                        ((addr >> 7) << 22));   // Column address. Note it starts at A1 not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

// Routines to set up and tear down the PIO program (and the RAM test)



static const mem_chip_variants_t ram4408_chip_variants = {
    .len = 2,
    .list = {
        {"TMS4408T (low)",  ram44080_read, ram44080_write},
        {"TMS4408B (high)", ram44081_read, ram44081_write},
    },
};

void ram4408_setup_pio(uint speed_grade, uint variant);

mem_chip_t ram4408_chip = {
    .setup_pio = ram4408_setup_pio,
    .teardown_pio = ram4b1r_teardown_pio,
    .ram_read = ram44080_read,
    .ram_write = ram44080_write,
    .mem_size = 8192,
    .bits = 4,
    .variants = &ram4408_chip_variants,
    .name = "4408 (8Kx4 use 4416skt)",
    .timing_family = "ram4408",
    .delay_sets = ram4408_delay_sets,
    .speed_names = {"120ns", "150ns", "200ns"}
};

// Only used for half-qualified 4408 devices
void ram4408_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram4408_chip);
    ram4408_chip.ram_read = ram4408_chip_variants.list[variant].ram_read;
    ram4408_chip.ram_write = ram4408_chip_variants.list[variant].ram_write;
    ram4b1r_setup_pio(ram4408_delay_sets.list[speed_grade], variant);
}
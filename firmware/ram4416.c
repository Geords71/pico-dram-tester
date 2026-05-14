#include <stdint.h>
#include "pico/types.h"
#include "ram4416.h"
#include "ram4b1r.pio.h"
#include "mem_chip.h"

#define RAM4416_DELAY_SET_ROWS 3

static const delay_sets_t ram4416_delay_sets = {
    .len = RAM4416_DELAY_SET_ROWS,
    .wid = RAM4B1R_DELAY_SET_COLS,
    .list = {
        {0, 0, 26, 3, 10,  7,  0,  0},  // 120ns
        {0, 0, 26, 3, 15,  3,  8,  0},  // 150ns
        {0,12, 20, 3, 21,  8,  12, 3},  // 200ns
    }
};

int ram4416_ram_read(int addr)
{
    uint d;
    // CCCCCCRRRRRRRR
    pio_sm_put(pio, sm, 0 |                      // Fast page mode flag
                        (0 << 1) |               // Write flag
                        (1 << 6) |               // Initial OE is high
                        ((addr & 0x0ff) << 7) |  // Row address
                        (0 << 20) |              // Final OE is low (for read)
                        ((addr >> 8) << 22));    // Column address. Note that it starts at A1, not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    d = pio_sm_get(pio, sm);                 // Return the data
    return d;
}

void ram4416_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        (1 << 1) |              // Write flag
                        (1 << 6) |              // OE is high
                        ((addr & 0xff) << 7) | // Row address
                        ((data & 0xf) << 16) |  // Data nibble
                        (1 << 20) |             // Final OE is still high (write mode)
                        ((addr >> 8) << 22));   // Column address. Note it starts at A1 not A0.

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

void ram4416_setup_pio(uint speed_grade, uint variant);

const mem_chip_t ram4416_chip  = {
    .setup_pio = ram4416_setup_pio,
    .teardown_pio = ram4b1r_teardown_pio,
    .ram_read = ram4416_ram_read,
    .ram_write = ram4416_ram_write,
    .mem_size = 16384,
    .bits = 4,
    .variants = NULL,
    .name = "4416 (16Kx4)",
    .timing_family = "ram4416",
    .delay_sets = ram4416_delay_sets,
    .speed_names = {"120ns", "150ns", "200ns"},
};

void ram4416_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram4416_chip);
    ram4b1r_setup_pio(ram4416_delay_sets.list[speed_grade], variant);
}
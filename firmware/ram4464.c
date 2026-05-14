#include <stdint.h>
#include "pico/types.h"
#include "ram4464.h"
#include "ram4b1r.pio.h"
#include "mem_chip.h"

#define RAM4464_DELAY_SET_ROWS 6

static const delay_sets_t ram4464_delay_sets = {
    .len = RAM4464_DELAY_SET_ROWS,
    .wid = RAM4B1R_DELAY_SET_COLS,
    .list = {
        {0, 0,  6, 2,  1,  2,  1,  0},  // 60ns
        {0, 0, 11, 2,  1,  2,  2,  0},  // 70ns
        {0, 0, 17, 3,  1,  2,  4,  0},  // 80ns
        {0, 0, 14, 3,  6,  5,  9,  0},  // 100ns
        {0, 0, 23, 3,  6,  5,  6,  0},  // 120ns
        {0, 0, 26, 3, 10,  6,  10, 0},  // 150ns
    },
};

int ram4464_ram_read(int addr)
{
    uint d;
    pio_sm_put(pio, sm, 0 |                      // Fast page mode flag
                        (0 << 1) |               // Write flag
                        (1 << 6) |               // Initial OE is high
                        ((addr & 0x0ff) << 7) |  // Row address
                        (0 << 20) |              // Final OE is low (for read)
                        ((addr >> 8) << 21));    // Column address

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    d = pio_sm_get(pio, sm);                 // Return the data
    return d;
}

void ram4464_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        (1 << 1) |              // Write flag
                        (1 << 6) |              // OE is high
                        ((addr & 0xff) << 7) | // Row address
                        ((data & 0xf) << 16) |  // Data nibble
                        (1 << 20) |             // Final OE is still high (write mode)
                        ((addr >> 8) << 21));   // Column address

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

void ram4464_setup_pio(uint speed_grade, uint variant);

const mem_chip_t ram4464_chip  = {
    .setup_pio = ram4464_setup_pio,
    .teardown_pio = ram4b1r_teardown_pio,
    .ram_read = ram4464_ram_read,
    .ram_write = ram4464_ram_write,
    .mem_size = 65536,
    .bits = 4,
    .variants = NULL,
    .name = "4464 (64Kx4)",
    .timing_family = "ram4464",
    .delay_sets = ram4464_delay_sets,
    .speed_names = {"60ns", "70ns", "80ns", "100ns", "120ns", "150ns"}
};

void ram4464_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram4464_chip);
    ram4b1r_setup_pio(ram4464_delay_sets.list[speed_grade], variant);
}

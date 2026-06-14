
#include <stdint.h>
#include "pico/types.h"
#include "ram44256.h"
#include "ram4b1r.pio.h"
#include "mem_chip.h"

#define RAM44256_DELAY_SET_ROWS 5

static const delay_sets_t ram44256_delay_sets = {
    .len = RAM44256_DELAY_SET_ROWS,
    .wid = RAM4B1R_DELAY_SET_COLS,
    .list = {
        {0, 0,  6, 2,  1,  7,  1,  0},  // 60ns
        {0, 0, 11, 2,  1,  7,  2,  0},  // 70ns
        {0, 0, 17, 3,  1,  7,  4,  0},  // 80ns
        {0, 0, 21, 3,  3,  7,  3,  0},  // 100ns
        {0, 0, 23, 3,  4,  7,  6,  0},  // 120ns
    },
};

int ram44256_ram_read(int addr)
{
    uint d;
    // fpm flag, write flag, 14 bits of data, oe, rasaddr, 14 bits of data, oe, casaddr
    // aaaaaaaaaodddd_aaaaaaaaaoddddwf

    pio_sm_put(pio, sm, 0 |                      // Fast page mode flag
                        (0 << 1) |               // Write flag
                        (1 << 6) |               // Initial OE is high
                        ((addr & 0x1ff) << 7) |  // Row address
                        (0 << 20) |              // Final OE is low (for read)
                        ((addr >> 9) << 21));    // Column address

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    d = pio_sm_get(pio, sm);                 // Return the data
    return d;
}

void ram44256_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        (1 << 1) |              // Write flag
                        (1 << 6) |              // OE is high
                        ((addr & 0x1ff) << 7) | // Row address
                        ((data & 0xf) << 16) |  // Data nibble
                        (1 << 20) |             // Final OE is still high (write mode)
                        ((addr >> 9) << 21));   // Column address

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

void ram44256_setup_pio(uint speed_grade, uint variant);

const mem_chip_t ram44256_chip = {
    .setup_pio = ram44256_setup_pio,
    .teardown_pio = ram4b1r_teardown_pio,
    .ram_read = ram44256_ram_read,
    .ram_write = ram44256_ram_write,
    .mem_size = 262144,
    .bits = 4,
    .variants = NULL,
    .name = "44256 (256Kx4)",
    .timing_family = "ram44256",
    .delay_sets = ram44256_delay_sets,
    .speed_names = {"60ns", "70ns", "80ns", "100ns", "120ns"}
};

void ram44256_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram44256_chip);
    ram4b1r_setup_pio(ram44256_delay_sets.list[speed_grade], variant);
}
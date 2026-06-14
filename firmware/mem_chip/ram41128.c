
#include <stdint.h>
#include "pico/types.h"
#include "mem_chip.h"
#include "ram41128.h"
#include "ram41128.pio.h"
#define RAM41128_DELAY_SET_ROWS 4
#define GPIO_LED 25

static const delay_sets_t ram41128_delay_sets = {
    .len = RAM41128_DELAY_SET_ROWS,
    .wid = RAM41128_DELAY_SET_COLS,
    .list = {
        {0,  0, 27,  4,  8,  6,  8},  // 120ns
        {0,  0, 27,  5, 11,  7, 12},  // 150ns
        {0, 11, 23,  7, 14, 12, 17},  // 200ns
        {0, 20, 23, 10, 21, 25,  9},  // 250ns
    },
};

static inline void ram41128_program_init(PIO pio, uint sm, uint offset, uint pin) {
    uint count;

    // Set up 17 total pins
    for (count = 0; count < 17; count++) {
        pio_gpio_init(pio, pin + count);
        gpio_set_slew_rate(pin + count, GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(pin + count, GPIO_DRIVE_STRENGTH_4MA);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 13, true); // true=output
    pio_sm_set_consecutive_pindirs(pio, sm, pin + 16, 1, false); // input

    pio_sm_set_clkdiv(pio, sm, 1); // should just be the default.

    pio_sm_config c = ram41128_program_get_default_config(offset);
// A0, A1, A2, A3, A4, A5, A6, A7, nc, D, WR, RAS, CAS, nc, nc, nc, IN
    sm_config_set_out_pins(&c, pin, 9);
    sm_config_set_set_pins(&c, pin + 9, 4); // Max is 5.
    sm_config_set_in_pins(&c, pin + 16);

    // Shift right, Autopull off, 19 bits (1 + 1 + 8 + 9) at a time
    sm_config_set_out_shift(&c, true, false, 19);
    // Shift left, Autopush on, 1 bit
    sm_config_set_in_shift(&c, false, true, 1);

 //   hw_set_bits(&pio->input_sync_bypass, 1u << pin); to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Routines for reading and writing memory through the FIFOs
int ram41128_ram_read(int addr)
{
    uint d;
    // daaaaaaaa_aaaaaaaawf
    // ccccccccrrrrrrrrb
    // Note: Initially tried addr MSB as the bank select
    // but this may be too slow to self refresh correctly.
    pio_sm_put(pio, sm, (addr) & 1 |                 // Use 2nd RAS line? addr >> 16
                        0 << 1 |                     // Write flag
                        ((addr >> 1) & 0xff) << 2 |  // Row address addr >> 0
                        ((addr >> 9) & 0xff) << 10 | // Column address addr >> 9
                        ((0 & 1) << 18));            // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}      // Wait for data to arrive
    d = pio_sm_get(pio, sm);                         // Return the data
    gpio_put(GPIO_LED, d);
    return d;
}

void ram41128_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, (addr) & 1 |                 // Use 2nd RAS line?
                        1 << 1 |                     // Write flag
                        ((addr >> 1) & 0xff) << 2 |  // Row address
                        ((addr >> 9) & 0xff) << 10 | // Column address
                        ((data & 1) << 18));         // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}      // Wait for dummy data
    pio_sm_get(pio, sm);                             // Discard the dummy data bit
}

// Routines to set up and tear down the PIO program (and the RAM test)
void ram41128_setup_pio(uint speed_grade, uint variant);

void ram41128_teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&ram41128_program, pio, sm, offset);
}

// This RAM chip configuration
const mem_chip_t ram41128_chip = {
    .setup_pio = ram41128_setup_pio,
    .teardown_pio = ram41128_teardown_pio,
    .ram_read = ram41128_ram_read,
    .ram_write = ram41128_ram_write,
    .mem_size = 131072, // 131072
    .bits = 1,
    .variants = NULL,
    .delay_sets = ram41128_delay_sets,
    .name = "41128 (128Kx1)",
    .timing_family = "ram41128",
    .speed_names = {"120ns", "150ns", "200ns", "250ns"}
};

void ram41128_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram41128_chip);
    const uint8_t *delay_set = ram41128_delay_sets.list[speed_grade];

    uint pin = 5;

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &ram41128_program, delay_set, RAM41128_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true
    );

    ram41128_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
}
#include <stdint.h>
#include "hardware/pio.h"
#include "fam_1bit_2ras_128k.h"
#include "fam_1bit_2ras_128k.pio.h"

static PIO pio;
static uint sm = 0;
static uint offset; // Returns offset of starting instruction

static void setup_pio(const uint8_t *delay_set)
{
    uint pin = 5;

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &fam_1bit_2ras_128k_program, delay_set, FAM_1BIT_2RAS_128K_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true
    );

    // Set up 17 total pins
    for (uint count = 0; count < 17; count++) {
        pio_gpio_init(pio, pin + count);
        gpio_set_slew_rate(pin + count, GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(pin + count, GPIO_DRIVE_STRENGTH_4MA);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 13, true); // true=output
    pio_sm_set_consecutive_pindirs(pio, sm, pin + 16, 1, false); // input

    pio_sm_set_clkdiv(pio, sm, 1); // should just be the default.

    pio_sm_config c = fam_1bit_2ras_128k_program_get_default_config(offset);

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

static void teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&fam_1bit_2ras_128k_program, pio, sm, offset);
}

// Routines for reading and writing memory through the FIFOs
static int read(int (*addr_func)(int addr), int addr) {
    addr = (addr_func != NULL) ? addr_func(addr) : addr;

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
    return d;
}

static void write(int (*addr_func)(int addr), int addr, int data) {
    addr = (addr_func != NULL) ? addr_func(addr) : addr;

    pio_sm_put(pio, sm, (addr) & 1 |                 // Use 2nd RAS line?
                        1 << 1 |                     // Write flag
                        ((addr >> 1) & 0xff) << 2 |  // Row address
                        ((addr >> 9) & 0xff) << 10 | // Column address
                        ((data & 1) << 18));         // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}      // Wait for dummy data
    pio_sm_get(pio, sm);                             // Discard the dummy data bit
}

static const mem_family_t self = {
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .read = read,
    .write = write,
    .bits = 1,
};

inline const mem_family_t *fam_1bit_2ras_128k() {
    return &self;
}
#include <stdint.h>
#include "hardware/pio.h"
#include "fam_1bit_stacked_32k.h"
#include "fam_1bit_stacked_32k.pio.h"

static PIO pio;
static uint sm = 0;
static uint offset; // Returns offset of starting instruction

// Routines to set up and tear down the PIO program (and the RAM test)
static void setup_pio(const uint8_t *delay_set)
{

    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &fam_1bit_stacked_32k_program, delay_set, FAM_1BIT_STACKED_32K_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true);

    // Set up 17 total pins
    for (uint count = 0; count < 17; count++) {
        pio_gpio_init(pio, pin + count);
        gpio_set_slew_rate(pin + count, GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(pin + count, GPIO_DRIVE_STRENGTH_4MA);
    }

    pio_sm_set_consecutive_pindirs(pio, sm, pin, 16, true); // true=output
    pio_sm_set_consecutive_pindirs(pio, sm, pin + 16, 1, false); // input

    pio_sm_set_clkdiv(pio, sm, 1); // should just be the default.

    pio_sm_config c = fam_1bit_stacked_32k_program_get_default_config(offset);

    // A0, A1, A2, A3, A4, A5, A6, A7, nc, D, nc, RAS1, RAS2, CAS1, CAS2, WE, IN
    sm_config_set_out_pins(&c, pin, 10);
    sm_config_set_set_pins(&c, pin + 10, 5); // Max is 5.
    sm_config_set_in_pins(&c, pin + 16);

    // Shift right, Autopull off, 21 bits (1 + 1 + 9 + 10) at a time
    sm_config_set_out_shift(&c, true, false, 21);

    // Shift left, Autopush on, 1 bit
    sm_config_set_in_shift(&c, false, true, 1);

 //   hw_set_bits(&pio->input_sync_bypass, 1u << pin); to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static void teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&fam_1bit_stacked_32k_program, pio, sm, offset);
}

static int read(int (*addr_func)(int addr), int addr)
{
    uint d;
    // Adress format: c=column, r=row, b=bankselect:
    // cccccccrrrrrrrb

    // PIO message format: d=data, a=address, w=write, f=RAS_select
    // daaaaaaaaa_aaaaaaaaawf

    // Note: Initially tried addr MSB as the bank select
    // but this may be too slow to self refresh correctly.

    pio_sm_put(
        pio,
        sm,
        (addr) & 1                 | // Use 2nd RAS line for odd numbered addresses
        0 << 1                     | // Write flag off
        ((addr >> 1) & 0x7f) << 2  | // Row address
        ((addr >> 8) & 0x7f) << 11 | // Column address
        ((0 & 1) << 20)              // Data bit
    ); 

    // Wait for data to arrive
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}

    // Return the data
    d = pio_sm_get(pio, sm);
    //gpio_put(GPIO_LED, d);
    return d;
}

static void write(int (*addr_func)(int addr), int addr, int data)
{
    pio_sm_put(pio, sm, (addr) & 1 |                 // Use 2nd RAS line?
                        1 << 1 |                     // Write flag
                        ((addr >> 1) & 0x7f) << 2 |  // Row address
                        ((addr >> 8) & 0x7f) << 11 | // Column address
                        ((data & 1) << 20));         // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}      // Wait for dummy data
    pio_sm_get(pio, sm);                             // Discard the dummy data bit
}

static const mem_family_t self = {
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .read = read,
    .write = write,
};

const mem_family_t *fam_1bit_stacked_32k() {
    return &self;
}
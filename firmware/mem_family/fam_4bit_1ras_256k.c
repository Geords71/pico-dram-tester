#include <stdint.h>
#include "hardware/pio.h"
#include "fam_4bit_1ras_256k.h"
#include "fam_4bit_1ras_256k.pio.h"

static PIO pio;
static uint sm = 0;
static uint offset; // Returns offset of starting instruction

static const mem_family_t self;

static void teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&fam_4bit_1ras_256k_program, pio, sm, offset);
}

static void setup_pio(const uint8_t *delay_set)
{
    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &fam_4bit_1ras_256k_program, delay_set, FAM_4BIT_1RAS_256K_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true);
    
    // Set up 17 total pins
    for (uint count = 0; count < 17; count++) {
        pio_gpio_init(pio, pin + count);
        gpio_set_slew_rate(pin + count, GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(pin + count, GPIO_DRIVE_STRENGTH_4MA);
    }

    pio_sm_set_consecutive_pindirs(pio, sm, pin + 4, 17, true); // true=output
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 4, false); // false=input

    pio_sm_set_clkdiv(pio, sm, 1); // should just be the default.

    pio_sm_config c = fam_4bit_1ras_256k_program_get_default_config(offset);

    sm_config_set_out_pins(&c, pin, 14);
    sm_config_set_set_pins(&c, pin + 14, 3); // Max is 5.
    sm_config_set_in_pins(&c, pin);

    // Shift right, Autopull off, 30 bits (1 + 1 + 14 + 14) at a time
    sm_config_set_out_shift(&c, true, false, 30);

    // Shift left, Autopull on, 4 bits
    sm_config_set_in_shift(&c, false, false, 4);

    hw_set_bits(&pio->input_sync_bypass, 0xf << pin); //to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static int read(int (*addr_func)(int addr), int addr)
{
    addr = (addr_func != NULL) ? addr_func(addr) : addr;

    // aaaaaaaaaodddd_aaaaaaaaaoddddwf
    pio_sm_put(
        pio, sm, 0            | // Fast page mode flag
        (0 << 1)              | // Write flag
        (1 << 6)              | // Initial OE is high
        ((addr & 0x1ff) << 7) | // Row address
        (0 << 20)             | // Final OE is low (for read)
        ((addr >> 9) << 21)     // Column address
    );

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}  // Wait for data to arrive
    uint d = pio_sm_get(pio, sm);                // Return the data
    return d;
}

static void write(int (*addr_func)(int addr), int addr, int data)
{
    addr = (addr_func != NULL) ? addr_func(addr) : addr;

    pio_sm_put(
        pio, sm, 0            | // Fast page mode flag
        (1 << 1)              | // Write flag
        (1 << 6)              | // OE is high
        ((addr & 0x1ff) << 7) | // Row address
        ((data & 0xf) << 16)  | // Data nibble
        (1 << 20)             | // Final OE is still high (write mode)
        ((addr >> 9) << 21)     // Column address
    );

    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

static const mem_family_t self = {
    .setup_pio = &setup_pio,
    .teardown_pio = &teardown_pio,
    .read = &read,
    .write = &write,
    .addr_pins = 9,
};

inline const mem_family_t *fam_4bit_1ras_256k() {
    return &self;
}
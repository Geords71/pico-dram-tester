
#include <stdint.h>
#include "pico/types.h"
#include "ram41256.h"
#include "ram41256.pio.h"
#include "mem_chip.h"

#define RAM41256_DELAY_SET_ROWS 6

static const delay_sets_t ram41256_delay_sets = {
    .len = RAM41256_DELAY_SET_ROWS,
    .wid = RAM41256_DELAY_SET_COLS,
    .list = {
        {0, 0, 10, 3,  5,  2,  0, 0},  // 70ns  - interpolated guess
        {0, 0, 13, 3,  8,  1,  0, 0},  // 80ns  - from TMS data sheet
        {0, 0, 14, 3, 10,  1,  0, 0},  // 85ns  - interpolated guess
        {0, 0, 18, 3, 13,  2,  0, 0},  // 100ns - from KM and TMS data sheets
        {0, 0, 18, 3, 13,  6,  0, 0},  // 120ns - from KM amd TMS data sheets
        {0, 0, 19, 9, 15,  4,  0, 0},  // 150ns - from TMS data sheet
    },
};

static inline void ram41256_program_init(PIO pio, uint sm, uint offset, uint pin) {
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

    pio_sm_config c = ram41256_program_get_default_config(offset);
    // A0, A1, A2, A3, A4, A5, A6, A7, nc, D, WR, RAS, CAS, nc, nc, nc, IN
    sm_config_set_out_pins(&c, pin, 10);
    sm_config_set_set_pins(&c, pin + 10, 3); // Max is 5.
    sm_config_set_in_pins(&c, pin + 16);

    // Shift right, Autopull off, 20 bits (1 + 1 + 8 + 10) at a time
    sm_config_set_out_shift(&c, true, false, 20);
    // Shift left, Autopull on, 1 bit
    sm_config_set_in_shift(&c, false, false, 1);

    hw_set_bits(&pio->input_sync_bypass, 1u << (pin + 16)); //to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Routines for reading and writing memory through the FIFOs
int ram41256_ram_read(int addr)
{
    uint d;
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        0 << 1 |                // Write flag
                        (addr & 0x1ff) << 2 |    // Row address
                        (addr >> 9) << 11|   // Column address
                        ((0 & 1) << 20));       // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for data to arrive
    d = pio_sm_get(pio, sm);                 // Return the data
    //gpio_put(GPIO_LED, d);
    return d;
}

void ram41256_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                        1 << 1 |                // Write flag
                        (addr & 0x1ff) << 2 |    // Row address
                        (addr >> 9) << 11|   // Column address
                        ((data & 1) << 20));    // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
    pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

static const mem_chip_variants_t ram41256_variants = {
    .len = 1,
    .list = {
        {"41128", ram41256_ram_read, ram41256_ram_write},
    },
};

void ram41256_setup_pio(uint speed_grade, uint variant);

void ram41256_teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&ram41256_program, pio, sm, offset);
}

// This RAM chip configuration
const mem_chip_t ram41256_chip = {
    .setup_pio = ram41256_setup_pio,
    .teardown_pio = ram41256_teardown_pio,
    .ram_read = ram41256_ram_read,
    .ram_write = ram41256_ram_write,
    .mem_size = 262144,
    .bits = 1,
    .variants = &ram41256_variants,
    .name = "41256 (256Kx1)",
    .short_name = "41256",
    .timing_family = "ram41256",
    .delay_sets = ram41256_delay_sets,
    .speed_names = {"70ns", "80ns", "85ns", "100ns", "120ns", "150ns"}
};

void ram41256_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram41256_chip);

    const uint8_t *delay_set = ram41256_delay_sets.list[speed_grade];

    uint pin = 5;

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &ram41256_program, delay_set, RAM41256_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true
    );
    ram41256_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
}
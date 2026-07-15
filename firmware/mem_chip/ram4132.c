#include <stdint.h>
#include "pico/types.h"
#include "ram4132.h"
#include "ram4132.pio.h"
#include "mem_chip.h"

#define RAM4132_DELAY_SET_ROWS 4

static const delay_sets_t ram4132_delay_sets = {
    .len = RAM4132_DELAY_SET_ROWS,
    .wid = RAM4132_DELAY_SET_COLS,
    .list = {
        {0, 31, 31, 4, 11, 11,  9}, // 150ns
        {0, 23, 24, 5, 14, 18, 13}, // 200ns
        {0, 21, 22, 8, 20, 21, 16}, // 250ns
        {0, 21, 22, 8, 23, 25, 24}, // 300ns}
    },
};

void ram4132_teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&ram4132_program, pio, sm, offset);
}

int ram4132_ram_read(int addr)
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

void ram4132_ram_write(int addr, int data)
{
    pio_sm_put(pio, sm, (addr) & 1 |                 // Use 2nd RAS line?
                        1 << 1 |                     // Write flag
                        ((addr >> 1) & 0x7f) << 2 |  // Row address
                        ((addr >> 8) & 0x7f) << 11 | // Column address
                        ((data & 1) << 20));         // Data bit
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}      // Wait for dummy data
    pio_sm_get(pio, sm);                             // Discard the dummy data bit
}


void ram4132_setup_pio(uint speed_grade, uint variant);

// This RAM chip configuration
const mem_chip_t ram4132_stk_chip = {
    .setup_pio = ram4132_setup_pio,
    .teardown_pio = ram4132_teardown_pio,
    .ram_read = ram4132_ram_read,
    .ram_write = ram4132_ram_write,
    .mem_size = 32768,
    .bits = 1,
    .variants = NULL,
    .name = "4132 (32Kx1, stacked)",
    .short_name = "4132stk",
    .timing_family = "ram4132",
    .delay_sets = ram4132_delay_sets,
    .speed_names = {"150ns", "200ns", "250ns", "300ns"},
};


// Routines to set up and tear down the PIO program (and the RAM test)
void ram4132_setup_pio(uint speed_grade, uint variant)
{
    get_ram_config(ram4132_stk_chip);

    const uint8_t *delay_set = ram4132_delay_sets.list[speed_grade];

    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &ram4132_program, delay_set, RAM4132_DELAY_SET_COLS
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

    pio_sm_config c = ram4132_program_get_default_config(offset);

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

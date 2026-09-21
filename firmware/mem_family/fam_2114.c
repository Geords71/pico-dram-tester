#include "fam_2114.h"
#include "fam_2114.pio.h"
#include <stdint.h>

static PIO pio;
static uint sm = 0;
static uint offset; // Returns offset of starting instruction

static const mem_family_t self;

#define ADDR_MAP_LEN 10
static const uint8_t addr_map[ADDR_MAP_LEN] = {
     5,
     2,
    15,
     6,
     7,
     8,
    12,
     9,
    10,
    11,
};

#define DATA_MAP_LEN 4
static const uint8_t data_map[DATA_MAP_LEN] = {
    14,
    16,
     1,
     0,
};

// 2114 is a bit scattered compared to the dram pin sequences so we'll use
// a mask. 1s denote input to pico.
#define PIN_DIR_MASK 0b10100000000000011

static void setup_pio(const uint8_t *delay_set) {

    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &fam_2114_program, delay_set, FAM_2114_DELAY_SET_COLS
        ),
        &pio, &sm, &offset, pin, 17, true
    );

    // Set up 17 total pins
    for (uint count = 0; count < 17; count++) {
        pio_gpio_init(pio, pin + count);
        gpio_set_slew_rate(pin + count, GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(pin + count, GPIO_DRIVE_STRENGTH_4MA);
    }

    // True is output - with respect to pico. So the data pins are set up for
    // read intitially. The pio asm code wll flip data pin directions as
    // required for read and write operations.
    pio_sm_set_pindirs_with_mask(pio, sm, true, ~PIN_DIR_MASK);
    pio_sm_set_pindirs_with_mask(pio, sm, false, PIN_DIR_MASK);

    pio_sm_set_clkdiv(pio, sm, 1); // should just be the default.

    pio_sm_config c = fam_2114_program_get_default_config(offset);

    // IO4, IO3, A1, CS, WE, A0, A3, A4, A5, A7, A8, A9, A6, nc, IO1, A2, IO2
    sm_config_set_out_pins(&c, pin, 17);
    sm_config_set_set_pins(&c, pin + 3, 2); // Max is 5.
    sm_config_set_in_pins(&c, pin);

    // Shift right, Autopull off, and last arg only used for autopull so can be zero
    sm_config_set_out_shift(&c, true, false, 0);

    // Shift right, Autopull off, so the bit threshold doesn't matter
    sm_config_set_in_shift(&c, true, false, 0);

    //hw_set_bits(&pio->input_sync_bypass, 1u << (pin + 16)); //to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    // Send the data bit mask to the state machine
    pio_sm_put(pio, sm, (PIN_DIR_MASK << 1) | 1);
}

static void teardown_pio() {
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&fam_2114_program, pio, sm, offset);
}

static inline int addr_to_fifo(int addr) {

    uint32_t fifo_word = 0;

    for (uint8_t i=0; i<ADDR_MAP_LEN; i++)
    {
        fifo_word = fifo_word | ((addr >> i) & (1u << addr_map[i]));
    };
    
    return fifo_word;
} 

static inline int data_to_fifo (int data) {

    int fifo_word = 0;

    for (uint8_t i=0; i<DATA_MAP_LEN; i++)
    {
        fifo_word = fifo_word | ((data >> i) & (1u << data_map[i]));
    };
    
    return fifo_word;
}

static inline int fifo_to_data(int fifo_word) {
    int data_word = 0;

    return data_word;
}

static int read(int (*addr_func)(int addr), int addr)  {
    uint32_t fifo_word = addr_to_fifo(addr);

    // All non-addr bits will be zeroed.
    pio_sm_put(pio, sm, fifo_word);
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for data to arrive
    int data = fifo_to_data(pio_sm_get(pio, sm));               // Return the data
    return data;
}

static void write(int (*addr_func)(int addr), int addr, int data)  {
    uint32_t fifo_word = addr_to_fifo(addr) | data_to_fifo(data);
    pio_sm_put(pio, sm, fifo_word);
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for data to arrive
    uint d = pio_sm_get(pio, sm);               // Return the data
}

static const mem_family_t self = {
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .read = read,
    .write = write,
    .addr_pins = 8,
    .bits = 1,
};

const mem_family_t *fam_2114() {
    return &self;

}
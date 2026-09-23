#include <stdint.h>
#include "fam_2114.h"
#include "fam_2114.pio.h"
#include "logging/logging.h"

static PIO pio;
static uint sm = 0;
static uint offset; // Returns offset of starting instruction

static const mem_family_t self;

// The pin mapping for the 2114 is very scattered as the pcb was not designed
// to accommodate it. So we need to send a value for every pin in our pio
// command word in the correct order. The command word bits are as follows.
// For read/write commands, pins 2-18 are the chip pins. For pin dir mapping
// they are the dedfault directions for these pins for a read command
// i.e. the IO pins are inputs and the A pins are outputs - with reference to
// the pico arm core and our c code. Read/Write encoding is as follows:
//
//  0: Is this a write (1) or a read (0) command?
//  1: SP0  = IO4
//  2: SP1  = IO3
//  3: SP2  = A1
//  4: SP3  = CS (must be 1/high before start of a read/write cycle)
//  5: SP4  = WE (must be 1/high before start of a read/write cycle)
//  6: SP5  = A0
//  7: SP6  = A3
//  8: SP7  = A4
//  9: SP8  = A5
// 10: SP9  = A7
// 11: SP10 = A8
// 12: SP11 = A9
// 13: SP12 = A6
// 14: SP13 = nc
// 15: SP14 = IO1
// 16: SP15 = A2
// 17: SP16 = IO2

// Create a lookup table for the physical address and data pins.
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

// 1s denote input to Pico2. They must match the pin locations in the data map.
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

    // Shift right, Autopull off, and last arg is only used for autopull, so it
    // can be zero.
    sm_config_set_out_shift(&c, false, false, 0);

    // Shift right, Autopull off, so the bit threshold doesn't matter
    sm_config_set_in_shift(&c, false, false, 0);

    // to bypass synchronization on an input
    // hw_set_bits(&pio->input_sync_bypass, 1u << (pin + 16)); 
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    // Send the data bit mask to the state machine
    // See comments at start of file for encoding rules.
    pio_sm_put(pio, sm, PIN_DIR_MASK);
}

static void teardown_pio() {
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&fam_2114_program, pio, sm, offset);
}

static inline uint32_t addr_to_pins(uint32_t addr) {

    uint32_t pin_word = 0;

    for (uint8_t i=0; i<ADDR_MAP_LEN; i++)
    {
        pin_word = pin_word | (((addr >> i) & 1u) << addr_map[i]);
    };
    
    return pin_word;
} 

static inline uint32_t data_to_pins (uint32_t data) {
    uint32_t pin_word = 0;

    for (uint8_t i=0; i<DATA_MAP_LEN; i++)
    {
        pin_word = pin_word | (((data >> i) & 1u) << data_map[i]);
    };
    return pin_word;
}

#define CS_BIT 3
#define WE_BIT 4

static inline uint32_t pins_to_fifo(uint32_t pin_word, uint32_t write) {
    // CS and WE must be one. Shoft one bit to right to add write bit.
    return (
        (pin_word | (1u << CS_BIT) | (1u << WE_BIT)) << 1 | (write & 1u)
    );  
}

static inline uint32_t read_fifo(uint32_t addr) {
    return pins_to_fifo(addr_to_pins(addr), 0);
}

static inline uint32_t write_fifo(uint32_t addr, uint32_t data) {
    return pins_to_fifo(addr_to_pins(addr) | data_to_pins(data), 1);
}

static inline uint32_t pins_to_data(uint32_t fifo_word) {
    // Pick out the data bits and put them in the correct order. :-)
    // Incoming fifo_word is for all pins, even if they are not inputs.
    
    uint32_t data_word = 0;

    for (uint8_t i=0; i<DATA_MAP_LEN; i++)
    {
        data_word = data_word | (((fifo_word >> data_map[i]) & 1u) << i);
    };
    return data_word;
}

static int read(int (*addr_func)(int addr), int addr)  {
    uint32_t fifo_word = read_fifo(addr);
    ULOG_INFO("Read Cmd : %032b", fifo_word);
    pio_sm_put(pio, sm, fifo_word);

    // Wait for data to arrive
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}

    int data = pins_to_data(pio_sm_get(pio, sm));
    ULOG_INFO("Read Val : %032b", data);
    return data;
}

static void write(int (*addr_func)(int addr), int addr, int data)  {
    uint32_t fifo_word = write_fifo(addr, data);
    ULOG_INFO("Write Cmd: %032b", fifo_word);
    pio_sm_put(pio, sm, fifo_word);

    // Wait for data to arrive
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}

    // Clear the ISR
    pio_sm_get(pio, sm);
}

static const mem_family_t self = {
    .setup_pio = setup_pio,
    .teardown_pio = teardown_pio,
    .read = read,
    .write = write,
    .addr_pins = 10,
    .bits = 4,
};

const mem_family_t *fam_2114() {
    return &self;
}
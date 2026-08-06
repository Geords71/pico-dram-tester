#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_chip.h"
#include "hardware/pio.h"
#include "ram1b1r.pio.h"
#include "ram4b1r.pio.h"
#include "ff.h"
#include "shared_storage.h"
#include "logging.h"

// Defined RAM pio programs
#include "mem_chip/ram4027.h"
#include "mem_chip/ram4108.h"
#include "mem_chip/ram4116.h"
#include "mem_chip/ram4816.h"
#include "mem_chip/ram4132stk.h"
#include "mem_chip/ram4132.h"
#include "mem_chip/ram4164.h"
#include "mem_chip/ram41256.h"
#include "mem_chip/ram4408.h"
#include "mem_chip/ram4416.h"
#include "mem_chip/ram4464.h"
#include "mem_chip/ram44256.h"
#include "mem_chip/ram41128.h"

PIO pio;
uint sm = 0;
uint offset; // Returns offset of starting instruction

const mem_chip_t *chip_list[NUM_CHIPS] = {
    &ram4027_chip,
    &ram4108_chip,
    &ram4116_chip,
    &ram4816_chip,
    &ram4132stk_chip,
    &ram4132_chip,
    &ram4164_chip,
    &ram41128_chip,
    &ram41256_chip,
    &ram4408_chip,
    &ram4416_chip,
    &ram4464_chip,
    &ram44256_chip,
};


struct pio_program *get_patched_program(const struct pio_program *program, const uint8_t *delay_set, uint8_t delay_set_size)
{
    static struct pio_program patched_program;
    static uint16_t patched_instructions[PIO_INSTRUCTION_COUNT];

    patched_program.length = program->length;
    patched_program.origin = program->origin;
    patched_program.pio_version = program->pio_version;
    patched_program.used_gpio_ranges = program->used_gpio_ranges;

    for (uint8_t i = 0; i < program->length; i++) {
        uint16_t instruction = program->instructions[i];
        uint8_t field = (instruction >> 8) & 0x1f;

        // 0 is reserved for instructions that don't use this feature
        if ((field > 0) && (field < delay_set_size)) {
            instruction = instruction & 0xe0ff;
            instruction |= ((delay_set[field] & 0x1f) << 8);
        }
        patched_instructions[i] = instruction;
    }

    patched_program.instructions = patched_instructions;
    return &patched_program;
}

void get_ram_config(mem_chip_t chip) {

    if (mount_shared_storage() != FR_OK) {
        ULOG_WARNING("Shared USB storage could not be mounted. Hard coded delay values will be used.");
        return;
    }

    char filename[12];
    sprintf(filename, "%s.csv", chip.timing_family);

    FIL fp;
    bool result = f_open(&fp, filename, FA_READ);

    if (result == FR_OK) {
        ULOG_INFO("Reading delay values from %s...", filename);
        uint8_t buffer[512] = {"\0"};

        for (uint8_t row=0; row<chip.delay_sets.len; row++) {

            if (f_gets(buffer, sizeof(buffer), &fp) == NULL) {
                ULOG_INFO("Reached delay file EOF.");
                break;
            }
            buffer[strcspn(buffer, "\n")] = 0;

            ULOG_INFO("    Patching delay line %d: %s", row, buffer);

            char *token;
            token = strtok(buffer, ",");
            for (uint8_t col=0; col<chip.delay_sets.wid; col++) {
                if (token == NULL) {
                    ULOG_ERROR("Encountered unexpected end of line. Continuing to next...");
                    continue;
                }
                chip.delay_sets.list[row][col] = atoi(token);
                token = strtok(NULL, ",");
            }
        }

        f_close(&fp);
        result = true;

    } else {
        ULOG_WARNING("Can't open %s. Using hard-coded delay values: %d", filename, result);
        result = false;
    }
    unmount_shared_storage();
}

void ram1b1r_setup_pio(const delay_set_t delay_set, uint8_t variant)
{
    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &ram1b1r_program, delay_set, RAM1B1R_DELAY_SET_COLS
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

    pio_sm_config c = ram1b1r_program_get_default_config(offset);

    // A0, A1, A2, A3, A4, A5, A6, A7, nc, D, WR, RAS, CAS, nc, nc, nc, IN
    sm_config_set_out_pins(&c, pin, 10);
    sm_config_set_set_pins(&c, pin + 10, 3); // Max is 5.
    sm_config_set_in_pins(&c, pin + 16);

    // Shift right, Autopull off, 20 bits (1 + 1 + 8 + 10) at a time
    sm_config_set_out_shift(&c, true, false, 20);

    // Shift left, Autopull on, 1 bit
    sm_config_set_in_shift(&c, false, false, 1);

 //   hw_set_bits(&pio->input_sync_bypass, 1u << pin); to bypass synchronization on an input
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void ram4b1r_setup_pio(const delay_set_t delay_set, uint8_t variant)
{
    uint pin = 5;
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
        get_patched_program(
            &ram4b1r_program, delay_set, RAM4B1R_DELAY_SET_COLS
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

    pio_sm_config c = ram4b1r_program_get_default_config(offset);

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
void ram1b1r_teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&ram1b1r_program, pio, sm, offset);
}

void ram4b1r_teardown_pio()
{
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&ram4b1r_program, pio, sm, offset);
}

int read_ram1b1r_8p(int addr)
{
    uint d;
    pio_sm_put(
        pio, 
        sm, 
        0         | // Fast page mode flag off
        0 << 1    | // Write flag off
        addr << 2 | // Row address
        0 << 19     // Data bit
    );     

    // Wait for data to arrive
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {} 

    // Return the data
    d = pio_sm_get(pio, sm);
    //gpio_put(GPIO_LED, d);
    return d;
}

void write_ram1b1r_8p(int addr, int data)
{
    pio_sm_put(
        pio,
        sm,
        0 |                     // Fast page mode flag
        1 << 1 |                // Write flag
        (addr & 0xff) << 2 |    // Row address
        (addr & 0xff00) << 2|   // Column address
        ((data & 1) << 19));    // Data bit

    // Wait for dummy data
    while (pio_sm_is_rx_fifo_empty(pio, sm)) {}

    // Discard the dummy data bit
    pio_sm_get(pio, sm);
}

int calc_6p(int addr) {
    // Because we only have six pins, need to do some bit shifting to be able
    // to re-use the 8pin read function. This works because what are 7th 
    // and 8th pins on 64k chips are not used for addresses on 4k examples.
    // 4k chips like 4027 and 4015 use the 4116 socket on this board.
    // The 4027's chip select pin is in the same location as address pin 6
    // on a 4116. So we need to always have this pin pulled low when testing
    // a 4027 in the 4116b socket. (Chip Select is active low). e.g.
    return (addr & 0x003f) | ((addr << 2) & 0x3f00);
}

int read_ram1b1r_6p(int addr) {
    return read_ram1b1r_8p(calc_6p(addr));
}

void write_ram1b1r_6p(int addr, int data) {
    write_ram1b1r_8p(calc_6p(addr), data);
}

int calc_7p_half_lc(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return (addr & 0x007f) | ((addr << 2) & 0x7e00);
}

int read_ram1b1r_7p_half_lc(int addr)
{
    return read_ram1b1r_8p(calc_7p_half_lc(addr));
}

void write_ram1b1r_7p_half_lc(int addr, int data)
{
    write_ram1b1r_8p(calc_7p_half_lc(addr), data);
}

int calc_7p_half_hc(int addr) {
    // Pin A0 for column select dictates if we are accessing low or high half.
    return (addr & 0x007f) | ((addr << 2) & 0x7e00) | 0x100;
}

int read_ram1b1r_7p_half_hc(int addr)
{
    return read_ram1b1r_8p(calc_7p_half_hc(addr));
}

void write_ram1b1r_7p_half_hc(int addr, int data)
{
    write_ram1b1r_8p(calc_7p_half_hc(addr), data);
}

int calc_7p(int addr) {
    // Because we only have seven pins, need to do some bit shifting to be able
    // to re-use the 8pin read function. This works because what is the 8th pin
    // on 64k chips is not connected on 16k examples.
    return (addr & 0x007f) | ((addr << 1) & 0x7f00);
}

int read_ram1b1r_7p(int addr) {
    return read_ram1b1r_8p(calc_7p(addr));
}

void write_ram1b1r_7p(int addr, int data) {
    write_ram1b1r_8p(calc_7p(addr), data);
}

int calc_8p_half_lr(int addr) {
    // Funkier: The column address starts at the MSB of the low (row) byte. So
    // we need to: shift column bits up by one; blat row byte's MSB; and then
    // AND/OR these values together using appropriate masking.
    return (addr & 0x007f) | ((addr << 1) & 0xff00);
}

int read_ram1b1r_8p_half_lr(int addr)
{
    return read_ram1b1r_8p(calc_8p_half_lr(addr));
}

void write_ram1b1r_8p_half_lr(int addr, int data)
{
    write_ram1b1r_8p(calc_8p_half_lr(addr), data);
}

int calc_8p_half_hr(int addr) {
    // Funkier: The column address starts at the MSB of the low (row) byte. So
    // we need to: shift column bits up by one; set row byte's MSB; and then
    // AND/OR these values together using appropriate masking.
    return ((addr & 0x007f) | 0x0080) | ((addr << 1) & 0xff00);
}

int read_ram1b1r_8p_half_hr(int addr)
{
    return read_ram1b1r_8p(calc_8p_half_hr(addr));
}

void write_ram1b1r_8p_half_hr(int addr, int data)
{
    write_ram1b1r_8p(calc_8p_half_hr(addr), data);
}

int calc_8p_half_lc(int addr)
{
    // Easy: Force the msb to 0 on the high byte - which is the column. 
    return addr & 0x7fff;
}

int read_ram1b1r_8p_half_lc(int addr)
{
    return read_ram1b1r_8p(calc_8p_half_lc(addr));
}

void write_ram1b1r_8p_half_lc(int addr, int data)
{
    write_ram1b1r_8p(calc_8p_half_lc(addr), data);
}


int calc_8p_half_hc (int addr)
{
    // Easy: Force the msb to 1 on the high byte - which is the column. 
    return (addr & 0xff) | ((addr & 0x7f00) | 0x8000);
}

int read_ram1b1r_8p_half_hc(int addr)
{
    return read_ram1b1r_8p(calc_8p_half_hc(addr));
}

void write_ram1b1r_8p_half_hc(int addr, int data)
{
    write_ram1b1r_8p(calc_8p_half_hc(addr), data);
}

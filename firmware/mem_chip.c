#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_chip.h"
#include "hardware/pio.h"
#include "ff.h"
#include "shared_storage.h"
#include "logging.h"

PIO pio;
uint sm = 0;
uint offset; // Returns offset of starting instruction

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
    // 4k chips like 4027 use the 4116 socket on this board.
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

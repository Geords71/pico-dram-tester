// Simple driver for ST7789 LCD controller (back‑buffered, dirty‑flush, non‑DMA)

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "st7789.h"
#include "fonts.h"

#define PIN_SPI_CS 1
#define PIN_SPI_SCK 2
#define PIN_SPI_DO 3
#define PIN_SPI_DC 0

#define RESET_DELAY 140

#define CMD_NOP      0x00
#define CMD_SWRESET  0x01
#define CMD_SLPIN    0x10
#define CMD_SLPOUT   0x11
#define CMD_NORON    0x13
#define CMD_INVOFF   0x20
#define CMD_INVON    0x21
#define CMD_DISPOFF  0x28
#define CMD_DISPON   0x29
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C
#define CMD_MADCTL   0x36
#define CMD_COLMOD   0x3A

// A few colors
#define COLOR_BLACK 0x0000
#define COLOR_DKBLUE 0xa800
#define COLOR_DKGREEN 0x0540
#define COLOR_DKCYAN 0xad40
#define COLOR_DKRED 0x0015
#define COLOR_DKMAGENTA 0xa815
#define COLOR_DKYELLOW 0x0555
#define COLOR_LTGRAY 0xad55
#define COLOR_DKGRAY 0x52aa
#define COLOR_BLUE 0xfaaa
#define COLOR_GREEN 0x57ea
#define COLOR_CYAN 0xffea
#define COLOR_RED 0x52bf
#define COLOR_MAGENTA 0xfabf
#define COLOR_YELLOW 0x57ff
#define COLOR_WHITE 0xffff

#define LCD_WIDTH   240
#define LCD_HEIGHT  135

// Display corner isn't always 0,0
static uint16_t _x_offset = 0;
static uint16_t _y_offset = 0;

// Back buffer
static uint16_t framebuffer[LCD_WIDTH * LCD_HEIGHT];

// Dirty flag: only flush when something changed
static bool framebuffer_dirty = false;

// Frame timing for st7789_update (30 Hz)
static uint64_t last_update_time = 0;
static const uint64_t frame_interval_us = 33333;

// Mode for transmitting a command
static inline void mode_cmd()
{
    gpio_put(PIN_SPI_DC, 0);
}

// Mode for transmitting data
static inline void mode_data()
{
    gpio_put(PIN_SPI_DC, 1);
}

// Select the device
static inline void cs_low()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 0);
    asm volatile("nop \n nop \n nop");
}

// Deselect the device
static inline void cs_high()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 1);
    asm volatile("nop \n nop \n nop");
}

// Write a command byte
static void write_command(uint8_t cmd, uint8_t num_bytes, uint8_t buf[], bool cs)
{
    if (cs) cs_low();
    mode_cmd();
    spi_write_blocking(spi0, &cmd, 1);
    mode_data();
    if (num_bytes > 0) {
        spi_write_blocking(spi0, buf, num_bytes);
    }
    if (cs) cs_high();
}

// Write a 16-bit value block
static void write_data16(size_t num_words, uint16_t *buf)
{
    hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    spi_write16_blocking(spi0, buf, num_words);
    hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
}

static void st7789_gpio_init()
{
    spi_init(spi0, 70 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_DO, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);

    gpio_init(PIN_SPI_DC);
    gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
    gpio_put(PIN_SPI_DC, 0);

    sleep_ms(RESET_DELAY);
}

// Set the window size for data writes (X → CASET, Y → RASET)
static inline void st7789_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool cs)
{
    uint16_t sx = x + _x_offset;
    uint16_t sy = y + _y_offset;
    uint16_t ex = x + width  - 1 + _x_offset;
    uint16_t ey = y + height - 1 + _y_offset;

    write_command(CMD_CASET, 4, (uint8_t []){sx >> 8, sx & 0xff, ex >> 8, ex & 0xff}, cs);
    write_command(CMD_RASET, 4, (uint8_t []){sy >> 8, sy & 0xff, ey >> 8, ey & 0xff}, cs);
}

// Low level display initialization.
void st7789_disp_init(uint16_t xoff, uint16_t yoff, uint16_t width, uint16_t height)
{
    _x_offset = xoff;
    _y_offset = yoff;
    write_command(CMD_SWRESET, 0, NULL, true);
    sleep_ms(120);
    write_command(CMD_SLPOUT, 0, NULL, true);
    sleep_ms(5);
    write_command(CMD_COLMOD, 1, (uint8_t []){ 0x55 }, true);    // 16‑bit color
    write_command(CMD_MADCTL, 1, (uint8_t []){ 0x78 }, true);    // landscape, RGB
    st7789_window(0, 0, width, height, true);
    write_command(CMD_INVON, 0, NULL, true);
    write_command(CMD_NORON, 0, NULL, true);
    write_command(CMD_DISPON, 0, NULL, true);
}

// Draw a filled rectangle (buffered).
void st7789_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t col)
{
    for (uint16_t y = sy; y < sy + height && y < LCD_HEIGHT; y++) {
        uint16_t *row = &framebuffer[y * LCD_WIDTH];
        for (uint16_t x = sx; x < sx + width && x < LCD_WIDTH; x++) {
            row[x] = col;
        }
    }
    framebuffer_dirty = true;
}

// Fill with 50% halftone (buffered)
void st7789_halftone_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t c1, uint16_t c2)
{
    uint8_t rowstate = 0;
    for (uint16_t row = 0; row < height; row++) {
        uint16_t y = sy + row;
        if (y >= LCD_HEIGHT) break;
        rowstate = ~rowstate;
        uint8_t colstate = rowstate;
        uint16_t *fbrow = &framebuffer[y * LCD_WIDTH];
        for (uint16_t col = 0; col < width; col++) {
            uint16_t x = sx + col;
            if (x >= LCD_WIDTH) break;
            colstate = ~colstate;
            fbrow[x] = colstate ? c1 : c2;
        }
    }
    framebuffer_dirty = true;
}

// Plots a bitmap (buffered). Must be 16bpp and match the display type (BGR 565)
void st7789_bitblt(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t *buf)
{
    for (uint16_t row = 0; row < height; row++) {
        uint16_t y = sy + row;
        if (y >= LCD_HEIGHT) break;
        uint16_t *dst = &framebuffer[y * LCD_WIDTH + sx];
        uint16_t *src = &buf[row * width];
        uint16_t w = width;
        if (sx + w > LCD_WIDTH) w = LCD_WIDTH - sx;
        for (uint16_t i = 0; i < w; i++) {
            dst[i] = src[i];
        }
    }
    framebuffer_dirty = true;
}

// Rotates the bitmap using a trick. Here we just copy line by line into buffer.
void st7789_bitblt_rot(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t *buf)
{
    for (int count = 0; count < height; count++) {
        uint16_t y = sy + count;
        if (y >= LCD_HEIGHT) break;
        uint16_t *dst = &framebuffer[y * LCD_WIDTH + sx];
        uint16_t *src = &buf[width * count];
        uint16_t w = width;
        if (sx + w > LCD_WIDTH) w = LCD_WIDTH - sx;
        for (uint16_t i = 0; i < w; i++) {
            dst[i] = src[i];
        }
    }
    framebuffer_dirty = true;
}

// Plot a single pixel (buffered)
static void pset(uint16_t x, uint16_t y, uint16_t col)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    framebuffer[y * LCD_WIDTH + x] = col;
    framebuffer_dirty = true;
}

// Get the width of a string using a particular font
uint16_t font_string_width(char *text, uint16_t max_len, const font_def_t *font, bool bold)
{
    char *text_buf = text;
    uint16_t total_width = 0;

    while (*text_buf) {
        if (*text_buf >= font->count) {
            text_buf++;
            continue;
        }
        if (text_buf >= text + max_len) {
            break;
        }
        total_width += font->widths[*(text_buf++)] + (bold ? 1 : 0);
    }
    return total_width;
}

// Draws a string at the specific coordinates using the default font (buffered)
void font_string(uint16_t x, uint16_t y, char *text, uint16_t max_len,
                 uint16_t fg_color, uint16_t bg_color,
                 const font_def_t *font, bool bold)
{
    uint8_t row, col;
    uint16_t total_width, width;
    uint16_t offset;
    uint8_t bytes_column;
    uint8_t byte;
    uint8_t db;
    uint8_t col_count;
    char *text_buf;
    uint8_t prev_bit;

    total_width = font_string_width(text, max_len, font, bold);

    for (row = 0; row < font->height; row++) {
        uint16_t py = y + row;
        if (py >= LCD_HEIGHT) break;

        text_buf = text;
        uint16_t px = x;

        while (*text_buf) {
            if (*text_buf >= font->count) {
                text_buf++;
                continue;
            }
            if (text_buf >= text + max_len) {
                break;
            }
            width = font->widths[*text_buf];
            bytes_column = (width + 7) >> 3;
            offset = font->offsets[*text_buf];
            offset += row * bytes_column;
            prev_bit = 0;
            uint16_t remaining = width;

            for (byte = 0; byte < bytes_column; byte++) {
                db = font->data[offset + byte];
                col_count = (remaining > 8) ? 8 : remaining;
                for (col = 0; col < col_count; col++) {
                    if (px >= LCD_WIDTH) break;
                    uint8_t bit = db & 0x1;
                    framebuffer[py * LCD_WIDTH + px] =
                        (bit || (bold && (prev_bit == 1))) ? fg_color : bg_color;
                    prev_bit = bit;
                    db = db >> 1;
                    px++;
                }
                remaining -= col_count;
            }
            if (bold && px < LCD_WIDTH) {
                framebuffer[py * LCD_WIDTH + px] =
                    (prev_bit) ? fg_color : bg_color;
                px++;
            }
            text_buf++;
        }
    }
    framebuffer_dirty = true;
}

// Draws a icon at the given coordinates. Requires an icon structure.
// Draws it upside down as is tradition. (buffered)
void draw_icon(uint16_t sx, uint16_t sy, const ico_def_t *ico)
{
    uint16_t x, y;
    int sp;
    const uint8_t *id = ico->image;
    const uint8_t *md = ico->mask;
    uint16_t c;

    for (y = sy + ico->height - 1; y >= sy; y--) {
        if (y >= LCD_HEIGHT) continue;
        for (x = sx; x < sx + ico->width; x+=8) {
            if (x >= LCD_WIDTH) break;
            for (sp = 0; sp < 8; sp++) {
                if (x + sp >= LCD_WIDTH) break;
                if (!((*md >> (7-sp)) & 1)) {
                    c = ico->pal[*id >> 4];
                    pset(x + sp, y, c);
                }
                sp++;
                if (sp >= 8 || x + sp >= LCD_WIDTH) break;
                if (!((*md >> (7-sp)) & 1)) {
                    c = ico->pal[*id & 0xf];
                    pset(x + sp, y, c);
                }
                id++;
            }
            md++;
        }
        if (y == 0) break;
    }
    framebuffer_dirty = true;
}

// Flush the framebuffer to the display, throttled to ~30 Hz, only if dirty
void st7789_update(void)
{
    if (!framebuffer_dirty)
        return;

    uint64_t now = time_us_64();
    if (now - last_update_time < frame_interval_us)
        return;

    last_update_time = now;

    cs_low();
    st7789_window(0, 0, LCD_WIDTH, LCD_HEIGHT, false);
    write_command(CMD_RAMWR, 0, NULL, false);
    write_data16(LCD_WIDTH * LCD_HEIGHT, framebuffer);
    cs_high();

    framebuffer_dirty = false;
}

// Optional: force immediate full flush
void st7789_force_flush(void)
{
    framebuffer_dirty = true;
    st7789_update();
}

// Initialize the display
void st7789_init()
{
    st7789_gpio_init();
    st7789_disp_init(40, 53, LCD_WIDTH, LCD_HEIGHT);

    // Clear framebuffer to blue (0x001F as in original)
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        framebuffer[i] = 0x001F;
    }
    framebuffer_dirty = true;

    st7789_update();
}

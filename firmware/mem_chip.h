#ifndef _MEMCHIP_H
#define _MEMCHIP_H
#include <stdint.h>
#include "pico/types.h"
#include "hardware/pio.h"
#include "mem_family.h"

#define NUM_CHIPS 13
#define MEMCHIP_MAX_VARIANTS 8

typedef struct mem_chip_variant_t {
    char *name;
    const int (*addr_func)(int addr);
    const int (*ram_read)(int);
    const void (*ram_write)(int, int);
} mem_chip_variant_t;

typedef struct {
    const uint8_t len;
    const mem_chip_variant_t list[MEMCHIP_MAX_VARIANTS];
} mem_chip_variants_t;


typedef struct {
    const mem_family_t *(*family)();
    void (*setup_pio)(uint speed_grade, uint variant);
    void (*teardown_pio)();
    int (*ram_read)(int addr);
    void (*ram_write)(int addr, int data);
    uint32_t mem_size;
    uint32_t bits;
    const mem_chip_variants_t variants;
    char *name;
    const char *short_name;
    const char *timing_family;
    delay_sets_t delay_sets;
} mem_chip_t;

extern PIO pio;
extern uint sm;
extern uint offset; // Returns offset of starting instruction

extern int read_ram1b1r_6p(int addr);
extern void write_ram1b1r_6p(int addr, int data);

extern int read_ram1b1r_7p(int addr);
extern int read_ram1b1r_7p_half_lc(int addr);
extern int read_ram1b1r_7p_half_hc(int addr);

extern void write_ram1b1r_7p(int addr, int data);
extern void write_ram1b1r_7p_half_lc(int addr, int data);
extern void write_ram1b1r_7p_half_hc(int addr, int data);

extern int read_ram1b1r_8p(int addr);
extern int read_ram1b1r_8p_half_lr(int addr);
extern int read_ram1b1r_8p_half_hr(int addr);
extern int read_ram1b1r_8p_half_lc(int addr);
extern int read_ram1b1r_8p_half_hc(int addr);

extern void write_ram1b1r_8p(int addr, int data);
extern void write_ram1b1r_8p_half_lr(int addr, int data);
extern void write_ram1b1r_8p_half_hr(int addr, int data);
extern void write_ram1b1r_8p_half_lc(int addr, int data);
extern void write_ram1b1r_8p_half_hc(int addr, int data);

extern void ram1b1r_setup_pio(const delay_set_t delay_set, uint8_t variant);
extern void ram4b1r_setup_pio(const delay_set_t delay_set, uint8_t variant);
extern void ram1b1r_teardown_pio();
extern void ram4b1r_teardown_pio();

extern void get_ram_config(const mem_chip_t chip);

#endif

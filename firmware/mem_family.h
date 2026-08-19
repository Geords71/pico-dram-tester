#ifndef _MEM_FAMILY_H
#define _MEM_FAMILY_H

#include "stdint.h"
#include "pico/types.h"
#include "hardware/pio.h"

#define MEMCHIP_MAX_DELAY_SET_ROWS 8
#define MEMCHIP_MAX_DELAY_SET_COLS 8

typedef struct {
    uint8_t len;
    uint8_t wid;
    char *names[MEMCHIP_MAX_DELAY_SET_ROWS];
    uint8_t list[MEMCHIP_MAX_DELAY_SET_ROWS][MEMCHIP_MAX_DELAY_SET_COLS];
} delay_sets_t;

typedef struct {
    void (*setup_pio)(const uint8_t *delay_set);
    void (*teardown_pio)();
    int (*read)(int (*addr_func)(int addr), int addr);
    void (*write)(int (*addr_func)(int addr), int addr, int data);
    const uint8_t addr_pins;
    const uint8_t bits;
} mem_family_t;
    
extern struct pio_program *get_patched_program(
    const struct pio_program *program,
    const uint8_t *delay_set,
    uint8_t delay_set_size
);

#endif
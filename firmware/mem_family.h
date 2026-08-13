#ifndef _MEM_FAMILY_H
#define _MEM_FAMILY_H

#include "stdint.h"
#include "pico/types.h"
#include "hardware/pio.h"

#define MEMCHIP_MAX_DELAY_SET_ROWS 8
#define MEMCHIP_MAX_DELAY_SET_COLS 8

typedef uint8_t delay_set_t[MEMCHIP_MAX_DELAY_SET_COLS];

typedef struct {
    uint8_t len;
    uint8_t wid;
    char *names[MEMCHIP_MAX_DELAY_SET_ROWS];
    delay_set_t list[MEMCHIP_MAX_DELAY_SET_ROWS];
} delay_sets_t;

typedef struct {
    void (*setup_pio)(const delay_set_t delay_set);
    void (*teardown_pio)();
    int (*read)(int (*addr_func)(int addr), int addr);
    void (*write)(int (*addr_func)(int addr), int addr, int data);
} mem_family_t;
    
extern struct pio_program *get_patched_program(
    const struct pio_program *program,
    const uint8_t *delay_set,
    uint8_t delay_set_size
);

#endif
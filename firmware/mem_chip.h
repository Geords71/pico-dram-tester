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
} mem_chip_variant_t;

typedef struct {
    const uint8_t len;
    const mem_chip_variant_t list[MEMCHIP_MAX_VARIANTS];
} mem_chip_variants_t;


typedef struct {
    const mem_family_t *(*get_family)();
    uint32_t mem_size;
    uint32_t bits;
    const mem_chip_variants_t variants;
    char *name;
    const char *short_name;
    const char *timing_family;
    delay_sets_t delay_sets;
} mem_chip_t;

extern void mem_chip_load_config(mem_chip_t *chip);

#endif
#ifndef _FONTDEF_H
#define _FONTDEF_H

#include <stdint.h>

// Font definition table
typedef struct {
    const uint16_t count;
    const uint8_t height;
    const uint8_t *widths;
    const uint16_t *offsets;
    const uint8_t *data;
} font_def_t;

#endif
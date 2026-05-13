#ifndef _ICONDEF_H
#define _ICONDEF_H

#include <stdint.h>

// Icon definition table
typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *pal;
    const uint8_t *image;
    const uint8_t *mask;
} ico_def_t;

#endif
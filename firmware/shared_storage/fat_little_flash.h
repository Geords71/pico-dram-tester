#ifndef _FAT_LITTLE_FLASH_H
#define _FAT_LITTLE_FLASH_H

#include <stdint.h>
#include <pico/types.h>
#include "fat_image.h"

void fat_little_flash_initialize(void);
bool fat_little_flash_read(int block, uint8_t *buffer);
bool fat_little_flash_write(int block, uint8_t *buffer);

#endif

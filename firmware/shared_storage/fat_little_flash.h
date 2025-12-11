
#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdint.h>
#include <pico/types.h>

void fat_little_flash_initialize(void);
bool fat_little_flash_read(int block, uint8_t *buffer);
bool fat_little_flash_write(int block, uint8_t *buffer);

#endif

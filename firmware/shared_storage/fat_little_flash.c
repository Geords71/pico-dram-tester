#include "fat_little_flash.h"
#include "fat.h"
#include "fat_image.h"
#include <ctype.h>
#include <math.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#define FLASH_FAT_BLOCK_SIZE   4096
#define FLASH_FAT_OFFSET       0x1F0000

void fat_little_flash_initialize(void) {
    uint32_t ints = save_and_disable_interrupts();

#if defined(ERASE_ALL_FLASH)
    flash_range_erase(FLASH_FAT_OFFSET, FLASH_SECTOR_SIZE * 16);
#else
    flash_range_erase(FLASH_FAT_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_FAT_OFFSET, (uint8_t *)disk_image, sizeof(disk_image));
#endif
    restore_interrupts(ints);
}

bool fat_little_flash_read(int block, uint8_t *buffer) {
    const uint8_t *data = (uint8_t *)(XIP_BASE + FLASH_FAT_OFFSET + FAT_BLOCK_SIZE * block);
    memcpy(buffer, data, FAT_BLOCK_SIZE);
    return true;
}

bool fat_little_flash_write(int block, uint8_t *buffer) {
    /*
     * NOTE: Flash memory must be erased and updated in blocks of 4096 bytes
     *       from the head, and updating at the halfway boundary will (probably)
     *       lead to undefined results.
     */
    uint8_t data[FLASH_SECTOR_SIZE];  // 4096 byte

    // Obtain the location of the FAT sector(512 byte) in the flash memory sector(4096 byte).
    int flash_sector = floor((block * FAT_BLOCK_SIZE) / FLASH_SECTOR_SIZE);
    int flash_sector_fat_offset = (block * FAT_BLOCK_SIZE) % FLASH_SECTOR_SIZE;
    // Retrieve the data in the flash memory sector and update only the data for the FAT sector.
    memcpy(data, (uint8_t *)(XIP_BASE + FLASH_FAT_OFFSET + FLASH_SECTOR_SIZE * flash_sector), sizeof(data));
    memcpy(data + flash_sector_fat_offset, buffer, FAT_BLOCK_SIZE);

    // Clear and update flash sectors.
    stdio_set_driver_enabled(&stdio_usb, false);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_FAT_OFFSET + flash_sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_FAT_OFFSET + flash_sector * FLASH_SECTOR_SIZE, data, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    stdio_set_driver_enabled(&stdio_usb, true);

    return true;
}
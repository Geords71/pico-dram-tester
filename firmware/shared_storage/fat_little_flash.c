#include "fat_little_flash.h"
//#include "fat_image_new.h"
#include "fat_image.h"
#include <ctype.h>
#include <math.h>
#include <pico/flash.h>
#include <hardware/flash.h>
#include "hardware/irq.h"
//#include <hardware/sync.h>
#include <hardware/watchdog.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"

#include <stdio.h>
#include <string.h>
#include <logging.h>

#define FLASH_FAT_BLOCK_SIZE   4096
#define FLASH_FAT_OFFSET       0x1F0000
#define FAT_MAGIC  (0x55AA)

typedef struct {
    // define parameters needed for safe write
    uint32_t flash_offs;
    uint8_t data[FLASH_SECTOR_SIZE];  // 4096 byte
    size_t count;
} write_flash_params_t;

void write_flash(void *write_params) {
    write_flash_params_t *wp = write_params; 
    flash_range_erase(wp->flash_offs, wp->count);
    flash_range_program(wp->flash_offs, wp->data, wp->count);
}

void fat_little_flash_reflash() {
    
    // flash_safe_execute(write_flash, (void *)&write_params, 1000);
    flash_range_erase(FLASH_FAT_OFFSET, FLASH_SECTOR_SIZE * 16);
    flash_range_program(FLASH_FAT_OFFSET, (uint8_t *)disk_image, sizeof(disk_image));

    ULOG_INFO("Flash FAT12 Filesystem restored.");
}

void fat_little_flash_initialize(void) {
    uint8_t buffer[FAT_BLOCK_SIZE];
    fat_little_flash_read(0, buffer);

    ULOG_INFO("Checking FAT Filesystem...");
    uint16_t magic = buffer[FAT_BLOCK_SIZE - 2] << 8 | buffer[FAT_BLOCK_SIZE - 1];
    if (magic != FAT_MAGIC) {
        ULOG_INFO("Valid FAT Filesystem missing: Initializing Flash FAT12");
        fat_little_flash_reflash();
    }

    ULOG_INFO("Valid FAT Filesystem Found.");
    fat_little_flash_read(2, buffer);
    fat_dir_entry_t *dir = (fat_dir_entry_t *)buffer;
    dir++;

    int8_t size = sizeof(dir->DIR_Name) + 1;
    char dir_name[size];
    snprintf(dir_name, size, "%.*s", size-1, dir->DIR_Name);
    
    ULOG_INFO("Checking for SYSTEM.CFG...");
    //ULOG_INFO("[%s]", dir_name);
    if (strcmp(dir_name, "SYSTEM  CFG") == 0) {
        ULOG_INFO("SYSTEM.CFG found.");
        return;
    }

    ULOG_INFO("SYSTEM.CFG missing: Initializing Flash FAT12");
    fat_little_flash_reflash();
    return;
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
    int flash_sector = floor((block * FAT_BLOCK_SIZE) / FLASH_SECTOR_SIZE);

    write_flash_params_t write_params;
    write_params.count = FLASH_SECTOR_SIZE;
    write_params.flash_offs = FLASH_FAT_OFFSET + flash_sector * FLASH_SECTOR_SIZE;

    // Obtain the location of the FAT sector(512 byte) in the flash memory sector(4096 byte).
    int flash_sector_fat_offset = (block * FAT_BLOCK_SIZE) % FLASH_SECTOR_SIZE;

    // Retrieve the data in the flash memory sector and update only the data for the FAT sector.
    memcpy(write_params.data, (uint8_t *)(XIP_BASE + write_params.flash_offs), sizeof(write_params.data));
    memcpy(write_params.data + flash_sector_fat_offset, buffer, FAT_BLOCK_SIZE);

    // Clear and update flash sectors.
//    multicore_lockout_start_blocking();
    //stdio_set_driver_enabled(&stdio_usb, false);
 //   uint32_t ints = save_and_disable_interrupts();

    
    //flash_range_erase(write_params.flash_offs, write_params.count);
    //flash_range_program(write_params.flash_offs, write_params.data, write_params.count);
    flash_safe_execute(write_flash, (void *)&write_params, UINT32_MAX);


  //  restore_interrupts(ints);
    //stdio_set_driver_enabled(&stdio_usb, true);
   // multicore_lockout_end_blocking();

    return true;
}
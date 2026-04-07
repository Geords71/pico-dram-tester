#include "fat_little_flash.h"
#include "fat_image_new.h"
#include <ctype.h>
#include <math.h>
#include "pico/flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include <stdio.h>
#include <string.h>
#include "logging.h"

#define FLASH_FAT_BLOCK_SIZE   4096
#define FLASH_FAT_OFFSET       0x1F0000
#define XIP_FAT_OFFSET (XIP_BASE + FLASH_FAT_OFFSET)
#define FAT12_BOOT_SIGNATURE  (0xAA55)

// As-is, where-is!
#pragma pack(push, 1)
typedef struct {
    uint8_t  jmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_small;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large;
    
    // Extended BPB (FAT12/16)
    uint8_t  drive_number;
    uint8_t  reserved;
    uint8_t  signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     system_id[8];
    
    uint8_t  boot_code[448];
    uint16_t boot_signature;
} fat_boot_sector_t;

typedef struct {
  uint8_t name[11];
  uint8_t attr;
  uint8_t nt_res;
  uint8_t crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t lst_acc_date;
  uint16_t fst_clus_hi;
  uint16_t wrt_time;
  uint16_t wrt_date;
  uint16_t fst_clus_lo;
  uint32_t file_size;
} fat_dir_entry_t;
#pragma pack(pop)

typedef struct {
    // define parameters needed for safe write
    uint32_t flash_offs;
    uint8_t data[FLASH_SECTOR_SIZE];  // 4096 byte
    size_t count;
} write_flash_params_t;

void write_flash(void *write_params) {
    write_flash_params_t *wp = write_params; 
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(wp->flash_offs, wp->count);
    flash_range_program(wp->flash_offs, wp->data, wp->count);
    restore_interrupts(ints);
}

static uint8_t staging_buf[FLASH_SECTOR_SIZE];

void fat_little_flash_reflash(void) {
    uint32_t irq_state = save_and_disable_interrupts();

    flash_range_erase(FLASH_FAT_OFFSET, fat_image_new_len);

    for(uint32_t chunk_offset = 0; chunk_offset < fat_image_new_len; chunk_offset += FLASH_SECTOR_SIZE) {
        uint8_t *src  = (uint8_t *)fat_image_new_data + chunk_offset;
        uint32_t write_addr = FLASH_FAT_OFFSET + chunk_offset;

        memcpy(staging_buf, src, FLASH_SECTOR_SIZE);
        flash_range_program(write_addr, staging_buf, FLASH_SECTOR_SIZE);
    }

    restore_interrupts(irq_state);
}

void fat_little_flash_initialize(void) {
    fat_boot_sector_t *boot_sector = (fat_boot_sector_t *)(XIP_FAT_OFFSET);

    ULOG_INFO("Checking FAT Filesystem...");
    if (boot_sector->boot_signature != FAT12_BOOT_SIGNATURE) {
        ULOG_INFO("Valid FAT12 signature missing: expecting 0x%x but found 0x%x.", FAT12_BOOT_SIGNATURE, boot_sector->boot_signature);
        ULOG_INFO("Initializing Flash FAT12...");
        fat_little_flash_reflash();
        ULOG_INFO("Flash FAT12 Filesystem restored.");
        return;
    }

    ULOG_INFO("Valid FAT Filesystem Signature Found.");
    ULOG_INFO("Checking for SYSTEM.CFG...");
    ULOG_INFO("Fat 12 Filesystem has root file limit of %d.", boot_sector->root_entries);
    uint32_t root_dir_offset = boot_sector->bytes_per_sector * (boot_sector->reserved_sectors +
        (boot_sector->fat_count * boot_sector->sectors_per_fat));

    ULOG_INFO("Fat 12 Filesystem root dir starts at 0x%x.", root_dir_offset);
    fat_dir_entry_t *dir = (fat_dir_entry_t *)(XIP_FAT_OFFSET + root_dir_offset);

    static int8_t dir_name_size = sizeof(dir->name);
    char dir_name[dir_name_size + 1];

    for (uint8_t i = 0; i < boot_sector->root_entries; i++) {
        if (dir->name[0] == 0) break;        // No more entries

        if (dir->name[0] == 0xE5 || // Deleted entry
            dir->attr == 0x0F    || // Long File Name entry
            dir->attr & 0x08)       // Volume label
        {
            dir++;
            continue;
        }

        memcpy(dir_name, dir->name, dir_name_size);
        dir_name[dir_name_size] = '\0';

        if (strcmp(dir_name, "SYSTEM  CFG") == 0) {
            ULOG_INFO("SYSTEM.CFG found. Filesystem is OK.");
            return;
        }
        dir++;
    }

    // If we got here system.cfg is missing so reflash.
    ULOG_INFO("SYSTEM.CFG missing: Initializing Flash FAT12...");
    fat_little_flash_reflash();
    ULOG_INFO("Flash FAT12 Filesystem restored.");
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
    //stdio_set_driver_enabled(&stdio_usb, false);
    write_flash((void *)&write_params);

    return true;
}
#include "fat_little_flash.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "logging.h"

#define FLASH_FAT_OFFSET       0x300000
#define XIP_FAT_OFFSET         (XIP_BASE + FLASH_FAT_OFFSET)
#define FAT12_BOOT_SIGNATURE   (0xAA55)

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
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
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
    uint32_t flash_offs;
    uint8_t  data[FLASH_SECTOR_SIZE];  // 4096 bytes
    size_t   count;
} write_flash_params_t;

static void write_flash(void *write_params)
{
    write_flash_params_t *wp = (write_flash_params_t *)write_params;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(wp->flash_offs, wp->count);
    flash_range_program(wp->flash_offs, wp->data, wp->count);
    restore_interrupts(ints);
}

// Reflash the entire FAT region from the compiled image, sector by sector.
// We only ever stage 4KB at a time in RAM, so we don't need a 1.44MB buffer.
static void fat_little_flash_reflash(void)
{
    ULOG_INFO("Reflashing FAT12 image to flash...");

    uint32_t remaining = FAT_TOTAL_SIZE;
    uint32_t src_off   = 0;
    uint32_t dst_off   = FLASH_FAT_OFFSET;

    while (remaining) {
        write_flash_params_t wp;
        wp.count     = FLASH_SECTOR_SIZE;
        wp.flash_offs = dst_off;

        // Stage one 4KB sector from the header image into RAM
        memcpy(wp.data,
               fat_image_data + src_off,
               FLASH_SECTOR_SIZE);

        // Erase + program this 4KB sector
        write_flash(&wp);

        remaining -= FLASH_SECTOR_SIZE;
        src_off   += FLASH_SECTOR_SIZE;
        dst_off   += FLASH_SECTOR_SIZE;
    }

    ULOG_INFO("FAT12 image reflashed.");
}

void fat_little_flash_initialize(void)
{
    fat_boot_sector_t *boot_sector = (fat_boot_sector_t *)(XIP_FAT_OFFSET + FAT_SECTOR_SIZE);

    ULOG_INFO("Checking FAT Filesystem...");
    if (boot_sector->boot_signature != FAT12_BOOT_SIGNATURE) {
        ULOG_INFO("Valid FAT12 signature missing: expecting 0x%x but found 0x%x.",
                  FAT12_BOOT_SIGNATURE, boot_sector->boot_signature);
        ULOG_INFO("Initializing Flash FAT12...");
        fat_little_flash_reflash();
        ULOG_INFO("Flash FAT12 Filesystem restored.");
        return;
    }

    ULOG_INFO("Valid FAT Filesystem Signature Found.");
    ULOG_INFO("Checking for SYSTEM.CFG...");
    ULOG_INFO("Fat 12 Filesystem has root file limit of %d.", boot_sector->root_entries);

    uint32_t root_dir_offset = (
        boot_sector->hidden_sectors +
        boot_sector->reserved_sectors +
        (boot_sector->fat_count * boot_sector->sectors_per_fat)
    ) * boot_sector->bytes_per_sector;        

    ULOG_INFO("Fat 12 Filesystem root dir starts at 0x%x.", root_dir_offset);
    fat_dir_entry_t *dir = (fat_dir_entry_t *)(XIP_FAT_OFFSET + root_dir_offset);

    static int8_t dir_name_size = sizeof(dir->name);
    char dir_name[sizeof(dir->name) + 1];

    for (uint16_t i = 0; i < boot_sector->root_entries; i++) {
        if (dir->name[0] == 0) break;        // No more entries

        if (dir->name[0] == 0xE5 ||          // Deleted entry
            dir->attr == 0x0F    ||          // Long File Name entry
            (dir->attr & 0x08))              // Volume label
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
}

bool fat_little_flash_read(int block, uint8_t *buffer)
{
    if (block < 0 || block >= FAT_TOTAL_SECTORS) {
        ULOG_ERROR("fat_little_flash_read: block out of range: %d", block);
        return false;
    }

    const uint8_t *data =
        (const uint8_t *)(XIP_BASE + FLASH_FAT_OFFSET + (uint32_t)block * FAT_SECTOR_SIZE);

    memcpy(buffer, data, FAT_SECTOR_SIZE);
    return true;
}

bool fat_little_flash_write(int block, uint8_t *buffer)
{
    if (block < 0 || block >= FAT_TOTAL_SECTORS) {
        ULOG_ERROR("fat_little_flash_write: block out of range: %d", block);
        return false;
    }

    // Which 4KB flash sector contains this 512-byte block?
    uint32_t flash_sector =
        ((uint32_t)block * FAT_SECTOR_SIZE) / FLASH_SECTOR_SIZE;

    write_flash_params_t wp;
    wp.count      = FLASH_SECTOR_SIZE;
    wp.flash_offs = FLASH_FAT_OFFSET + flash_sector * FLASH_SECTOR_SIZE;

    // Offset of this 512-byte block within the 4KB sector
    uint32_t sector_offset =
        ((uint32_t)block * FAT_SECTOR_SIZE) % FLASH_SECTOR_SIZE;

    // Stage the entire 4KB sector into RAM BEFORE erase/program
    memcpy(wp.data,
           (const uint8_t *)(XIP_BASE + wp.flash_offs),
           FLASH_SECTOR_SIZE);

    // Patch in the new 512-byte block
    memcpy(wp.data + sector_offset, buffer, FAT_SECTOR_SIZE);

    // Erase + program this sector from RAM
    write_flash(&wp);

    return true;
}

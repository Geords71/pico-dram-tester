#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <tusb.h>
#include <logging.h>
#include "fat_little_flash.h"

static bool ejected = false;

void tud_msc_inquiry_cb(uint8_t lun,
                        uint8_t vendor_id[8],
                        uint8_t product_id[16],
                        uint8_t product_rev[4])
{
    (void) lun;

    const char vid[] = "TinyUSB";
    const char pid[] = "MassStorage";
    const char rev[] = "1.0";

    memcpy(vendor_id,  vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void) lun;

    if (ejected) {
        tud_msc_set_sense(lun,
                          SCSI_SENSE_NOT_READY,
                          0x3a, 0x00);
        return false;
    }

    return true;
}

void tud_msc_capacity_cb(uint8_t lun,
                         uint32_t* block_count,
                         uint16_t* block_size)
{
    (void) lun;
    *block_count = FAT_TOTAL_SECTORS;
    *block_size  = FAT_SECTOR_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun,
                           uint8_t power_condition,
                           bool start,
                           bool load_eject)
{
    (void) lun;
    (void) power_condition;

    if (load_eject && !start) {
        ejected = true;
    }

    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun,
                          uint32_t lba,
                          uint32_t offset,
                          void* buffer,
                          uint32_t bufsize)
{
    (void) lun;

    uint8_t* buf = (uint8_t*)buffer;
    uint32_t remaining = bufsize;

    while (remaining) {

        if (lba >= FAT_TOTAL_SECTORS) {
            ULOG_WARNING("READ10 out of range: LBA=%u", lba);
            return -1;
        }

        uint8_t sector[FAT_SECTOR_SIZE];
        fat_little_flash_read(lba, sector);

        uint32_t chunk = FAT_SECTOR_SIZE - offset;
        if (chunk > remaining)
            chunk = remaining;

        memcpy(buf, sector + offset, chunk);

        buf      += chunk;
        remaining -= chunk;
        lba++;
        offset = 0;
    }

    return (int32_t)bufsize;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void) lun;
    return true;
}

int32_t tud_msc_write10_cb(uint8_t lun,
                           uint32_t lba,
                           uint32_t offset,
                           uint8_t* buffer,
                           uint32_t bufsize)
{
    (void) lun;

    uint8_t* buf = buffer;
    uint32_t remaining = bufsize;

    while (remaining) {

        if (lba >= FAT_TOTAL_SECTORS) {
            ULOG_ERROR("WRITE10 out of range: LBA=%u", lba);
            return -1;
        }

        uint8_t sector[FAT_SECTOR_SIZE];
        fat_little_flash_read(lba, sector);

        uint32_t chunk = FAT_SECTOR_SIZE - offset;
        if (chunk > remaining)
            chunk = remaining;

        memcpy(sector + offset, buf, chunk);
        fat_little_flash_write(lba, sector);

        buf      += chunk;
        remaining -= chunk;
        lba++;
        offset = 0;
    }

    return (int32_t)bufsize;
}

// ---------------------------------------------------------------------------
// SCSI FALLBACK
// ---------------------------------------------------------------------------
int32_t tud_msc_scsi_cb(uint8_t lun,
                        uint8_t const scsi_cmd[16],
                        void* buffer,
                        uint16_t bufsize)
{
    (void) lun;
    (void) buffer;
    (void) bufsize;

    // Everything we need is handled by TinyUSB or our callbacks.
    tud_msc_set_sense(lun,
                      SCSI_SENSE_ILLEGAL_REQUEST,
                      0x20, 0x00);
    return -1;
}

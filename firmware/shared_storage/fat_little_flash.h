
#ifndef _FAT_LITTLE_FLASH_H
#define _FAT_LITTLE_FLASH_H

#include <stdint.h>
#include <pico/types.h>

#define FAT_BLOCK_NUM          128  // 64KB
#define FAT_BLOCK_SIZE         512

typedef struct {
  uint8_t DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t DIR_NTRes;
  uint8_t DIR_CrtTimeTenth;
  uint16_t DIR_CrtTime;
  uint16_t DIR_CrtDate;
  uint16_t DIR_LstAccDate;
  uint16_t DIR_FstClusHI;
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  uint32_t DIR_FileSize;
} fat_dir_entry_t;

void fat_little_flash_initialize(void);
bool fat_little_flash_read(int block, uint8_t *buffer);
bool fat_little_flash_write(int block, uint8_t *buffer);

#endif

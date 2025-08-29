#pragma once

#include "mik32sd.h"
#include "mik32fat_types.h"
#include "mik32fat_const.h"

int mik32fat_wheels_single_read(void *__restrict cookie, uint32_t sector_addr, uint8_t *dst);
int mik32fat_wheels_single_write(void *__restrict cookie, uint32_t sector_addr, const uint8_t *src);
int mik32fat_wheels_single_erase(void *__restrict cookie, uint32_t sector_addr);

MIK32FAT_Status_TypeDef __mik32fat_sector_sread(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector);
MIK32FAT_Status_TypeDef __mik32fat_sector_swrite(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector);
MIK32FAT_Status_TypeDef __mik32fat_sector_serase(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector);
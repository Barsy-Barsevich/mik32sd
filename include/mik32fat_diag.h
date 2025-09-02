#pragma once

#include <stdio.h>
#include <stdint.h>
#include "mik32fat_const.h"
#include "mik32fat_types.h"

void mik32fat_decode_attr(uint8_t attrubute);
void mik32fat_decode_entire(MIK32FAT_Entire_TypeDef *entire);
void mik32fat_decode_status(MIK32FAT_Status_TypeDef status);
void mik32fat_diag_fat_info(MIK32FAT_Descriptor_TypeDef *fs);
void mik32fat_diag_decode_file(MIK32FAT_File_TypeDef *file);
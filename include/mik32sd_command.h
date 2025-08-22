#pragma once

#include "mik32sd.h"
#include "mik32sd_diag.h"

void mik32sd_command_sector_read(MIK32SD_Descriptor_TypeDef* sd, uint32_t sector_address, char *__restrict dst);
void mik32sd_command_sector_dump(uint32_t sector_address);
void mik32sd_command_sector_write(MIK32SD_Descriptor_TypeDef *sd, uint32_t sector_address, char *src);
void mik32sd_command_sector_erase(MIK32SD_Descriptor_TypeDef *sd, uint32_t sector_address);
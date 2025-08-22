#pragma once

#include "mik32sd_spi.h"
#include "mik32sd_const.h"
#include "mik32sd_types.h"
#include "mik32sd_param.h"

MIK32SD_Status_TypeDef mik32_sd_init(MIK32SD_Descriptor_TypeDef *sd, MIK32SD_Config_TypeDef *cfg);
MIK32SD_Status_TypeDef mik32_sd_send_command(MIK32SD_Descriptor_TypeDef *sd, MIK32SD_CMD_TypeDef command, uint32_t operand, uint8_t crc, uint8_t* resp);
MIK32SD_Status_TypeDef mik32_sd_single_read(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr, uint8_t* buf);
MIK32SD_Status_TypeDef mik32_sd_single_write(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr, uint8_t* buf);
MIK32SD_Status_TypeDef mik32_sd_single_erase(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr);

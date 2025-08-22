#pragma once

#include <stdint.h>
#include "mik32sd_spi.h"

typedef union
{
    uint8_t all;
    struct
    {
        uint8_t idle_state : 1;
        uint8_t erase_reset : 1;
        uint8_t illegal_command : 1;
        uint8_t crc_error : 1;
        uint8_t erase_sequence_error : 1;
        uint8_t address_error : 1;
        uint8_t parameter_error : 1;
        uint8_t __res : 1;
    };
} MIK32SD_R1_Response_TypeDef;

typedef struct
{
    uint8_t card_is_locked : 1;
    uint8_t wp_erase_skip : 1;
    uint8_t error : 1;
    uint8_t cc_error : 1;
    uint8_t card_ecc_failed : 1;
    uint8_t wp_violation : 1;
    uint8_t erase_param : 1;
    uint8_t out_of_range :1;
} MIK32SD_R2_Response_TypeDef;

typedef struct
{
    uint8_t error : 1;
    uint8_t cc_error : 1;
    uint8_t ecc_failed : 1;
    uint8_t out_of_range : 1;
    uint8_t __res : 4;
} MIK32SD_SREAD_ErrToken_TypeDef;

typedef struct
{
    mik32_sd_spi_cfg_t spi;
} MIK32SD_Config_TypeDef;

typedef struct
{
    mik32_sd_spi_t spi;
    MIK32SD_Type_TypeDef type;
    MIK32SD_Voltage_TypeDef voltage;
} MIK32SD_Descriptor_TypeDef;
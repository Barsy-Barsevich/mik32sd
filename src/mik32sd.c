#include "mik32sd.h"

#define __SD_ERROR_CHECK(__errcode)     do {\
    if (__errcode != MIK32SD_STATUS_OK)\
    {\
        mik32_sd_spi_cs_up(&sd->spi);\
        return __errcode;\
    }\
} while(0);

MIK32SD_Status_TypeDef mik32sd_r1_decode
(
    uint8_t r1_res
)
{
    MIK32SD_R1_Response_TypeDef r1 = (MIK32SD_R1_Response_TypeDef)r1_res;
    if (r1.__res)
    {
        return MIK32SD_STATUS_R1_MSB;
    }
    else if (r1.parameter_error)
    {
        return MIK32SD_STATUS_R1_PARAMETER_ERROR;
    }
    else if (r1.address_error)
    {
        return MIK32SD_STATUS_R1_ADDRESS_ERROR;
    }
    else if (r1.erase_sequence_error)
    {
        return MIK32SD_STATUS_R1_ERASE_SEQ_ERROR;
    }
    else if (r1.crc_error)
    {
        return MIK32SD_STATUS_R1_COM_CRC_ERROR;
    }
    else if (r1.illegal_command)
    {
        return MIK32SD_STATUS_R1_ILLEGAL_COMMAND;
    }
    else if (r1.erase_reset)
    {
        return MIK32SD_STATUS_R1_ERASE_RESET;
    }
    else if (r1.idle_state)
    {
        return MIK32SD_STATUS_R1_IDLE_STATE;
    }
    else return MIK32SD_STATUS_OK;
}

MIK32SD_Status_TypeDef mik32_sd_send_command
(
    MIK32SD_Descriptor_TypeDef* sd,
    MIK32SD_CMD_TypeDef command,
    uint32_t operand, uint8_t crc,
    uint8_t* resp
)
{
    unsigned attempt_cnt = 0;
    uint8_t byte_res = 0xFF;
    do {
        byte_res = mik32_sd_spi_ex(&sd->spi, 0xFF);
        attempt_cnt += 1;
        if (attempt_cnt > MIK32SD_PRE_CMD_WAIT_CYCLES)
        {
            return MIK32SD_STATUS_COMMUNICATION_ERROR;
        }
    } while (byte_res != 0xFF);

    mik32_sd_spi_ex(&sd->spi, command);
    mik32_sd_spi_ex(&sd->spi, (operand>>24) & 0xFF);
    mik32_sd_spi_ex(&sd->spi, (operand>>16) & 0xFF);
    mik32_sd_spi_ex(&sd->spi, (operand>>8) & 0xFF);
    mik32_sd_spi_ex(&sd->spi, operand & 0xFF);
    mik32_sd_spi_ex(&sd->spi, crc);
    
    attempt_cnt = 0;
    do {
        byte_res = mik32_sd_spi_ex(&sd->spi, 0xFF);
        attempt_cnt += 1;
        if (attempt_cnt > MIK32SD_RESPONSE_WAIT_CYCLES)
        {
            return MIK32SD_STATUS_COMMUNICATION_ERROR;
        }
    } while (byte_res == 0xFF);
    resp[0] = byte_res;
    
    /* R7 response */
    if ((command == MIK32SD_CMD8) || (command == MIK32SD_CMD58))
    {
        resp[1] = mik32_sd_spi_ex(&sd->spi, 0xFF);
        resp[2] = mik32_sd_spi_ex(&sd->spi, 0xFF);
        resp[3] = mik32_sd_spi_ex(&sd->spi, 0xFF);
        resp[4] = mik32_sd_spi_ex(&sd->spi, 0xFF);
    }
    
    return MIK32SD_STATUS_OK;
}

MIK32SD_Status_TypeDef mik32_sd_init
(
    MIK32SD_Descriptor_TypeDef* sd,
    MIK32SD_Config_TypeDef *cfg
)
{
    bool init_res = mik32_sd_spi_init(&sd->spi, &cfg->spi);
    if (!init_res) return MIK32SD_STATUS_SPI_ERROR;

    /* 80 ticks on the SCK line */
    for (uint8_t i=0; i<10; i++)
    {
        mik32_sd_spi_ex(&sd->spi, 0xFF);
    }

    uint8_t resp_arr[5];
    MIK32SD_R1_Response_TypeDef *resp_r1 = (MIK32SD_R1_Response_TypeDef*)resp_arr;

    mik32_sd_spi_cs_down(&sd->spi);
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD0, 0, 0x95, resp_arr) );
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD8, 0x1AA, 0x87, resp_arr) );
    
    /* It is v1 SD-card or not-SD-card */
    // if (resp_arr[0] & SD_R1_ILLEGAL_COMMAND_M)
    // printf("R1 of CMD8 : 0x%02X\n", resp_arr[0]);
    if (resp_r1->illegal_command)
    {
        __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD58, 0, 0xFF, resp_arr) );
        uint32_t ocr = (
            (uint32_t)resp_arr[1]<<24 |
            (uint32_t)resp_arr[2]<<16 |
            (uint32_t)resp_arr[3]<<8 |
            (uint32_t)resp_arr[4]
        );
        // printf("R1 of CMD58 : 0x%02X\n", resp_arr[0]);
        // printf("CMD8 incorrect, OCR: 0x%08X, vol: 0x%08X\n", ocr, MIK32SD_VOLTAGE);
        if ( !(MIK32SD_VOLTAGE & ocr) )
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_INCORRECT_VOLTAGE;
        }
        if (resp_r1->illegal_command)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_UNKNOWN_CARD;
        }

        /* Trying to send AMIK32SD_CMD41 */
        __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD55, 0, 0xFF, resp_arr) );
        __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_ACMD41, 0x40000000, 0xFF, resp_arr) );

        /* It is a MMC */
        if (resp_r1->illegal_command)
        {
            /* >74 clock cycles on SCK */
            for (uint8_t i=0; i<10; i++)
            {
                mik32_sd_spi_ex(&sd->spi, 0xFF);
            }
            /* Go from idle_mode */
            uint32_t attempts_cnt = 0;
            while (resp_r1->idle_state)
            {
                __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD1, 0, 0xFF, resp_arr) );
                if (attempts_cnt++ > MIK32SD_GO_FROM_WAIT_CYCLES)
                {
                    mik32_sd_spi_cs_up(&sd->spi);
                    return MIK32SD_STATUS_TIMEOUT_ERROR;
                }
            }
            sd->type = MIK32SD_TYPE_MMC;
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_OK;
        }
        /* It is a SDv1 */
        else
        {
            /* Go from idle_mode */
            uint32_t attempts_cnt = 0;
            while (resp_r1->idle_state)
            {
                __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD55, 0, 0xFF, resp_arr) );
                __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_ACMD41, 0x40000000, 0xFF, resp_arr) );
                if (attempts_cnt++ > MIK32SD_GO_FROM_WAIT_CYCLES)
                {
                    mik32_sd_spi_cs_up(&sd->spi);
                    return MIK32SD_STATUS_TIMEOUT_ERROR;
                }
            }
            sd->type = MIK32SD_TYPE_SDV1;
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_OK;
        }
    }

    /* It is SD v2, SDHC or SDXC card */
    else
    {
        /* check the check_pattern */
        if (resp_arr[4] != 0xAA)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_COMMUNICATION_ERROR;
        }

        /* Check the card's valid voltage */
        __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD58, 0, 0xFF, resp_arr) );
        uint32_t ocr = (
            (uint32_t)resp_arr[1]<<24 |
            (uint32_t)resp_arr[2]<<16 |
            (uint32_t)resp_arr[3]<<8 |
            (uint32_t)resp_arr[4]);
        if ( !(MIK32SD_VOLTAGE & ocr) )
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_INCORRECT_VOLTAGE;
        }

        /* >74 clock cycles on SCK */
        for (uint8_t i=0; i<10; i++) 
        {
            mik32_sd_spi_ex(&sd->spi, 0xFF);
        }

        /* Go from idle_mode */
        uint32_t attempts_cnt = 0;
        while (resp_r1->idle_state)
        {
            __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD55, 0, 0xFF, resp_arr) );
            __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_ACMD41, 0x40000000, 0xFF, resp_arr) );
            if (attempts_cnt > 200)
            {
                mik32_sd_spi_cs_up(&sd->spi);
                return MIK32SD_STATUS_TIMEOUT_ERROR;
            }
        }

        /* Read the CCS value */
        __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD58, 0, 0xFF, resp_arr) );
        uint8_t ccs = resp_arr[1] & 0b01000000;
        if (ccs == 0)
        {
            sd->type = MIK32SD_TYPE_SDV2;
        }
        else
        {
            sd->type = MIK32SD_TYPE_SDHC;
        }
        mik32_sd_spi_cs_up(&sd->spi);
    }
    mik32_sd_spi_increase_clock_speed(&sd->spi);
    return MIK32SD_STATUS_OK;
}

MIK32SD_Status_TypeDef mik32_sd_single_read
(
    MIK32SD_Descriptor_TypeDef* sd,
    uint32_t addr,
    uint8_t* buf
)
{
    uint8_t resp;
    mik32_sd_spi_cs_down(&sd->spi);
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD17, addr, 0xff, &resp) );
    printf("%02X %02X\n", resp, mik32_sd_spi_ex(&sd->spi, 0xFF));
    __SD_ERROR_CHECK( mik32sd_r1_decode(resp) );

    // printf("CMD17 resp: 0x%02X\n", resp);
    if (resp != 0)
    {
        mik32_sd_spi_cs_up(&sd->spi);
        return MIK32SD_STATUS_OPERATION_ERROR;
    }
    
    unsigned counter = 0;
    while (resp != 0xFE)// && resp != 0xFC)
    {
        resp = mik32_sd_spi_ex(&sd->spi, 0xFF);
        if (counter >= MIK32SD_RESPONSE_WAIT_CYCLES)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_TIMEOUT_ERROR;
        }
        counter += 1;
    }

    /* Data receiving & transmitting via DMA */
    dma_status_t dma_res;
    dma_res = mik32_sd_spi_sector_read(&sd->spi, buf, MIK32SD_SECTOR_LEN_BYTES);
    if (dma_res != 0)
    {
        mik32_sd_spi_cs_up(&sd->spi);
        return MIK32SD_STATUS_OPERATION_ERROR;
    }

    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_cs_up(&sd->spi);
    return MIK32SD_STATUS_OK;
}

MIK32SD_Status_TypeDef mik32_sd_single_write
(
    MIK32SD_Descriptor_TypeDef* sd,
    uint32_t addr,
    uint8_t* buf
)
{
    uint8_t resp;
    mik32_sd_spi_cs_down(&sd->spi);
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD24, addr, 0xFF, &resp) );
    __SD_ERROR_CHECK( mik32sd_r1_decode(resp) );
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_ex(&sd->spi, MIK32SD_TOKEN_SINGLE_WRITE_START);
    
    /* Data transmitting via DMA */
    dma_status_t dma_res;
    dma_res = mik32_sd_spi_sector_write(&sd->spi, buf, MIK32SD_SECTOR_LEN_BYTES);
    if ((int)dma_res != 0)
    {
        mik32_sd_spi_cs_up(&sd->spi);
        return MIK32SD_STATUS_OPERATION_ERROR;
    }

    uint8_t data_response = 0xFF;
    unsigned data_resp_cnt = 0;
    while (data_response == 0xFF)
    {
        data_response = mik32_sd_spi_ex(&sd->spi, 0xFF);
        if (data_resp_cnt++ > MIK32SD_RESPONSE_WAIT_CYCLES)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_TIMEOUT_ERROR;
        }
    }
    
    data_response &= 0b00011111;
    uint8_t accepted = data_response ^ MIK32SD_DATA_RESP_ACCEPTED;

    uint8_t busy = 0;
    unsigned busy_cnt = 0;
    while (busy == 0)
    {
        busy = mik32_sd_spi_ex(&sd->spi, 0xFF);
        if (busy_cnt++ > MIK32SD_BUSY_WAIT_CYCLES)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_TIMEOUT_ERROR;
        }
    }

    if (accepted != 0)
    {
        printf("Data resp: 0x%02X\n", data_response);
        mik32_sd_spi_cs_up(&sd->spi);
        uint8_t rejected_crc = data_response ^ MIK32SD_DATA_RESP_REJECTED_CRC;
        if (rejected_crc == 0)
        {
            return MIK32SD_STATUS_DATA_RESP_CRC_ERROR;
        }
        uint8_t rejected_write_error = data_response ^ MIK32SD_DATA_RESP_REJECTED_WRITE_ERROR;
        if (rejected_write_error == 0)
        {
            return MIK32SD_STATUS_DATA_RESP_WRITE_ERROR;
        
        }
        else
        {
            return MIK32SD_STATUS_DATA_RESP_INCORRECT;
        }
    }
    
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    for (int i=0; i<10; i++) mik32_sd_spi_ex(&sd->spi, 0xFF);
    
    mik32_sd_spi_cs_up(&sd->spi);
    
    return MIK32SD_STATUS_OK;
}

MIK32SD_Status_TypeDef mik32_sd_single_erase
(
    MIK32SD_Descriptor_TypeDef* sd,
    uint32_t addr
)
{
    uint8_t resp;
    mik32_sd_spi_cs_down(&sd->spi);
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD32, addr, 0xFF, &resp) );
    __SD_ERROR_CHECK( mik32sd_r1_decode(resp) );
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD33, addr, 0xFF, &resp) );
    __SD_ERROR_CHECK( mik32sd_r1_decode(resp) );
    __SD_ERROR_CHECK( mik32_sd_send_command(sd, MIK32SD_CMD38, 0, 0xFF, &resp) );
    __SD_ERROR_CHECK( mik32sd_r1_decode(resp) );
    uint8_t busy = 0;
    unsigned busy_cnt = 0;
    while (busy == 0)
    {
        busy = mik32_sd_spi_ex(&sd->spi, 0xFF);
        if (busy_cnt++ > MIK32SD_BUSY_WAIT_CYCLES)
        {
            mik32_sd_spi_cs_up(&sd->spi);
            return MIK32SD_STATUS_TIMEOUT_ERROR;
        }
    }
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_ex(&sd->spi, 0xFF);
    mik32_sd_spi_cs_up(&sd->spi);
    return MIK32SD_STATUS_OK;
}
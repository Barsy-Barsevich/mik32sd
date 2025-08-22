#include "mik32sd_diag.h"

void mik32sd_diag_decode_status(MIK32SD_Status_TypeDef status)
{
    printf("Decode status %u: ", status);
    switch (status)
    {
    case MIK32SD_STATUS_OK:
        printf("OK");
        break;
    case MIK32SD_STATUS_INCORRECT_VOLTAGE:
        printf("Incorrect voltage");
        break;
    case MIK32SD_STATUS_UNKNOWN_CARD:
        printf("Unknown card");
        break;
    case MIK32SD_STATUS_TIMEOUT_ERROR:
        printf("Timeout error");
        break;
    case MIK32SD_STATUS_COMMUNICATION_ERROR:
        printf("Communication error");
        break;
    case MIK32SD_STATUS_OPERATION_ERROR:
        printf("Operation error");
        break;
    case MIK32SD_STATUS_SPI_ERROR:
        printf("SPI error");
        break;
    case MIK32SD_STATUS_R1_MSB:
        printf("R1 response MSB set");
        break;
    case MIK32SD_STATUS_R1_PARAMETER_ERROR:
        printf("Parameter error");
        break;
    case MIK32SD_STATUS_R1_ADDRESS_ERROR:
        printf("Address error");
        break;
    case MIK32SD_STATUS_R1_ERASE_SEQ_ERROR:
        printf("Erase seq error");
        break;
    case MIK32SD_STATUS_R1_COM_CRC_ERROR:
        printf("Com CRC error");
        break;
    case MIK32SD_STATUS_R1_ILLEGAL_COMMAND:
        printf("Illegal command");
        break;
    case MIK32SD_STATUS_R1_ERASE_RESET:
        printf("Erase reset");
        break;
    case MIK32SD_STATUS_R1_IDLE_STATE:
        printf("Idle state");
        break;
    case MIK32SD_STATUS_DATA_RESP_CRC_ERROR:
        printf("Data response: CRC error while writing");
        break;
    case MIK32SD_STATUS_DATA_RESP_WRITE_ERROR:
        printf("Data response: Write error while writing");
        break;
    case MIK32SD_STATUS_DATA_RESP_INCORRECT:
        printf("Data response: incorrect data resp format");
        break;
    default:
        printf("Unexpected error");
    }
    printf("\n");
}

void mik32sd_diag_decode_sd_type(MIK32SD_Type_TypeDef type)
{
    printf("Decoding type: ");
    switch (type)
    {
    case MIK32SD_TYPE_SDV1:
        printf("SDv1");
        break;
    case MIK32SD_TYPE_SDV2:
        printf("SDv2");
        break;
    case MIK32SD_TYPE_SDHC:
        printf("SDHC");
        break;
    case MIK32SD_TYPE_MMC:
        printf("MMC");
        break;
    case MIK32SD_TYPE_UNKNOWN:
        printf("Unknown");
        break;
    default:
        printf("Unexpected value (0x%02X)", (unsigned)type);
    }
    printf("\n");
}
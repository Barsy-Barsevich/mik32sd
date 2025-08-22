#include "mik32sd_command.h"

void mik32sd_command_sector_read(MIK32SD_Descriptor_TypeDef *sd, uint32_t sector_address, char *__restrict dst)
{
    printf("mik32sd_sector_read: ");
    mik32sd_diag_decode_status(mik32_sd_single_read(sd, sector_address, dst));
}

void mik32sd_command_sector_dump(uint32_t sector_address)
{

}

void mik32sd_command_sector_write(MIK32SD_Descriptor_TypeDef *sd, uint32_t sector_address, char *src)
{
    printf("mik32sd_sector_write: ");
    mik32sd_diag_decode_status(mik32_sd_single_write(sd, sector_address, src));
}

void mik32sd_command_sector_erase(MIK32SD_Descriptor_TypeDef *sd, uint32_t sector_address)
{
    printf("mik32sd_sector_erase: ");
    mik32sd_diag_decode_status(mik32_sd_single_erase(sd, sector_address));
}
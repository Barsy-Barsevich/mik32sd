#include "mik32sd_command.h"


void mik32sd_command_csdown(MIK32SD_Descriptor_TypeDef *sd)
{
    mik32_sd_spi_cs_down(&sd->spi);
}

void mik32sd_command_csup(MIK32SD_Descriptor_TypeDef *sd)
{
    mik32_sd_spi_cs_up(&sd->spi);
}

void mik32sd_command_spiex(MIK32SD_Descriptor_TypeDef *sd, const char *din)
{
    uint8_t byte_din = 0;
    if (din[0] != '\0')
    {
        byte_din = din[0]-0x30;
        if (byte_din > 9) byte_din -= 7;
        if (byte_din > 15) byte_din -= 0x20;
        if (din[1] != '\0')
        {
            byte_din <<= 4;
            uint8_t dummy = din[1];
            if (dummy > 9) dummy -= 7;
            if (dummy > 15) dummy -= 0x20;
            byte_din |= dummy;
        }
    }
    uint8_t res = mik32_sd_spi_ex(&sd->spi, byte_din);
    printf("0x%02X\n", res);
}



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
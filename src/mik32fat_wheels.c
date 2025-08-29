#include "mik32fat_wheels.h"

int mik32fat_wheels_single_read
(
    void *__restrict cookie,
    uint32_t sector_addr,
    uint8_t *dst)
{
    if (cookie == NULL || dst == NULL)
    {
        return -1;
    }
    MIK32SD_Descriptor_TypeDef *sd = (MIK32SD_Descriptor_TypeDef*)cookie;
    MIK32SD_Status_TypeDef res;
    res = mik32_sd_single_read(sd, sector_addr, dst);
    return (int)res;
}

int mik32fat_wheels_single_write
(
    void *__restrict cookie,
    uint32_t sector_addr,
    const uint8_t *src
)
{
    if (cookie == NULL || src == NULL)
    {
        return -1;
    }
    MIK32SD_Descriptor_TypeDef *sd = (MIK32SD_Descriptor_TypeDef*)cookie;
    MIK32SD_Status_TypeDef res;
    res = mik32_sd_single_write(sd, sector_addr, src);
    return (int)res;
}

int mik32fat_wheels_single_erase
(
    void *__restrict cookie,
    uint32_t sector_addr
)
{
    if (cookie == NULL)
    {
        return -1;
    }
    MIK32SD_Descriptor_TypeDef *sd = (MIK32SD_Descriptor_TypeDef*)cookie;
    MIK32SD_Status_TypeDef res;
    res = mik32_sd_single_erase(sd, sector_addr);
    return (int)res;
}


MIK32FAT_Status_TypeDef __mik32fat_sector_sread(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector)
{
    /* Read sector only if has not already been buffered */
    if (sector != fs->prev_sector)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
        fs->prev_sector = sector;
    }
    return MIK32FAT_STATUS_OK;
}

MIK32FAT_Status_TypeDef __mik32fat_sector_swrite(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector)
{
    return mik32fat_wheels_single_write(fs->card, sector, fs->buffer);
}

MIK32FAT_Status_TypeDef __mik32fat_sector_serase(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector)
{
    return mik32fat_wheels_single_erase(fs->card, sector);
}
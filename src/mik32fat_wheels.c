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
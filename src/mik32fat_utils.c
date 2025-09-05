#include "mik32fat_utils.h"


/* !
 * @brief 
 */
MIK32FAT_Status_TypeDef mik32fat_utils_ls(MIK32FAT_Descriptor_TypeDef *fs, FILE *output)
{
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    uint32_t entires_per_sector = fs->sector_len_bytes / sizeof(MIK32FAT_Entire_TypeDef);
    char name[20];
    MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
    while (res == MIK32FAT_STATUS_OK)
    {   
        uint32_t sector = fs->data_region_begin + fs->temp.cluster * fs->param.sec_per_clust;
        for (int i=0; i < fs->param.sec_per_clust; i++)
        {
            res = __mik32fat_sector_sread(fs, sector+i);
            if (res != MIK32FAT_STATUS_OK)
            {
                fs->temp = temp;
                return res;
            }
            MIK32FAT_Entire_TypeDef *entire = (MIK32FAT_Entire_TypeDef*)fs->buffer;
            for (int j=0; j<entires_per_sector; j++)
            {
                uint8_t first_name_sym = ((uint8_t*)entire[j].Name)[0];
                if (first_name_sym == 0x00) break;
                if (first_name_sym != 0xE5)
                {
                    mik32fat_name_read_from_entire(entire+j, name);
                    fprintf(output, "%06b %s\n", entire[j].Attr, name);
                }
            }
        }
        res = mik32fat_find_next_cluster(fs);
    }
    if (res == MIK32FAT_STATUS_NOT_FOUND)
    {
        res = MIK32FAT_STATUS_OK;
    }
    fs->temp = temp;
    return res;
}
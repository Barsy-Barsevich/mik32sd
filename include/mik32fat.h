#pragma once

#include <string.h>
#include <stdbool.h>
#include "mik32sd.h"
#include "mik32fat_const.h"
#include "mik32fat_types.h"
#include "mik32fat_param.h"
#include "mik32fat_wheels.h"

#include "mik32fat_file.h"

MIK32FAT_Status_TypeDef mik32fat_init(MIK32FAT_Descriptor_TypeDef *fs, MIK32SD_Descriptor_TypeDef *sd_card);
MIK32FAT_Status_TypeDef mik32fat_set_pointer_to_root(MIK32FAT_Descriptor_TypeDef *fs);
MIK32FAT_Status_TypeDef mik32fat_find_next_cluster(MIK32FAT_Descriptor_TypeDef* fs);
MIK32FAT_Status_TypeDef mik32fat_find_by_name(MIK32FAT_Descriptor_TypeDef *fs, const char *name);
MIK32FAT_Status_TypeDef mik32fat_find_by_path(MIK32FAT_Descriptor_TypeDef *fs, const char *path);
MIK32FAT_Status_TypeDef mik32fat_find_or_create_by_path(MIK32FAT_Descriptor_TypeDef *fs, const char *path);
MIK32FAT_Status_TypeDef mik32fat_take_free_cluster(MIK32FAT_Descriptor_TypeDef* fs, uint32_t clust, uint32_t* new_clust);
MIK32FAT_Status_TypeDef mik32fat_create(MIK32FAT_Descriptor_TypeDef* fs, const char* name, bool dir);
MIK32FAT_Status_TypeDef mik32fat_delete(MIK32FAT_Descriptor_TypeDef* fs, const char* name);

inline MIK32FAT_Status_TypeDef MIK32FAT_I(MIK32FAT_Descriptor_TypeDef *fs, MIK32SD_Descriptor_TypeDef *sd_card)
{
    return mik32fat_init(fs, sd_card);
}

inline MIK32FAT_Status_TypeDef MIK32FAT_SPR(MIK32FAT_Descriptor_TypeDef *fs)
{
    return mik32fat_set_pointer_to_root(fs);
}

inline MIK32FAT_Status_TypeDef MIK32FAT_FNC(MIK32FAT_Descriptor_TypeDef* fs)
{
    return mik32fat_find_next_cluster(fs);
}

inline MIK32FAT_Status_TypeDef MIK32FAT_FBN(MIK32FAT_Descriptor_TypeDef *fs, const char *name)
{
    return mik32fat_find_by_name(fs, name);
}

inline MIK32FAT_Status_TypeDef MIK32FAT_FBP(MIK32FAT_Descriptor_TypeDef *fs, const char *path)
{
    return mik32fat_find_by_path(fs, path);
}
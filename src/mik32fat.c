#include "mik32fat.h"

uint32_t __mik32fat_get_sector(MIK32FAT_Descriptor_TypeDef *fs, uint32_t cluster, uint32_t sector_offset)
{
    return fs->data_region_begin + cluster * fs->param.sec_per_clust + sector_offset;
}

int mik32fat_name_read_from_entire(const MIK32FAT_Entire_TypeDef *entire, char *dst_name)
{
    int i = 0;
    int j = 0;
    while (i < 8 && entire->Name[i] != ' ')
    {
        dst_name[j++] = entire->Name[i++];
    }
    if (entire->Extention[0] != ' ')
    {
        dst_name[j++] = '.';
        i = 0;
        while (i < 3 && entire->Extention[i] != ' ')
        {
            dst_name[j++] = entire->Extention[i++];
        }
    }
    dst_name[j++] = '\0';
    return j;
}


MIK32FAT_Status_TypeDef mik32fat_init(MIK32FAT_Descriptor_TypeDef* fs, MIK32SD_Descriptor_TypeDef* disk)
{
    if (fs == NULL || disk == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    fs->sector_len_bytes = 512;
    fs->card = disk;
    /* Read Master boot record */
    __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, 0, fs->buffer) );
    /* Read LBAs */
    uint8_t counter = 0;
    uint8_t type_code;
    uint8_t* ptr = fs->buffer + MIK32FAT_MBR_PART0_OFFSET;
    /* Find not-empty partition */
    while (counter < 4)
    {
        type_code = ptr[MIK32FAT_PART_TYPECODE_OFFSET];
        if (type_code == MIK32FAT_FS_CODE_FAT32_WITHOUT_LFN)
        {
            break;
        }
        else if (type_code == MIK32FAT_FS_CODE_FAT32_WITH_LFN)
        {
            break;
        }
        counter += 1;
        ptr += MIK32FAT_MBR_PARTITION_LEN_BYTES;
    }
    if (counter == 4)
    {
        return MIK32FAT_STATUS_DISK_NOT_FORM;
    }
    /* Read LBA startaddr. It is a start address of file system */
    fs->fs_begin = (uint32_t)(ptr[MIK32FAT_PART_LBA_BEGIN_OFFSET+3]<<24) |
        (uint32_t)(ptr[MIK32FAT_PART_LBA_BEGIN_OFFSET+2]<<16) |
        (uint32_t)(ptr[MIK32FAT_PART_LBA_BEGIN_OFFSET+1]<<8) |
        (uint32_t)ptr[MIK32FAT_PART_LBA_BEGIN_OFFSET];
    /* Read LBA sector */
    __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fs_begin, fs->buffer) );
    fs->prev_sector = fs->fs_begin;

    fs->param.sec_per_clust = fs->buffer[MIK32FAT_BPB_SECTORS_PER_CLUSTER_OFFSET];
    /* Read number of sectors of reserved file system's region */
    uint16_t num_of_res_sec = (uint16_t)(fs->buffer[MIK32FAT_BPB_RES_SECTORS_COUNT_OFFSET+1]<<8) |
        fs->buffer[MIK32FAT_BPB_RES_SECTORS_COUNT_OFFSET];
    /* Read FAT's startaddesses and length */
    fs->fat1_begin = fs->fs_begin + num_of_res_sec;
    fs->param.num_of_fats = fs->buffer[MIK32FAT_BPB_NUM_OF_FATS_OFFSET];
    fs->param.fat_length = (uint32_t)(fs->buffer[MIK32FAT_BPB_FAT_SIZE_OFFSET+3]<<24) |
        (uint32_t)(fs->buffer[MIK32FAT_BPB_FAT_SIZE_OFFSET+2]<<16) |
        (uint32_t)(fs->buffer[MIK32FAT_BPB_FAT_SIZE_OFFSET+1]<<8) |
        (uint32_t)(fs->buffer[MIK32FAT_BPB_FAT_SIZE_OFFSET]);
    if (fs->param.num_of_fats == 2)
    {
        fs->fat2_begin = fs->fat1_begin + fs->param.fat_length;
    }
    fs->param.clust_len_bytes = fs->sector_len_bytes * fs->param.sec_per_clust;
    /* Calculate a start address of file system's data region */
    fs->data_region_begin = fs->fat1_begin + fs->param.num_of_fats * fs->param.fat_length;
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief Set file system temp-pointer [fs.temp.cluster] to root directory
 * @param fs file system's descriptor-structure
 * @return none
 */
MIK32FAT_Status_TypeDef mik32fat_set_pointer_to_root(MIK32FAT_Descriptor_TypeDef* fs)
{
    if (fs == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    //fs->temp.cluster = fs->data_region_begin;
    fs->temp.cluster = 0;
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief The function finds the cluster next to [fs.temp.cluster] cluster and puts
 * number of new cluster to [fs.temp.cluster]
 * @param fs file system's descriptor-structure
 * @returns
 * - MIK32FAT_STATUS_OK the next cluster was found succesfully, its pointer was saved to [fs.temp.cluster]
 * - MIK32FAT_STATUS_DISK_ERROR - the driver error occured, [fs.temp.cluster] not changed
 * - MIK32FAT_STATUS_NOT_FOUND - there are not any next clusters, [fs.temp.cluster] not changed
 */
MIK32FAT_Status_TypeDef mik32fat_find_next_cluster(MIK32FAT_Descriptor_TypeDef* fs)
{
    if (fs == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    
    uint32_t fat_sector = (fs->temp.cluster + 2) / (fs->sector_len_bytes/sizeof(uint32_t));
    uint32_t link_idx = (fs->temp.cluster + 2) % (fs->sector_len_bytes/sizeof(uint32_t));
    // printf("Link idx: %u\n", (unsigned)link_idx);
    uint32_t* link_in_sector = (uint32_t*)fs->buffer;
    
    __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, fs->fat1_begin + fat_sector) );
    uint32_t link = link_in_sector[link_idx];
    // printf("Link: 0x%X\n", link);
    if ((link & 0x0FFFFFFF) >= 0x0FFFFFF7)
    {
        return MIK32FAT_STATUS_NOT_FOUND;
    }
    else
    {
        fs->temp.cluster = link-2;
        return MIK32FAT_STATUS_OK;
    }
}


/**
 * @brief Find the number of 1st cluster of file/subdirectory in the directory [fs.temp.cluster]
 * and puts it to [fs.temp.cluster]
 * @param fs pointer to file system's structure-descriptor
 * @param name string of name. The last byte should be '\0' or '/'.
 * If the name contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @return cluster - the 1st cluster of file/dir. dir_sector - the number of sector of directory.
 * len - length of file. entire_in_dir_clust - number of entire in dir_sector. status - status of file/dir
 */
MIK32FAT_Status_TypeDef mik32fat_find_by_name(MIK32FAT_Descriptor_TypeDef *fs, const char *name)
{
    if (fs == NULL || name == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    // Special case of '.' name
    if (name[0] == '.' && name[1] == '\0')
    {
        if ((fs->temp.status & MIK32FAT_ATTR_DIRECTORY) == 0)
        {
            return MIK32FAT_STATUS_ERROR;
        }
        else if (fs->temp.cluster == 0)
        {
            return MIK32FAT_STATUS_NOT_FOUND;
        }
        else
        {
            return MIK32FAT_STATUS_OK;
        }
    }

    // printf("Name: *");
    // int u = 0;
    // while (name[u] != '\0' && name[u] != '/')
    // {
    //     putchar(name[u++]);
    // }
    // printf("*\n");

    char name_str[11];
    /* Preparing the name string */
    size_t i = 0;
    size_t j = 0;
    bool point_detected = false;
    while (name[i] != '\0' && name[i] != '/')
    {
        if (!point_detected)
        {
            if (j > 8)
            {
                printf("p1");
                return MIK32FAT_STATUS_NAME_ERROR;
            }
        }
        else
        {
            if (j >= 11)
            {
                printf("p2");
                return MIK32FAT_STATUS_NAME_ERROR;
            }
        }
        if (name[i] == '.')
        {
            if (i == 0)
            {
                if (name[1] == '\0' || name[1] == '/')
                {
                    name_str[0] = '.';
                    j = 1;
                    while (j < 11)
                    {
                        name_str[j++] = ' ';
                    }
                    break;
                }
                else if (name[1] == '.' && (name[2] == '\0' || name[2] == '/'))
                {
                    name_str[0] = '.';
                    name_str[1] = '.';
                    j = 2;
                    while (j < 11)
                    {
                        name_str[j++] = ' ';
                    }
                    break;
                }
                else 
                {
                    printf("p3");
                    return MIK32FAT_STATUS_NAME_ERROR;
                }
            }
            while (j < 8)
            {
                name_str[j++] = ' ';
            }
            i += 1;
            point_detected = true;
        }
        else
        {
            name_str[j++] = name[i++];
        }
    }
    while (j < 11)
    {
        name_str[j++] = ' ';
    }

    MIK32FAT_TempData_TypeDef temp = fs->temp;
    /* Finding process */
    MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
    while (res == MIK32FAT_STATUS_OK)
    {
        uint32_t sector;
        for (uint8_t sec=0; sec < fs->param.sec_per_clust; sec++)
        {
            /* Read sector data */
            // sector = fs->data_region_begin + fs->temp.cluster * fs->param.sec_per_clust + sec;
            sector = __mik32fat_get_sector(fs, fs->temp.cluster, sec);
            res = __mik32fat_sector_sread(fs, sector);
            if (res != MIK32FAT_STATUS_OK)
            {
                fs->temp = temp;
                return res;
            }
            /* Try to find the name in sector */
            uint16_t entire = 0;
            while (entire < fs->sector_len_bytes && memcmp(name_str, fs->buffer+entire, 11) != 0)
            {
                entire += MIK32FAT_ENTIRE_SIZE_BYTES;
            }
            if (entire >= fs->sector_len_bytes) continue;
            /* The correct name has been found */
            /* Save parameters */
            fs->temp.entire_in_dir_clust = entire;
            fs->temp.dir_cluster = fs->temp.cluster;
            fs->temp.dir_sec_offset = sec;
            fs->temp.cluster = (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FIRST_CLUS_HI_OFFSET+0]<<16 |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FIRST_CLUS_HI_OFFSET+1]<<24 |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FIRST_CLUS_LO_OFFSET+0] |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FIRST_CLUS_LO_OFFSET+1]<<8;
            if (fs->temp.cluster > 2) fs->temp.cluster -= 2;
            fs->temp.len = (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FILE_SIZE_OFFSET+0] |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FILE_SIZE_OFFSET+1]<<8 |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FILE_SIZE_OFFSET+2]<<16 |
                (uint32_t)fs->buffer[entire+MIK32FAT_DIR_FILE_SIZE_OFFSET+3]<<24;
            fs->temp.status = fs->buffer[entire+MIK32FAT_DIR_ATTR_OFFSET];
            // strcpy(fs->temp.name, name);
            printf("FBN: temp: %u, dir: %u\n", fs->temp.cluster, fs->temp.dir_cluster);
            return MIK32FAT_STATUS_OK;
        }
        res = mik32fat_find_next_cluster(fs);
    }
    fs->temp = temp;
    return res;
}


/**
 * @brief Find the number of 1st cluster of file by the path
 * @param fs pointer to file system's structure-descriptor
 * @param path string of path. The last byte should be '\0'.
 * If the path contain subdirectories, they are separated by '/' symbol (i.e.: "FOLDER/FILE").
 * If the name of file or subdir contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @return cluster - the 1st cluster of file/dir. dir_sector - the number of sector of directory.
 * len - length of file. entire_in_dir_clust - number of entire in dir_sector. status - status of file/dir
 */
MIK32FAT_Status_TypeDef mik32fat_find_by_path(MIK32FAT_Descriptor_TypeDef *fs, const char *path)
{
    if (fs == NULL || path == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    /* calculate number of '/' symbols */
    uint8_t descend_number = 1;
    uint8_t i = 0;
    while (path[i] != '\0')
    {
        if (path[i] == '/') descend_number += 1;
        i += 1;
    }
    /* Descend into directories and files */
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    MIK32FAT_Status_TypeDef res;
    size_t j = 0;
    for (uint8_t k=0; k<descend_number; k++)
    {
        res = mik32fat_find_by_name(fs, path+j);
        if (res != MIK32FAT_STATUS_OK)
        {
            fs->temp = temp;
            return res;
        }
        if (k == descend_number-1)
        {
            strcpy(fs->temp.name, path+j);
        }
        else
        {
            /* Find next name in path */
            while (path[j++] != '/');
        }
    }
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief Find the number of 1st cluster of the file by path. If there are
 * not any subdirectories of file in the path, the function creates them 
 * @param fs pointer to file system's structure-descriptor
 * @param path string of path. The last byte should be '\0'.
 * If the path contain subdirectories, they are separated by '/' symbol (i.e.: "FOLDER/FILE").
 * If the name of file or subdir contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @return cluster - the 1st cluster of file/dir. dir_sector - the number of sector of directory.
 * len - length of file. entire_in_dir_clust - number of entire in dir_sector. status - status of file/dir
 */
MIK32FAT_Status_TypeDef mik32fat_find_or_create_by_path(MIK32FAT_Descriptor_TypeDef *fs, const char *path, bool dir)
{
    if (fs == NULL || path == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    /* Adopted FAT_FBP. If dir/file not found, create it */
    /* calculate number of '/' symbols */
    uint8_t descend_number = 1;
    uint8_t i = 0;
    while (path[i] != '\0')
    {
        if (path[i] == '/') descend_number += 1;
        i += 1;
    }
    /* Descend into directories and files */
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    MIK32FAT_Status_TypeDef res;
    bool not_found = false;
    // char* ptr = path;
    size_t j = 0;
    for (uint8_t k=0; k<descend_number; k++)
    {
        if (!not_found)
        {
            res = mik32fat_find_by_name(fs, path+j);
            if (res == MIK32FAT_STATUS_NOT_FOUND)
            {
                not_found = true;
            }
            else if (res != MIK32FAT_STATUS_OK)
            {
                fs->temp = temp;
                return res;
            }
        }
        if (not_found)
        {
            __MIK32FAT_ERROR_CHECK( mik32fat_create(fs, path+j, k != (descend_number-1) ? true : dir) );
            /* Descend into created object */
            __MIK32FAT_ERROR_CHECK( mik32fat_find_by_name(fs, path+j) );
        }
        while (path[j] != '/') j += 1;
        j += 1;
    }
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief Finding new free cluster
 * @param fs pointer to file system's structure-descriptor
 * @param new_cluster pointer to new cluster variable
 * @returns
 * - MIK32FAT_STATUS_OK
 * - MIK32FAT_STATUS_DISK_ERROR error while reading occurs
 * - MIK32FAT_STATUS_NO_FREE_SPACE there are not free space on the partition
 */
MIK32FAT_Status_TypeDef mik32fat_find_free_cluster(MIK32FAT_Descriptor_TypeDef *fs, uint32_t *new_cluster)
{
    if (fs == NULL || new_cluster == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    /* Find free cluster in FAT */
    uint32_t* ptr;
    int32_t x = -1;
    int32_t link;
    do {
        x += 1;
        __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, fs->fat1_begin + x) );
        link = -4;
        do {
            link += 4;
            ptr = (uint32_t*)(fs->buffer + link);
            //xprintf("*%08X*\n", *ptr);
        } while ((*ptr != 0) && (link < 512));
    }
    while ((*ptr != 0) && (x < fs->param.fat_length));
    if (x >= fs->param.fat_length)
    {
        return MIK32FAT_STATUS_NO_FREE_SPACE;
    }
    /* link is number of free cluster in fat sector */
    /* Save number of new cluster */
    *new_cluster = (x * 128 + (link>>2));
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief Continue file/directory with new free cluster
 * @param fs pointer to file system's structure-descriptor
 * @param cluster temporary file/directory cluster
 * @param new_cluster pointer to new cluster variable
 * @returns
 * - MIK32FAT_STATUS_OK
 * - MIK32FAT_STATUS_DISK_ERROR error while reading occurs
 * - MIK32FAT_STATUS_NO_FREE_SPACE there are not free space on the partition
 */
MIK32FAT_Status_TypeDef mik32fat_take_free_cluster
(
    MIK32FAT_Descriptor_TypeDef *fs,
    uint32_t cluster,
    uint32_t *new_cluster
)
{
    if (fs == NULL || new_cluster == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    /* Find free cluster in FAT */
    uint32_t fat_sector = 0;
    uint32_t _newclus = 0;
    uint32_t *link = (uint32_t*)fs->buffer;
    uint32_t links_per_sec = fs->sector_len_bytes / sizeof(uint32_t);
    while (fat_sector < fs->param.fat_length && _newclus == 0)
    {
        __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, fs->fat1_begin + fat_sector) );
        for (size_t i=0; i<links_per_sec; i++)
        {
            if (fat_sector == 0 && i < 2) continue;
            if (link[i] == 0)
            {
                _newclus = fat_sector * links_per_sec + i;
                link[i] = 0x0FFFFFFF;
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat1_begin + fat_sector) );
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat2_begin + fat_sector) );
                break;
            }
        }
        fat_sector += 1;
    }
    if (_newclus == 0)
    {
        return MIK32FAT_STATUS_NO_FREE_SPACE;
    }
    _newclus -= 2;
    *new_cluster = _newclus;

    /* Find sector of FAT containing previous cluster link */
    uint32_t prev_link_fat1_sector = fs->fat1_begin + ((cluster+2) / (fs->sector_len_bytes / sizeof(uint32_t)));
    uint32_t prev_link_fat2_sector = fs->fat2_begin + ((cluster+2) / (fs->sector_len_bytes / sizeof(uint32_t)));
    __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, prev_link_fat1_sector) );
    // uint32_t *link = (uint32_t*)fs->buffer;
    uint32_t prev_clus_link_idx = (cluster+2) % (fs->sector_len_bytes/sizeof(uint32_t));
    link[prev_clus_link_idx] = _newclus+2;
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, prev_link_fat1_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, prev_link_fat1_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, prev_link_fat2_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, prev_link_fat2_sector) );
    /* Find sector of FAT containing previous cluster link */
    uint32_t this_link_fat1_sector = fs->fat1_begin + ((cluster+2) / (fs->sector_len_bytes / sizeof(uint32_t)));
    uint32_t this_link_fat2_sector = fs->fat2_begin + ((cluster+2) / (fs->sector_len_bytes / sizeof(uint32_t)));
    __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, this_link_fat1_sector) );
    uint32_t this_clus_link_idx = (_newclus+2) % (fs->sector_len_bytes/sizeof(uint32_t));
    link[this_clus_link_idx] = 0x0FFFFFFF;
    //xprintf("Write link 0x0FFFFFFF on field %04X\n", new_clus);
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, this_link_fat1_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, this_link_fat1_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, this_link_fat2_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, this_link_fat2_sector) );
    return MIK32FAT_STATUS_OK;
}


/**
 * @brief File or directory creation
 * @param fs pointer to file system's structure-descriptor
 * @param path string of path. The last byte should be '\0'.
 * If the path contain subdirectories, they are separated by '/' symbol (i.e.: "FOLDER/FILE").
 * If the name of file or subdir contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @param dir true - create directory, false - create file
 * @returns
 */
MIK32FAT_Status_TypeDef mik32fat_create
(
    MIK32FAT_Descriptor_TypeDef *fs,
    const char *name,
    bool dir
)
{
    if (fs == NULL || name == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    char name_str[11];
    /* Preparing the name string */
    size_t i = 0;
    size_t j = 0;
    bool point_detected = false;
    while (name[i] != '\0' && name[i] != '/')
    {
        if (!point_detected)
        {
            if (j > 8)
            {
                return MIK32FAT_STATUS_NAME_ERROR;
            }
        }
        else
        {
            if (j >= 11)
            {
                return MIK32FAT_STATUS_NAME_ERROR;
            }
        }
        if (name[i] == '.')
        {
            if (i == 0)
            {
                return MIK32FAT_STATUS_NAME_ERROR;
            }
            while (j < 8)
            {
                name_str[j++] = ' ';
            }
            i += 1;
            point_detected = true;
        }
        else
        {
            name_str[j++] = name[i++];
            printf("%c", name_str[j-1]);
        }
    }
    while (j < 11)
    {
        name_str[j++] = ' ';
    }

    /* Find free cluster in FAT */
    size_t fat_sector = 0;
    uint32_t new_cluster = 0;
    uint32_t *link = (uint32_t*)fs->buffer;
    uint32_t links_per_sec = fs->sector_len_bytes / sizeof(uint32_t);
    while (fat_sector < fs->param.fat_length && new_cluster == 0)
    {
        __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, fs->fat1_begin + fat_sector) );
        for (size_t i=0; i<links_per_sec; i++)
        {
            if (fat_sector == 0 && i < 2) continue;
            if (link[i] == 0)
            {
                new_cluster = fat_sector * links_per_sec + i;
                link[i] = 0x0FFFFFFF;
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat1_begin + fat_sector) );
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat2_begin + fat_sector) );
                break;
            }
        }
        fat_sector += 1;
    }
    if (new_cluster == 0)
    {
        return MIK32FAT_STATUS_NO_FREE_SPACE;
    }

    /* Set /. and /.. entires in new cluster if directory (look MS FAT Specification) */
    if (dir == true)
    {
        memset(fs->buffer, 0x00, fs->sector_len_bytes);
        // Clear all the sectors of new dir cluster
        uint32_t sector = fs->data_region_begin+(new_cluster-2)*fs->param.sec_per_clust;
        for (int i=1; i<fs->param.sec_per_clust; i++)
        {
            __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, sector+i) );
            __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, sector+i) );
        }
        MIK32FAT_Entire_TypeDef *ent = (MIK32FAT_Entire_TypeDef*)fs->buffer;
        memcpy(ent[0].Name, ".          ", 11);
        ent[0].Attr = MIK32FAT_ATTR_DIRECTORY;
        ent[0].FstClusLO = (uint16_t)new_cluster;
        ent[0].FstClusHI = (uint16_t)(new_cluster >> 16);
        memcpy(ent[1].Name, "..         ", 11);
        ent[1].Attr = MIK32FAT_ATTR_DIRECTORY;
        uint32_t cluster = fs->temp.cluster;
        if (cluster > 2) cluster += 2;
        ent[1].FstClusLO = (uint16_t)cluster;
        ent[1].FstClusHI = (uint16_t)(cluster >> 16);
        /* Write data */
        __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, sector) );
        __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, sector) );
    }

    /* Find free space for descriptor in directory */
    uint32_t sector;
    uint16_t entire = 0;
    MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    while (res == MIK32FAT_STATUS_OK)
    {
        sector = fs->data_region_begin + fs->temp.cluster * fs->param.sec_per_clust;
        for (uint8_t idx=0; idx < fs->param.sec_per_clust; idx++)
        {
            __SAVING_TEMP_ERROR_CHECK( __mik32fat_sector_sread(fs, sector) );
            sector += 1;
            entire = 0;
            while (entire < fs->sector_len_bytes)
            {
                if ((fs->buffer[entire] == 0x00) || (fs->buffer[entire] == 0xE5)) break;
                entire += 32;
            }
            if (entire < fs->sector_len_bytes)
            {
                sector -= 1;
                break;
            }
        }
        if (entire < fs->sector_len_bytes) break;
        res = mik32fat_find_next_cluster(fs);
    }
    /* FAT_FNC error. if next cluster not found, take a free cluster */
    if (res == MIK32FAT_STATUS_NOT_FOUND)
    {
        uint32_t value;
        __SAVING_TEMP_ERROR_CHECK( mik32fat_take_free_cluster(fs, fs->temp.cluster, &value));
        entire = 0;
        sector = fs->data_region_begin + value * fs->param.sec_per_clust;
        //__DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
    }
    else if (res != MIK32FAT_STATUS_OK) return res;
    /* entire contains pointer to descriptor in directory's sector (sector) */
    memset(fs->buffer+entire+11, 0x00, 32-11);

    __SAVING_TEMP_ERROR_CHECK( __mik32fat_sector_sread(fs, sector) );
    fs->prev_sector = sector;
    MIK32FAT_Entire_TypeDef *new_obj = (MIK32FAT_Entire_TypeDef*)(fs->buffer + entire);
    new_obj->FileSize = 0;
    new_obj->Attr = (dir ? MIK32FAT_ATTR_DIRECTORY : 0);
    new_obj->FstClusLO = (uint16_t)new_cluster;
    new_obj->FstClusHI = (uint16_t)(new_cluster >> 16);
    memcpy(new_obj->Name, name_str, 11);
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, sector) );
    return MIK32FAT_STATUS_OK;
}


MIK32FAT_Status_TypeDef mik32fat_delete_temp_object(MIK32FAT_Descriptor_TypeDef* fs)
{
    /* Set the 0th byte of entire as 0xE5 */
    uint32_t dir_sector = __mik32fat_get_sector(fs, fs->temp.dir_cluster, fs->temp.dir_sec_offset);
    __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, dir_sector) );
    uint8_t start_token = fs->buffer[fs->temp.entire_in_dir_clust];
    /* Check, shall we erase sector or not needed */
    if ((start_token & ~0xE5) != 0)
    {
        __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, dir_sector) );
    }
    fs->buffer[fs->temp.entire_in_dir_clust] = 0xE5;
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, dir_sector) );

    /* Clear link to all file's clusters in FATs */
    uint32_t cluster = fs->temp.cluster + 2;
    bool first = true;
    uint32_t prev_fat_sector = 0;
    while ((cluster & 0x0FFFFFFF) < 0x0FFFFFF7)
    {
        uint32_t fat_sector = cluster / (fs->sector_len_bytes/sizeof(uint32_t));
        /* write FAT sector only if new FAT sector is other */
        if (!first)
        {
            if (fat_sector != prev_fat_sector)
            {
                __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, fs->fat1_begin + prev_fat_sector) );
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat1_begin + prev_fat_sector) );
                __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, fs->fat2_begin + prev_fat_sector) );
                __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat2_begin + prev_fat_sector) );
            }
        }

        uint32_t link_position = cluster % (fs->sector_len_bytes/sizeof(uint32_t));
        uint32_t *link = (uint32_t*)fs->buffer;
        __DISK_ERROR_CHECK( __mik32fat_sector_sread(fs, fs->fat1_begin + fat_sector) );
        cluster = link[link_position];
        link[link_position] = 0x00000000;
        
        prev_fat_sector = fat_sector;
        first = false;
    }
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, fs->fat1_begin + prev_fat_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat1_begin + prev_fat_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_serase(fs, fs->fat2_begin + prev_fat_sector) );
    __DISK_ERROR_CHECK( __mik32fat_sector_swrite(fs, fs->fat2_begin + prev_fat_sector) );

    return MIK32FAT_STATUS_OK;
}

/**
 * @brief File or directory deletion
 * @param fs pointer to file system's structure-descriptor
 * @param path string of path. The last byte should be '\0'.
 * If the path contain subdirectories, they are separated by '/' symbol (i.e.: "FOLDER/FILE").
 * If the name of file or subdir contains a point '.' symbol, it cannot contain more than 8 meanung
 * symbols before point and more than 3 meaning symbols after point. Else, the
 * name cannot contain more than 8 meaning symbols
 * @returns
 */
MIK32FAT_Status_TypeDef mik32fat_delete(MIK32FAT_Descriptor_TypeDef* fs, const char* path)
{
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    __DISK_ERROR_CHECK( mik32fat_find_by_path(fs, path) );
    MIK32FAT_Status_TypeDef res = mik32fat_delete_temp_object(fs);
    fs->temp = temp;
    return res;
}

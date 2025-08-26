#include "mik32fat.h"

#define __DISK_ERROR_CHECK(error)    do {\
    if (error != 0)\
    {\
        return MIK32FAT_STATUS_DISK_ERROR;\
    }\
} while(0);

#define __MIK32FAT_ERROR_CHECK(error)   do{\
    if (error != MIK32FAT_STATUS_OK)\
    {\
        return error;\
    }\
} while(0);

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
    /* Read FAT */
    uint32_t bias = (fs->temp.cluster / (fs->sector_len_bytes/sizeof(uint32_t)));
    __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + bias, fs->buffer) );
    fs->prev_sector = fs->fat1_begin + bias;
    /* Read field */
    uint32_t* ptr = (uint32_t*)fs->buffer;
    uint32_t link = ptr[fs->temp.cluster % (fs->sector_len_bytes/sizeof(uint32_t))];
    if ((link & 0x0FFFFFFF) >= 0x0FFFFFF7)
    {
        return MIK32FAT_STATUS_NOT_FOUND;
    }
    else
    {
        fs->temp.cluster = link;
        return MIK32FAT_STATUS_OK;
    }
}

uint32_t __mik32fat_get_sector(MIK32FAT_Descriptor_TypeDef *fs, uint32_t cluster, uint32_t sector_offset)
{
    return fs->data_region_begin + cluster * fs->param.sec_per_clust + sector_offset;
}

MIK32FAT_Status_TypeDef __mik32fat_sector_single_read(MIK32FAT_Descriptor_TypeDef *fs, uint32_t sector)
{
    /* Read sector only if has not already been buffered */
    if (sector != fs->prev_sector)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
        fs->prev_sector = sector;
    }
    return MIK32FAT_STATUS_OK;
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

    printf("Name: *");
    int u = 0;
    while (name[u] != '\0' && name[u] != '/')
    {
        putchar(name[u++]);
    }
    printf("*\n");

    char name_str[11];
    /* Preparing the name string */
    size_t i = 0;
    size_t j = 0;
    bool point_detected = false;
    while (name[i] != '\0' && name[i] != '/')
    {
        if (!point_detected)
        {
            if (j >= 8)
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

    uint32_t temp_cluster = fs->temp.cluster;

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
            res = __mik32fat_sector_single_read(fs, sector);
            if (res != MIK32FAT_STATUS_OK)
            {
                fs->temp.cluster = temp_cluster;
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
            fs->temp.dir_sector = fs->temp.cluster * fs->param.sec_per_clust + sec;
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
            return MIK32FAT_STATUS_OK;
        }
        res = mik32fat_find_next_cluster(fs);
        if (res != MIK32FAT_STATUS_OK)
        {
            fs->temp.cluster = temp_cluster;
        }
    }
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
        /* Find next name in path */
        while (path[j++] != '/');
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
MIK32FAT_Status_TypeDef mik32fat_find_or_create_by_path(MIK32FAT_Descriptor_TypeDef *fs, const char *path)
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
    MIK32FAT_Status_TypeDef res;
    bool not_found = false;
    char* ptr = path;
    for (uint8_t k=0; k<descend_number; k++)
    {
        if (!not_found)
        {
            res = mik32fat_find_by_name(fs, ptr);
            if (res == MIK32FAT_STATUS_NOT_FOUND)
            {
                not_found = true;
            }
            else if (res != MIK32FAT_STATUS_OK)
            {
                return res;
            }
        }
        if (not_found)
        {
            __MIK32FAT_ERROR_CHECK( mik32fat_create(fs, ptr, k != (descend_number-1)) );
            /* Descend into created object */
            __MIK32FAT_ERROR_CHECK( mik32fat_find_by_name(fs, ptr) );
        }
        while (*ptr != '/') ptr += 1;
        ptr += 1;
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
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + x, fs->buffer) );
        fs->prev_sector = fs->fat1_begin + x;
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
MIK32FAT_Status_TypeDef mik32fat_take_free_cluster(MIK32FAT_Descriptor_TypeDef* fs, uint32_t cluster, uint32_t* new_cluster)
{
    if (fs == NULL || new_cluster == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    // Из-за некоей подставы FAT32
    cluster += 2;
    /* Мониторим FAT */
    uint32_t* ptr;
    int32_t x = -1;
    int32_t link;
    do {
        x += 1;
        /* Read sector only if has not already been buffered */
        if (fs->fat1_begin + x != fs->prev_sector)
        {
            __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + x, fs->buffer) );
            fs->prev_sector = fs->fat1_begin + x;
        }
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
    uint32_t new_clus = (x * 128 + (link>>2));
    *new_cluster = new_clus - 2;
    /* Find sector of FAT containing previous cluster link */
    if (fs->fat1_begin + (cluster / (512/4)) != fs->prev_sector)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + (cluster / (512/4)), fs->buffer) );
        fs->prev_sector = fs->fat1_begin + (cluster / (512/4));
    }
    ptr = (uint32_t*)(fs->buffer + (cluster * fs->param.sec_per_clust) % 512);
    *ptr = new_clus;
    //xprintf("Write link %04X on field %04X\n", new_clus, cluster);
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat1_begin + (cluster / (512/4))) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat1_begin + (cluster / (512/4)), fs->buffer) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat2_begin + (cluster / (512/4))) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat2_begin + (cluster / (512/4)), fs->buffer) );
    /* Find sector of FAT containing previous cluster link */
    if (fs->fat1_begin + (new_clus / (512/4)) != fs->prev_sector)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + (new_clus / (512/4)), fs->buffer) );
        fs->prev_sector = fs->fat1_begin + (new_clus / (512/4));
    }
    ptr = (uint32_t*)(fs->buffer + link);
    *ptr = 0x0FFFFFFF;
    //xprintf("Write link 0x0FFFFFFF on field %04X\n", new_clus);
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat1_begin + (new_clus / (512/4))) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat1_begin + (new_clus / (512/4)), fs->buffer) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat2_begin + (new_clus / (512/4))) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat2_begin + (new_clus / (512/4)), fs->buffer) );
    return MIK32FAT_STATUS_OK;
}


MIK32FAT_Status_TypeDef mik32fat_file_open
(
    MIK32FAT_File_TypeDef *file,
    MIK32FAT_Descriptor_TypeDef *fs,
    const char *path,
    const char modificator
)
{
    if (file == NULL || fs == NULL || path == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    file->fs = fs;
    file->modificator = modificator;
    MIK32FAT_Status_TypeDef res;
    switch (modificator)
    {
        case 'R':
            mik32fat_set_pointer_to_root(file->fs);
            __MIK32FAT_ERROR_CHECK( mik32fat_find_by_path(file->fs, path) );
            /* Access settings */
            file->cluster = file->fs->temp.cluster;
            file->dir_sector = file->fs->temp.dir_sector;
            file->entire_in_dir_clust = file->fs->temp.entire_in_dir_clust;
            file->status = file->fs->temp.status;
            file->addr = 0;
            file->len = file->fs->temp.len;
            break;
        case 'W':
            mik32fat_set_pointer_to_root(file->fs);
            res = mik32fat_find_or_create_by_path(file->fs, path);
            if (res != MIK32FAT_STATUS_OK) return res;
            /* Access settings */
            file->dir_sector = file->fs->temp.dir_sector;
            file->entire_in_dir_clust = file->fs->temp.entire_in_dir_clust;
            file->status = file->fs->temp.status;
            file->addr = file->fs->temp.len;
            file->len = file->fs->temp.len;
            file->writing_not_finished = false;
            do {
                res = mik32fat_find_next_cluster(file->fs);
                //xprintf("Cluster change, status: %u\n", res);
            } while (res == MIK32FAT_STATUS_OK);
            if (res != MIK32FAT_STATUS_NOT_FOUND) return res;
            file->cluster = file->fs->temp.cluster;
            break;
        case 'A':
            mik32fat_set_pointer_to_root(file->fs);
            res = mik32fat_delete(file->fs, path);
            if ((res != MIK32FAT_STATUS_OK) && (res != MIK32FAT_STATUS_NOT_FOUND)) return res;
            mik32fat_set_pointer_to_root(file->fs);
            res = mik32fat_find_or_create_by_path(file->fs, path);
            if (res != MIK32FAT_STATUS_OK) return res;
            /* Access settings */
            file->cluster = file->fs->temp.cluster;
            file->dir_sector = file->fs->temp.dir_sector;
            file->entire_in_dir_clust = file->fs->temp.entire_in_dir_clust;
            file->status = file->fs->temp.status;
            file->addr = 0;
            file->len = 0;
            file->writing_not_finished = false;
            break;
        default: return MIK32FAT_STATUS_ERROR;
    }
    return MIK32FAT_STATUS_OK;
}



/**
 * @brief File closing. It is necessary if file was written.
 * @param file pointer to file's structure-descriptor
 * @returns
 */
MIK32FAT_Status_TypeDef mik32fat_file_close(MIK32FAT_File_TypeDef* file)
{
    if (file == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    if ((file->modificator == 'W') || (file->modificator == 'A'))
    {
        uint32_t sector;
        /* if writing_not_finished flag is set, write data into SD */
        if (file->writing_not_finished == true)
        {
            file->writing_not_finished = false;
            uint32_t clust_len = 512 * file->fs->param.sec_per_clust;
            sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + (file->addr % clust_len) / 512;
            //xprintf("Write sector: %u", sector);
            __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(file->fs->card, sector) );
            __DISK_ERROR_CHECK( mik32fat_wheels_single_write(file->fs->card, sector, file->fs->buffer) );
        }
        /* Write new file length */
        sector = file->fs->data_region_begin + file->dir_sector;
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(file->fs->card, sector, file->fs->buffer) );
        file->fs->prev_sector = sector;
        uint32_t* ptr = (uint32_t*)&file->fs->buffer[file->entire_in_dir_clust + MIK32FAT_DIR_FILE_SIZE_OFFSET];
        *ptr = file->len;
        __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(file->fs->card, sector) );
        __DISK_ERROR_CHECK( mik32fat_wheels_single_write(file->fs->card, sector, file->fs->buffer) );
    }
    return MIK32FAT_STATUS_OK;
}




/**
 * @brief Read data from the file
 * @param file pointer to file's structure-descriptor
 * @param buf buffer for data
 * @param quan number of bytes to read
 * @return number of read bytes
 */
uint32_t mik32fat_read_file(MIK32FAT_File_TypeDef *file, char *dst, uint32_t quan)
{
    if (file == NULL || dst == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    if (file->modificator != 'R')
    {
        return 0;
    }

    MIK32FAT_Descriptor_TypeDef *fs = file->fs;
    uint32_t counter = 0;
    uint32_t start_addr = (file->addr - file->addr % fs->param.clust_len_bytes);

    while ((quan > 0) && (file->len > 0))
    {
        /* Cluster */
        uint32_t new_cluster = (file->addr - start_addr) / fs->param.clust_len_bytes;
        if (new_cluster != 0)
        {
            start_addr += fs->param.clust_len_bytes;
        }
        file->fs->temp.cluster = file->cluster;
        while (new_cluster > 0)
        {
            mik32fat_find_next_cluster(file->fs);
            new_cluster -= 1;
        }
        file->cluster = fs->temp.cluster;
        /* Read sector data */
        uint32_t sector = fs->data_region_begin + file->cluster * fs->param.sec_per_clust +
            ((file->addr - start_addr) / 512);
        /* Read sector only if has not already buffered */
        if (fs->prev_sector != sector)
        {
            if (mik32fat_wheels_single_read(fs->card, sector, fs->buffer) != 0) return counter;
            fs->prev_sector = sector;
        }
        //xprintf("*%u*\n", sector);
        /* Reading sector */
        uint16_t x = file->addr % 512;
        while ((x < 512) && (quan > 0) && (file->len > 0))
        {
            dst[counter] = fs->buffer[x];
            counter += 1;
            x += 1;
            quan -= 1;
            file->addr += 1;
            file->len -= 1;
        }
    }
    return counter;
}



/**
 * @brief Writing file into SD card
 * @param file pointer to file's structure-descriptor
 * @param buf buffer for data
 * @param quan number of bytes to write
 * @return number of writen bytes
 */
uint32_t mik32fat_write_file(MIK32FAT_File_TypeDef* file, const char* buf, uint32_t quan)
{
    if ((file->modificator != 'W') && (file->modificator != 'A')) return 0;
    /* Index of buffer */
    uint32_t buf_idx = file->addr % 512;
    /* Number of written bytes */
    uint32_t counter = 0;
    //uint32_t clust_len = 512 * file->fs->param.sec_per_clust;

    while (quan > counter)
    {
        /* if writing is not started with 0 fs buffer address, read sector data to not lost previously written data */
        if ((buf_idx != 0) && (file->writing_not_finished == false))
        {
            uint32_t sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + (file->addr % file->fs->param.clust_len_bytes) / 512;
            if (mik32fat_wheels_single_read(file->fs->card, sector, file->fs->buffer) != 0) return counter;
            file->fs->prev_sector = sector;
        }
        /* if data is written into a new cluster, find and mark new cluster */
        if ((file->addr % file->fs->param.clust_len_bytes == 0) && (file->addr != 0))
        {
            //Make new cluster
            uint32_t value_buf;
            MIK32FAT_Status_TypeDef res = mik32fat_take_free_cluster(file->fs, file->cluster, &value_buf);
            //xprintf("Clus %u -> clus %u\n", file->cluster, value_buf);
            if (res != MIK32FAT_STATUS_OK) return counter;
            file->cluster = value_buf;
        }
        /* Copying source data into a fs buffer */
        while ((quan > counter) && (buf_idx < 512))
        {
            file->fs->buffer[buf_idx] = buf[counter];
            counter += 1;
            buf_idx += 1;
            file->addr += 1;
        }
        /* if source data was all written, switch on writing_not_finished flag */
        if (counter == quan) file->writing_not_finished = true;
        /* if fs buffer was overloaded, save its data & reset writing_not_finished flag */
        if (buf_idx >= 512)
        {
            file->writing_not_finished = false;
            uint32_t sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + ((file->addr-1) % file->fs->param.clust_len_bytes) / 512;
            if (mik32fat_wheels_single_erase(file->fs->card, sector) != 0) return counter;
            if (mik32fat_wheels_single_write(file->fs->card, sector, file->fs->buffer) != 0) return counter;
            //
            buf_idx = 0;
        }
    }
    file->len += counter;
    return counter;
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
MIK32FAT_Status_TypeDef mik32fat_create(MIK32FAT_Descriptor_TypeDef* fs, char* name, bool dir)
{
    /* Find free cluster in FAT */
    uint32_t new_cluster;
    uint32_t* ptr;
    int32_t x = -1;
    int32_t link;
    do {
        x += 1;
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, fs->fat1_begin + x, fs->buffer) );
        fs->prev_sector = fs->fat1_begin + x;
        link = -4;
        do {
            link += 4;
            ptr = (uint32_t*)(fs->buffer + link);
            //xprintf("*%08X*\n", *ptr);
        } while ((*ptr != 0) && (link < 512));
    }
    while ((*ptr != 0) && (x < fs->param.fat_length));
    if (x >= fs->param.fat_length) return MIK32FAT_STATUS_NO_FREE_SPACE;
    /* link is number of free cluster in fat sector */
    /* Save number of new cluster */
    new_cluster = (x * 128 + (link>>2));
    *ptr = 0x0FFFFFFF;
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat1_begin + x) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat1_begin + x, fs->buffer) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->fat2_begin + x) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->fat2_begin + x, fs->buffer) );

    /* Set /. and /.. entires in new cluster if directory (look MS FAT Specification) */
    if (dir == true)
    {
        memset(fs->buffer, 0x00, 512);
        MIK32FAT_Entire_TypeDef* ent = (MIK32FAT_Entire_TypeDef*)fs->buffer;
        memcpy(ent[0].Name, ".          ", 11);
        ent[0].Attr = 0x10;
        ent[0].FstClusLO = (uint16_t)new_cluster;
        ent[0].FstClusHI = (uint16_t)(new_cluster >> 16);
        memcpy(ent[1].Name, "..         ", 11);
        ent[1].Attr = 0x10;
        ent[1].FstClusLO = 0;
        ent[1].FstClusHI = 0;
        /* Write data */
        __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, fs->data_region_begin+(new_cluster-2)*fs->param.sec_per_clust) );
        __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, fs->data_region_begin+(new_cluster-2)*fs->param.sec_per_clust, fs->buffer) );
    }

    /* Find free space for descriptor in directory */
    uint32_t sector;
    uint16_t entire;
    MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
    while (res == MIK32FAT_STATUS_OK)
    {
        sector = fs->data_region_begin + fs->temp.cluster * fs->param.sec_per_clust;
        for (uint8_t idx=0; idx < fs->param.sec_per_clust; idx++)
        {
            __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
            fs->prev_sector = sector;
            sector += 1;
            entire = 0;
            while (entire < 512)
            {
                if ((fs->buffer[entire] == 0x00) || (fs->buffer[entire] == 0xE5)) break;
                entire += 32;
            }
            if (entire < 512)
            {
                sector -= 1;
                break;
            }
        }
        if (entire < 512) break;
        res = mik32fat_find_next_cluster(fs);
    }
    /* FAT_FNC error. if next cluster not found, take a free cluster */
    if (res == MIK32FAT_STATUS_NOT_FOUND)
    {
        uint32_t value;
        res = mik32fat_take_free_cluster(fs, fs->temp.cluster, &value);
        if (res != MIK32FAT_STATUS_OK) return res;
        entire = 0;
        sector = fs->data_region_begin + value * fs->param.sec_per_clust;
        //__DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
    }
    else if (res != MIK32FAT_STATUS_OK) return res;
    /* entire contains pointer to descriptor in directory's sector (sector) */
    memset(fs->buffer+entire+11, 0x00, 32-11);

    __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
    fs->prev_sector = sector;
    MIK32FAT_Entire_TypeDef* new_obj = (MIK32FAT_Entire_TypeDef*)(fs->buffer + entire);
    new_obj->FileSize = 0;
    new_obj->Attr = (dir ? MIK32FAT_ATTR_DIRECTORY : 0);
    new_obj->FstClusLO = (uint16_t)new_cluster;
    new_obj->FstClusHI = (uint16_t)(new_cluster >> 16);
    memset(new_obj->Name, 0x20, 8);
    memset(new_obj->Extention, 0x20, 3);
    uint8_t i=0;
    while ((name[i] != '.') && (name[i] != '\0') && (name[i] != '/') && (i<8))
    {
        new_obj->Name[i] = name[i];
        i += 1;
    }
    if ((name[i] != '\0') && (name[i] != '/') && (i < 8))
    {
        i += 1;
        uint8_t j=0;
        while ((name[i] != '\0') && (name[i] != '/') && (j<3))
        {
            new_obj->Extention[j] = name[i];
            i += 1;
            j += 1;
        }
    }
    __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, sector) );
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, sector, fs->buffer) );
    return MIK32FAT_STATUS_OK;
}


MIK32FAT_Status_TypeDef mik32fat_deleteTemp(MIK32FAT_Descriptor_TypeDef* fs)
{
    uint32_t sector;
    /* Set the 0th byte of entire as 0xE5 */
    sector = fs->data_region_begin + fs->temp.dir_sector;
    /* Read sector only if has not already been buffered */
    if (sector != fs->prev_sector)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
        fs->prev_sector = sector;
    }
    if ((fs->buffer[fs->temp.entire_in_dir_clust] & ~0xE5) != 0)
    {
        __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, sector) );
    }
    fs->buffer[fs->temp.entire_in_dir_clust] = 0xE5;
    __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, sector, fs->buffer) );

    /* Clear link to all file's clusters in FATs */
    uint32_t cluster = fs->temp.cluster + 2;
    uint32_t* ptr;
    sector = 0;
    do {
        if (sector != fs->fat1_begin + (cluster / (512/4)))
        {
            uint32_t dummy = sector;
            sector = fs->fat1_begin + (cluster / (512/4));
            if (dummy != 0)
            {
                __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(fs->card, dummy) );
                __DISK_ERROR_CHECK( mik32fat_wheels_single_write(fs->card, dummy, fs->buffer) );
            }
            __DISK_ERROR_CHECK( mik32fat_wheels_single_read(fs->card, sector, fs->buffer) );
            fs->prev_sector = sector;
        }
        ptr = (uint32_t*)(fs->buffer + (cluster * fs->param.sec_per_clust) % 512);
        cluster = *ptr;
        *ptr = 0;
    } while ((cluster & 0x0FFFFFFF) < 0x0FFFFFF7);
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
MIK32FAT_Status_TypeDef mik32fat_delete(MIK32FAT_Descriptor_TypeDef* fs, char* path)
{
    MIK32FAT_Status_TypeDef res;
    __DISK_ERROR_CHECK( mik32fat_find_by_path(fs, path) );
    return mik32fat_deleteTemp(fs);
}

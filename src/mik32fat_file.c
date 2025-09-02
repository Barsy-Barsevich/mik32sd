// #include "mik32fat_file.h"

// MIK32FAT_Status_TypeDef mik32fat_file_open
// (
//     MIK32FAT_File_TypeDef *file,
//     MIK32FAT_Descriptor_TypeDef *fs,
//     const char *path,
//     const char modificator
// )
// {
//     if (file == NULL || fs == NULL || path == NULL)
//     {
//         return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
//     }

//     file->fs = fs;
//     file->modificator = modificator;
//     MIK32FAT_Status_TypeDef res;
//     switch (modificator)
//     {
//         case 'R':
//             mik32fat_set_pointer_to_root(fs);
//             __MIK32FAT_ERROR_CHECK( mik32fat_find_by_path(fs, path) );
//             /* Access settings */
//             file->cluster = fs->temp.cluster;
//             file->dir_sector = fs->temp.dir_cluster * fs->param.sec_per_clust + fs->temp.dir_sec_offset;
//             file->entire_in_dir_clust = fs->temp.entire_in_dir_clust;
//             file->status = fs->temp.status;
//             file->addr = 0;
//             file->len = fs->temp.len;
//             break;
//         case 'W':
//             mik32fat_set_pointer_to_root(fs);
//             res = mik32fat_find_or_create_by_path(fs, path);
//             if (res != MIK32FAT_STATUS_OK) return res;
//             /* Access settings */
//             file->dir_sector = fs->temp.dir_cluster * fs->param.sec_per_clust + fs->temp.dir_sec_offset;
//             file->entire_in_dir_clust = fs->temp.entire_in_dir_clust;
//             file->status = fs->temp.status;
//             file->addr = fs->temp.len;
//             file->len = fs->temp.len;
//             file->writing_not_finished = false;
//             do {
//                 res = mik32fat_find_next_cluster(fs);
//                 //xprintf("Cluster change, status: %u\n", res);
//             } while (res == MIK32FAT_STATUS_OK);
//             if (res != MIK32FAT_STATUS_NOT_FOUND) return res;
//             file->cluster = fs->temp.cluster;
//             break;
//         case 'A':
//             mik32fat_set_pointer_to_root(fs);
//             res = mik32fat_delete(fs, path);
//             if ((res != MIK32FAT_STATUS_OK) && (res != MIK32FAT_STATUS_NOT_FOUND)) return res;
//             mik32fat_set_pointer_to_root(fs);
//             res = mik32fat_find_or_create_by_path(fs, path);
//             if (res != MIK32FAT_STATUS_OK) return res;
//             /* Access settings */
//             file->cluster = fs->temp.cluster;
//             file->dir_sector = fs->temp.dir_cluster * fs->param.sec_per_clust + fs->temp.dir_sec_offset;
//             file->entire_in_dir_clust = fs->temp.entire_in_dir_clust;
//             file->status = fs->temp.status;
//             file->addr = 0;
//             file->len = 0;
//             file->writing_not_finished = false;
//             break;
//         default: return MIK32FAT_STATUS_ERROR;
//     }
//     return MIK32FAT_STATUS_OK;
// }



// /**
//  * @brief File closing. It is necessary if file was written.
//  * @param file pointer to file's structure-descriptor
//  * @returns
//  */
// MIK32FAT_Status_TypeDef mik32fat_file_close(MIK32FAT_File_TypeDef* file)
// {
//     if (file == NULL)
//     {
//         return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
//     }

//     if ((file->modificator == 'W') || (file->modificator == 'A'))
//     {
//         uint32_t sector;
//         /* if writing_not_finished flag is set, write data into SD */
//         if (file->writing_not_finished == true)
//         {
//             file->writing_not_finished = false;
//             uint32_t clust_len = 512 * file->fs->param.sec_per_clust;
//             sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + (file->addr % clust_len) / 512;
//             //xprintf("Write sector: %u", sector);
//             __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(file->fs->card, sector) );
//             __DISK_ERROR_CHECK( mik32fat_wheels_single_write(file->fs->card, sector, file->fs->buffer) );
//         }
//         /* Write new file length */
//         sector = file->fs->data_region_begin + file->dir_sector;
//         __DISK_ERROR_CHECK( mik32fat_wheels_single_read(file->fs->card, sector, file->fs->buffer) );
//         file->fs->prev_sector = sector;
//         uint32_t* ptr = (uint32_t*)&file->fs->buffer[file->entire_in_dir_clust + MIK32FAT_DIR_FILE_SIZE_OFFSET];
//         *ptr = file->len;
//         __DISK_ERROR_CHECK( mik32fat_wheels_single_erase(file->fs->card, sector) );
//         __DISK_ERROR_CHECK( mik32fat_wheels_single_write(file->fs->card, sector, file->fs->buffer) );
//     }
//     return MIK32FAT_STATUS_OK;
// }




// /**
//  * @brief Read data from the file
//  * @param file pointer to file's structure-descriptor
//  * @param buf buffer for data
//  * @param quan number of bytes to read
//  * @return number of read bytes
//  */
// uint32_t mik32fat_read_file(MIK32FAT_File_TypeDef *file, char *dst, uint32_t quan)
// {
//     if (file == NULL || dst == NULL)
//     {
//         return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
//     }
//     if (file->modificator != 'R')
//     {
//         return 0;
//     }

//     MIK32FAT_Descriptor_TypeDef *fs = file->fs;
//     uint32_t counter = 0;
//     uint32_t start_addr = (file->addr - file->addr % fs->param.clust_len_bytes);

//     while ((quan > 0) && (file->len > 0))
//     {
//         /* Cluster */
//         uint32_t new_cluster = (file->addr - start_addr) / fs->param.clust_len_bytes;
//         if (new_cluster != 0)
//         {
//             start_addr += fs->param.clust_len_bytes;
//         }
//         file->fs->temp.cluster = file->cluster;
//         while (new_cluster > 0)
//         {
//             mik32fat_find_next_cluster(file->fs);
//             new_cluster -= 1;
//         }
//         file->cluster = fs->temp.cluster;
//         /* Read sector data */
//         uint32_t sector = fs->data_region_begin + file->cluster * fs->param.sec_per_clust +
//             ((file->addr - start_addr) / 512);
//         /* Read sector only if has not already buffered */
//         if (fs->prev_sector != sector)
//         {
//             if (mik32fat_wheels_single_read(fs->card, sector, fs->buffer) != 0) return counter;
//             fs->prev_sector = sector;
//         }
//         //xprintf("*%u*\n", sector);
//         /* Reading sector */
//         uint16_t x = file->addr % 512;
//         while ((x < 512) && (quan > 0) && (file->len > 0))
//         {
//             dst[counter] = fs->buffer[x];
//             counter += 1;
//             x += 1;
//             quan -= 1;
//             file->addr += 1;
//             file->len -= 1;
//         }
//     }
//     return counter;
// }



// /**
//  * @brief Writing file into SD card
//  * @param file pointer to file's structure-descriptor
//  * @param buf buffer for data
//  * @param quan number of bytes to write
//  * @return number of writen bytes
//  */
// uint32_t mik32fat_write_file(MIK32FAT_File_TypeDef* file, const char* buf, uint32_t quan)
// {
//     if ((file->modificator != 'W') && (file->modificator != 'A')) return 0;
//     /* Index of buffer */
//     uint32_t buf_idx = file->addr % 512;
//     /* Number of written bytes */
//     uint32_t counter = 0;
//     //uint32_t clust_len = 512 * file->fs->param.sec_per_clust;

//     MIK32FAT_Descriptor_TypeDef *fs = file->fs;

//     while (quan > counter)
//     {
//         /* if writing is not started with 0 fs buffer address, read sector data to not lost previously written data */
//         if ((buf_idx != 0) && (file->writing_not_finished == false))
//         {
//             uint32_t sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + (file->addr % file->fs->param.clust_len_bytes) / 512;
//             if (mik32fat_wheels_single_read(file->fs->card, sector, file->fs->buffer) != 0) return counter;
//             file->fs->prev_sector = sector;
//         }
//         /* if data is written into a new cluster, find and mark new cluster */
//         if ((file->addr % file->fs->param.clust_len_bytes == 0) && (file->addr != 0))
//         {
//             //Make new cluster
//             uint32_t value_buf;
//             MIK32FAT_Status_TypeDef res = mik32fat_take_free_cluster(file->fs, file->cluster, &value_buf);
//             //xprintf("Clus %u -> clus %u\n", file->cluster, value_buf);
//             if (res != MIK32FAT_STATUS_OK) return counter;
//             file->cluster = value_buf;
//         }
//         /* Copying source data into a fs buffer */
//         while ((quan > counter) && (buf_idx < 512))
//         {
//             file->fs->buffer[buf_idx] = buf[counter];
//             counter += 1;
//             buf_idx += 1;
//             file->addr += 1;
//         }
//         /* if source data was all written, switch on writing_not_finished flag */
//         if (counter == quan) file->writing_not_finished = true;
//         /* if fs buffer was overloaded, save its data & reset writing_not_finished flag */
//         if (buf_idx >= 512)
//         {
//             file->writing_not_finished = false;
//             uint32_t sector = file->fs->data_region_begin + file->cluster * file->fs->param.sec_per_clust + ((file->addr-1) % file->fs->param.clust_len_bytes) / 512;
//             MIK32FAT_Status_TypeDef res;
//             res = __mik32fat_sector_serase(fs, sector);
//             if (res != MIK32FAT_STATUS_OK)
//             {
//                 return counter;
//             }
//             res = __mik32fat_sector_swrite(fs, sector);
//             if (res != MIK32FAT_STATUS_OK)
//             {
//                 return counter;
//             }
            
//             buf_idx = 0;
//         }
//     }
//     file->len += counter;
//     return counter;
// }

#include "mik32fat_file.h"

MIK32FAT_Status_TypeDef mik32fat_file_open
(
    MIK32FAT_File_TypeDef *file,
    MIK32FAT_Descriptor_TypeDef *fs,
    const char *name,
    const char *mod
)
{
    if (file == NULL || fs == NULL || name == NULL || mod == NULL)
    {
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    MIK32FAT_TempData_TypeDef temp = fs->temp;
    
    if (strcmp(mod, "r") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_by_path(fs, name) );
        file->r.enable = true;
        file->w.enable = false;
        file->r.idx = 0;
        file->r.temp_cluster = fs->temp.cluster;
        file->r.temp_sector_in_cluster = 0;
        file->data_to_read = fs->temp.len;
    }
    else if (strcmp(mod, "r+") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_by_path(fs, name) );
        file->r.enable = true;
        file->w.enable = true;
        file->r.idx = 0;
        file->r.temp_cluster = fs->temp.cluster;
        file->r.temp_sector_in_cluster = 0;
        file->w.idx = 0;
        file->w.temp_cluster = fs->temp.cluster;
        file->w.temp_sector_in_cluster = 0;
    }
    else if (strcmp(mod, "w") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_or_create_by_path(fs, name, false) );
        file->r.enable = false;
        file->w.enable = true;
        file->w.idx = 0;
        file->w.temp_cluster = fs->temp.cluster;
        file->w.temp_sector_in_cluster = 0;
    }
    else if (strcmp(mod, "w+") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_or_create_by_path(fs, name, false) );
        file->r.enable = true;
        file->w.enable = true;
        file->r.idx = 0;
        file->r.temp_cluster = fs->temp.cluster;
        file->r.temp_sector_in_cluster = 0;
        file->w.idx = 0;
        file->w.temp_cluster = fs->temp.cluster;
        file->w.temp_sector_in_cluster = 0;
        file->data_to_read = 0;
    }
    else if (strcmp(mod, "a") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_or_create_by_path(fs, name, false) );
        file->r.enable = false;
        file->w.enable = true;
        uint32_t len = fs->temp.len;
        uint32_t clusters_num = len / fs->param.clust_len_bytes;
        uint32_t addr_in_cluster = len % fs->param.clust_len_bytes;
        file->w.temp_sector_in_cluster = addr_in_cluster / fs->sector_len_bytes;
        file->w.idx = addr_in_cluster % fs->sector_len_bytes;
        MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;

        printf("Clusters num: %u\n", (unsigned)clusters_num);

        while (res == MIK32FAT_STATUS_OK && clusters_num-- > 0)
        {
            res = mik32fat_find_next_cluster(fs);
        }
        if (res != MIK32FAT_STATUS_OK)
        {
            fs->temp = temp;
            if (res == MIK32FAT_STATUS_NOT_FOUND)
            {
                return MIK32FAT_STATUS_INCORRECT_FILE_LENGHT;
            }
            else
            {
                return res;
            }
        }
        file->w.temp_cluster = fs->temp.cluster;
    }
    else if (strcmp(mod, "a+") == 0)
    {
        __SAVING_TEMP_ERROR_CHECK( mik32fat_find_or_create_by_path(fs, name, false) );
        file->r.enable = true;
        file->w.enable = true;
        uint32_t len = fs->temp.len;
        uint32_t clusters_num = len / fs->param.clust_len_bytes;
        uint32_t addr_in_cluster = len % fs->param.clust_len_bytes;
        file->r.temp_sector_in_cluster = addr_in_cluster / fs->sector_len_bytes;
        file->w.temp_sector_in_cluster = addr_in_cluster / fs->sector_len_bytes;
        file->r.idx = addr_in_cluster % fs->sector_len_bytes;
        file->w.idx = addr_in_cluster % fs->sector_len_bytes;
        MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
        while (res == MIK32FAT_STATUS_OK && clusters_num-- > 0)
        {
            res = mik32fat_find_next_cluster(fs);
        }
        if (res != MIK32FAT_STATUS_OK)
        {
            fs->temp = temp;
            if (res == MIK32FAT_STATUS_NOT_FOUND)
            {
                return MIK32FAT_STATUS_INCORRECT_FILE_LENGHT;
            }
            else
            {
                return res;
            }
        }
        file->r.temp_cluster = fs->temp.cluster;
        file->w.temp_cluster = fs->temp.cluster;
        file->data_to_read = fs->temp.len;
    }
    else
    {
        fs->temp = temp;
        return MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }

    file->fs = fs;
    file->param = fs->temp;
    file->writing_not_finished = false;
    
    fs->temp = temp;
    return MIK32FAT_STATUS_OK;
}


int mik32fat_file_read_byte
(
    MIK32FAT_File_TypeDef *file,
    char *symbol
)
{
    if (file == NULL || symbol == NULL)
    {
        file->errcode = MIK32FAT_STATUS_INCORRECT_ARGUMENT;
        return (int)MIK32FAT_STATUS_INCORRECT_ARGUMENT;
    }
    if (!file->r.enable)
    {
        file->errcode = MIK32FAT_STATUS_ERROR;
        return (int)MIK32FAT_STATUS_ERROR;
    }
    if (file->data_to_read == 0)
    {
        file->errcode = MIK32FAT_STATUS_END_OF_FILE;
        return (int)MIK32FAT_STATUS_END_OF_FILE;
    }
    file->data_to_read -= 1;
    
    MIK32FAT_Descriptor_TypeDef *fs = file->fs;

    uint32_t sector = fs->data_region_begin + file->r.temp_cluster *
        fs->param.sec_per_clust + file->r.temp_sector_in_cluster;
    MIK32FAT_Status_TypeDef res = __mik32fat_sector_sread(fs, sector);
    if (res != MIK32FAT_STATUS_OK)
    {
        file->errcode = res;
        return (int)res;
    }
    *symbol = fs->buffer[file->r.idx];
    if (file->r.idx == fs->sector_len_bytes-1)
    {
        file->r.idx = 0;
        if (file->r.temp_sector_in_cluster == fs->param.sec_per_clust-1)
        {
            file->r.temp_sector_in_cluster = 0;
            MIK32FAT_TempData_TypeDef temp = fs->temp;
            fs->temp.cluster = file->r.temp_cluster;
            MIK32FAT_Status_TypeDef res = mik32fat_find_next_cluster(fs);
            if (res != MIK32FAT_STATUS_OK)
            {
                fs->temp = temp;
                if (res == MIK32FAT_STATUS_NOT_FOUND)
                {
                    file->errcode = MIK32FAT_STATUS_INCORRECT_FILE_LENGHT;
                    return (int)MIK32FAT_STATUS_INCORRECT_FILE_LENGHT;
                }
                else
                {
                    file->errcode = res;
                    return (int)res;
                }
            }
            file->r.temp_cluster = fs->temp.cluster;
            fs->temp = temp;
        }
        else
        {
            file->r.temp_sector_in_cluster += 1;
        }
    }
    else
    {
        file->r.idx += 1;
    }
    file->errcode = MIK32FAT_STATUS_OK;
    return 0;
}

// /**
//  * @brief Read data from the file
//  * @param file pointer to file's structure-descriptor
//  * @param buf buffer for data
//  * @param quan number of bytes to read
//  * @return number of read bytes
//  */
// uint32_t mik32fat_read_file
// (
//     MIK32FAT_File_TypeDef *file,
//     char *dst,
//     uint32_t quan
// )
// {
//     if (file == NULL || dst == NULL)
//     {
//         file->errcode = MIK32FAT_STATUS_INCORRECT_ARGUMENT;
//         return 0;
//     }
//     if (file->w.enable)
//     {
//         file->errcode = MIK32FAT_STATUS_ERROR;
//         return 0;
//     }

//     MIK32FAT_Descriptor_TypeDef *fs = file->fs;
//     uint32_t counter = 0;

//     while (quan > 0 && file->r.idx > 0)
//     {

//     }









//     while ((quan > 0) && (file->len > 0))
//     {
//         /* Cluster */
//         uint32_t new_cluster = (file->addr - start_addr) / fs->param.clust_len_bytes;
//         if (new_cluster != 0)
//         {
//             start_addr += fs->param.clust_len_bytes;
//         }
//         file->fs->temp.cluster = file->cluster;
//         while (new_cluster > 0)
//         {
//             mik32fat_find_next_cluster(file->fs);
//             new_cluster -= 1;
//         }
//         file->cluster = fs->temp.cluster;
//         /* Read sector data */
//         uint32_t sector = fs->data_region_begin + file->cluster * fs->param.sec_per_clust +
//             ((file->addr - start_addr) / 512);
//         /* Read sector only if has not already buffered */
//         if (fs->prev_sector != sector)
//         {
//             if (mik32fat_wheels_single_read(fs->card, sector, fs->buffer) != 0) return counter;
//             fs->prev_sector = sector;
//         }
//         //xprintf("*%u*\n", sector);
//         /* Reading sector */
//         uint16_t x = file->addr % 512;
//         while ((x < 512) && (quan > 0) && (file->len > 0))
//         {
//             dst[counter] = fs->buffer[x];
//             counter += 1;
//             x += 1;
//             quan -= 1;
//             file->addr += 1;
//             file->len -= 1;
//         }
//     }
//     return counter;
// }

#include "mik32fat_diag.h"

void mik32fat_decode_attr(uint8_t attrubute)
{
    printf("Decoding attribute: 0x%02X. ", attrubute);
    printf("It is a %s", (attrubute & MIK32FAT_ATTR_DIRECTORY) ? "directory" : "file");
    if (attrubute & MIK32FAT_ATTR_READ_ONLY)
    {
        printf(", read-only");
    }
    if (attrubute & MIK32FAT_ATTR_HIDDEN)
    {
        printf(", hidden");
    }
    if (attrubute & MIK32FAT_ATTR_SYSTEM)
    {
        printf(", system");
    }
    if (attrubute & MIK32FAT_ATTR_ARCHIVE)
    {
        printf(", archived");
    }
    if (attrubute & MIK32FAT_ATTR_VOLUME_ID)
    {
        printf(", volume id");
    }
    printf(".\n");
}

void mik32fat_decode_entire(MIK32FAT_Entire_TypeDef *entire)
{
    printf("Decoding entire.\n");
    char name[9];
    for (int i=0; i<8; i++)
    {
        name[i] = entire->Name[i];
    }
    name[8] = '\0';
    char extention[4];
    for (int i=0; i<3; i++)
    {
        extention[i] = entire->Extention[i];
    }
    extention[3] = '\0';
    printf("Name: %s.%s\n", name, extention);
    mik32fat_decode_attr(entire->Attr);
    uint32_t first_cluster = (uint32_t)entire->FstClusHI << 16 | entire->FstClusLO;
    printf("First cluster: %u\n", (unsigned)first_cluster);
    printf("File size: %u\n", (unsigned)entire->FileSize);
}

void mik32fat_decode_status(MIK32FAT_Status_TypeDef status)
{
    printf("Decoding status: ");
    switch (status)
    {
        case MIK32FAT_STATUS_OK:
            printf("OK");
            break;
        case MIK32FAT_STATUS_DISK_ERROR:
            printf("Disk error");
            break;
        case MIK32FAT_STATUS_DISK_NOT_FORM:
            printf("Disk not formatted for FAT32");
            break;
        case MIK32FAT_STATUS_ERROR:
            printf("Error");
            break;
        case MIK32FAT_STATUS_NOT_FOUND:
            printf("Not found");
            break;
        case MIK32FAT_STATUS_NO_FREE_SPACE:
            printf("No free space");
            break;
        case MIK32FAT_STATUS_INCORRECT_ARGUMENT:
            printf("Incorrect argument");
            break;
        case MIK32FAT_STATUS_NAME_ERROR:
            printf("Name error");
            break;
        case MIK32FAT_STATUS_INCORRECT_FILE_LENGHT:
            printf("Incorrect file length");
            break;
        case MIK32FAT_STATUS_END_OF_FILE:
            printf("End of file");
            break;
        default:
            printf("Unexpected error (%d)", (int)status);
    }
    printf("\n");
}

MIK32FAT_Status_TypeDef mik32fat_diag_sector_list
(
    MIK32FAT_Descriptor_TypeDef *f,
    bool hidden
)
{
    printf("\nList:\n");
    MIK32FAT_Status_TypeDef res = MIK32FAT_STATUS_OK;
    uint8_t sector_counter = 0;
    uint32_t sector = f->data_region_begin + (f->temp.cluster * f->param.sec_per_clust);
    
    do {
        int sd_res = (int)mik32_sd_single_read(f->card, sector, f->buffer);
        if (res != 0)
        {
            return MIK32FAT_STATUS_DISK_ERROR;
        }
        
        uint16_t entire_counter;
        MIK32FAT_Entire_TypeDef *entire = (MIK32FAT_Entire_TypeDef*)f->buffer;
        entire_counter = 0;
        while ((entire->Name[0] != 0x00) && (entire_counter < 512))
        {
            /* Pass deleted objects */
            if ((uint8_t)entire->Name[0] == 0xE5)
            {
                entire += 1;
                entire_counter += 32;
                continue;
            }
            if (!hidden)
            {
                /* Pass the hidden objects */
                if (entire->Attr & MIK32FAT_ATTR_HIDDEN)
                {
                    entire += 1;
                    entire_counter += 32;
                    continue;
                }
            }
            /* Printing attributes */
            printf("%c%c%c  ", \
            (entire->Attr & MIK32FAT_ATTR_DIRECTORY) ? 'd' : '-',
            'r',
            (entire->Attr & MIK32FAT_ATTR_READ_ONLY) ? '-' : 'w');
            /* Printing entire contains */
            uint8_t i=0;
            while ((entire->Name[i] != 0x20) && (i < 8))
            {
                xputc(entire->Name[i]);
                i += 1;
            }
            if (entire->Extention[0] != 0x20)
            {
                xputc('.');
                i = 0;
                while ((entire->Extention[i] != 0x20) && (i < 3))
                {
                    xputc(entire->Extention[i]);
                    i += 1;
                }
            }
            xputc('\n');

            entire += 1;
            entire_counter += 32;
        }
        if (entire_counter <= 512) break;
        sector_counter += 1;
        sector += 1;
    } while (sector_counter < f->param.sec_per_clust);

    if (res != MIK32FAT_STATUS_OK) return res;
    return res;
}

void mik32fat_diag_fat_info(MIK32FAT_Descriptor_TypeDef *fs)
{
    printf("FAT Info.\n");
    printf("* FS startaddr ------- %u\n", (unsigned)fs->fs_begin);
    printf("* FAT1 begin --------- %u\n", (unsigned)fs->fat1_begin);
    printf("* FAT2 begin --------- %u\n", (unsigned)fs->fat2_begin);
    printf("* Data region begin -- %u\n", (unsigned)fs->data_region_begin);
    printf("* Prev read sector --- %u\n", (unsigned)fs->prev_sector);
    printf("FS parameters:\n");
    printf("* Sec per clust ------ %u\n", (unsigned)fs->param.sec_per_clust);
    printf("* Num of FATs -------- %u\n", (unsigned)fs->param.num_of_fats);
    printf("* FAT length, sec ---- %u\n", (unsigned)fs->param.fat_length);
    printf("* Clust length ------- %u\n", (unsigned)fs->param.clust_len_bytes);
    printf("Temporary object parameters:\n");
    printf("* 1st file/dir cluster ------------------- %u\n", (unsigned)fs->temp.cluster);
    printf("* 1st clust of dir contains temp obj------ %u\n", (unsigned)fs->temp.dir_cluster);
    printf("* Sector in dir clust with obj entire ---- %u\n", fs->temp.dir_sec_offset);
    printf("* File/dir entire in this sector --------- %u\n", (unsigned)fs->temp.entire_in_dir_clust);
    printf("* File length (0 for directories) -------- %u\n", (unsigned)fs->temp.len);
    printf("* File/dir status ------------------------ %u\n", fs->temp.status);
}

void mik32fat_diag_decode_file(MIK32FAT_File_TypeDef *file)
{
    printf("Decoding file..\n");
    printf("* File system desc addr ------------------ %p\n", file->fs);
    printf("* Writing not finished ------------------- %s\n", file->writing_not_finished ? "true" : "false");
    printf("Const parameters:\n");
    // printf("* Name ----------------------------------- %s\n", file->param.name);
    printf("* 1st cluster ---------------------------- %u\n", (unsigned)file->param.cluster);
    printf("* 1st cluster of parent directory -------- %u\n", (unsigned)file->param.dir_cluster);
    printf("* Sector in dir clust with file entire --- %u\n", file->param.dir_sec_offset);
    printf("* File entire in this sector ------------- %u\n", (unsigned)file->param.entire_in_dir_clust);
    printf("* File length (when opening) ------------- %u\n", (unsigned)file->param.len);
    printf("* File status ---------------------------- %u\n", file->param.status);
    printf("Reading info:\n");
    printf("* Reading enabled ------------------------ %s\n", file->r.enable ? "true" : "false");
    printf("* Temporary cluster ---------------------- %u\n", (unsigned)file->r.temp_cluster);
    printf("* Temporary sector in cluster ------------ %u\n", file->r.temp_sector_in_cluster);
    printf("* Temporary index ------------------------ %u\n", (unsigned)file->r.idx);
    printf("Writing info:\n");
    printf("* Writing enabled ------------------------ %s\n", file->w.enable ? "true" : "false");
    printf("* Temporary cluster ---------------------- %u\n", (unsigned)file->w.temp_cluster);
    printf("* Temporary sector in cluster ------------ %u\n", file->w.temp_sector_in_cluster);
    printf("* Temporary index ------------------------ %u\n", (unsigned)file->w.idx);
}
#pragma once

#include <stdint.h>
#include "mik32sd.h"
#include "mik32fat_param.h"

typedef struct
{
    char Name[8];
    char Extention[3];
    uint8_t Attr;
    uint8_t NTRes;
    uint8_t CrtTimeTenth;
    uint16_t CrtTime;
    uint16_t CrtDate;
    uint16_t LstAccDate;
    uint16_t FstClusHI;
    uint16_t WrtTime;
    uint16_t WrtDate;
    uint16_t FstClusLO;
    uint32_t FileSize;
} MIK32FAT_Entire_TypeDef;

typedef struct
{
    uint8_t read_only : 1;
    uint8_t hidden : 1;
    uint8_t system : 1;
    uint8_t volume_id : 1;
    uint8_t directory : 1;
    uint8_t archive : 1;
    uint8_t __res : 2;
} MIK32FAT_ObjAttr_TypeDef;


typedef struct
{
    uint32_t cluster;               //< Cluster of temp file / subdirectory
    uint32_t dir_cluster;           //< Cluster of directory contains temporary file/dir
    uint8_t dir_sec_offset;         //< Number of sector in dir_cluster contains entire
    uint32_t entire_in_dir_clust;   //< Number of entire of file in dir's sector
    
    uint32_t len;                   //< Length of file (always 0 for directories)
    uint8_t status;                 //< Status of temp file / subdirectory

    char name[8+1+3+1];
} MIK32FAT_TempData_TypeDef;

typedef struct 
{
    /* SD card descriptor */
    MIK32SD_Descriptor_TypeDef *card;
    uint8_t buffer[MIK32FAT_BUFFER_LENGTH_BYTES];    //<  One-sector buffer
    uint32_t sector_len_bytes;
    /**
     * The file system startaddr
     * It is a pointer to 0th cluster of file system containing information about
     */
    uint32_t fs_begin;
    uint32_t fat1_begin;        //< 1st FAT sector startaddr
    uint32_t fat2_begin;        //< 2nd FAT sector startaddr
    uint32_t data_region_begin; //< data region sector startaddr
    uint32_t prev_sector;       //< Previously read sector
    
    // File system parameters
    struct
    {
        uint8_t sec_per_clust;  //< Number of sectors per cluster
        uint8_t num_of_fats;    //< Number of FATs
        uint32_t fat_length;    //< Length of one FAT
        uint32_t clust_len_bytes;     //< Length of 1 cluster
    } param;

    // Temporary object parameters
    MIK32FAT_TempData_TypeDef temp;
} MIK32FAT_Descriptor_TypeDef;



typedef struct 
{
    // Указатель на дескриптор файловой системы
    MIK32FAT_Descriptor_TypeDef* fs;
    /**
     * @brief Если бы файл был единым непрерывным массивом данных,
     * addr - это адрес, с которого начинается запись или чтение
     */
    uint32_t addr;
    /**
     * @brief Номер текущего кластера файла, Значение по адресу addr
     * попадает в текущий кластер
     */
    uint32_t cluster;
    /**
     * @brief Номер сектора директории, в котором лежит дескриптор файла
     */
    uint32_t dir_sector;
    /**
     * @brief Адрес дескриптора файла в секторе директории
     */
    uint32_t entire_in_dir_clust;
    /**
     * @brief Длина файла. При чтении декрементируется, при записи инкрементируется
     */
    uint32_t len;
    /**
     * @brief Статус файла
     */
    uint8_t status;
    /**
     * @brief Модификатор доступа к файлу
     */
    char modificator;
    /**
     * @brief
     */
    bool writing_not_finished;
} _MIK32FAT_File_TypeDef;

typedef struct
{
    MIK32FAT_Descriptor_TypeDef *fs;
    MIK32FAT_TempData_TypeDef param;
    uint32_t data_to_read_idx;
    uint32_t data_to_write_idx;
    struct
    {
        uint32_t temp_cluster;
        uint32_t idx;
        uint8_t temp_sector_in_cluster;
        bool enable;
    } r;
    struct
    {
        uint32_t temp_cluster;
        uint32_t idx;
        uint8_t temp_sector_in_cluster;
        bool enable;
    } w;
    bool writing_not_finished;
} MIK32FAT_File_TypeDef;
#pragma once

#include <string.h>
#include <stdbool.h>
#include "mik32sd.h"
#include "mik32fat_const.h"
#include "mik32fat_types.h"
#include "mik32fat_param.h"
#include "mik32fat_wheels.h"
#include "mik32fat.h"

// MIK32FAT_Status_TypeDef mik32fat_file_open(MIK32FAT_File_TypeDef* file, MIK32FAT_Descriptor_TypeDef* fs, const char* name, const char modificator);
// MIK32FAT_Status_TypeDef mik32fat_file_close(MIK32FAT_File_TypeDef* file);
// uint32_t mik32fat_read_file(MIK32FAT_File_TypeDef* file, char* buf, uint32_t quan);
// uint32_t mik32fat_write_file(MIK32FAT_File_TypeDef* file, const char* buf, uint32_t quan);

MIK32FAT_Status_TypeDef mik32fat_file_open(MIK32FAT_File_TypeDef *file, MIK32FAT_Descriptor_TypeDef *fs, const char *name, const char *mod);
int mik32fat_file_read_byte(MIK32FAT_File_TypeDef *file, char *symbol);
int mik32fat_file_read(MIK32FAT_File_TypeDef *file, char *dst, size_t len);
int mik32fat_file_write_byte(MIK32FAT_File_TypeDef *file, char symbol);
int mik32fat_file_write(MIK32FAT_File_TypeDef *file, const char *src, size_t len);
MIK32FAT_Status_TypeDef mik32fat_file_close(MIK32FAT_File_TypeDef *file);
#pragma once

#include <string.h>
#include <stdbool.h>
#include "mik32sd.h"
#include "mik32fat_const.h"
#include "mik32fat_types.h"
#include "mik32fat_param.h"
#include "mik32fat_wheels.h"

#include "mik32fat.h"

MIK32FAT_Status_TypeDef mik32fat_utils_ls(MIK32FAT_Descriptor_TypeDef *fs, FILE *output);
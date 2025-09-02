#pragma once

#include "mik32sd_const.h"

#define MIK32SD_GO_FROM_WAIT_CYCLES     200
#define MIK32SD_BUSY_WAIT_CYCLES        10000
#define MIK32SD_RESPONSE_WAIT_CYCLES    10000
#define MIK32SD_PRE_CMD_WAIT_CYCLES     10000
#define MIK32SD_VOLTAGE                 MIK32SD_VOLT_3_2_TO_3_3


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

#define __SAVING_TEMP_ERROR_CHECK(error) do{\
    if (error != MIK32FAT_STATUS_OK)\
    {\
        fs->temp = temp;\
        return error;\
    }\
} while(0);

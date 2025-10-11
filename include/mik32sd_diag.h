#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include "mik32sd_const.h"
#include "mik32sd_types.h"

void mik32sd_diag_decode_status(MIK32SD_Status_TypeDef status);
void mik32sd_diag_decode_sd_type(MIK32SD_Type_TypeDef type);

#if defined(__cplusplus)
}
#endif

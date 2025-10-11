#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "mik32sd_spi.h"
#include "mik32sd.h"
#include "mik32sd_command.h"
#include "mik32fat.h"
#include "mik32fat_diag.h"
#include "mik32fat_utils.h"

void cli_command(void);
void cli_help(void);
void cli_sd_init(void);
void cli_spi_init(void);
void cli_spi_ex(void);
void cli_csdown(void);
void cli_csup(void);
void cli_spi_write(int v);
void cli_fatinit(void);
void cli_sread();
void cli_stest();
void cli_send512();
void cli_read512(void);
void cli_swrite(void);
void cli_fat_spr(void);
void cli_fat_mkdir(void);
void cli_fat_mkfile(void);
void cli_fat_rm(void);
void cli_fsparam(void);
void cli_fileopen(void);
void cli_fat_fbn(void);
void cli_fat_fbp(void);
void cli_fat_fcbp(void);
void cli_fat_ls(void);
void cli_fat_pwd(void);
void cli_file_readbyte(void);
void cli_file_writebyte(void);
void cli_fileclose(void);

#if defined(__cplusplus)
}
#endif
